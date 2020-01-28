/*
 *  Copyright (C) 2020 Badger Technologies, LLC
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */
/* *************************************************************
 * Desc: AMCL Node for 3D AMCL
 * Author: Tyler Buchman (tyler_buchman@jabil.com)
 ***************************************************************/

#ifndef AMCL_NODE_NODE_3D_H
#define AMCL_NODE_NODE_3D_H

#include <message_filters/subscriber.h>
#include <octomap_msgs/Octomap.h>
#include <ros/duration.h>
#include <ros/node_handle.h>
#include <ros/subscriber.h>
#include <ros/time.h>
#include <ros/timer.h>
#include <sensor_msgs/PointCloud2.h>
#include <tf/message_filter.h>

#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

#include "amcl/AMCLConfig.h"
#include "map/octomap.h"
#include "pf/pf_vector.h"
#include "sensors/point_cloud_scanner.h"

namespace amcl
{

class Node;

class Node3D
{
public:
  Node3D(Node* node, int map_type, std::mutex& configuration_mutex);
  void reconfigure(amcl::AMCLConfig& config);
  void setOctomapBoundsFromOccupancyMap(std::shared_ptr<std::vector<double>> map_min,
                                        std::shared_ptr<std::vector<double>> map_max);
  void updateFreeSpaceIndices();
  void globalLocalizationCallback();
  double scorePose(const PFVector& p);
private:
  void scanReceived(const sensor_msgs::PointCloud2ConstPtr& point_cloud_scan);
  void initFromNewMap();
  void mapMsgReceived(const octomap_msgs::OctomapConstPtr& msg);
  std::shared_ptr<OctoMap> convertMap(const octomap_msgs::Octomap& map_msg);
  void checkScanReceived(const ros::TimerEvent& event);
  bool initFrameToScanner(const sensor_msgs::PointCloud2ConstPtr& point_cloud_scan,
                          int* scanner_index);
  void updatePf(const sensor_msgs::PointCloud2ConstPtr& point_cloud_scan,
                int scanner_index, bool* resampled);
  bool resamplePf(const sensor_msgs::PointCloud2ConstPtr& point_cloud_scan);

  std::shared_ptr<OctoMap> map_;
  std::shared_ptr<octomap::OcTree> octree_;
  std::shared_ptr<PointCloudData> latest_scan_data_;
  std::shared_ptr<std::vector<double>> occupancy_map_min_, occupancy_map_max_;
  std::shared_ptr<std::vector<bool>> scanners_update_;
  std::shared_ptr<PFSampleSet> fake_sample_set_;
  std::shared_ptr<ParticleFilter> pf_;
  std::unique_ptr<message_filters::Subscriber<sensor_msgs::PointCloud2>> scan_sub_;
  std::unique_ptr<tf::MessageFilter<sensor_msgs::PointCloud2>> scan_filter_;
  std::string scan_topic_;
  std::string odom_frame_id_;
  std::string base_frame_id_;
  std::string global_frame_id_;
  std::string global_alt_frame_id_;
  std::map<std::string, int> frame_to_scanner_;
  std::mutex& configuration_mutex_;
  std::vector<std::shared_ptr<PointCloudScanner> > scanners_;
  PFSample fake_sample_;
  PointCloudModelType model_type_;
  PointCloudScanner scanner_;
  Node* node_;
  ros::NodeHandle nh_;
  ros::NodeHandle private_nh_;
  ros::Subscriber map_sub_;
  ros::Duration scanner_check_interval_;
  ros::Timer check_scanner_timer_;
  ros::Time latest_scan_received_ts_;
  tf::TransformListener tf_;
  tf::StampedTransform scanner_to_footprint_tf_;
  int map_type_;
  int max_beams_;
  int resample_interval_;
  int resample_count_;
  bool first_octomap_received_;
  bool occupancy_bounds_received_;
  bool first_map_only_;
  bool wait_for_occupancy_map_;
  bool force_update_;  // used to temporarily let amcl update samples even when no motion occurs...
  double scanner_height_;
  double gompertz_a_;
  double gompertz_b_;
  double gompertz_c_;
  double gompertz_input_shift_;
  double gompertz_input_scale_;
  double gompertz_output_shift_;
  double sensor_likelihood_max_dist_;
  double off_map_factor_;
  double non_free_space_factor_;
  double non_free_space_radius_;
  double z_hit_, z_short_, z_max_, z_rand_, sigma_hit_, lambda_short_;
  double global_localization_off_map_factor_;
  double global_localization_non_free_space_factor_;
  bool global_localization_active_;
};
}  // namespace amcl

#endif // AMCL_NODE_NODE_3D_H
