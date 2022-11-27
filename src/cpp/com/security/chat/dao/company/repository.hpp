#pragma once

#include "../base/entity.hpp"
#include "../base/repository.hpp"
#include "./entity.hpp"

#include "../../module/all.hpp"
using namespace chat::module::exception;

#include <mysqlx/devapi/table_crud.h>
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

  CompanyRepository(L repoLogger) : BaseRepository(repoLogger, "company"){};

  R findByName(mysqlx::Session &session, std::string name) {
    if (module::secure::verifyUserInput(name)) {
      return findBy(session, fmt::v9::format("name='{}'", name));
    } else {
      const auto msg =
          fmt::v9::format("CompanyRepository: name={} is invalid format", name);
      repoLogger->error(msg);
      throw EntityException(msg);
    }
  }

  R findById(mysqlx::Session &session, uint64_t id) override {
    return findBy(session, fmt::v9::format("company_id={}", id));
  }

  R save(mysqlx::Session &session, E entity) override {
    std::lock_guard<std::mutex> lock(sessionMutex);
    try {
      auto tableInsert = getTable(session, tableName).insert("name");
      const auto company = std::dynamic_pointer_cast<Company>(entity);
      if (!module::secure::verifyUserInput(company->getName())) {
        const auto msg = fmt::v9::format(
            "CompanyRepository: name={} is invalid format", company->getName());
        repoLogger->error(msg);
        throw EntityException(msg);
      }
      const auto row = mysqlx::Row(company->getName());
      const auto result = tableInsert.values(row).execute();
      return findById(session, result.getAutoIncrementValue());
    } catch (const std::exception &e) {
      auto msg = fmt::v9::format("CompanyRepository: {}", e.what());
      repoLogger->error(msg);
      throw EntityException(msg);
    }
  }

  R update(mysqlx::Session &session, E entity) override {
    std::lock_guard<std::mutex> lock(sessionMutex);
    try {
      auto tableUpdate = getTable(session, tableName).update();
      auto company = std::dynamic_pointer_cast<Company>(entity);
      if (!module::secure::verifyUserInput(company->getName())) {
        const auto msg = fmt::v9::format(
            "CompanyRepository: name={} is invalid format", company->getName());
        repoLogger->error(msg);
        throw EntityException(msg);
      }
      const auto condition = fmt::v9::format("company_id={}", company->getId());
      const auto result =
          tableUpdate.set("name", company->getName())
              .set("last_modified_at",
                   module::convertToLocalTimeString(module::getCurrentTime()))
              .where(condition)
              .execute();
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
      auto tableRemove = getTable(session, tableName).remove();
      const auto company = std::dynamic_pointer_cast<Company>(entity);
      const auto condition = fmt::v9::format("company_id={}", company->getId());
      const auto result = tableRemove.where(condition).execute();
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

  R findBy(mysqlx::Session &session, const std::string condition) {
    try {
      auto tableSelect =
          getTable(session, tableName)
              .select("name", "company_id",
                      getUnixTimestampFormatter("created_at"),
                      getUnixTimestampFormatter("last_modified_at"));
      auto result = tableSelect.where(condition).execute();

      if (result.count() != 1) {
        const auto msg = fmt::v9::format(
            "CompanyRepository : more than one rows are selected");
        throw EntityException(msg);
      }
      auto row = result.fetchOne();

      if (row.isNull() != true) {
        const auto entity = R(new Company(
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
