/*
 *  Player - One Hell of a Robot Server
 *  Copyright (C) 2000  Brian Gerkey et al.
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
///////////////////////////////////////////////////////////////////////////
//
// Desc: 2D LIDAR sensor model for AMCL
// Author: Andrew Howard
// Maintainter: Tyler Buchman (tyler_buchman@jabil.com)
//
///////////////////////////////////////////////////////////////////////////

#ifndef AMCL_PLANAR_SCANNER_H
#define AMCL_PLANAR_SCANNER_H

#include "sensors/sensor.h"

#include <stddef.h>

#include <vector>

#include "map/occupancy_map.h"
#include "pf/particle_filter.h"
#include "pf/pf_vector.h"

namespace amcl
{

typedef enum
{
  PLANAR_MODEL_BEAM,
  PLANAR_MODEL_LIKELIHOOD_FIELD,
  PLANAR_MODEL_LIKELIHOOD_FIELD_PROB,
  PLANAR_MODEL_LIKELIHOOD_FIELD_GOMPERTZ,
} PlanarModelType;

// Planar sensor data
class PlanarData : public SensorData
{
  public:
    PlanarData () {ranges_=NULL;};
    virtual ~PlanarData() {delete [] ranges_;};
    // Planar range data (range, bearing tuples)
    int range_count_;
    double range_max_;
    double (*ranges_)[2];
};

// Planar sensor model
class PlanarScanner : public Sensor
{
  // Default constructor
  public:
    PlanarScanner(size_t max_beams, OccupancyMap* map);
    ~PlanarScanner();

    void setModelBeam(double z_hit,
                      double z_short,
                      double z_max,
                      double z_rand,
                      double sigma_hit,
                      double labda_short);

    void setModelLikelihoodField(double z_hit,
                                 double z_rand,
                                 double sigma_hit,
                                 double max_occ_dist);

    //a more probabilistically correct model - also with the option to do beam skipping
    void setModelLikelihoodFieldProb(double z_hit,
					                 double z_rand,
					                 double sigma_hit,
					                 double max_occ_dist, 
					                 bool do_beamskip, 
					                 double beam_skip_distance, 
					                 double beam_skip_threshold, 
					                 double beam_skip_error_threshold);

    void setModelLikelihoodFieldGompertz(double z_hit,
                                         double z_rand,
                                         double sigma_hit,
                                         double max_occ_dist,
                                         double gompertz_a,
                                         double gompertz_b,
                                         double gompertz_c,
                                         double input_shift,
                                         double input_scale,
                                         double output_shift);

    // Set factors related to a poses position on the map.
    // off_map_factor: factor to multiply a sample weight by when map out of bounds.
    // non_free_space_factor: factor to multiply by when not in free space
    // non_free_space_radius: non_free_space_factor is interploated up to 1.0 based on this parameter.
    //   The formula is non_free_space_factor += (1.0 - non_free_space_factor) * (distance_to_non_free_space / radius)
    void setMapFactors(double off_map_factor,
                       double non_free_space_factor,
                       double non_free_space_radius);

    // Update the filter based on the sensor model.  Returns true if the
    // filter has been updated.
    virtual bool updateSensor(ParticleFilter *pf, SensorData *data);

    // Update a sample set based on the sensor model.
    // Returns total weights of particles, or 0.0 on failure.
    static double applyModelToSampleSet(SensorData *data, PFSampleSet *set);

    // Set the scanner's pose after construction
    void setPlanarScannerPose(PFVector& scanner_pose) {
      this->planar_scanner_pose_ = scanner_pose;
    }

    // Apply gompertz transform function to given input
    double applyGompertz( double p );


  private:
    // Determine the probability for the given pose
    static double calcBeamModel(PlanarData *data, PFSampleSet* set);

    // Determine the probability for the given pose
    static double calcLikelihoodFieldModel(PlanarData *data, PFSampleSet* set);

    // Determine the probability for the given pose - more probablistic model 
    static double calcLikelihoodFieldModelProb(PlanarData *data, PFSampleSet* set);

    // Determine the probability for the given pose and apply a Gompertz function
    static double calcLikelihoodFieldModelGompertz(PlanarData *data, PFSampleSet* set);

    void reallocTempData(int max_samples, int max_obs);

    PlanarModelType model_type_;

    // The occupancy map
    OccupancyMap *map_;

    // Planar scanner offset relative to robot
    PFVector planar_scanner_pose_;

    // Max beams to consider
    int max_beams_;

    // Beam skipping parameters (used by LikelihoodFieldModelProb model)
    bool do_beamskip_; 
    double beam_skip_distance_; 
    double beam_skip_threshold_; 
    //threshold for the ratio of invalid beams - at which all beams are integrated to the likelihoods 
    //this would be an error condition 
    double beam_skip_error_threshold_;

    //temp data that is kept before observations are integrated to each particle (requried for beam skipping)
    int max_samples_;
    int max_obs_;
    double **temp_obs_;

    // Scanner model params
    // Mixture params for the components of the model; must sum to 1
    double z_hit_;
    double z_short_;
    double z_max_;
    double z_rand_;
    // Stddev of Gaussian model for laser hits.
    double sigma_hit_;
    // Decay rate of exponential model for short readings.
    double lambda_short_;

    // Parameters for applying Gompertz function to sample weights
    double gompertz_a_;
    double gompertz_b_;
    double gompertz_c_;
    double input_shift_;
    double input_scale_;
    double output_shift_;

    double off_map_factor_;
    double non_free_space_factor_;
    double non_free_space_radius_;

    // Vector to store converted map coordinates.
    // Making this a class variable reduces the number of
    // times we need to create an instance of this vector.
    std::vector<int> map_vec_;
};

}

#endif