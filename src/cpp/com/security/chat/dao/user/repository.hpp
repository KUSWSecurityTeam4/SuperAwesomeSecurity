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
    if (instance == nullptr) {
      instance = std::make_shared<UserRepository>(repoLogger);
    }
    return instance;
  }

  UserRepository(L repoLogger) : BaseRepository(repoLogger){};

  R findById(mysqlx::Session &session, uint64_t id) override {
    return findBy(session, fmt::v9::format("user_id={}", id));
  }

  std::list<R> findByName(mysqlx::Session &session, std::string name) {
    return findAllBy(session, fmt::v9::format("name='{}'", name));
  }

  R findByEmail(mysqlx::Session &session, std::string email) {
    return findBy(session, fmt::v9::format("email='{}'", email));
  }

  std::list<R> findAllByCompanyId(mysqlx::Session &session,
                                  uint64_t companyId) {
    return findAllBy(session, fmt::v9::format("company_id={}", companyId));
  }

  std::list<R> findAllByRole(mysqlx::Session &session, std::string role) {
    return findAllBy(session, fmt::v9::format("role='{}'", role));
  }

  std::list<R> findAllByRoleInCompany(mysqlx::Session &session,
                                      std::string role, uint64_t companyId) {
    return findAllBy(session, fmt::v9::format("role='{}' AND company_id={}",
                                              role, companyId));
  }

  R save(mysqlx::Session &session, E entity) override {
    std::lock_guard<std::mutex> lock(sessionMutex);
    try {
      auto user = std::dynamic_pointer_cast<User>(entity);
      auto query = fmt::v9::format(
          "INSERT INTO chat_user(company_id, name, role, email) VALUES({}, "
          "'{}', '{}', '{}')",
          user->getCompanyId(), user->getName(), user->getRole(),
          user->getEmail());

      auto stmt = session.sql(query);
      auto result = stmt.execute();

      // Email is one of candidate keys
      return findByEmail(session, user->getEmail());
    } catch (const std::exception &e) {
      auto msg = fmt::v9::format("UserRepository: {}", e.what());
      repoLogger->error(msg);
      throw EntityException(msg);
    }
  }

  R update(mysqlx::Session &session, E entity) override {
    std::lock_guard<std::mutex> lock(sessionMutex);
    try {
      auto user = std::dynamic_pointer_cast<User>(entity);
      auto query =
          fmt::v9::format("UPDATE chat_user SET company_id={}, name='{}', "
                          "role='{}', email='{}', "
                          "last_modified_at=NOW() WHERE user_id={}",
                          user->getCompanyId(), user->getName(),
                          user->getRole(), user->getEmail(), user->getId());

      auto stmt = session.sql(query);
      auto result = stmt.execute();
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
      auto user = std::dynamic_pointer_cast<User>(entity);
      auto query = fmt::v9::format("DELETE FROM chat_user WHERE user_id={}",
                                   user->getId());
      auto stmt = session.sql(query);
      auto result = stmt.execute();
      return true;
    } catch (const std::exception &e) {
      auto msg = fmt::v9::format("UserRepository: {}", e.what());
      repoLogger->error(msg);
      throw EntityException(msg);
    }
  }

private:
  static std::shared_ptr<UserRepository> instance;
  UserRepository() = delete;

  R findBy(mysqlx::Session &session, std::string condition) {
    try {
      auto query = fmt::v9::format(
          "SELECT company_id, name, role, email, user_id, {}, {} FROM "
          "chat_user WHERE {}",
          getUnixTimestampFormatter("created_at"),
          getUnixTimestampFormatter("last_modified_at"), condition);
      auto stmt = session.sql(query);
      auto result = stmt.execute();
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
      auto query = fmt::v9::format(
          "SELECT company_id, name, role, email, user_id, {}, {} FROM "
          "chat_user WHERE {}",
          getUnixTimestampFormatter("created_at"),
          getUnixTimestampFormatter("last_modified_at"), condition);
      auto stmt = session.sql(query);
      auto result = stmt.execute();

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
} // namespace chat::dao