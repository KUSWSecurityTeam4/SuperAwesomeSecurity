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

class CompanyRepository : public BaseRepository {
public:
  static std::shared_ptr<CompanyRepository> getInstance(L repoLogger) {
    std::lock_guard<std::mutex> lock(createMutex);
    if (instance == nullptr) {
      instance = std::make_shared<CompanyRepository>(repoLogger);
    }
    return instance;
  }

  CompanyRepository(L repoLogger) : BaseRepository(repoLogger){};

  R findByName(mysqlx::Session &session, std::string name) {
    return findBy(session, fmt::v9::format("name='{}'", name));
  }

  R findById(mysqlx::Session &session, uint64_t id) override {
    return findBy(session, fmt::v9::format("company_id={}", id));
  }

  R save(mysqlx::Session &session, E entity) override {
    std::lock_guard<std::mutex> lock(sessionMutex);
    try {
      auto company = std::dynamic_pointer_cast<Company>(entity);
      auto query = fmt::v9::format("INSERT INTO company(name) VALUES('{}')",
                                   company->getName());
      auto stmt = session.sql(query);
      auto result = stmt.execute();
      return findByName(session, company->getName());
    } catch (const std::exception &e) {
      auto msg = fmt::v9::format("CompanyRepository: {}", e.what());
      repoLogger->error(msg);
      throw EntityException(msg);
    }
  }

  R update(mysqlx::Session &session, E entity) override {
    std::lock_guard<std::mutex> lock(sessionMutex);
    try {
      auto company = std::dynamic_pointer_cast<Company>(entity);
      auto query = fmt::v9::format("UPDATE company SET name='{}', "
                                   "last_modified_at=NOW() WHERE company_id={}",
                                   company->getName(), company->getId());
      auto stmt = session.sql(query);
      auto result = stmt.execute();

      return findById(session, company->getId());
    } catch (const std::exception &e) {
      auto msg = fmt::v9::format("CompanyRepository: {}", e.what());
      repoLogger->error(msg);
      throw EntityException(msg);
    }
  }

  bool remove(mysqlx::Session &session, E entity) override {
    std::lock_guard<std::mutex> lock(sessionMutex);
    try {
      auto company = std::dynamic_pointer_cast<Company>(entity);
      auto query = fmt::v9::format("DELETE FROM company WHERE company_id={}",
                                   company->getId());
      auto stmt = session.sql(query);
      auto result = stmt.execute();
      return true;
    } catch (const std::exception &e) {
      auto msg = fmt::v9::format("CompanyRepository: {}", e.what());
      repoLogger->error(msg);
      throw EntityException(msg);
    }
  }

private:
  static std::shared_ptr<CompanyRepository> instance;
  static std::mutex createMutex;
  CompanyRepository() = delete;

  R findBy(mysqlx::Session &session, std::string condition) {
    try {
      auto query = fmt::v9::format(
          "SELECT name, company_id, {}, {} FROM company WHERE {}",
          getUnixTimestampFormatter("created_at"),
          getUnixTimestampFormatter("last_modified_at"), condition);

      auto stmt = session.sql(query);
      auto result = stmt.execute();

      // SELECT로 선택된 ROW가 없다면 Data는 있되, NULL ROW가 담긴다.
      auto row = result.fetchOne();
      if (row.isNull() != true) {
        auto entity = R(new Company(
            std::string(row.get(0)), uint64_t(row.get(1)),
            convertToTimeT(row.get(2)), convertToTimeT(row.get(3))));
        return entity;
      } else {
        return nullptr;
      }
    } catch (const std::exception &e) {
      auto msg = fmt::v9::format("CompanyRepository: {}", e.what());
      repoLogger->error(msg);
      throw EntityException(msg);
    }
  }
};

std::shared_ptr<CompanyRepository> CompanyRepository::instance = nullptr;
std::mutex CompanyRepository::createMutex{};
} // namespace chat::dao
