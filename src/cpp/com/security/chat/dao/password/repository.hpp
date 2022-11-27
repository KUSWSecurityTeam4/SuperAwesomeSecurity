#pragma once

#include "../base/entity.hpp"
#include "../base/repository.hpp"
#include "./entity.hpp"

#include "../../module/all.hpp"
using namespace chat::module::exception;

#include <mysqlx/xdevapi.h>

#include <fmt/core.h>

#include <exception>
#include <memory>
#include <mutex>
#include <string>

namespace chat::dao {

class PasswordRepository : public BaseRepository {
public:
  static std::shared_ptr<PasswordRepository> getInstance(L repoLogger) {
    std::lock_guard<std::mutex> lock(createMutex);
    if (instance == nullptr) {
      instance = std::make_shared<PasswordRepository>(repoLogger);
    }
    return instance;
  }

  PasswordRepository(L repoLogger)
      : BaseRepository(repoLogger, "chat_password"){};

  R findById(mysqlx::Session &session, uint64_t id) override {
    auto msg = fmt::v9::format("PasswordRepository: findById not implemented");
    repoLogger->error(msg);
    throw NotImplementedException(msg);
  }

  R findByUserId(mysqlx::Session &session, uint64_t userId) {
    try {
      auto tableSelect =
          getTable(session, tableName)
              .select("user_id", "salt", "hashed_pw", "pw_id",
                      getUnixTimestampFormatter("created_at"),
                      getUnixTimestampFormatter("last_modified_at"));
      const auto condition = fmt::v9::format("user_id={}", userId);
      auto result = tableSelect.where(condition).execute();

      if (result.count() != 1) {
        const auto msg = fmt::v9::format(
            "PasswordRepository : more than one rows are selected");
        throw EntityException(msg);
      }
      auto row = result.fetchOne();

      if (row.isNull() != true) {
        auto entity = R(new Password(
            std::uint64_t(row.get(0)), std::string(row.get(1)),
            std::string(row.get(2)), -1, uint64_t(row.get(3)),
            convertToTimeT(row.get(4)), convertToTimeT(row.get(5))));
        return entity;
      } else {
        return nullptr;
      }
    } catch (const std::exception &e) {
      auto msg = fmt::v9::format("PasswordRepository: {}", e.what());
      repoLogger->error(msg);
      throw EntityException(msg);
    }
  }

  R findByCompanyId(mysqlx::Session &session, uint64_t companyId) {
    try {
      auto tableSelect =
          getTable(session, tableName)
              .select("salt", "hashed_pw", "company_id", "pw_id",
                      getUnixTimestampFormatter("created_at"),
                      getUnixTimestampFormatter("last_modified_at"));
      const auto condition = fmt::v9::format("company_id={}", companyId);
      auto result = tableSelect.where(condition).execute();

      if (result.count() != 1) {
        const auto msg = fmt::v9::format(
            "PasswordRepository : more than one rows are selected");
        throw EntityException(msg);
      }
      auto row = result.fetchOne();

      if (row.isNull() != true) {
        auto entity = R(new Password(
            -1, std::string(row.get(0)), std::string(row.get(1)),
            uint64_t(row.get(2)), uint64_t(row.get(3)),
            convertToTimeT(row.get(4)), convertToTimeT(row.get(5))));
        return entity;
      } else {
        return nullptr;
      }
    } catch (const std::exception &e) {
      auto msg = fmt::v9::format("PasswordRepository: {}", e.what());
      repoLogger->error(msg);
      throw EntityException(msg);
    }
  }

  R save(mysqlx::Session &session, E entity) override {
    auto msg = fmt::v9::format("PasswordRepository: save not implemented");
    repoLogger->error(msg);
    throw NotImplementedException(msg);
  }

  R saveWithUserId(mysqlx::Session &session, E entity) {
    std::lock_guard<std::mutex> lock(sessionMutex);
    try {
      auto tableInsert =
          getTable(session, tableName).insert("user_id", "salt", "hashed_pw");
      const auto password = std::dynamic_pointer_cast<Password>(entity);
      const auto row = mysqlx::Row(password->getUserId(), password->getSalt(),
                                   password->getHashedPw());
      const auto result = tableInsert.values(row).execute();
      return findByUserId(session, password->getUserId());
    } catch (const std::exception &e) {
      auto msg = fmt::v9::format("PasswordRepository: {}", e.what());
      repoLogger->error(msg);
      throw EntityException(msg);
    }
  }

  R saveWithCompanyId(mysqlx::Session &session, E entity) {
    std::lock_guard<std::mutex> lock(sessionMutex);
    try {
      auto tableInsert = getTable(session, tableName)
                             .insert("company_id", "salt", "hashed_pw");
      const auto password = std::dynamic_pointer_cast<Password>(entity);
      const auto row =
          mysqlx::Row(password->getCompanyId(), password->getSalt(),
                      password->getHashedPw());
      const auto result = tableInsert.values(row).execute();
      return findByCompanyId(session, password->getCompanyId());
    } catch (const std::exception &e) {
      auto msg = fmt::v9::format("PasswordRepository: {}", e.what());
      repoLogger->error(msg);
      throw EntityException(msg);
    }
  }

  R update(mysqlx::Session &session, E entity) override {
    auto msg = fmt::v9::format("PasswordRepository: update not implemented");
    repoLogger->error(msg);
    throw NotImplementedException(msg);
  }

  R updateOfUserId(mysqlx::Session &session, E entity) {
    std::lock_guard<std::mutex> lock(sessionMutex);
    try {
      auto tableUpdate = getTable(session, tableName).update();
      const auto password = std::dynamic_pointer_cast<Password>(entity);
      const auto condition =
          fmt::v9::format("user_id={}", password->getUserId());
      const auto result =
          tableUpdate.set("salt", password->getSalt())
              .set("hashed_pw", password->getHashedPw())
              .set("last_modified_at",
                   module::convertToLocalTimeString(module::getCurrentTime()))
              .where(condition)
              .execute();
      return findByUserId(session, password->getUserId());
    } catch (const std::exception &e) {
      auto msg = fmt::v9::format("PasswordRepository: {}", e.what());
      repoLogger->error(msg);
      throw EntityException(msg);
    }
  }

  R updateOfCompanyId(mysqlx::Session &session, E entity) {
    std::lock_guard<std::mutex> lock(sessionMutex);
    try {
      auto tableUpdate = getTable(session, tableName).update();
      const auto password = std::dynamic_pointer_cast<Password>(entity);
      const auto condition =
          fmt::v9::format("company_id={}", password->getCompanyId());
      const auto result =
          tableUpdate.set("salt", password->getSalt())
              .set("hashed_pw", password->getHashedPw())
              .set("last_modified_at",
                   module::convertToLocalTimeString(module::getCurrentTime()))
              .where(condition)
              .execute();
      return findByCompanyId(session, password->getCompanyId());
    } catch (const std::exception &e) {
      auto msg = fmt::v9::format("PasswordRepository: {}", e.what());
      repoLogger->error(msg);
      throw EntityException(msg);
    }
  }

  bool remove(mysqlx::Session &session, E entity) override {
    std::lock_guard<std::mutex> lock(sessionMutex);
    try {
      auto tableRemove = getTable(session, tableName).remove();
      const auto password = std::dynamic_pointer_cast<Password>(entity);
      const auto condition = fmt::v9::format("pw_id={}", password->getId());
      const auto result = tableRemove.where(condition).execute();
      return true;
    } catch (const std::exception &e) {
      auto msg = fmt::v9::format("PasswordRepository: {}", e.what());
      repoLogger->error(msg);
      throw EntityException(msg);
    }
  }

private:
  static std::shared_ptr<PasswordRepository> instance;
  static std::mutex createMutex;
  PasswordRepository() = delete;
};

std::shared_ptr<PasswordRepository> PasswordRepository::instance = nullptr;
std::mutex PasswordRepository::createMutex{};
} // namespace chat::dao
