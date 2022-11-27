#pragma once

#include "../base/entity.hpp"
#include "../base/repository.hpp"
#include "./entity.hpp"

#include "../../module/all.hpp"
using namespace chat::module::exception;

#include <mysqlx/xdevapi.h>

#include <fmt/core.h>

#include <algorithm>
#include <chrono>
#include <exception>
#include <list>
#include <memory>
#include <mutex>
#include <string>

namespace chat::dao {

class UserRepository : public BaseRepository {
public:
  static std::shared_ptr<UserRepository> getInstance(L repoLogger) {
    std::lock_guard<std::mutex> lock(createMutex);
    if (instance == nullptr) {
      instance = std::make_shared<UserRepository>(repoLogger);
    }
    return instance;
  }

  UserRepository(L repoLogger) : BaseRepository(repoLogger, "chat_user"){};

  R findById(mysqlx::Session &session, uint64_t id) override {
    return findBy(session, fmt::v9::format("user_id={}", id));
  }

  std::list<R> findByName(mysqlx::Session &session, std::string name) {
    if (module::secure::verifyUserInput(name)) {
      return findAllBy(session, fmt::v9::format("name='{}'", name));
    } else {
      const auto msg =
          fmt::v9::format("UserRepository: name={} is invalid format", name);
      repoLogger->error(msg);
      throw EntityException(msg);
    }
  }

  R findByEmail(mysqlx::Session &session, std::string email) {
    if (module::secure::verifyEmail(email)) {
      return findBy(session, fmt::v9::format("email='{}'", email));
    } else {
      const auto msg =
          fmt::v9::format("UserRepository: email={} is invalid format", email);
      repoLogger->error(msg);
      throw EntityException(msg);
    }
  }

  std::list<R> findAllByCompanyId(mysqlx::Session &session,
                                  uint64_t companyId) {
    return findAllBy(session, fmt::v9::format("company_id={}", companyId));
  }

  std::list<R> findAllByRole(mysqlx::Session &session, std::string role) {
    if (module::secure::verifyUserInput(role)) {
      return findAllBy(session, fmt::v9::format("role='{}'", role));
    } else {
      const auto msg =
          fmt::v9::format("UserRepository: role={} is invalid format", role);
      repoLogger->error(msg);
      throw EntityException(msg);
    }
  }

  std::list<R> findAllByRoleInCompany(mysqlx::Session &session,
                                      std::string role, uint64_t companyId) {
    if (module::secure::verifyUserInput(role)) {
      return findAllBy(session, fmt::v9::format("role='{}' AND company_id={}",
                                                role, companyId));
    } else {
      const auto msg =
          fmt::v9::format("UserRepository: role={} is invalid format", role);
      repoLogger->error(msg);
      throw EntityException(msg);
    }
  }

  R save(mysqlx::Session &session, E entity) override {
    std::lock_guard<std::mutex> lock(sessionMutex);
    try {
      auto tableInsert = getTable(session, tableName)
                             .insert("company_id", "name", "role", "email");
      auto user = std::dynamic_pointer_cast<User>(entity);

      if (!module::secure::verifyUserInput(user->getName())) {
        const auto msg = fmt::v9::format(
            "UserRepository: name={} is invalid format", user->getName());
        repoLogger->error(msg);
        throw EntityException(msg);
      }
      if (!module::secure::verifyUserInput(user->getRole())) {
        const auto msg = fmt::v9::format(
            "UserRepository: role={} is invalid format", user->getRole());
        repoLogger->error(msg);
        throw EntityException(msg);
      }
      if (!module::secure::verifyEmail(user->getEmail())) {
        const auto msg = fmt::v9::format(
            "UserRepository: email={} is invalid format", user->getEmail());
        repoLogger->error(msg);
        throw EntityException(msg);
      }
      const auto row = mysqlx::Row(user->getCompanyId(), user->getName(),
                                   user->getRole(), user->getEmail());
      const auto result = tableInsert.values(row).execute();
      return findById(session, result.getAutoIncrementValue());
    } catch (const std::exception &e) {
      auto msg = fmt::v9::format("UserRepository: {}", e.what());
      repoLogger->error(msg);
      throw EntityException(msg);
    }
  }

  R update(mysqlx::Session &session, E entity) override {
    std::lock_guard<std::mutex> lock(sessionMutex);
    try {
      auto tableUpdate = getTable(session, tableName).update();
      const auto user = std::dynamic_pointer_cast<User>(entity);

      if (!module::secure::verifyUserInput(user->getName())) {
        const auto msg = fmt::v9::format(
            "UserRepository: name={} is invalid format", user->getName());
        repoLogger->error(msg);
        throw EntityException(msg);
      }
      if (!module::secure::verifyUserInput(user->getRole())) {
        const auto msg = fmt::v9::format(
            "UserRepository: role={} is invalid format", user->getRole());
        repoLogger->error(msg);
        throw EntityException(msg);
      }
      if (!module::secure::verifyEmail(user->getEmail())) {
        const auto msg = fmt::v9::format(
            "UserRepository: email={} is invalid format", user->getEmail());
        repoLogger->error(msg);
        throw EntityException(msg);
      }
      const auto condition = fmt::v9::format("user_id={}", user->getId());
      const auto result =
          tableUpdate.set("company_id", user->getCompanyId())
              .set("name", user->getName())
              .set("role", user->getRole())
              .set("email", user->getEmail())
              .set("last_modified_at",
                   module::convertToLocalTimeString(module::getCurrentTime()))
              .where(condition)
              .execute();
      return findById(session, user->getId());
    } catch (const std::exception &e) {
      auto msg = fmt::v9::format("UserRepository: {}", e.what());
      repoLogger->error(msg);
      throw EntityException(msg);
    }
  }
  bool remove(mysqlx::Session &session, E entity) override {
    std::lock_guard<std::mutex> lock(sessionMutex);
    try {
      auto tableRemove = getTable(session, tableName).remove();
      const auto user = std::dynamic_pointer_cast<User>(entity);
      const auto condition = fmt::v9::format("user_id={}", user->getId());
      const auto result = tableRemove.where(condition).execute();
      return true;
    } catch (const std::exception &e) {
      auto msg = fmt::v9::format("UserRepository: {}", e.what());
      repoLogger->error(msg);
      throw EntityException(msg);
    }
  }

private:
  static std::shared_ptr<UserRepository> instance;
  static std::mutex createMutex;
  UserRepository() = delete;

  R findBy(mysqlx::Session &session, std::string condition) {
    try {
      auto tableSelect =
          getTable(session, tableName)
              .select("company_id", "name", "role", "email", "user_id",
                      getUnixTimestampFormatter("created_at"),
                      getUnixTimestampFormatter("last_modified_at"));
      auto result = tableSelect.where(condition).execute();

      if (result.count() != 1) {
        const auto msg =
            fmt::v9::format("UserRepository : more than one rows are selected");
        throw EntityException(msg);
      }
      auto row = result.fetchOne();

      if (row.isNull() != true) {
        auto entity =
            R(new User(uint64_t(row.get(0)), std::string(row.get(1)),
                       std::string(row.get(2)), std::string(row.get(3)),
                       uint64_t(row.get(4)), convertToTimeT(row.get(5)),
                       convertToTimeT(row.get(6))));
        return entity;
      } else {
        return nullptr;
      }
    } catch (const std::exception &e) {
      auto msg = fmt::v9::format("UserRepository: {}", e.what());
      repoLogger->error(msg);
      throw EntityException(msg);
    }
  }

  std::list<R> findAllBy(mysqlx::Session &session, std::string condition) {
    try {
      auto tableSelect =
          getTable(session, tableName)
              .select("company_id", "name", "role", "email", "user_id",
                      getUnixTimestampFormatter("created_at"),
                      getUnixTimestampFormatter("last_modified_at"));
      auto result = tableSelect.where(condition).execute();

      auto rawList = result.fetchAll();
      auto filteredRawList = std::list<mysqlx::Row>{};
      std::copy_if(rawList.begin(), rawList.end(),
                   std::back_inserter(filteredRawList),
                   [](mysqlx::Row row) { return row.isNull() != true; });

      auto userList = std::list<R>{};
      std::transform(
          filteredRawList.begin(), filteredRawList.end(),
          std::back_inserter(userList), [](mysqlx::Row row) -> R {
            return R(new User(uint64_t(row.get(0)), std::string(row.get(1)),
                              std::string(row.get(2)), std::string(row.get(3)),
                              uint64_t(row.get(4)), convertToTimeT(row.get(5)),
                              convertToTimeT(row.get(6))));
          });
      return userList;
    } catch (const std::exception &e) {
      auto msg = fmt::v9::format("UserRepository: {}", e.what());
      repoLogger->error(msg);
      throw EntityException(msg);
    }
  }
};

std::shared_ptr<UserRepository> UserRepository::instance = nullptr;
std::mutex UserRepository::createMutex{};
} // namespace chat::dao