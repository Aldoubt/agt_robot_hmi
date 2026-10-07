// AGT modification, 2026-10-07. See upstream LICENSE (GPL v2 text).
#pragma once
#include <QJsonArray>
#include <QJsonObject>
#include <QStandardItemModel>
#include <cmath>
#include <stdexcept>

class FieldRouteModel : public QStandardItemModel {
 public:
  explicit FieldRouteModel(QObject *parent = nullptr) : QStandardItemModel(parent) {
    setHorizontalHeaderLabels({"ID", "X", "Y", "Yaw (rad)", "Wait(s)", "Action"});
  }
  void addPoint(QString id, double x = 0, double y = 0, double yaw = 0, double dwell = 0) {
    QList<QStandardItem *> items;
    for (QString text : {id, QString::number(x, 'g', 15), QString::number(y, 'g', 15), QString::number(yaw, 'g', 15), QString::number(dwell, 'g', 15), QString("wait")}) items << new QStandardItem(text);
    items[5]->setEditable(false);
    appendRow(items);
  }
  bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole) override {
    if (role == Qt::EditRole && index.column() >= 1 && index.column() <= 4) {
      bool ok = false;
      double v = value.toDouble(&ok);
      if (!ok || !std::isfinite(v) || (index.column() == 4 && v < 0)) return false;
    }
    if (role == Qt::EditRole && index.column() == 5 && value.toString() != "wait") return false;
    return QStandardItemModel::setData(index, value, role);
  }
  bool movePoint(int row, int direction) {
    int dest = row + direction;
    if (row < 0 || row >= rowCount() || dest < 0 || dest >= rowCount()) return false;
    auto values = takeRow(row);
    insertRow(dest, values);
    return true;
  }
  QJsonObject toRoute(QString id, QJsonObject binding) const {
    QJsonArray points;
    for (int row = 0; row < rowCount(); ++row) {
      QJsonObject p{{"id", item(row, 0)->text()}, {"sequence", row}, {"action", "wait"}};
      const QStringList names = {"x", "y", "yaw", "dwell_seconds"};
      for (int col = 1; col <= 4; ++col) {
        bool ok = false;
        double value = item(row, col)->text().toDouble(&ok);
        if (!ok || !std::isfinite(value) || (col == 4 && value < 0)) throw std::runtime_error("Invalid waypoint numeric value");
        p[names[col - 1]] = value;
      }
      points.append(p);
    }
    return {{"schema_version", 1}, {"route_id", id}, {"map", binding}, {"waypoints", points}};
  }
  bool fromRoute(QJsonObject route, QJsonObject binding, QString *error = nullptr) {
    if (route["schema_version"].toInt() != 1 || route["map"].toObject() != binding || route["waypoints"].toArray().isEmpty()) {
      if (error) *error = "Route map binding/schema mismatch";
      return false;
    }
    QJsonArray points = route["waypoints"].toArray();
    for (const auto &entry : points) {
      auto p = entry.toObject();
      if (p["action"].toString("wait") != "wait" || !p["x"].isDouble() || !p["y"].isDouble() || !p["yaw"].isDouble() || p["dwell_seconds"].toDouble(0) < 0) {
        if (error) *error = "Invalid waypoint";
        return false;
      }
    }
    removeRows(0, rowCount());
    for (const auto &entry : points) {
      auto p = entry.toObject();
      addPoint(p["id"].toString(), p["x"].toDouble(), p["y"].toDouble(), p["yaw"].toDouble(), p["dwell_seconds"].toDouble(0));
    }
    return true;
  }
};
