#include "map_lifecycle_client.h"
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <atomic>

MapLifecycleClient::MapLifecycleClient(QObject *parent) : QObject(parent) {
  static std::atomic_uint32_t instance{0};
  node_ = std::make_shared<rclcpp::Node>(
      "agt_robot_hmi_map_lifecycle_" + std::to_string(++instance));
  list_client_ = node_->create_client<agt_robot_interfaces::srv::ListMapPackages>("/agt/map/list");
  validate_client_ = node_->create_client<agt_robot_interfaces::srv::ValidateMap>("/agt/map/validate");
  activate_client_ = node_->create_client<agt_robot_interfaces::srv::ActivateMap>("/agt/map/activate");
  discard_client_ = node_->create_client<agt_robot_interfaces::srv::DiscardMap>("/agt/map/discard");
  start_edit_client_ = node_->create_client<agt_robot_interfaces::srv::StartMapEdit>("/agt/map/edit/start");
  publish_edit_client_ = node_->create_client<agt_robot_interfaces::srv::PublishMapEdit>("/agt/map/edit/publish");
  cancel_edit_client_ = node_->create_client<agt_robot_interfaces::srv::CancelMapEdit>("/agt/map/edit/cancel");
  control_mode_pub_ = node_->create_publisher<std_msgs::msg::String>("/agt/control/mode", 10);
  events_sub_ = node_->create_subscription<std_msgs::msg::String>(
      "/agt/map/events", 10, [this](const std_msgs::msg::String::SharedPtr msg) {
        if (msg->data.find("MAP_GENERATED") != std::string::npos ||
            msg->data.find("MAP_ACTIVATED") != std::string::npos) Refresh();
      });
  executor_.add_node(node_);
  thread_ = std::thread([this]() { executor_.spin(); });
}

MapLifecycleClient::~MapLifecycleClient() {
  // A normal HMI shutdown must return velocity ownership to Nav2. If the HMI
  // crashes before this message is sent, the guard remains fail-closed in
  // manual mode and publishes only zero after its command timeout.
  SetManualControl(false);
  executor_.cancel();
  if (thread_.joinable()) thread_.join();
}

void MapLifecycleClient::Refresh() {
  if (!list_client_->service_is_ready()) return;
  using ListClient = rclcpp::Client<agt_robot_interfaces::srv::ListMapPackages>;
  auto future = list_client_->async_send_request(
      std::make_shared<agt_robot_interfaces::srv::ListMapPackages::Request>(),
      [this](ListClient::SharedFuture future) { publishMaps(future.get()); });
  (void)future;
}

void MapLifecycleClient::publishMaps(
    const agt_robot_interfaces::srv::ListMapPackages::Response::SharedPtr &response) {
  QJsonArray rows;
  for (const auto &map : response->packages) {
    QJsonObject row;
    row["map_id"] = QString::fromStdString(map.map_id);
    row["version"] = QString::fromStdString(map.map_version);
    row["status"] = QString::fromStdString(map.status);
    row["active"] = map.active;
    row["valid"] = map.valid;
    row["reason"] = QString::fromStdString(map.reason);
    rows.append(row);
  }
  emit mapsReceived(QString::fromUtf8(QJsonDocument(rows).toJson(QJsonDocument::Compact)));
}

void MapLifecycleClient::Validate(const QString &id, const QString &version) {
  if (!validate_client_->service_is_ready()) { emit operationFinished(false, "Map Manager unavailable", ""); return; }
  auto request = std::make_shared<agt_robot_interfaces::srv::ValidateMap::Request>();
  request->map_id = id.toStdString(); request->map_version = version.toStdString();
  using ValidateClient = rclcpp::Client<agt_robot_interfaces::srv::ValidateMap>;
  validate_client_->async_send_request(request, [this](ValidateClient::SharedFuture future) {
    auto result = future.get();
    emit operationFinished(result->success, QString::fromStdString(result->message), QString::fromStdString(result->report));
  });
}

void MapLifecycleClient::Activate(const QString &id, const QString &version) {
  if (!activate_client_->service_is_ready()) { emit operationFinished(false, "Map Manager unavailable", ""); return; }
  auto request = std::make_shared<agt_robot_interfaces::srv::ActivateMap::Request>();
  request->map_id = id.toStdString(); request->map_version = version.toStdString();
  using ActivateClient = rclcpp::Client<agt_robot_interfaces::srv::ActivateMap>;
  activate_client_->async_send_request(request, [this](ActivateClient::SharedFuture future) {
    auto result = future.get();
    emit operationFinished(result->success, QString::fromStdString(result->message), "");
    if (result->success) Refresh();
  });
}

void MapLifecycleClient::Discard(const QString &id, const QString &version) {
  if (!discard_client_->service_is_ready()) { emit operationFinished(false, "Map Manager unavailable", ""); return; }
  auto request = std::make_shared<agt_robot_interfaces::srv::DiscardMap::Request>();
  request->map_id = id.toStdString(); request->map_version = version.toStdString();
  using DiscardClient = rclcpp::Client<agt_robot_interfaces::srv::DiscardMap>;
  discard_client_->async_send_request(request, [this](DiscardClient::SharedFuture future) {
    auto result = future.get();
    emit operationFinished(result->success, QString::fromStdString(result->message), "");
    if (result->success) Refresh();
  });
}

void MapLifecycleClient::StartEdit(const QString &id, const QString &version) {
  if (!start_edit_client_->service_is_ready()) {
    emit editStarted(false, "Map Manager unavailable", "", "");
    return;
  }
  auto request = std::make_shared<agt_robot_interfaces::srv::StartMapEdit::Request>();
  request->map_id = id.toStdString(); request->map_version = version.toStdString();
  using Client = rclcpp::Client<agt_robot_interfaces::srv::StartMapEdit>;
  start_edit_client_->async_send_request(request, [this](Client::SharedFuture future) {
    auto result = future.get();
    emit editStarted(result->success, QString::fromStdString(result->message),
                     QString::fromStdString(result->session.session_id),
                     QString::fromStdString(result->session.navigation_map_yaml));
  });
}

void MapLifecycleClient::PublishEdit(const QString &session_id, const QString &target_map_id,
                                     const QString &target_map_version, bool activate) {
  if (!publish_edit_client_->service_is_ready()) {
    emit editPublished(false, "Map Manager unavailable", "", "");
    return;
  }
  auto request = std::make_shared<agt_robot_interfaces::srv::PublishMapEdit::Request>();
  request->session_id = session_id.toStdString();
  request->target_map_id = target_map_id.toStdString();
  request->target_map_version = target_map_version.toStdString();
  request->activate = activate;
  using Client = rclcpp::Client<agt_robot_interfaces::srv::PublishMapEdit>;
  publish_edit_client_->async_send_request(request, [this](Client::SharedFuture future) {
    auto result = future.get();
    emit editPublished(result->success, QString::fromStdString(result->message),
                       QString::fromStdString(result->package.map_id),
                       QString::fromStdString(result->package.map_version));
    if (result->success) Refresh();
  });
}

void MapLifecycleClient::CancelEdit(const QString &session_id) {
  if (!cancel_edit_client_->service_is_ready()) {
    emit editCancelled(false, "Map Manager unavailable");
    return;
  }
  auto request = std::make_shared<agt_robot_interfaces::srv::CancelMapEdit::Request>();
  request->session_id = session_id.toStdString();
  using Client = rclcpp::Client<agt_robot_interfaces::srv::CancelMapEdit>;
  cancel_edit_client_->async_send_request(request, [this](Client::SharedFuture future) {
    auto result = future.get();
    emit editCancelled(result->success, QString::fromStdString(result->message));
  });
}

void MapLifecycleClient::SetManualControl(bool enabled) {
  std_msgs::msg::String mode;
  mode.data = enabled ? "manual" : "navigation";
  control_mode_pub_->publish(mode);
}
