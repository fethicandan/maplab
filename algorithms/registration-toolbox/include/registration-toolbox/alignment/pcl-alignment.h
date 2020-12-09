#ifndef REGISTRATION_TOOLBOX_ALIGNMENT_PCL_ALIGNMENT_H_
#define REGISTRATION_TOOLBOX_ALIGNMENT_PCL_ALIGNMENT_H_

#include <glog/logging.h>
#include <map-resources/resource-conversion.h>
#include <pcl/filters/voxel_grid.h>
#include <pcl/registration/gicp.h>
#include <pcl/registration/icp.h>

#include "registration-toolbox/alignment/base-alignment.h"
#include "registration-toolbox/common/registration-gflags.h"

namespace regbox {

template <typename T>
using PclPointCloudPtr = typename boost::shared_ptr<pcl::PointCloud<T>>;

template <typename T_alignment, typename T_point>
class PclAlignment : public BaseAlignment<resources::PointCloud> {
 public:
  PclAlignment() {
    CHECK_GT(FLAGS_regbox_pcl_downsample_leaf_size_m, 0);
    voxel_grid_.setLeafSize(
        FLAGS_regbox_pcl_downsample_leaf_size_m,
        FLAGS_regbox_pcl_downsample_leaf_size_m,
        FLAGS_regbox_pcl_downsample_leaf_size_m);
  }
  virtual ~PclAlignment() = default;

 protected:
  RegistrationResult registerCloudImpl(
      const resources::PointCloud& target, const resources::PointCloud& source,
      const aslam::Transformation& prior_T_target_source) override;

 private:
  T_alignment aligner_;
  pcl::VoxelGrid<T_point> voxel_grid_;
};

template <typename T_alignment, typename T_point>
RegistrationResult PclAlignment<T_alignment, T_point>::registerCloudImpl(
    const resources::PointCloud& target, const resources::PointCloud& source,
    const aslam::Transformation& prior_T_target_source) {
  PclPointCloudPtr<T_point> target_pcl(new pcl::PointCloud<T_point>);
  PclPointCloudPtr<T_point> source_pcl(new pcl::PointCloud<T_point>);

  backend::convertPointCloudType(source, source_pcl.get());
  backend::convertPointCloudType(target, target_pcl.get());

  PclPointCloudPtr<T_point> target_ds(new pcl::PointCloud<T_point>);
  PclPointCloudPtr<T_point> source_ds(new pcl::PointCloud<T_point>);

  voxel_grid_.setInputCloud(target_pcl);
  voxel_grid_.filter(*target_ds);
  voxel_grid_.setInputCloud(source_pcl);
  voxel_grid_.filter(*source_ds);

  bool is_converged = true;
  PclPointCloudPtr<T_point> transformed(new pcl::PointCloud<T_point>);
  aligner_.setMaximumIterations(300);
  aligner_.setRANSACIterations(1);
  aligner_.setTransformationEpsilon(0.00001);
  aligner_.setEuclideanFitnessEpsilon(1);
  aligner_.setRANSACOutlierRejectionThreshold(0.05);
  aligner_.setMaxCorrespondenceDistance(0.05);
  aligner_.setUseReciprocalCorrespondences(false);

  try {
    const Eigen::Matrix4f prior =
        prior_T_target_source.getTransformationMatrix().cast<float>();
    aligner_.setInputTarget(target_ds);
    aligner_.setInputSource(source_ds);
    aligner_.align(*transformed, prior);
  } catch (std::exception& e) {
    LOG(ERROR) << "PCL registration failed to align the point clouds.";
    VLOG(3) << "PCL registration error: " << e.what();
    is_converged = false;  // just a procaution.
  }
  const Eigen::Matrix4f T = aligner_.getFinalTransformation();
  const Eigen::MatrixXd cov = Eigen::MatrixXd::Identity(6, 6) * 1;
  resources::PointCloud source_registered = source;
  const auto T_kindr = this->convertEigenToKindr(T.cast<double>());
  source_registered.applyTransformation(T_kindr);
  return RegistrationResult(
      source_registered, cov, T_kindr, aligner_.hasConverged() & is_converged);
}

using IcpAlignment = PclAlignment<
    pcl::IterativeClosestPoint<pcl::PointXYZI, pcl::PointXYZI>, pcl::PointXYZI>;
using GeneralizedIcpAlignment = PclAlignment<
    pcl::GeneralizedIterativeClosestPoint<pcl::PointXYZI, pcl::PointXYZI>,
    pcl::PointXYZI>;

}  // namespace regbox

#endif  // REGISTRATION_TOOLBOX_ALIGNMENT_PCL_ALIGNMENT_H_
