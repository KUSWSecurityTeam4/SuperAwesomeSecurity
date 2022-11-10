#pragma once

#include <mysqlx/xdevapi.h>

#include <memory>
#include <mutex>
#include <string>

namespace chat::module {

class Connection {
public:
  static std::shared_ptr<Connection>
  getInstance(mysqlx::ClientSettings setting) {
    std::lock_guard<std::mutex> lock(m);
    if (instance == nullptr) {
      instance = std::make_shared<Connection>(setting);
    }
    return instance;
  }
  Connection(mysqlx::ClientSettings setting) {
    client = std::make_unique<mysqlx::Client>(mysqlx::getClient(setting));
  }
  std::unique_ptr<mysqlx::Client> client;

private:
  static std::shared_ptr<Connection> instance;
  static std::mutex m;

  Connection() = delete;
};

std::shared_ptr<Connection> Connection::instance = nullptr;
std::mutex Connection::m{};
} // namespace chat::module
