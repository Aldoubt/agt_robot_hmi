// AGT modification, 2026-10-07. See upstream LICENSE (GPL v2 text).
#pragma once
#include <QComboBox>
#include <QFormLayout>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QTabWidget>
#include <QTimer>
#include <QVBoxLayout>
#include <QWidget>
#include "widgets/field_client.h"
#include "widgets/nav_goal_table_view.h"

class FieldPanel : public QWidget {
  Q_OBJECT
 signals:
  void mapActivated(const QString& path);

 public:
  explicit FieldPanel(NavGoalTableView* table, QWidget* parent = nullptr) : QWidget(parent), table_(table), client_(this) {
    auto layout = new QVBoxLayout(this);
    layout->addWidget(new QLabel("AGT YHS CONTROL"));
    status_ = new QLabel("Runtime OFFLINE");
    status_->setWordWrap(true);
    layout->addWidget(status_);
    auto tabs = new QTabWidget(this);
    layout->addWidget(tabs);
    auto system = page(tabs, "System");
    devices_ = new QLabel();
    devices_->setWordWrap(true);
    system->addWidget(devices_);
    button(system, "Test Connection / Preflight", "PREFLIGHT");
    button(system, "Start MID360", "START_SENSOR");
    button(system, "Stop MID360", "STOP_SENSOR");
    button(system, "Activate CAN", "CAN_UP");
    button(system, "Deactivate CAN", "CAN_DOWN");
    button(system, "IDLE", "IDLE");
    auto manual = new QPushButton("Manual Control (Motion Guard)");
    system->addWidget(manual);
    connect(manual, &QPushButton::clicked, this, [this]() { client_.request({{"command", "CONTROL_MODE"}, {"mode", "manual"}}, [this](QJsonObject r) { details_->setPlainText(QJsonDocument(r).toJson()); if (!r["ok"].toBool()) QMessageBox::warning(this, "AGT", r["error"].toString()); }); });
    auto automatic = new QPushButton("Return to Navigation Control");
    system->addWidget(automatic);
    connect(automatic, &QPushButton::clicked, this, [this]() { client_.request({{"command", "CONTROL_MODE"}, {"mode", "navigation"}}, [this](QJsonObject r) { details_->setPlainText(QJsonDocument(r).toJson()); if (!r["ok"].toBool()) QMessageBox::warning(this, "AGT", r["error"].toString()); }); });
    auto mapping = page(tabs, "Mapping");
    auto form = new QFormLayout();
    map_id_ = new QLineEdit("field_map");
    version_ = new QLineEdit("1");
    form->addRow("Map Bundle ID", map_id_);
    form->addRow("Version", version_);
    mapping->addLayout(form);
    auto bundles = new QComboBox();
    mapping->addWidget(bundles);
    auto refresh = new QPushButton("Refresh Map Bundles");
    mapping->addWidget(refresh);
    connect(refresh, &QPushButton::clicked, this, [this, bundles]() {
      client_.request({{"command", "LIST_MAPS"}}, [this, bundles](QJsonObject response) {
        if (!response["ok"].toBool()) {
          QMessageBox::warning(this, "AGT", response["error"].toString());
          return;
        }
        bundles->clear();
        for (auto item : response["result"].toArray()) {
          auto map = item.toObject();
          bundles->addItem(map["map_bundle_id"].toString() + " / " + map["map_version"].toString() + " (" + map["status"].toString() + ")", map);
        }
      });
    });
    connect(bundles, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this, bundles](int index) {
      if (index < 0) return;
      auto map = bundles->itemData(index).toJsonObject();
      map_id_->setText(map["map_bundle_id"].toString());
      version_->setText(map["map_version"].toString());
    });
    button(mapping, "Preflight", "PREFLIGHT");
    button(mapping, "Start Mapping", "START_MAPPING");
    button(mapping, "Stop & Build Map", "STOP_MAPPING");
    button(mapping, "Review Map (MapStudio)", "REVIEW_MAP");
    button(mapping, "Confirm & Seal Bundle", "CONFIRM_MAP");
    button(mapping, "Activate Map", "ACTIVATE_MAP");
    mapping_status_ = new QLabel("Mapping STOPPED");
    mapping_status_->setWordWrap(true);
    mapping->addWidget(mapping_status_);
    auto navigation = page(tabs, "Navigation");
    button(navigation, "Start Localization / Navigation Mode", "START_NAVIGATION");
    route_id_ = new QLineEdit("field_route");
    navigation->addWidget(new QLabel("Route ID"));
    navigation->addWidget(route_id_);
    auto edit = new QHBoxLayout();
    navigation->addLayout(edit);
    for (auto name : {"Add", "Delete", "Up", "Down"}) {
      auto b = new QPushButton(name);
      edit->addWidget(b);
      connect(b, &QPushButton::clicked, this, [this, name]() {
        auto m = table_->field_model_;
        if (!m) return;
        int row = table_->currentIndex().row();
        if (QString(name) == "Add")
          table_->AddItem();
        else if (QString(name) == "Delete" && row >= 0)
          m->removeRow(row);
        else if (QString(name) == "Up" || QString(name) == "Down") {
          int d = QString(name) == "Up" ? -1 : 1;
          if (m->movePoint(row, d)) table_->selectRow(row + d);
        }
      });
    }
    button(navigation, "Save Route", "SAVE_ROUTE");
    button(navigation, "Load Route", "LOAD_ROUTE");
    start_ = button(navigation, "Start Mission", "START");
    button(navigation, "Pause", "PAUSE");
    button(navigation, "Resume", "RESUME");
    button(navigation, "Cancel", "CANCEL");
    navigation->addWidget(new QLabel("First waypoint is a route target.\n2D Pose Estimate remains a separate debug/fallback localization tool."));
    auto recording = page(tabs, "Recording");
    button(recording, "Start Recording", "START_RECORDING");
    button(recording, "Stop Recording", "STOP_RECORDING");
    record_status_ = new QLabel();
    record_status_->setWordWrap(true);
    recording->addWidget(record_status_);
    auto diagnostics = page(tabs, "Diagnostics");
    button(diagnostics, "Doctor / Report", "DOCTOR");
    details_ = new QPlainTextEdit();
    details_->setReadOnly(true);
    diagnostics->addWidget(details_);
    auto stop = new QPushButton("STOP ALL");
    layout->addWidget(stop);
    connect(stop, &QPushButton::clicked, this, [this]() { command("STOP_ALL"); });
    timer_ = new QTimer(this);
    connect(timer_, &QTimer::timeout, this, [this]() { poll(); });
    timer_->start(500);
    poll();
  }

 private:
  NavGoalTableView* table_;
  FieldClient client_;
  QLabel *status_, *devices_, *mapping_status_, *record_status_;
  QLineEdit *map_id_, *version_, *route_id_;
  QPlainTextEdit* details_;
  QPushButton* start_;
  QTimer* timer_;
  bool polling_ = false;
  QVBoxLayout* page(QTabWidget* tabs, QString title) {
    auto w = new QWidget();
    auto layout = new QVBoxLayout(w);
    tabs->addTab(w, title);
    return layout;
  }
  QPushButton* button(QVBoxLayout* layout, QString label, QString cmd) {
    auto b = new QPushButton(label);
    layout->addWidget(b);
    connect(b, &QPushButton::clicked, this, [this, cmd]() { command(cmd); });
    return b;
  }
  void command(QString name) {
    QJsonObject request{{"command", name}, {"map_bundle_id", map_id_->text()}, {"map_version", version_->text()}, {"route_id", route_id_->text()}};
    if (name == "SAVE_ROUTE") {
      try {
        request["route"] = table_->field_model_->toRoute(route_id_->text(), table_->field_binding_);
      } catch (const std::exception& e) {
        QMessageBox::warning(this, "Route", e.what());
        return;
      }
    }
    client_.request(request, [this, name](QJsonObject response) {
      if (!response["ok"].toBool()) {
        QMessageBox::warning(this, "AGT", response["error"].toString());
        return;
      }
      auto result = response["result"].toObject();
      if (name == "LOAD_ROUTE") {
        QString error;
        if (!table_->field_model_->fromRoute(result, table_->field_binding_, &error)) QMessageBox::warning(this, "Route", error);
      }
      if (name == "DOCTOR" || name == "PREFLIGHT") {
        details_->setPlainText(QJsonDocument(result).toJson(QJsonDocument::Indented));
      }
      poll();
    });
  }
  void poll() {
    if (polling_) return;
    polling_ = true;
    client_.request({{"command", "STATUS"}}, [this](QJsonObject response) {
      polling_ = false;
      if (!response["ok"].toBool()) {
        status_->setText("Runtime OFFLINE: " + response["error"].toString());
        start_->setEnabled(false);
        table_->setEnabled(false);
        return;
      }
      auto s = response["result"].toObject();
      auto map = s["map"].toObject();
      auto mission = s["mission"].toObject();
      if (table_->field_binding_ != map) {
        table_->field_model_->removeRows(0, table_->field_model_->rowCount());
        if (!map.isEmpty()) emit mapActivated(s["bundle_path"].toString() + "/navigation/map.yaml");
      }
      table_->field_binding_ = map;
      QString state = mission["state"].toString();
      bool editable = state != "NAVIGATING" && state != "DWELLING" && state != "PAUSED";
      table_->setEnabled(editable);
      start_->setEnabled(s["localization"].toString() == "READY" && (state == "READY" || state == "COMPLETED" || state == "CANCELLED") && !mission["cancel_pending"].toBool());
      status_->setText(QString("Robot: YHS %1\nMode: %2 | Map: %3 / %4\nLocalization: %5 | Mission: %6 P%7\nRoute: %8 | Dwell remaining: %9 s\n%10")
                           .arg(s["mock"].toBool() ? "MOCK" : "")
                           .arg(s["mode"].toString())
                           .arg(map["map_bundle_id"].toString())
                           .arg(map["map_version"].toString())
                           .arg(s["localization"].toString())
                           .arg(state)
                           .arg(mission["waypoint_index"].toInt() + 1)
                           .arg(mission["route_id"].toString())
                           .arg(mission["dwell_remaining"].toDouble(), 0, 'f', 1)
                           .arg(s["last_error"].toString()));
      QString text;
      auto devices = s["devices"].toObject();
      for (auto it = devices.begin(); it != devices.end(); ++it) {
        auto d = it.value().toObject();
        text += QString("%1: %2 %3 Hz | last: %4\n").arg(it.key()).arg(d["state"].toString()).arg(d["frequency"].toDouble()).arg(d["last_timestamp"].toDouble());
      }
      auto sensor = s["sensors"].toObject();
      auto base = s["base"].toObject();
      text += QString("MID360 Ethernet/UDP\nInterface: %1 | Host: %2 | Sensor: %3\nDriver: Livox | Topic: %8\nCAN: %4 | bitrate: %5\nROS1 master: %6\nProfile: %7")
                  .arg(sensor["interface"].toString("CONFIG_REQUIRED"))
                  .arg(sensor["host_ip"].toString("CONFIG_REQUIRED"))
                  .arg(sensor["sensor_ip"].toString("CONFIG_REQUIRED"))
                  .arg(base["can_interface"].toString("CONFIG_REQUIRED"))
                  .arg(base["can_bitrate"].isNull() ? "CONFIG_REQUIRED" : QString::number(base["can_bitrate"].toInt()))
                  .arg(base["ros1_master_uri"].toString("CONFIG_REQUIRED"))
                  .arg(s["profiles"].toString())
                  .arg(s["topics"].toObject()["lidar"].toString());
      devices_->setText(text);
      QString stage = s["mapping"].toString();
      mapping_status_->setText("Mapping: " + stage + "\nDuration: " + QString::number(s["mapping_duration"].toDouble(), 'f', 1) + " s\n" + (stage == "READY" ? "3D Mapping PASS\nLocalization Assets PASS\n2D Navigation PASS\nMap Review CONFIRMED" : "Finish → Verify → Localization Assets → Grid → Review → Ready"));
      record_status_->setText(QString("%1 | %2 s\nDisk free: %3 GiB\n%4").arg(s["recording"].toString()).arg(s["recording_duration"].toDouble(), 0, 'f', 1).arg(s["disk_free"].toDouble() / 1073741824., 0, 'f', 1).arg(s["bag_path"].toString()));
    });
  }
};
