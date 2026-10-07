#include <QApplication>
#include <QJsonDocument>
#include <iostream>
#include "../../src/app/widgets/field_client.h"
#include "../../src/app/widgets/field_labels.h"
#include "../../src/app/widgets/field_route_model.h"
#define REQUIRE(expr)                                                        \
  do {                                                                       \
    if (!(expr)) {                                                           \
      std::cerr << "failed line " << __LINE__ << ": " << #expr << std::endl; \
      return 1;                                                              \
    }                                                                        \
  } while (0)
int main(int argc, char** argv) {
  QApplication app(argc, argv);
  FieldRouteModel model;
  QJsonObject binding{{"map_bundle_id", "test"}, {"map_version", "1"}, {"bundle_sha256", QString(64, 'a')}};
  model.addPoint("P1", 1, 2, .5, 5);
  model.addPoint("P2", 3, 4, 0, 20);
  model.addPoint("P3", 5, 6, 1, 0);
  REQUIRE(model.rowCount() == 3);
  REQUIRE(model.headerData(4, Qt::Horizontal).toString() == "停留（秒）");
  REQUIRE(model.data(model.index(0, 5)).toString() == "等待");
  REQUIRE(model.item(0, 5)->text() == "wait");
  REQUIRE(fieldLabel("READY") == "就绪");
  for (const auto& state : {"STOPPED", "RUNNING", "FINISH", "PROCESSING", "VERIFY", "LOCALIZATION_ASSET_BUILD", "GRID_BUILD", "REVIEW_REQUIRED", "READY", "CANCELLED", "ERROR"}) {
    REQUIRE(fieldLabel(state) != state);
  }
  REQUIRE(fieldLabel("CONFIG_REQUIRED") == "待配置");
  REQUIRE(fieldLabel("LOCALIZATION_ASSET_BUILD") == "生成定位资产");
  REQUIRE(fieldLabel("FUTURE_STATE") == "FUTURE_STATE");
  REQUIRE(model.setData(model.index(0, 4), 7.5));
  REQUIRE(!model.setData(model.index(0, 4), -1));
  REQUIRE(!model.setData(model.index(0, 3), "NaN"));
  REQUIRE(model.movePoint(0, 1));
  REQUIRE(model.item(1, 0)->text() == "P1");
  REQUIRE(model.toRoute("route", binding)["waypoints"].toArray()[0].toObject()["action"] == "wait");
  auto saved = QJsonDocument(model.toRoute("route", binding)).toJson();
  FieldRouteModel loaded;
  QString error;
  REQUIRE(loaded.fromRoute(QJsonDocument::fromJson(saved).object(), binding, &error));
  REQUIRE(loaded.item(1, 4)->text().toDouble() == 7.5);
  REQUIRE(loaded.rowCount() == 3);
  QJsonObject wrong = binding;
  wrong["map_version"] = "2";
  REQUIRE(!loaded.fromRoute(QJsonDocument::fromJson(saved).object(), wrong, &error));
  REQUIRE(loaded.removeRow(1));
  REQUIRE(loaded.rowCount() == 2);
  REQUIRE(!loaded.movePoint(0, -1));
  auto points = loaded.toRoute("r", binding)["waypoints"].toArray();
  REQUIRE(points[1].toObject()["sequence"].toInt() == 1);
  std::cout << "HMI add/delete/reorder/yaw/dwell/save/load/map binding PASS" << std::endl;
  return 0;
}
