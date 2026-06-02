#include <cmath>
#include <unordered_map>

#include <Eigen/Core>
#include <Eigen/Geometry>
#include <pcl/common/transforms.h>
#include <pcl/filters/voxel_grid.h>
#include <pcl/point_cloud.h>
#include <pcl/point_types.h>
#include <pcl_conversions/pcl_conversions.h>

#include <nav_msgs/msg/odometry.hpp>
#include <rclcpp/rclcpp.hpp>
#include <sensor_msgs/msg/point_cloud2.hpp>

struct VoxelKey {
  int x;
  int y;
  int z;
  bool operator==(const VoxelKey& o) const {
    return x == o.x && y == o.y && z == o.z;
  }
};

struct VoxelKeyHash {
  size_t operator()(const VoxelKey& k) const {
    return (static_cast<size_t>(k.x) * 73856093u) ^
           (static_cast<size_t>(k.y) * 19349663u) ^
           (static_cast<size_t>(k.z) * 83492791u);
  }
};

class VoxelMapNode : public rclcpp::Node {
 public:
  VoxelMapNode() : Node("voxel_map") {
    map_frame_ = declare_parameter<std::string>("map_frame", "world");
    odom_topic_ = declare_parameter<std::string>("odom_topic", "/loop_fusion/odometry_rect");
    cloud_topic_ = declare_parameter<std::string>("cloud_topic", "/camera/depth/points");
    map_topic_ = declare_parameter<std::string>("map_topic", "/voxel_map/occupancy");
    voxel_size_ = declare_parameter<double>("voxel_size", 0.15);
    input_leaf_ = declare_parameter<double>("input_leaf", 0.05);
    window_size_ = declare_parameter<double>("window_size", 50.0);
    publish_hz_ = declare_parameter<double>("publish_hz", 2.0);
    T_body_cam_.matrix() << 0.0, 0.0, 1.0, 0.0, -1.0, 0.0, 0.0, 0.05, 0.0, -1.0, 0.0, 0.0, 0.0,
        0.0, 0.0, 1.0;
    Eigen::Isometry3d Ry = Eigen::Isometry3d::Identity();
    Ry.linear() = Eigen::AngleAxisd(-M_PI / 2.0, Eigen::Vector3d::UnitY()).toRotationMatrix();
    Eigen::Isometry3d Rx = Eigen::Isometry3d::Identity();
    Rx.linear() = Eigen::AngleAxisd(M_PI / 2.0, Eigen::Vector3d::UnitX()).toRotationMatrix();
    T_cloud_rot_ = Ry * Rx;

    sub_odom_ = create_subscription<nav_msgs::msg::Odometry>(
        odom_topic_, rclcpp::QoS(50),
        [this](const nav_msgs::msg::Odometry::SharedPtr msg) { last_odom_ = msg; });

    sub_cloud_ = create_subscription<sensor_msgs::msg::PointCloud2>(
        cloud_topic_, rclcpp::SensorDataQoS(),
        std::bind(&VoxelMapNode::cloudCallback, this, std::placeholders::_1));

    pub_map_ = create_publisher<sensor_msgs::msg::PointCloud2>(map_topic_, rclcpp::QoS(1));

    if (publish_hz_ > 0.0) {
      const auto period = std::chrono::duration<double>(1.0 / publish_hz_);
      timer_ = create_wall_timer(
          std::chrono::duration_cast<std::chrono::nanoseconds>(period),
          std::bind(&VoxelMapNode::publishMap, this));
    }
  }

 private:
  static Eigen::Isometry3d odomToWorld(const nav_msgs::msg::Odometry& odom) {
    Eigen::Isometry3d T = Eigen::Isometry3d::Identity();
    T.translation() =
        Eigen::Vector3d(odom.pose.pose.position.x, odom.pose.pose.position.y,
                        odom.pose.pose.position.z);
    T.linear() = Eigen::Quaterniond(odom.pose.pose.orientation.w, odom.pose.pose.orientation.x,
                                      odom.pose.pose.orientation.y, odom.pose.pose.orientation.z)
                     .toRotationMatrix();
    return T;
  }

  VoxelKey index(const Eigen::Vector3d& p) const {
    const double inv = 1.0 / voxel_size_;
    return {static_cast<int>(std::floor(p.x() * inv)),
            static_cast<int>(std::floor(p.y() * inv)),
            static_cast<int>(std::floor(p.z() * inv))};
  }

  Eigen::Vector3d center(const VoxelKey& k) const {
    return Eigen::Vector3d((k.x + 0.5) * voxel_size_, (k.y + 0.5) * voxel_size_,
                           (k.z + 0.5) * voxel_size_);
  }

  void prune(const Eigen::Vector3d& c) {
    const double half = 0.5 * window_size_;
    for (auto it = voxels_.begin(); it != voxels_.end();) {
      const Eigen::Vector3d p = center(it->first);
      if (std::abs(p.x() - c.x()) > half || std::abs(p.y() - c.y()) > half ||
          std::abs(p.z() - c.z()) > half) {
        it = voxels_.erase(it);
      } else {
        ++it;
      }
    }
  }

  void cloudCallback(const sensor_msgs::msg::PointCloud2::SharedPtr msg) {
    if (!last_odom_) {
      return;
    }
    const nav_msgs::msg::Odometry& odom = *last_odom_;

    const Eigen::Isometry3d T_world_cam = odomToWorld(odom) * T_body_cam_ * T_cloud_rot_;

    pcl::PointCloud<pcl::PointXYZ>::Ptr raw(new pcl::PointCloud<pcl::PointXYZ>());
    pcl::fromROSMsg(*msg, *raw);
    if (raw->empty()) {
      return;
    }

    pcl::PointCloud<pcl::PointXYZ>::Ptr filtered(new pcl::PointCloud<pcl::PointXYZ>());
    if (input_leaf_ > 0.0) {
      pcl::VoxelGrid<pcl::PointXYZ> grid;
      grid.setLeafSize(static_cast<float>(input_leaf_), static_cast<float>(input_leaf_),
                       static_cast<float>(input_leaf_));
      grid.setInputCloud(raw);
      grid.filter(*filtered);
    } else {
      filtered = raw;
    }

    pcl::PointCloud<pcl::PointXYZ> world;
    pcl::transformPointCloud(*filtered, world, T_world_cam.matrix().cast<float>());

    const Eigen::Vector3d robot(odom.pose.pose.position.x, odom.pose.pose.position.y,
                                odom.pose.pose.position.z);
    for (const auto& pt : world.points) {
      if (!std::isfinite(pt.x) || !std::isfinite(pt.y) || !std::isfinite(pt.z)) {
        continue;
      }
      voxels_[index(Eigen::Vector3d(pt.x, pt.y, pt.z))] = 1;
    }
    prune(robot);
  }

  void publishMap() {
    pcl::PointCloud<pcl::PointXYZ> cloud;
    cloud.reserve(voxels_.size());
    for (const auto& kv : voxels_) {
      const Eigen::Vector3d c = center(kv.first);
      cloud.push_back(pcl::PointXYZ(static_cast<float>(c.x()), static_cast<float>(c.y()),
                                    static_cast<float>(c.z())));
    }
    if (cloud.empty()) {
      return;
    }
    sensor_msgs::msg::PointCloud2 out;
    pcl::toROSMsg(cloud, out);
    out.header.stamp = now();
    out.header.frame_id = map_frame_;
    pub_map_->publish(out);
  }

  std::string map_frame_;
  std::string odom_topic_;
  std::string cloud_topic_;
  std::string map_topic_;
  double voxel_size_;
  double input_leaf_;
  double window_size_;
  double publish_hz_;
  Eigen::Isometry3d T_body_cam_{Eigen::Isometry3d::Identity()};
  Eigen::Isometry3d T_cloud_rot_{Eigen::Isometry3d::Identity()};

  nav_msgs::msg::Odometry::SharedPtr last_odom_;
  std::unordered_map<VoxelKey, uint8_t, VoxelKeyHash> voxels_;

  rclcpp::Subscription<nav_msgs::msg::Odometry>::SharedPtr sub_odom_;
  rclcpp::Subscription<sensor_msgs::msg::PointCloud2>::SharedPtr sub_cloud_;
  rclcpp::Publisher<sensor_msgs::msg::PointCloud2>::SharedPtr pub_map_;
  rclcpp::TimerBase::SharedPtr timer_;
};

int main(int argc, char** argv) {
  rclcpp::init(argc, argv);
  rclcpp::spin(std::make_shared<VoxelMapNode>());
  rclcpp::shutdown();
  return 0;
}
