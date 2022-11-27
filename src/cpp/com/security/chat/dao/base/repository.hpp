#pragma once

#include "./entity.hpp"

#include <mysqlx/devapi/table_crud.h>
#include <mysqlx/xdevapi.h>

#include <fmt/core.h>

#include <spdlog/logger.h>

#include <memory>
#include <mutex>

namespace chat::dao {

class BaseRepository {
public:
  using R = std::shared_ptr<Base>;
  using E = std::shared_ptr<Base>;
  using L = std::shared_ptr<spdlog::logger>;
  virtual R findById(mysqlx::Session &session, uint64_t id) = 0;
  virtual R save(mysqlx::Session &session, E entity) = 0;
  virtual R update(mysqlx::Session &session, E entity) = 0;
  virtual bool remove(mysqlx::Session &session, E entity) = 0;

protected:
  std::mutex sessionMutex;
  L repoLogger;
  std::string tableName;

  BaseRepository(L repoLogger, std::string tableName)
      : repoLogger(repoLogger), sessionMutex(std::mutex()),
        tableName(tableName){};
  BaseRepository() = delete;

  static std::string getUnixTimestampFormatter(std::string time) {
    return fmt::v9::format("UNIX_TIMESTAMP({})", time);
  }
  static time_t convertToTimeT(mysqlx::Value value) {
    return time_t(uint32_t(value));
  }

  mysqlx::Table getTable(mysqlx::Session &session, const std::string name) {
    return session.getDefaultSchema().getTable(name, true);
  }
};
} // namespace chat::dao