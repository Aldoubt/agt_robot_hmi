// Integration probe only: LD_PRELOAD into a Mock HMI with an activated test bundle.
#include <QAbstractButton>
#include <QApplication>
#include <QDebug>
#include <QMainWindow>
#include <QMessageBox>
#include <QTimer>
#include <QToolButton>
#include <cstdlib>
static bool success = false;
static QMainWindow* mainWindow() {
  for (auto w : QApplication::topLevelWidgets())
    if (auto m = qobject_cast<QMainWindow*>(w)) return m;
  return nullptr;
}
static void setup() {
  QTimer::singleShot(0, [] {
    auto scan = new QTimer(qApp);
    QObject::connect(scan, &QTimer::timeout, [] {
      for (auto w : QApplication::topLevelWidgets())
        if (auto box = qobject_cast<QMessageBox*>(w)) {
          if (!box->isVisible()) continue;
          qInfo() << "MAP_SAVE_DIALOG" << box->windowTitle() << box->text();
          if (box->windowTitle() == "确认地图编辑")
            box->button(QMessageBox::Yes)->click();
          else if (box->windowTitle() == "保存成功") {
            success = true;
            box->button(QMessageBox::Ok)->click();
          } else if (box->windowTitle() == "保存失败") {
            box->button(QMessageBox::Ok)->click();
            std::_Exit(3);
          }
        }
    });
    scan->start(100);
    QTimer::singleShot(3500, [] {
      auto m = mainWindow();
      if (!m) std::_Exit(2);
      m->showNormal();
      m->resize(1800, 1000);
      for (auto b : m->findChildren<QToolButton*>())
        if (b->text() == "保存地图") {
          b->click();
          return;
        }
      std::_Exit(4);
    });
    QTimer::singleShot(7000, [] {
      if (!success) std::_Exit(5);
      auto m = mainWindow();
      if (!qEnvironmentVariable("AGT_SAVE_PROBE_SCREENSHOT").isEmpty())
        m->grab().save(qEnvironmentVariable("AGT_SAVE_PROBE_SCREENSHOT"));
      qInfo() << "ACTUAL_QT_SAVE_FLOW_PASS";
      std::_Exit(0);
    });
  });
}
__attribute__((constructor)) static void registration() { qAddPreRoutine(setup); }
