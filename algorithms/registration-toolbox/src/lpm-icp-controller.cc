#include "registration-toolbox/lpm-icp-controller.h"

namespace regbox {

RegistrationResult LpmIcpController::align(
    const resources::PointCloud& target, const resources::PointCloud& source,
    const aslam::Transformation& prior_T_target_source) {
  return controller_.align(target, source, prior_T_target_source);
}

}  // namespace regbox
