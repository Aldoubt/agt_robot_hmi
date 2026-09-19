#pragma once

#include <QObject>
#include <QString>
#include <memory>
#include <thread>
#include <rclcpp/rclcpp.hpp>
#include <std_msgs/msg/string.hpp>
#include <agt_robot_interfaces/srv/list_map_packages.hpp>
#include <agt_robot_interfaces/srv/validate_map.hpp>
#include <agt_robot_interfaces/srv/activate_map.hpp>
#include <agt_robot_interfaces/srv/discard_map.hpp>
#include <agt_robot_interfaces/srv/start_map_edit.hpp>
#include <agt_robot_interfaces/srv/publish_map_edit.hpp>
#include <agt_robot_interfaces/srv/cancel_map_edit.hpp>

class MapLifecycleClient : public QObject {
  Q_OBJECT
 public:
  explicit MapLifecycleClient(QObject *parent = nullptr);
  ~MapLifecycleClient() override;
  void Refresh();
  void Validate(const QString &map_id, const QString &version);
  void Activate(const QString &map_id, const QString &version);
  void Discard(const QString &map_id, const QString &version);
  void StartEdit(const QString &map_id, const QString &version);
  void PublishEdit(const QString &session_id, const QString &target_map_id,
                   const QString &target_map_version, bool activate);
  void CancelEdit(const QString &session_id);
  void SetManualControl(bool enabled);

 signals:
  void mapsReceived(const QString &json);
  void operationFinished(bool success, const QString &message, const QString &report);
  void editStarted(bool success, const QString &message, const QString &session_id,
                   const QString &navigation_map_yaml);
  void editPublished(bool success, const QString &message, const QString &map_id,
                     const QString &map_version);
  void editCancelled(bool success, const QString &message);

 private:
  void publishMaps(const agt_robot_interfaces::srv::ListMapPackages::Response::SharedPtr &response);
  rclcpp::Node::SharedPtr node_;
  rclcpp::executors::SingleThreadedExecutor executor_;
  std::thread thread_;
  rclcpp::Client<agt_robot_interfaces::srv::ListMapPackages>::SharedPtr list_client_;
  rclcpp::Client<agt_robot_interfaces::srv::ValidateMap>::SharedPtr validate_client_;
  rclcpp::Client<agt_robot_interfaces::srv::ActivateMap>::SharedPtr activate_client_;
  rclcpp::Client<agt_robot_interfaces::srv::DiscardMap>::SharedPtr discard_client_;
  rclcpp::Client<agt_robot_interfaces::srv::StartMapEdit>::SharedPtr start_edit_client_;
  rclcpp::Client<agt_robot_interfaces::srv::PublishMapEdit>::SharedPtr publish_edit_client_;
  rclcpp::Client<agt_robot_interfaces::srv::CancelMapEdit>::SharedPtr cancel_edit_client_;
  rclcpp::Publisher<std_msgs::msg::String>::SharedPtr control_mode_pub_;
  rclcpp::Subscription<std_msgs::msg::String>::SharedPtr events_sub_;
};
