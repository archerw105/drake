#pragma once

#include <limits>
#include <memory>
#include <string>
#include <utility>

#include "drake/common/drake_copyable.h"
#include "drake/multibody/multibody_tree/joints/joint.h"
#include "drake/multibody/multibody_tree/multibody_forces.h"
#include "drake/multibody/multibody_tree/revolute_mobilizer.h"

namespace drake {
namespace multibody {

/// This Joint allows two bodies to rotate relatively to one another around a
/// common axis.
/// That is, given a frame F attached to the parent body P and a frame M
/// attached to the child body B (see the Joint class's documentation),
/// this Joint allows frames F and M to rotate with respect to each other about
/// an axis â. The rotation angle's sign is defined such that child body B
/// rotates about axis â according to the right hand rule, with thumb aligned in
/// the axis direction.
/// Axis â is constant and has the same measures in both frames F and M, that
/// is, `â_F = â_M`.
///
/// @tparam T The scalar type. Must be a valid Eigen scalar.
///
/// Instantiated templates for the following kinds of T's are provided:
/// - double
/// - AutoDiffXd
///
/// They are already available to link against in the containing library.
/// No other values for T are currently supported.
template <typename T>
class RevoluteJoint final : public Joint<T> {
 public:
  DRAKE_NO_COPY_NO_MOVE_NO_ASSIGN(RevoluteJoint)

  template<typename Scalar>
  using Context = systems::Context<Scalar>;

  /// Constructor to create a revolute joint between two bodies so that
  /// frame F attached to the parent body P and frame M attached to the child
  /// body B, rotate relatively to one another about a common axis. See this
  /// class's documentation for further details on the definition of these
  /// frames and rotation angle.
  /// The first three arguments to this constructor are those of the Joint class
  /// constructor. See the Joint class's documentation for details.
  /// The additional parameter `axis` is:
  /// @param[in] axis
  ///   A vector in ℝ³ specifying the axis of revolution for this joint. Given
  ///   that frame M only rotates with respect to F and their origins are
  ///   coincident at all times, the measures of `axis` in either frame F or M
  ///   are exactly the same, that is, `axis_F = axis_M`. In other words,
  ///   `axis_F` (or `axis_M`) is the eigenvector of `R_FM` with eigenvalue
  ///   equal to one.
  ///   This vector can have any length, only the direction is used. This method
  ///   aborts if `axis` is the zero vector.
  RevoluteJoint(const std::string& name,
                const Frame<T>& frame_on_parent, const Frame<T>& frame_on_child,
                const Vector3<double>& axis) :
      Joint<T>(name, frame_on_parent, frame_on_child) {
    const double kEpsilon = std::numeric_limits<double>::epsilon();
    DRAKE_DEMAND(!axis.isZero(kEpsilon));
    axis_ = axis.normalized();
  }

  /// Returns the axis of revolution of `this` joint as a unit vector.
  /// Since the measures of this axis in either frame F or M are the same (see
  /// this class's documentation for frames's definitions) then,
  /// `axis = axis_F = axis_M`.
  const Vector3<double>& get_revolute_axis() const {
    return axis_;
  }

  /// @name Context-dependent value access
  ///
  /// These methods require the provided context to be an instance of
  /// MultibodyTreeContext. Failure to do so leads to a std::logic_error.
  /// @{

  /// Gets the rotation angle of `this` mobilizer from `context`.
  /// @param[in] context
  ///   The context of the MultibodyTree this joint belongs to.
  /// @returns The angle coordinate of `this` joint stored in the `context`.
  const T& get_angle(const Context<T>& context) const {
    return get_mobilizer()->get_angle(context);
  }

  /// Sets the `context` so that the generalized coordinate corresponding to the
  /// rotation angle of `this` joint equals `angle`.
  /// @param[in] context
  ///   The context of the MultibodyTree this joint belongs to.
  /// @param[in] angle
  ///   The desired angle in radians to be stored in `context`.
  /// @returns a constant reference to `this` joint.
  const RevoluteJoint<T>& set_angle(
      Context<T>* context, const T& angle) const {
    get_mobilizer()->set_angle(context, angle);
    return *this;
  }

  /// Gets the rate of change, in radians per second, of `this` joint's
  /// angle (see get_angle()) from `context`.
  /// @param[in] context
  ///   The context of the MultibodyTree this joint belongs to.
  /// @returns The rate of change of `this` joint's angle as stored in the
  /// `context`.
  const T& get_angular_rate(const Context<T>& context) const {
    return get_mobilizer()->get_angular_rate(context);
  }

  /// Sets the rate of change, in radians per second, of this `this` joint's
  /// angle to `theta_dot`. The new rate of change `theta_dot` gets stored in
  /// `context`.
  /// @param[in] context
  ///   The context of the MultibodyTree this joint belongs to.
  /// @param[in] theta_dot
  ///   The desired rate of change of `this` joints's angle in radians per
  ///   second.
  /// @returns a constant reference to `this` joint.
  const RevoluteJoint<T>& set_angular_rate(
      Context<T>* context, const T& angle) const {
    get_mobilizer()->set_angular_rate(context, angle);
    return *this;
  }

  /// @}

  /// Adds into `forces` a given `torque` for `this` joint that is to be applied
  /// about the joint's axis. The torque is defined to be positive according to
  /// the right-hand-rule with the thumb aligned in the direction of `this`
  /// joint's axis. That is, a positive torque causes a positive rotational
  /// acceleration according to the right-hand-rule around the joint's axis.
  ///
  /// @note A torque is the moment of a set of forces whose resultant is zero.
  void AddInTorque(
      const systems::Context<T>& context,
      const T& torque,
      MultibodyForces<T>* forces) const {
    DRAKE_DEMAND(forces != nullptr);
    DRAKE_DEMAND(forces->CheckHasRightSizeForModel(this->get_parent_tree()));
    this->AddInOneForce(context, 0, torque, forces);
  }

 private:
  // Joint<T> override called through public NVI. Therefore arguments were
  // already checked to be valid.
  void DoAddInOneForce(
      const systems::Context<T>&,
      int joint_dof,
      const T& joint_tau,
      MultibodyForces<T>* forces) const override {
    // Right now we assume all the forces in joint_tau go into a single
    // mobilizer.
    DRAKE_DEMAND(joint_dof == 0);
    Eigen::VectorBlock<Eigen::Ref<VectorX<T>>> tau_mob =
        get_mobilizer()->get_mutable_generalized_forces_from_array(
            &forces->mutable_generalized_forces());
    tau_mob(joint_dof) += joint_tau;
  }

  int do_get_num_dofs() const override {
    return 1;
  }

  // Joint<T> overrides:
  std::unique_ptr<typename Joint<T>::BluePrint>
  MakeImplementationBlueprint() const override {
    auto blue_print = std::make_unique<typename Joint<T>::BluePrint>();
    blue_print->mobilizers_.push_back(
        std::make_unique<RevoluteMobilizer<T>>(
            this->get_frame_on_parent(), this->get_frame_on_child(), axis_));
    return std::move(blue_print);
  }

  std::unique_ptr<Joint<double>> DoCloneToScalar(
      const MultibodyTree<double>& tree_clone) const override;

  std::unique_ptr<Joint<AutoDiffXd>> DoCloneToScalar(
      const MultibodyTree<AutoDiffXd>& tree_clone) const override;

  // Make RevoluteJoint templated on every other scalar type a friend of
  // RevoluteJoint<T> so that CloneToScalar<ToAnyOtherScalar>() can access
  // private members of RevoluteJoint<T>.
  template <typename> friend class RevoluteJoint;

  // Friend class to facilitate testing.
  friend class JointTester;

  // Returns the mobilizer implementing this joint.
  // The internal implementation of this joint could change in a future version.
  // However its public API should remain intact.
  const RevoluteMobilizer<T>* get_mobilizer() const {
    // This implementation should only have one mobilizer.
    DRAKE_DEMAND(this->get_implementation().get_num_mobilizers() == 1);
    const RevoluteMobilizer<T>* mobilizer =
        dynamic_cast<const RevoluteMobilizer<T>*>(
            this->get_implementation().mobilizers_[0]);
    DRAKE_DEMAND(mobilizer != nullptr);
    return mobilizer;
  }

  // Helper method to make a clone templated on ToScalar.
  template <typename ToScalar>
  std::unique_ptr<Joint<ToScalar>> TemplatedDoCloneToScalar(
      const MultibodyTree<ToScalar>& tree_clone) const;

  // This is the joint's axis expressed in either M or F since axis_M = axis_F.
  Vector3<double> axis_;
};

}  // namespace multibody
}  // namespace drake