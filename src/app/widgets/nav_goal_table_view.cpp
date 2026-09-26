#include "widgets/nav_goal_table_view.h"
#include <QComboBox>
#include <QFileDialog>
#include <QHeaderView>
#include <QLabel>
#include <QPushButton>
#include <QSignalBlocker>
#include <set>
#include <fstream>
#include <nlohmann/json.hpp>
#include "algorithm.h"
#include "config/config_manager.h"
#include "logger/logger.h"

namespace {

void PopulatePointCombo(QComboBox *combo_box,
                        const std::vector<TopologyMap::PointInfo> &points,
                        const QString &selected_name) {
  if (combo_box == nullptr) {
    return;
  }

  QSignalBlocker blocker(combo_box);
  combo_box->clear();
  combo_box->addItem("");

  std::set<std::string> seen_names;
  for (const auto &point : points) {
    if (!point.name.empty() && seen_names.insert(point.name).second) {
      combo_box->addItem(QString::fromStdString(point.name));
    }
  }

  const int selected_index = combo_box->findText(selected_name);
  combo_box->setCurrentIndex(selected_index >= 0 ? selected_index : 0);
}

}  // namespace

NavGoalTableView::NavGoalTableView(QWidget *_parent_widget)
    : QTableView(_parent_widget) {
  table_model_ = new QStandardItemModel();
  setModel(table_model_);
  QStringList table_h_headers;
  table_h_headers << "点位名"
                  << "任务状态"
                  << "删除"
                  << "运行";
  QHeaderView *headerView = new QHeaderView(Qt::Horizontal);
  headerView->setSectionResizeMode(QHeaderView::ResizeToContents);
  headerView->setSelectionBehavior(QAbstractItemView::SelectRows);
  headerView->setCascadingSectionResizes(false);
  setSelectionBehavior(QAbstractItemView::SelectRows);
  setSelectionMode(QAbstractItemView::SingleSelection);
  this->setHorizontalHeader(headerView);
  // 添加数据模型
  table_model_->setHorizontalHeaderLabels(table_h_headers);
  connect(table_model_, &QStandardItemModel::itemChanged, this,
          &NavGoalTableView::onItemChanged);
}

NavGoalTableView::~NavGoalTableView() {}

void NavGoalTableView::onItemChanged(QStandardItem *item) {
  if (item->column() == 0) {
    qDebug() << "点位名: " << item->text();
  } else if (item->column() == 2) {
    qDebug() << "任务状态: " << item->checkState();
  }
}
void NavGoalTableView::UpdateTopologyMap(const TopologyMap &_topology_map) {
  topologyMap_ = _topology_map;
  for (int row = 0; row < table_model_->rowCount(); ++row) {
    auto *combo_box = qobject_cast<QComboBox *>(
        indexWidget(table_model_->index(row, 0)));
    if (combo_box != nullptr) {
      PopulatePointCombo(combo_box, topologyMap_.points, combo_box->currentText());
    }
  }
}
void NavGoalTableView::UpdateSelectPoint(const TopologyMap::PointInfo &point) {
  if (!this->isEnabled())
    return;

  QWidget *widget =
      indexWidget(model()->index(table_model_->rowCount() - 1, 0));
  if (widget) {
    QComboBox *comboBox = static_cast<QComboBox *>(widget);
    if (comboBox->currentText() == "")
      comboBox->setCurrentText(point.name.c_str());
  }
}
void NavGoalTableView::AddItem() {
  QComboBox *comboBox = new QComboBox();
  PopulatePointCombo(comboBox, topologyMap_.points, "");
  QLabel *label_status = new QLabel("None");
  QPushButton *button_remove = new QPushButton("Delete");
  QPushButton *button_run = new QPushButton("Run");
  int row = table_model_->rowCount();

  connect(button_remove, &QPushButton::clicked, [this, row]() {
    QModelIndexList selectedIndexes = selectionModel()->selectedRows();
    if (selectedIndexes.size() == 1) {
      table_model_->removeRow(selectedIndexes[0].row());
    }
  });
  table_model_->insertRow(row);

  setIndexWidget(table_model_->index(row, 0), comboBox);
  setIndexWidget(table_model_->index(row, 1), label_status);
  setIndexWidget(table_model_->index(row, 2), button_remove);
  setIndexWidget(table_model_->index(row, 3), button_run);
}
void NavGoalTableView::StartTaskChain(bool is_loop) {
  if (is_loop) {
    LOG_WARN("Unbounded HMI task-chain loops are disabled in Mission V4");
  }
  std::vector<TopologyMap::PointInfo> points;
  for (int row = 0; row < table_model_->rowCount(); ++row) {
    auto *combo = qobject_cast<QComboBox *>(indexWidget(model()->index(row, 0)));
    auto *label = qobject_cast<QLabel *>(indexWidget(model()->index(row, 1)));
    if (combo == nullptr || label == nullptr) continue;
    auto point = topologyMap_.GetPoint(combo->currentText().toStdString());
    if (point.name.empty()) {
      label->setText("Point Not Found!");
      return;
    }
    label->setText("Queued");
    points.push_back(point);
  }
  if (points.empty()) return;
  is_task_chain_running_ = true;
  emit signalStartRoute(points);
}
bool NavGoalTableView::LoadTaskChain(const std::string &name) {
  // 清空模型
  table_model_->removeRows(0, table_model_->rowCount());
  std::ifstream file(name);
  try {
    nlohmann::json j;
    file >> j;
    task_chain_ = j.get<TaskChain>();
  } catch (const std::exception& e) {
    fprintf(stderr, "Error parsing struct %s\n", e.what());
    file.close();
    return false;
  }
  file.close();
  for (auto point : task_chain_.points) {
    QComboBox *comboBox = new QComboBox();
    bool find_point = false;
    for (auto p : topologyMap_.points) {
      comboBox->addItem(p.name.c_str());
      if (point.name == p.name) {
        find_point = true;
      }
    }
    if (!find_point) {
      LOG_ERROR(
          "Can't find point " << point.name << " in topology map skip this point!");
      delete comboBox;
      continue;
    }
    comboBox->setCurrentText(QString::fromStdString(point.name));
    QLabel *label_status = new QLabel("None");
    QPushButton *button_remove = new QPushButton("Delete");
    QPushButton *button_run = new QPushButton("Run");
    int row = table_model_->rowCount();

    connect(button_remove, &QPushButton::clicked, [this, row]() {
      QModelIndexList selectedIndexes = selectionModel()->selectedRows();
      if (selectedIndexes.size() == 1) {
        table_model_->removeRow(selectedIndexes[0].row());
      }
    });
    table_model_->insertRow(row);

    setIndexWidget(table_model_->index(row, 0), comboBox);
    setIndexWidget(table_model_->index(row, 1), label_status);
    setIndexWidget(table_model_->index(row, 2), button_remove);
    setIndexWidget(table_model_->index(row, 3), button_run);
  }
  return true;
}
bool NavGoalTableView::SaveTaskChain(const std::string &name) {
  for (int row = 0; row < table_model_->rowCount(); ++row) {
    QComboBox *comboBoxName =
        static_cast<QComboBox *>(indexWidget(model()->index(row, 0)));
    QLabel *label_status =
        static_cast<QLabel *>(indexWidget(model()->index(row, 1)));
    label_status->setText("Running");
    TopologyMap::PointInfo point =
        topologyMap_.GetPoint(comboBoxName->currentText().toStdString());
    if (point.name == "") {
      label_status->setText("Point Not Found!");
      continue;
    }
    task_chain_.points.push_back(point);
  }
  nlohmann::json j = task_chain_;
  std::string pretty_json = j.dump(2);
  return Config::ConfigManager::writeStringToFile(name, pretty_json);
}
void NavGoalTableView::StopTaskChain() {
  if (is_task_chain_running_) {
    is_task_chain_running_ = false;
    emit signalStopRoute();
  }
}
void NavGoalTableView::UpdateRobotPose(const RobotPose &pose) {
  robot_pose_ = pose;
}
void NavGoalTableView::SetMissionState(uint32_t index, uint8_t state,
                                       const QString &waypoint) {
  if (index < static_cast<uint32_t>(table_model_->rowCount())) {
    auto *label = qobject_cast<QLabel *>(indexWidget(model()->index(index, 1)));
    if (label != nullptr) {
      label->setText(state == 2 ? "Waiting Task" :
                     state == 3 ? "Paused" :
                     state == 4 ? "Completed" :
                     state == 5 ? "Failed" :
                     state == 6 ? "Canceled" : "Navigating");
    }
  }
  if (state == 4 || state == 5 || state == 6) {
    is_task_chain_running_ = false;
    emit signalTaskFinish();
  }
  (void)waypoint;
}
