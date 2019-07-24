#ifndef MOCO_MOCOORIENTATIONTRACKINGCOST_H
#define MOCO_MOCOORIENTATIONTRACKINGCOST_H
/* -------------------------------------------------------------------------- *
 * OpenSim Moco: MocoOrientationTrackingCost.h                                *
 * -------------------------------------------------------------------------- *
 * Copyright (c) 2019 Stanford University and the Authors                     *
 *                                                                            *
 * Author(s): Nicholas Bianco                                                 *
 *                                                                            *
 * Licensed under the Apache License, Version 2.0 (the "License"); you may    *
 * not use this file except in compliance with the License. You may obtain a  *
 * copy of the License at http://www.apache.org/licenses/LICENSE-2.0          *
 *                                                                            *
 * Unless required by applicable law or agreed to in writing, software        *
 * distributed under the License is distributed on an "AS IS" BASIS,          *
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.   *
 * See the License for the specific language governing permissions and        *
 * limitations under the License.                                             *
 * -------------------------------------------------------------------------- */

#include "../Common/TableProcessor.h"
#include "../MocoWeightSet.h"
#include "MocoCost.h"

#include <OpenSim/Common/GCVSplineSet.h>
#include <OpenSim/Common/TimeSeriesTable.h>
#include <OpenSim/Simulation/Model/Frame.h>

namespace OpenSim {

using SimTK::Rotation;

/// The squared difference between a model frame's orientation and a reference
/// orientation value, summed over the frames for which a reference is provided,
/// and integrated over the phase. This can be used to track orientation
/// quantities in the model that don't correspond to model degrees of freedom.
/// The reference can be provided as a trajectory of SimTK::Rotation%s
/// representing the orientation reference data, or as a model-compatible states
/// trajectory from which the tracked rotation reference is computed. Both
/// rotation and states references can be provided as a file name to a STO or
/// CSV file (or other file types for which there is a FileAdapter), or
/// programmatically as a TimeSeriesTable_<SimTK::Rotation> (for the rotation
/// reference) or as a scalar TimeSeriesTable (for the states reference).
///
/// This cost requires realization to SimTK::Stage::Position. The cost is
/// computed by creating a SimTK::Rotation between the model frame and the
/// reference data, and then converting the rotation to an angle-axis
/// representation and minimizing the angle value. The angle value is
/// equivalent to the orientation error between the model frame and the
/// reference data, so we only need to minimize this single scalar value per
/// tracked frame, compared to other more complicated approaches which could
/// require multiple minimized error values (e.g. Euler angle errors, etc).
///
/// Tracking problems in direct collocation perform best when tracking smooth
/// data, so it is recommended to filter the data in the reference you provide
/// to the cost.
/// @ingroup mococost
class OSIMMOCO_API MocoOrientationTrackingCost : public MocoCost {
    OpenSim_DECLARE_CONCRETE_OBJECT(MocoOrientationTrackingCost, MocoCost);

public:
    MocoOrientationTrackingCost() { constructProperties(); }
    MocoOrientationTrackingCost(std::string name) : MocoCost(std::move(name)) {
        constructProperties();
    }
    MocoOrientationTrackingCost(std::string name, double weight)
            : MocoCost(std::move(name), weight) {
        constructProperties();
    }

    /// Set directly the rotations of individual frames in ground to be tracked
    /// in the cost. The column labels of the provided reference must
    /// be paths to frames in the model, e.g. `/bodyset/torso`. If the
    /// frame_paths property is empty, all frames with data in this reference
    /// will be tracked. Otherwise, only the frames specified via
    /// setFramePaths() will be tracked. Calling this function clears the values
    /// provided via setStatesReference(), setRotationReference(), or the
    /// `states_reference_file` property, if any.
    void setRotationReferenceFile(const std::string& filepath) {
        set_states_reference(TableProcessor());
        m_rotation_table = TimeSeriesTable_<Rotation>();
        set_rotation_reference_file(filepath);
    }
    /// Each column label must be the path of a valid frame path (see
    /// seeRotationReferenceFile()). Calling this function clears the
    /// `states_reference_file` and `rotation_reference_file` properties or the
    /// table provided via setStatesReference(), if any.
    void setRotationReference(const TimeSeriesTable_<Rotation>& ref) {
        set_states_reference(TableProcessor());
        set_rotation_reference_file("");
        m_rotation_table = ref;
    }
    /// Provide a table containing values of model state variables. These data
    /// are used to create a StatesTrajectory internally, from which the
    /// rotation data for the frames specified in setFramePaths() are computed.
    /// Each column label in the reference must be the path of a state variable,
    /// e.g., `/jointset/ankle_angle_r/value`. Calling this function clears the
    /// table provided via setRotationReference(), or the
    /// `rotation_reference_file` property, if any. The table is not loaded
    /// until the MocoProblem is initialized.
    void setStatesReference(const TableProcessor& ref) {
        set_rotation_reference_file("");
        m_rotation_table = TimeSeriesTable_<Rotation>();
        set_states_reference(std::move(ref));
    }
    /// Set the paths to frames in the model that this cost term will track. The
    /// names set here must correspond to OpenSim::Component%s that derive from
    /// OpenSim::Frame, which includes SimTK::Rotation as an output.
    /// Replaces the frame path set if it already exists.
    void setFramePaths(const std::vector<std::string>& paths) {
        updProperty_frame_paths().clear();
        for (const auto& path : paths) { append_frame_paths(path); }
    }
    /// Set the weight for an individual frame's rotation tracking. If a weight
    /// is already set for the requested frame, then the provided weight
    /// replaces the previous weight. An exception is thrown if a weight
    /// for an unknown frame is provided.
    void setWeight(const std::string& frameName, const double& weight) {
        if (get_rotation_weights().contains(frameName)) {
            upd_rotation_weights().get(frameName).setWeight(weight);
        } else {
            upd_rotation_weights().cloneAndAppend({frameName, weight});
        }
    }
    /// Provide a MocoWeightSet to weight frame rotation tracking in the cost.
    /// Replaces the weight set if it already exists.
    void setWeightSet(const MocoWeightSet& weightSet) {
        upd_rotation_weights() = weightSet;
    }
    /// If no states reference has been provided, this returns an empty
    /// processor.
    const TableProcessor& getStatesReference() const {
        return get_states_reference();
    }
    /// If no rotation reference file has been provided, this returns an empty
    /// string.
    std::string getRotationReferenceFile() const {
        return get_rotation_reference_file();
    }

protected:
    void initializeOnModelImpl(const Model& model) const override;
    int getNumIntegralsImpl() const override { return 1; }
    void calcIntegrandImpl(
            const SimTK::State& state, double& integrand) const override;
    void calcCostImpl(
            const CostInput& input, SimTK::Real& cost) const override {
        cost = input.integral;
    }

private:
    OpenSim_DECLARE_PROPERTY(states_reference, TableProcessor,
            "Trajectories of model state "
            "variables from which tracked rotation data is computed. Column "
            "labels should be model state paths, "
            "e.g., '/jointset/ankle_angle_r/value'");
    OpenSim_DECLARE_PROPERTY(rotation_reference_file, std::string,
            "Path to file (.sto, .csv, ...) containing orientation reference "
            "data to track. Column labels should be paths to frames in the "
            "model, e.g. `/bodyset/torso`.");
    OpenSim_DECLARE_LIST_PROPERTY(frame_paths, std::string,
            "The frames in the model that this cost term will track. "
            "The names set here must correspond to Components that "
            "derive from class Frame, which includes "
            "Rotation as an output.");
    OpenSim_DECLARE_PROPERTY(rotation_weights, MocoWeightSet,
            "Set of weight objects to weight the tracking of "
            "individual "
            "frames' rotations in the cost.");

    void constructProperties() {
        constructProperty_states_reference(TableProcessor());
        constructProperty_rotation_reference_file("");
        constructProperty_frame_paths();
        constructProperty_rotation_weights(MocoWeightSet());
    }

    TimeSeriesTable_<Rotation> m_rotation_table;
    mutable GCVSplineSet m_ref_splines;
    mutable std::vector<SimTK::ReferencePtr<const Frame>> m_model_frames;
    mutable std::vector<double> m_rotation_weights;
};

} // namespace OpenSim

#endif // MOCO_MOCOORIENTATIONTRACKINGCOST_H