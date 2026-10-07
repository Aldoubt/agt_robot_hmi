// AGT modification. Human-facing labels only; wire protocol values stay unchanged.
#pragma once
#include <QHash>
#include <QString>

inline QString fieldLabel(const QString& value) {
  static const QHash<QString, QString> labels{
      {"IDLE", "空闲"}, {"MAPPING", "建图"}, {"NAVIGATION", "导航"}, {"STOPPED", "已停止"}, {"STARTING", "启动中"}, {"RELOCALIZING", "全局重定位中"}, {"READY", "就绪"}, {"DEGRADED", "降级"}, {"LOST", "定位丢失"}, {"ERROR", "错误"}, {"RUNNING", "运行中"}, {"NAVIGATING", "导航中"}, {"DWELLING", "停留中"}, {"PAUSED", "已暂停"}, {"COMPLETED", "已完成"}, {"CANCELLED", "已取消"}, {"ONLINE", "在线"}, {"OFFLINE", "离线"}, {"ACTIVE", "已启用"}, {"DOWN", "未启用"}, {"CONFIG_REQUIRED", "待配置"}, {"CALIBRATION_REQUIRED", "待标定"}, {"FINISH", "结束建图"}, {"PROCESS", "处理中"}, {"VERIFY", "校验中"}, {"LOCALIZATION_ASSET_BUILD", "生成定位资产"}, {"GRID_BUILD", "生成栅格地图"}, {"REVIEW_CONFIRMED", "审核已确认"}, {"REVIEW_REQUIRED", "待审核"}, {"RECORDING", "录制中"}, {"STOPPING", "停止中"}, {"BUILDING", "生成中"}, {"LiDAR", "激光雷达"}, {"IMU", "惯性测量单元"}, {"Wheel odom", "轮速里程计"}, {"Wheel Odom", "轮速里程计"}, {"YHS", "YHS 底盘"}, {"Add", "添加"}, {"Delete", "删除"}, {"Up", "上移"}, {"Down", "下移"}, {"wait", "等待"}};
  return labels.value(value, value);  // Unknown states remain visible for diagnostics.
}
