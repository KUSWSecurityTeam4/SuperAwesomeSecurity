#pragma once

#include "../dao/company/entity.hpp"
#include "../dao/company/repository.hpp"

#include "../module/all.hpp"
using namespace chat::module::exception;

#include "base.hpp"
#include "password.hpp"

#include <mysqlx/xdevapi.h>

#include <fmt/core.h>

#include <exception>
#include <memory>
#include <mutex>
#include <string>

namespace chat::service {

class CompanyService : public BaseService {
public:
  static std::shared_ptr<CompanyService> getInstance(L serverLogger, CN conn) {
    std::lock_guard<std::mutex> lock(createMutex);
    if (instance == nullptr) {
      instance = std::make_shared<CompanyService>(serverLogger, conn);
    }
    return instance;
  }
  CompanyService(L serverLogger, CN conn)
      : BaseService(serverLogger, conn),
        companyRepository(dao::CompanyRepository::getInstance(serverLogger)),
        passwordService(PasswordService::getInstance(serverLogger, conn)) {}

  R findById(uint64_t companyId) {
    auto session = std::unique_ptr<mysqlx::Session>(nullptr);
    try {
      session = std::make_unique<mysqlx::Session>(conn->client->getSession());
      session->startTransaction();
      auto company = companyRepository->findById(*session, companyId);
      if (company != nullptr) {
        session->commit();
        return company;
      } else {
        throw NotFoundEntityException(
            fmt::v9::format("CompanyService: id={} not in Company", companyId));
      }
    } catch (const NotFoundEntityException &e) {
      if (session != nullptr) {
        session->rollback();
      }
      serverLogger->error(e.what());
      throw;
    } catch (const std::exception &e) {
      if (session != nullptr) {
        session->rollback();
      }
      auto msg = fmt::v9::format("CompanyService : {}", e.what());
      serverLogger->error(msg);
      throw ServiceException(msg);
    }
  }

  R findByName(std::string companyName) {
    auto session = std::unique_ptr<mysqlx::Session>(nullptr);
    try {
      session = std::make_unique<mysqlx::Session>(conn->client->getSession());
      session->startTransaction();
      auto company =
          std::dynamic_pointer_cast<dao::CompanyRepository>(companyRepository)
              ->findByName(*session, companyName);

      if (company != nullptr) {
        session->commit();
        return company;
      } else {
        throw NotFoundEntityException(fmt::v9::format(
            "CompanyService: name={} not in Company", companyName));
      }
    } catch (const NotFoundEntityException &e) {
      if (session != nullptr) {
        session->rollback();
      }
      serverLogger->error(e.what());
      throw;
    } catch (const std::exception &e) {
      if (session != nullptr) {
        session->rollback();
      }
      auto msg = fmt::v9::format("CompanyService : {}", e.what());
      serverLogger->error(msg);
      throw ServiceException(msg);
    }
  }

  R updateName(uint64_t companyId, std::string companyName) {
    auto session = std::unique_ptr<mysqlx::Session>(nullptr);
    try {
      session = std::make_unique<mysqlx::Session>(conn->client->getSession());
      session->startTransaction();
      auto company = companyRepository->findById(*session, companyId);
      if (company != nullptr) {
        if (std::dynamic_pointer_cast<dao::Company>(company)->getName() ==
            companyName) {
          throw DuplicatedEntityException(fmt::v9::format(
              "CompanyService: name={} already in Company", companyName));
        } else {
          std::dynamic_pointer_cast<dao::Company>(company)->setName(
              companyName);
          company = companyRepository->update(*session, company);
          if (company != nullptr) {
            session->commit();
            return company;
          } else {
            throw NotUpdatedEntityException(fmt::v9::format(
                "CompanyService: id={} cannot updated", companyId));
          }
        }
      } else {
        throw NotFoundEntityException(
            fmt::v9::format("CompanyService: id={} not in Company", companyId));
      }
    } catch (const NotFoundEntityException &e) {
      if (session != nullptr) {
        session->rollback();
      }
      serverLogger->error(e.what());
      throw;
    } catch (const DuplicatedEntityException &e) {
      if (session != nullptr) {
        session->rollback();
      }
      serverLogger->error(e.what());
      throw;
    } catch (const NotUpdatedEntityException &e) {
      if (session != nullptr) {
        session->rollback();
      }
      serverLogger->error(e.what());
      throw;
    } catch (const std::exception &e) {
      if (session != nullptr) {
        session->rollback();
      }
      auto msg = fmt::v9::format("CompanyService : {}", e.what());
      serverLogger->error(msg);
      throw ServiceException(msg);
    }
  }

  R updatePw(uint64_t companyId, std::string pw) {
    auto session = std::unique_ptr<mysqlx::Session>(nullptr);
    try {
      session = std::make_unique<mysqlx::Session>(conn->client->getSession());
      session->startTransaction();
      auto company = companyRepository->findById(*session, companyId);
      if (company != nullptr) {
        passwordService->updateCompanyPw(*session, companyId, pw);
        session->commit();
        return company;
      } else {
        throw NotFoundEntityException(
            fmt::v9::format("CompanyService: id={} not in Company", companyId));
      }
    } catch (const NotFoundEntityException &e) {
      if (session != nullptr) {
        session->rollback();
      }
      serverLogger->error(e.what());
      throw;
    } catch (const std::exception &e) {
      if (session != nullptr) {
        session->rollback();
      }
      auto msg = fmt::v9::format("CompanyService : {}", e.what());
      serverLogger->error(msg);
      throw ServiceException(msg);
    }
  }

private:
  static std::shared_ptr<CompanyService> instance;
  static std::mutex createMutex;

  RP companyRepository;
  std::shared_ptr<PasswordService> passwordService;

  CompanyService() = delete;
};

std::shared_ptr<CompanyService> CompanyService::instance = nullptr;
std::mutex CompanyService::createMutex{};
} // namespace chat::service
