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
#include <QScrollArea>
#include <QTabWidget>
#include <QTimer>
#include <QVBoxLayout>
#include <QWidget>
#include "widgets/field_client.h"
#include "widgets/field_labels.h"
#include "widgets/nav_goal_table_view.h"

class FieldPanel : public QWidget {
  Q_OBJECT
 signals:
  void mapActivated(const QString& path);

 public:
  explicit FieldPanel(NavGoalTableView* table, QWidget* parent = nullptr) : QWidget(parent), table_(table), client_(this) {
    setObjectName("agtFieldPanel");
    setMinimumWidth(380);
    setStyleSheet(R"(
      QWidget#agtFieldPanel { background: #f5f5f5; color: #333333; }
      QLabel { color: #333333; background: transparent; }
      QLabel#fieldTitle { color: #1976d2; font-size: 16px; font-weight: 600; }
      QLabel#fieldSummary { background: white; border: 1px solid #e0e0e0;
                            border-radius: 6px; padding: 10px; }
      QTabWidget::pane { border: 1px solid #e0e0e0; background: white; border-radius: 6px; }
      QTabBar::tab { background: #f5f5f5; color: #555555; padding: 8px 10px;
                    border: 1px solid #e0e0e0; }
      QTabBar::tab:selected { background: white; color: #1976d2; }
      QScrollArea, QScrollArea > QWidget > QWidget { background: white; border: none; }
      QPushButton { background: #1976d2; color: white; border: none; border-radius: 6px;
                    padding: 8px 12px; font-weight: 500; }
      QPushButton:hover { background: #1565c0; }
      QPushButton:pressed { background: #0d47a1; }
      QPushButton:disabled { background: #e0e0e0; color: #999999; }
      QPushButton#stopAll { background: #c62828; }
      QPushButton#stopAll:hover { background: #b71c1c; }
      QLineEdit, QComboBox, QPlainTextEdit { background: white; color: #333333;
        border: 1px solid #e0e0e0; border-radius: 4px; padding: 6px; }
    )");
    auto layout = new QVBoxLayout(this);
    layout->setContentsMargins(12, 12, 12, 12);
    layout->setSpacing(10);
    auto title = new QLabel("AGT YHS 控制中心");
    title->setObjectName("fieldTitle");
    layout->addWidget(title);
    status_ = new QLabel("运行服务：离线");
    status_->setObjectName("fieldSummary");
    status_->setWordWrap(true);
    layout->addWidget(status_);
    auto tabs = new QTabWidget(this);
    layout->addWidget(tabs);
    auto system = page(tabs, "系统");
    devices_ = new QLabel();
    devices_->setWordWrap(true);
    system->addWidget(devices_);
    button(system, "连接测试 / 启动检查", "PREFLIGHT");
    button(system, "启动 MID360", "START_SENSOR");
    button(system, "停止 MID360", "STOP_SENSOR");
    button(system, "启用 CAN", "CAN_UP");
    button(system, "停用 CAN", "CAN_DOWN");
    button(system, "返回空闲模式", "IDLE");
    auto manual = new QPushButton("手动控制（运动保护）");
    system->addWidget(manual);
    connect(manual, &QPushButton::clicked, this, [this]() { client_.request({{"command", "CONTROL_MODE"}, {"mode", "manual"}}, [this](QJsonObject r) { details_->setPlainText(QJsonDocument(r).toJson()); if (!r["ok"].toBool()) QMessageBox::warning(this, "AGT 控制中心", r["error"].toString()); }); });
    auto automatic = new QPushButton("返回导航控制");
    system->addWidget(automatic);
    connect(automatic, &QPushButton::clicked, this, [this]() { client_.request({{"command", "CONTROL_MODE"}, {"mode", "navigation"}}, [this](QJsonObject r) { details_->setPlainText(QJsonDocument(r).toJson()); if (!r["ok"].toBool()) QMessageBox::warning(this, "AGT 控制中心", r["error"].toString()); }); });
    auto mapping = page(tabs, "建图");
    auto form = new QFormLayout();
    map_id_ = new QLineEdit("field_map");
    version_ = new QLineEdit("1");
    form->addRow("地图包编号", map_id_);
    form->addRow("版本", version_);
    mapping->addLayout(form);
    auto bundles = new QComboBox();
    mapping->addWidget(bundles);
    auto refresh = new QPushButton("刷新地图包");
    mapping->addWidget(refresh);
    connect(refresh, &QPushButton::clicked, this, [this, bundles]() {
      client_.request({{"command", "LIST_MAPS"}}, [this, bundles](QJsonObject response) {
        if (!response["ok"].toBool()) {
          QMessageBox::warning(this, "AGT 控制中心", response["error"].toString());
          return;
        }
        bundles->clear();
        for (auto item : response["result"].toArray()) {
          auto map = item.toObject();
          bundles->addItem(map["map_bundle_id"].toString() + " / " + map["map_version"].toString() + " (" + fieldLabel(map["status"].toString()) + ")", map);
        }
      });
    });
    connect(bundles, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this, bundles](int index) {
      if (index < 0) return;
      auto map = bundles->itemData(index).toJsonObject();
      map_id_->setText(map["map_bundle_id"].toString());
      version_->setText(map["map_version"].toString());
    });
    button(mapping, "启动检查", "PREFLIGHT");
    button(mapping, "开始建图", "START_MAPPING");
    button(mapping, "停止并生成地图", "STOP_MAPPING");
    button(mapping, "审核地图（MapStudio）", "REVIEW_MAP");
    button(mapping, "确认并封存地图包", "CONFIRM_MAP");
    button(mapping, "激活地图", "ACTIVATE_MAP");
    mapping_status_ = new QLabel("建图：已停止");
    mapping_status_->setWordWrap(true);
    mapping->addWidget(mapping_status_);
    auto navigation = page(tabs, "导航");
    button(navigation, "启动定位 / 进入导航模式", "START_NAVIGATION");
    route_id_ = new QLineEdit("field_route");
    navigation->addWidget(new QLabel("路线编号"));
    navigation->addWidget(route_id_);
    auto edit = new QHBoxLayout();
    navigation->addLayout(edit);
    for (auto name : {"Add", "Delete", "Up", "Down"}) {
      auto b = new QPushButton(fieldLabel(name));
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
    button(navigation, "保存路线", "SAVE_ROUTE");
    button(navigation, "加载路线", "LOAD_ROUTE");
    start_ = button(navigation, "开始任务", "START");
    button(navigation, "暂停", "PAUSE");
    button(navigation, "继续", "RESUME");
    button(navigation, "取消", "CANCEL");
    navigation->addWidget(new QLabel("首个航点是路线目标，不会修改机器人定位。\n手动初始位姿仅用于调试或定位回退。"));
    auto recording = page(tabs, "录制");
    button(recording, "开始录制", "START_RECORDING");
    button(recording, "停止录制", "STOP_RECORDING");
    record_status_ = new QLabel();
    record_status_->setWordWrap(true);
    recording->addWidget(record_status_);
    auto diagnostics = page(tabs, "诊断");
    button(diagnostics, "系统检查 / 诊断报告", "DOCTOR");
    details_ = new QPlainTextEdit();
    details_->setReadOnly(true);
    diagnostics->addWidget(details_);
    auto stop = new QPushButton("停止全部任务");
    stop->setObjectName("stopAll");
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
    layout->setContentsMargins(10, 10, 10, 10);
    layout->setSpacing(8);
    layout->setAlignment(Qt::AlignTop);
    auto scroll = new QScrollArea();
    scroll->setWidgetResizable(true);
    scroll->setWidget(w);
    tabs->addTab(scroll, title);
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
        QMessageBox::warning(this, "路线", e.what());
        return;
      }
    }
    client_.request(request, [this, name](QJsonObject response) {
      if (!response["ok"].toBool()) {
        QMessageBox::warning(this, "AGT 控制中心", response["error"].toString());
        return;
      }
      auto result = response["result"].toObject();
      if (name == "LOAD_ROUTE") {
        QString error;
        if (!table_->field_model_->fromRoute(result, table_->field_binding_, &error)) QMessageBox::warning(this, "路线", error);
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
        status_->setText("运行服务离线：" + response["error"].toString());
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
      status_->setText(QString("机器人：YHS %1\n模式：%2｜地图：%3 / %4\n定位：%5｜任务：%6（航点 %7）\n路线：%8｜剩余停留：%9 秒\n%10")
                           .arg(s["mock"].toBool() ? "（模拟）" : "（实车）")
                           .arg(fieldLabel(s["mode"].toString()))
                           .arg(map["map_bundle_id"].toString())
                           .arg(map["map_version"].toString())
                           .arg(fieldLabel(s["localization"].toString()))
                           .arg(fieldLabel(state))
                           .arg(mission["waypoint_index"].toInt() + 1)
                           .arg(mission["route_id"].toString())
                           .arg(mission["dwell_remaining"].toDouble(), 0, 'f', 1)
                           .arg(s["last_error"].toString()));
      QString text;
      QString timestamps;
      auto devices = s["devices"].toObject();
      for (auto it = devices.begin(); it != devices.end(); ++it) {
        auto d = it.value().toObject();
        text += QString("%1：%2｜%3 Hz\n").arg(fieldLabel(it.key())).arg(fieldLabel(d["state"].toString())).arg(d["frequency"].toDouble());
        timestamps += QString("%1 最近时间戳：%2\n").arg(fieldLabel(it.key())).arg(QString::number(d["last_timestamp"].toDouble(), 'f', 3));
      }
      auto sensor = s["sensors"].toObject();
      auto base = s["base"].toObject();
      text += QString("MID360 以太网 / UDP\n网卡：%1\n主机 IP：%2｜传感器 IP：%3\n驱动：Livox｜话题：%8\nCAN 接口：%4｜比特率：%5\nROS1 主节点：%6\n配置目录：%7")
                  .arg(sensor["interface"].toString("待配置"))
                  .arg(sensor["host_ip"].toString("待配置"))
                  .arg(sensor["sensor_ip"].toString("待配置"))
                  .arg(base["can_interface"].toString("待配置"))
                  .arg(base["can_bitrate"].isNull() ? "待配置" : QString::number(base["can_bitrate"].toInt()))
                  .arg(base["ros1_master_uri"].toString("待配置"))
                  .arg(s["profiles"].toString())
                  .arg(s["topics"].toObject()["lidar"].toString());
      devices_->setText(text);
      devices_->setToolTip(timestamps);
      devices_->setMinimumHeight(devices_->heightForWidth(qMax(200, devices_->width())));
      QString stage = s["mapping"].toString();
      mapping_status_->setText("建图：" + fieldLabel(stage) + "\n持续时间：" + QString::number(s["mapping_duration"].toDouble(), 'f', 1) + " 秒\n" + (stage == "READY" ? "三维建图：通过\n定位资产：通过\n二维导航地图：通过\n地图审核：已确认" : "结束建图 → 校验 → 定位资产 → 栅格地图 → 审核 → 就绪"));
      record_status_->setText(QString("状态：%1｜时长：%2 秒\n磁盘可用：%3 GiB\n保存路径：%4").arg(fieldLabel(s["recording"].toString())).arg(s["recording_duration"].toDouble(), 0, 'f', 1).arg(s["disk_free"].toDouble() / 1073741824., 0, 'f', 1).arg(s["bag_path"].toString()));
    });
  }
};
