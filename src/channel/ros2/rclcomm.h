/*
 * @Author: chengyang chengyangkj@outlook.com
 * @Date: 2023-04-20 15:46:29
 * @LastEditors: chengyangkj chengyangkj@qq.com
 * @LastEditTime: 2023-10-07 14:16:09
 * @FilePath: /ros_qt5_gui_app/include/rclcomm.h
 * @Description: 这是默认设置,请设置`customMade`, 打开koroFileHeader查看配置
 * 进行设置: https://github.com/OBKoro1/koro1FileHeader/wiki/%E9%85%8D%E7%BD%AE
 */
#ifndef RCLCOMM_H
#define RCLCOMM_H
#include "sensor_msgs/msg/image.hpp"

#include <cv_bridge/cv_bridge.h>
#include <rclcpp/rclcpp.hpp>
#include <rclcpp_action/rclcpp_action.hpp>
#include <agt_navigation_interfaces/action/navigate_to.hpp>
#include <agt_navigation_interfaces/msg/navigation_health.hpp>
#include <agt_mission_interfaces/action/execute_route.hpp>
#include <agt_mission_interfaces/msg/mission_state.hpp>
#include <std_srvs/srv/trigger.hpp>
#include "algorithm.h"
#include "geometry_msgs/msg/pose_stamped.hpp"
#include "geometry_msgs/msg/pose_with_covariance_stamped.hpp"
#include "geometry_msgs/msg/twist.hpp"
#include "geometry_msgs/msg/polygon_stamped.hpp"
#include "nav_msgs/msg/occupancy_grid.hpp"
#include "nav_msgs/msg/odometry.hpp"
#include "nav_msgs/msg/path.hpp"
#include "point_type.h"
#include "sensor_msgs/msg/battery_state.hpp"
#include "sensor_msgs/msg/laser_scan.hpp"
#include "diagnostic_msgs/msg/diagnostic_array.hpp"
#include "std_msgs/msg/int32.hpp"
#include "tf2/LinearMath/Quaternion.h"
#include "tf2_geometry_msgs/tf2_geometry_msgs.h"
#include "tf2_ros/buffer.h"
#include "tf2_ros/transform_listener.h"
#include "virtual_channel_node.h"
#include "topology_msgs/msg/topology_map.hpp"
#include "core/framework/framework.h"

class rclcomm : public VirtualChannelNode {
 public:
  rclcomm();
  ~rclcomm() override = default;

 private:
  void recv_callback(const std_msgs::msg::Int32::SharedPtr msg);
  void map_callback(const nav_msgs::msg::OccupancyGrid::SharedPtr msg);
  void odom_callback(const nav_msgs::msg::Odometry::SharedPtr msg);
  void localCostMapCallback(const nav_msgs::msg::OccupancyGrid::SharedPtr msg);
  void globalCostMapCallback(const nav_msgs::msg::OccupancyGrid::SharedPtr msg);
  void laser_callback(const sensor_msgs::msg::LaserScan::SharedPtr msg);
  void path_callback(const nav_msgs::msg::Path::SharedPtr msg);
  void BatteryCallback(const sensor_msgs::msg::BatteryState::SharedPtr msg);
  void diagnostic_callback(const diagnostic_msgs::msg::DiagnosticArray::SharedPtr msg);
  void getRobotPose();
  void local_path_callback(const nav_msgs::msg::Path::SharedPtr msg);
  void robotFootprintCallback(const geometry_msgs::msg::PolygonStamped::SharedPtr msg);
  void topologyMapCallback(const topology_msgs::msg::TopologyMap::SharedPtr msg);

 public:
  bool Start() override;
  bool Stop() override;
  void Process() override;
  std::string Name() override { return "ROS2"; };
  void PubRelocPose(const basic::RobotPose &pose);
  void PubNavGoal(const basic::RobotPose &pose);
  void PubRobotSpeed(const basic::RobotSpeed &speed);
  basic::RobotPose getTransform(std::string from, std::string to);
  TopologyMap ConvertFromRosMsg(const topology_msgs::msg::TopologyMap::SharedPtr msg);
  topology_msgs::msg::TopologyMap ConvertToRosMsg(const TopologyMap& topology_map);

 private:
  rclcpp::Publisher<geometry_msgs::msg::Twist>::SharedPtr speed_publisher_;
  using NavigateTo = agt_navigation_interfaces::action::NavigateTo;
  using ExecuteRoute = agt_mission_interfaces::action::ExecuteRoute;
  rclcpp_action::Client<NavigateTo>::SharedPtr navigation_client_;
  rclcpp_action::Client<ExecuteRoute>::SharedPtr mission_client_;
  rclcpp::Client<std_srvs::srv::Trigger>::SharedPtr mission_stop_client_;
  rclcpp::Subscription<agt_mission_interfaces::msg::MissionState>::SharedPtr
      mission_state_subscriber_;
  rclcpp::Subscription<agt_navigation_interfaces::msg::NavigationHealth>::SharedPtr
      navigation_health_subscriber_;
  std::atomic_bool navigation_ready_{false};
  std::atomic<int64_t> navigation_health_ms_{0};
  void SubmitMissionRoute(const std::vector<TopologyMap::PointInfo> &points);
  void StopMissionRoute();
  rclcpp::Subscription<nav_msgs::msg::OccupancyGrid>::SharedPtr map_subscriber_;
  rclcpp::Subscription<nav_msgs::msg::OccupancyGrid>::SharedPtr
      local_cost_map_subscriber_;
  rclcpp::Subscription<nav_msgs::msg::OccupancyGrid>::SharedPtr
      global_cost_map_subscriber_;
  rclcpp::Subscription<sensor_msgs::msg::LaserScan>::SharedPtr
      laser_scan_subscriber_;
  rclcpp::Subscription<sensor_msgs::msg::BatteryState>::SharedPtr
      battery_state_subscriber_;
  rclcpp::Subscription<diagnostic_msgs::msg::DiagnosticArray>::SharedPtr
      diagnostic_subscriber_;
  rclcpp::Subscription<nav_msgs::msg::Odometry>::SharedPtr odometry_subscriber_;
  rclcpp::Subscription<nav_msgs::msg::Path>::SharedPtr local_path_subscriber_;
  rclcpp::Subscription<nav_msgs::msg::Path>::SharedPtr global_path_subscriber_;
  rclcpp::Subscription<geometry_msgs::msg::PolygonStamped>::SharedPtr
      robot_footprint_subscriber_;
  rclcpp::Subscription<topology_msgs::msg::TopologyMap>::SharedPtr
      topology_map_subscriber_;
  rclcpp::Publisher<topology_msgs::msg::TopologyMap>::SharedPtr
      topology_map_update_publisher_;
  std::vector<rclcpp::Subscription<sensor_msgs::msg::Image>::SharedPtr> image_subscriber_list_;
  std::unique_ptr<tf2_ros::Buffer> tf_buffer_;
  std::shared_ptr<tf2_ros::TransformListener> transform_listener_;
  std::shared_ptr<rclcpp::Node> node;
  basic::OccupancyMap occ_map_;
  basic::RobotPose m_currPose;
  rclcpp::executors::MultiThreadedExecutor *m_executor;
  rclcpp::CallbackGroup::SharedPtr callback_group_laser;
  rclcpp::CallbackGroup::SharedPtr callback_group_other;
  std::atomic_bool init_flag_{false};

 private:
};

#endif  // RCLCOMM_H
