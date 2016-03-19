#include <iostream>
#include <pcl/io/pcd_io.h>
#include <pcl/point_types.h>
#include "icp.h"

using namespace pcl;
const float epsilon = 1e-4;

int main(int argc, char* argv[]) {
  std::vector<std::vector<float>> ref_pts = {
     {8.1472, 0.9754, 1.5761},
     {9.0579, 2.7850, 9.7059},
     {1.2699, 5.4688, 9.5717},
     {9.1338, 9.5751, 4.8538},
     {6.3236, 9.6489, 8.0028}
  };
  std::vector<std::vector<float>> src_pts = {
    { 8.8294,  3.6256,  2.4910},
    {10.8155,  4.5095, 10.5825},
    { 2.8133,  5.8770, 11.9861},
    { 9.1199, 11.7824,  6.8562},
    { 6.8916, 10.9934, 10.3535}
  };
  std::vector<int> matched = {0, 1, 2, 3, 4};

  PointCloud<PointXYZ> ref_cloud;
  PointCloud<PointXYZ> src_cloud;

  ref_cloud.width = 5;
  ref_cloud.height = 1;
  ref_cloud.is_dense = true;
  ref_cloud.points.resize(ref_cloud.width*ref_cloud.height);

  src_cloud.width = 5;
  src_cloud.height = 1;
  src_cloud.is_dense = true;
  src_cloud.points.resize(src_cloud.width*src_cloud.height);

  for (size_t i=0; i<ref_cloud.points.size(); i++) {
    ref_cloud.points[i].x = ref_pts[i][0];
    ref_cloud.points[i].y = ref_pts[i][1];
    ref_cloud.points[i].z = ref_pts[i][2];
    src_cloud.points[i].x = src_pts[i][0];
    src_cloud.points[i].y = src_pts[i][1];
    src_cloud.points[i].z = src_pts[i][2];
  }

  Eigen::Matrix<float, 4, 4> m = localize(ref_cloud.makeShared(),
      src_cloud.makeShared(), matched);

  bool passed=true;
  for (size_t i=0; i<src_cloud.points.size(); i++) {
    Eigen::Matrix<float, 4, 1> src_v;
    src_v << 0.f, 0.f, 0.f, 1.f;
    src_v.block(0, 0, 3, 1) = src_cloud.points[i].getVector3fMap();
    Eigen::Matrix<float, 4, 1> ref_v;
    ref_v << 0.f, 0.f, 0.f, 1.f;
    ref_v.block(0, 0, 3, 1) = ref_cloud.points[i].getVector3fMap();
    passed = passed && ((ref_v - m*src_v).norm() < epsilon);
  }
  if (!passed) {
    std::cout << ":-( localize failed" << std::endl;
  } else {
    std::cout << ":-D localize passed" << std::endl;
  }

  if (argc < 2) return 0;

  // open point cloud file
  pcl::PointCloud<pcl::PointXYZ>::Ptr real_cloud(new pcl::PointCloud<pcl::PointXYZ>);
  if (pcl::io::loadPCDFile<pcl::PointXYZ>(argv[1], *real_cloud) == -1) {
    std::cout << "Error reading file " << argv[1] << std::endl;
    exit(1);
  }
  std::cout << "Read " << real_cloud->height << " clouds of " << real_cloud->width <<
    " points." << std::endl;

  std::vector<int> cloud_i(real_cloud->width);
  for (size_t i=0; i<cloud_i.size(); i++) {
    cloud_i[i] = i;
  }
  pcl::PointCloud<pcl::PointXYZ> real_ref_cloud(*real_cloud, cloud_i);
  for (size_t i=0; i<cloud_i.size(); i++) {
    cloud_i[i] = i+cloud_i.size();
  }
  pcl::PointCloud<pcl::PointXYZ> real_src_cloud(*real_cloud, cloud_i);

  Eigen::Matrix<float, 4, 4> Trs = Eigen::Matrix<float, 4, 4>::Identity();

  ICP(real_ref_cloud.makeShared(), real_src_cloud.makeShared(), Trs);

  pcl::PointCloud<pcl::PointXYZRGB> out_cloud;
  out_cloud.height = 1;
  out_cloud.width = 307200 * 3;
  out_cloud.is_dense = false;
  out_cloud.resize(307200 * 3);

  for (size_t i=0; i<real_src_cloud.points.size(); i++) {
    out_cloud.points[0*307200 + i].rgb = 0x00ff0000;
    out_cloud.points[0*307200 + i].getVector4fMap() =
      real_ref_cloud.points[i].getVector4fMap();

    out_cloud.points[1*307200 + i].rgb = 0x0000ff00;
    out_cloud.points[1*307200 + i].getVector4fMap() = Trs *
      real_ref_cloud.points[i].getVector4fMap();

    out_cloud.points[2*307200 + i].rgb = 0x000000ff;
    out_cloud.points[2*307200 + i].getVector4fMap() =
      real_src_cloud.points[i].getVector4fMap();
  }

  pcl::io::savePCDFileBinary("registeredclouds.pcd", out_cloud);
}
