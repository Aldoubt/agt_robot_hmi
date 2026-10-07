// AGT modification, 2026-10-07. Local control socket only; no arbitrary shell.
#pragma once
#include <QJsonDocument>
#include <QJsonObject>
#include <QLocalSocket>
#include <QObject>
#include <QTimer>
#include <functional>
#include <memory>

class FieldClient : public QObject {
 public:
  using Callback = std::function<void(QJsonObject)>;
  explicit FieldClient(QObject *parent = nullptr) : QObject(parent) {}
  void request(QJsonObject request, Callback callback) {
    auto socket = new QLocalSocket(this);
    auto timer = new QTimer(socket);
    timer->setSingleShot(true);
    auto buffer = std::make_shared<QByteArray>();
    auto done = std::make_shared<bool>(false);
    auto finish = [socket, callback, done](QJsonObject response) {if(*done)return;*done=true;callback(response);socket->deleteLater(); };
    connect(timer, &QTimer::timeout, socket, [finish]() { finish({{"ok", false}, {"error", "Runtime request timeout"}}); });
    connect(socket, &QLocalSocket::connected, socket, [socket, request]() { socket->write(QJsonDocument(request).toJson(QJsonDocument::Compact) + "\n"); });
    connect(socket, &QLocalSocket::readyRead, socket, [socket, buffer, finish]() {
      buffer->append(socket->readAll());
      if (buffer->size() > 1048576) {
        finish({{"ok", false}, {"error", "Runtime response too large"}});
        return;
      }
      int end = buffer->indexOf('\n');
      if (end < 0) return;
      QJsonParseError error;
      auto doc = QJsonDocument::fromJson(buffer->left(end), &error);
      finish(error.error == QJsonParseError::NoError ? doc.object() : QJsonObject{{"ok", false}, {"error", "Invalid runtime response"}});
    });
    connect(socket, QOverload<QLocalSocket::LocalSocketError>::of(&QLocalSocket::errorOccurred), socket, [finish, socket](auto) { finish({{"ok", false}, {"error", socket->errorString()}}); });
    timer->start(request.value("command").toString() == "DOCTOR" ? 90000 : 10000);
    socket->connectToServer(qEnvironmentVariable("AGT_FIELD_SOCKET"));
  }
};
