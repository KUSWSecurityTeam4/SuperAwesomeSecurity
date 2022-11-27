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
      auto query = fmt::v9::format(
          "SELECT user_id, salt, hashed_pw, pw_id, {}, {} FROM "
          "chat_password WHERE user_id={}",
          getUnixTimestampFormatter("created_at"),
          getUnixTimestampFormatter("last_modified_at"), userId);
      auto stmt = session.sql(query);
      auto result = stmt.execute();

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
      auto query = fmt::v9::format(
          "SELECT salt, hashed_pw, company_id, pw_id, {}, {} FROM "
          "chat_password WHERE company_id={}",
          getUnixTimestampFormatter("created_at"),
          getUnixTimestampFormatter("last_modified_at"), companyId);
      auto stmt = session.sql(query);
      auto result = stmt.execute();

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
      auto password = std::dynamic_pointer_cast<Password>(entity);
      auto query = fmt::v9::format("INSERT INTO chat_password(user_id, salt, "
                                   "hashed_pw) VALUES({}, '{}', '{}')",
                                   password->getUserId(), password->getSalt(),
                                   password->getHashedPw());
      auto stmt = session.sql(query);
      auto result = stmt.execute();
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
      auto password = std::dynamic_pointer_cast<Password>(entity);
      auto query =
          fmt::v9::format("INSERT INTO chat_password(company_id, salt, "
                          "hashed_pw) VALUES({}, '{}', '{}')",
                          password->getCompanyId(), password->getSalt(),
                          password->getHashedPw());
      auto stmt = session.sql(query);
      auto result = stmt.execute();
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
      auto password = std::dynamic_pointer_cast<Password>(entity);
      auto query = fmt::v9::format("UPDATE chat_password SET "
                                   "salt='{}', hashed_pw='{}', "
                                   "last_modified_at=NOW() WHERE user_id={}",
                                   password->getSalt(), password->getHashedPw(),
                                   password->getUserId());
      auto stmt = session.sql(query);
      auto result = stmt.execute();
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
      auto password = std::dynamic_pointer_cast<Password>(entity);
      auto query = fmt::v9::format("UPDATE chat_password SET "
                                   "salt='{}', hashed_pw='{}', "
                                   "last_modified_at=NOW() WHERE company_id={}",
                                   password->getSalt(), password->getHashedPw(),
                                   password->getCompanyId());
      auto stmt = session.sql(query);
      auto result = stmt.execute();
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
      auto password = std::dynamic_pointer_cast<Password>(entity);
      auto query = fmt::v9::format("DELETE FROM chat_password WHERE pw_id={}",
                                   password->getId());
      auto stmt = session.sql(query);
      auto result = stmt.execute();
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
