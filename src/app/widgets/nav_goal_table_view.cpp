// AGT field-mode modification, 2026-10-07; retain upstream LICENSE.
#include "widgets/nav_goal_table_view.h"
#include <QComboBox>
#include <QFileDialog>
#include <QHeaderView>
#include <QLabel>
#include <QFile>
#include <QPushButton>
#include <QtConcurrent>
#include <fstream>
#include <nlohmann/json.hpp>
#include "algorithm.h"
#include "config/config_manager.h"
#include "logger/logger.h"
NavGoalTableView::NavGoalTableView(QWidget *_parent_widget)
    : QTableView(_parent_widget) {
  is_task_chain_running_ = false;
  if (!qEnvironmentVariable("AGT_FIELD_SOCKET").isEmpty()) {
    field_model_=new FieldRouteModel(this);table_model_=field_model_;setModel(field_model_);
    horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    setSelectionBehavior(QAbstractItemView::SelectRows);setSelectionMode(QAbstractItemView::SingleSelection);return;
  }
  table_model_ = new QStandardItemModel(this);
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
}
void NavGoalTableView::UpdateSelectPoint(const TopologyMap::PointInfo &point) {
  if (FieldMode()) {
    if (!isEnabled())return;
    if(field_model_->rowCount()==0)AddItem();int row=field_model_->rowCount()-1;
    field_model_->setData(field_model_->index(row,0),QString::fromStdString(point.name));
    field_model_->setData(field_model_->index(row,1),point.x);field_model_->setData(field_model_->index(row,2),point.y);
    field_model_->setData(field_model_->index(row,3),point.theta);return;
  }
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
  if (FieldMode()) {
    int next = 1;
    while (true) {
      const QString id = "P" + QString::number(next++);
      bool found = false;
      for (int row = 0; row < field_model_->rowCount(); ++row)
        if (field_model_->item(row, 0)->text() == id) found = true;
      if (!found) { field_model_->addPoint(id); break; }
    }
    return;
  }
  QComboBox *comboBox = new QComboBox();
  for (auto point : topologyMap_.points) {
    comboBox->addItem(point.name.c_str());
  }
  comboBox->addItem("");
  comboBox->setCurrentText("");
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
  if(FieldMode()){auto client=new FieldClient(this);client->request({{"command","START"}},[this,client](auto response){if(!response["ok"].toBool())qWarning()<<response["error"].toString();client->deleteLater();emit signalTaskFinish();});return;}
  is_task_chain_running_ = true;
  QtConcurrent::run([this, is_loop]() {
    do {
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
        RobotPose target_pose = point.ToRobotPose();
        emit signalSendNavGoal(target_pose);
        RobotPose diff = absoluteDifference(target_pose, robot_pose_);
        while (diff.mod() > 0.2 || fabs(diff.theta) > deg2rad(15)) {
          LOG_INFO("Task chain is running diff:" << diff << " mode:" << diff.mod() << " deg:" << rad2deg(fabs(diff.theta)));
          diff = absoluteDifference(target_pose, robot_pose_);
          if (!is_task_chain_running_) {
            emit signalTaskFinish();
            LOG_INFO("Task chain is stopped");
            return;
          }
          QThread::msleep(100);
        }
        label_status->setText("Finish");
      }
    } while (is_loop);

    LOG_INFO("Task chain is finished");
    emit signalTaskFinish();
  });
}
bool NavGoalTableView::LoadTaskChain(const std::string &name) {
  if(FieldMode()){QFile file(QString::fromStdString(name));if(!file.open(QIODevice::ReadOnly))return false;return field_model_->fromRoute(QJsonDocument::fromJson(file.readAll()).object(),field_binding_);}
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
  if(FieldMode()){QFile file(QString::fromStdString(name));if(!file.open(QIODevice::WriteOnly))return false;file.write(QJsonDocument(field_model_->toRoute("field_route",field_binding_)).toJson());return true;}
  task_chain_.points.clear();
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
  if(FieldMode()){auto client=new FieldClient(this);client->request({{"command","CANCEL"}},[client](auto){client->deleteLater();});return;}
  if (is_task_chain_running_) {
    is_task_chain_running_ = false;
  }
}
void NavGoalTableView::UpdateRobotPose(const RobotPose &pose) {
  robot_pose_ = pose;
}
