#pragma once

#include "../dao/base/entity.hpp"
#include "../dao/base/repository.hpp"

#include "../module/connection.hpp"

#include <fmt/core.h>

#include <spdlog/logger.h>

#include <memory>

namespace chat::service {

class BaseService {
public:
  using R = std::shared_ptr<dao::Base>;
  using E = std::shared_ptr<dao::Base>;
  using L = std::shared_ptr<spdlog::logger>;
  using CN = std::shared_ptr<module::Connection>;
  using RP = std::shared_ptr<dao::BaseRepository>;

protected:
  L serverLogger;
  CN conn;

  BaseService(L serverLogger, CN conn)
      : serverLogger(serverLogger), conn(conn) {}
  BaseService() = delete;

  // For polymorphism, at lease one virtual method is required.
  virtual ~BaseService() = default;
};
} // namespace chat::service
