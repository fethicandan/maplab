#include "registration-toolbox/pcl-gicp-controller.h"

namespace regbox {

RegistrationResult PclGeneralizedIcpController::align(
    const resources::PointCloud& target, const resources::PointCloud& source,
    const aslam::Transformation& prior_T_target_source) {
  return controller_.align(target, source, prior_T_target_source);
}

}  // namespace regbox
