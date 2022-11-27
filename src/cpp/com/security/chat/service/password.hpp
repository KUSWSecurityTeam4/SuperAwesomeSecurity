#pragma once

#include "../dao/company/entity.hpp"
#include "../dao/company/repository.hpp"
#include "../dao/password/entity.hpp"
#include "../dao/password/repository.hpp"
#include "../dao/user/entity.hpp"
#include "../dao/user/repository.hpp"

#include "../module/all.hpp"
using namespace chat::module::exception;

#include "base.hpp"

#include <mysqlx/xdevapi.h>

#include <fmt/core.h>

#include <exception>
#include <memory>
#include <mutex>
#include <string>

namespace chat::service {

class PasswordService : public BaseService {
public:
  static std::shared_ptr<PasswordService> getInstance(L serverLogger, CN conn) {
    std::lock_guard<std::mutex> lock(createMutex);
    if (instance == nullptr) {
      instance = std::make_shared<PasswordService>(serverLogger, conn);
    }
    return instance;
  }
  PasswordService(L serverLogger, CN conn)
      : BaseService(serverLogger, conn),
        passwordRepository(dao::PasswordRepository::getInstance(serverLogger)),
        userRepository(dao::UserRepository::getInstance(serverLogger)),
        companyRepository(dao::CompanyRepository::getInstance(serverLogger)),
        saltLength(100) {}

  R findByCompanyId(uint64_t companyId) {
    auto session = std::unique_ptr<mysqlx::Session>(nullptr);
    try {
      session = std::make_unique<mysqlx::Session>(conn->client->getSession());
      session->startTransaction();
      auto password =
          std::dynamic_pointer_cast<dao::PasswordRepository>(passwordRepository)
              ->findByCompanyId(*session, companyId);
      if (password != nullptr) {
        session->commit();
        return password;
      } else {
        throw NotFoundEntityException(fmt::v9::format(
            "PasswordService: company={} not in Password", companyId));
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
      auto msg = fmt::v9::format("PasswordService : {}", e.what());
      serverLogger->error(msg);
      throw ServiceException(msg);
    }
  }

  R findByUserId(uint64_t userId) {
    auto session = std::unique_ptr<mysqlx::Session>(nullptr);
    try {
      session = std::make_unique<mysqlx::Session>(conn->client->getSession());
      session->startTransaction();
      auto password =
          std::dynamic_pointer_cast<dao::PasswordRepository>(passwordRepository)
              ->findByUserId(*session, userId);
      if (password != nullptr) {
        session->commit();
        return password;
      } else {
        throw NotFoundEntityException(fmt::v9::format(
            "PasswordService: user={} not in Password", userId));
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
      auto msg = fmt::v9::format("PasswordService : {}", e.what());
      serverLogger->error(msg);
      throw ServiceException(msg);
    }
  }

  bool compareWithCompanyPw(uint64_t companyId, std::string receivedPw) {
    auto session = std::unique_ptr<mysqlx::Session>(nullptr);
    try {
      session = std::make_unique<mysqlx::Session>(conn->client->getSession());
      session->startTransaction();
      auto password =
          std::dynamic_pointer_cast<dao::PasswordRepository>(passwordRepository)
              ->findByCompanyId(*session, companyId);
      session->commit();
      if (password != nullptr) {
        auto salt =
            std::dynamic_pointer_cast<dao::Password>(password)->getSalt();
        auto storedPw =
            std::dynamic_pointer_cast<dao::Password>(password)->getHashedPw();

        return module::secure::compare(receivedPw, storedPw, salt);
      } else {
        throw NotFoundEntityException(fmt::v9::format(
            "PasswordService: company={} not in Password", companyId));
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
      auto msg = fmt::v9::format("PasswordService : {}", e.what());
      serverLogger->error(msg);
      throw ServiceException(msg);
    }
  }

  bool compareWithUserPw(uint64_t userId, std::string receivedPw) {
    auto session = std::unique_ptr<mysqlx::Session>(nullptr);
    try {
      session = std::make_unique<mysqlx::Session>(conn->client->getSession());
      session->startTransaction();
      auto password =
          std::dynamic_pointer_cast<dao::PasswordRepository>(passwordRepository)
              ->findByUserId(*session, userId);
      session->commit();
      if (password != nullptr) {
        auto salt =
            std::dynamic_pointer_cast<dao::Password>(password)->getSalt();
        auto storedPw =
            std::dynamic_pointer_cast<dao::Password>(password)->getHashedPw();

        return module::secure::compare(receivedPw, storedPw, salt);
      } else {
        throw NotFoundEntityException(fmt::v9::format(
            "PasswordService: user={} not in Password", userId));
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
      auto msg = fmt::v9::format("PasswordService : {}", e.what());
      serverLogger->error(msg);
      throw ServiceException(msg);
    }
  }

  /**
   * update, save, and remove of password is done with user or company.
   * So, the session of passwordRepository MUST be same with user and company
   * repository
   */
  R updateCompanyPw(mysqlx::Session &session, uint64_t companyId,
                    std::string updatedPw) {
    try {
      auto password =
          std::dynamic_pointer_cast<dao::PasswordRepository>(passwordRepository)
              ->findByCompanyId(session, companyId);
      if (password != nullptr) {
        auto salt = module::secure::generateFixedLengthCode(saltLength);
        auto hashedPw = module::secure::hash(updatedPw, salt);

        std::dynamic_pointer_cast<dao::Password>(password)->setHashedPw(
            hashedPw);
        std::dynamic_pointer_cast<dao::Password>(password)->setSalt(salt);

        password = std::dynamic_pointer_cast<dao::PasswordRepository>(
                       passwordRepository)
                       ->updateOfCompanyId(session, password);
        if (password != nullptr) {
          return password;
        } else {
          throw NotUpdatedEntityException(fmt::v9::format(
              "PasswordService: company={} cannot updated", companyId));
        }
      } else {
        throw NotFoundEntityException(fmt::v9::format(
            "PasswordService: company={} not in Password", companyId));
      }
    } catch (const NotFoundEntityException &e) {
      serverLogger->error(e.what());
      throw;
    } catch (const NotUpdatedEntityException &e) {
      serverLogger->error(e.what());
      throw;
    } catch (const std::exception &e) {
      auto msg = fmt::v9::format("PasswordService : {}", e.what());
      serverLogger->error(msg);
      throw ServiceException(msg);
    }
  }

  R updateUserPw(mysqlx::Session &session, uint64_t userId,
                 std::string updatedPw) {
    try {
      auto password =
          std::dynamic_pointer_cast<dao::PasswordRepository>(passwordRepository)
              ->findByUserId(session, userId);

      if (password != nullptr) {
        auto salt = module::secure::generateFixedLengthCode(saltLength);
        auto hashedPw = module::secure::hash(updatedPw, salt);

        std::dynamic_pointer_cast<dao::Password>(password)->setHashedPw(
            hashedPw);
        std::dynamic_pointer_cast<dao::Password>(password)->setSalt(salt);

        password = std::dynamic_pointer_cast<dao::PasswordRepository>(
                       passwordRepository)
                       ->updateOfUserId(session, password);
        if (password != nullptr) {
          return password;
        } else {
          throw NotUpdatedEntityException(fmt::v9::format(
              "PasswordService: user={} cannot updated", userId));
        }
      } else {
        throw NotFoundEntityException(fmt::v9::format(
            "PasswordService: user={} not in Password", userId));
      }
    } catch (const NotFoundEntityException &e) {
      serverLogger->error(e.what());
      throw;
    } catch (const NotUpdatedEntityException &e) {
      serverLogger->error(e.what());
      throw;
    } catch (const std::exception &e) {
      auto msg = fmt::v9::format("PasswordService : {}", e.what());
      serverLogger->error(msg);
      throw ServiceException(msg);
    }
  }

  R saveWithUserId(mysqlx::Session &session, uint64_t userId, std::string pw) {
    try {
      auto password =
          std::dynamic_pointer_cast<dao::PasswordRepository>(passwordRepository)
              ->findByUserId(session, userId);

      if (password == nullptr) {
        auto user = std::dynamic_pointer_cast<dao::User>(
            userRepository->findById(session, userId));

        auto userName = user->getName();
        auto createdAt = module::convertToLocalTimeString(user->getCreatedAt());

        auto salt = module::secure::generateFixedLengthCode(saltLength);
        auto hashedPw = module::secure::hash(pw, salt);

        R password = std::make_shared<dao::Password>(userId, salt, hashedPw);
        password = std::dynamic_pointer_cast<dao::PasswordRepository>(
                       passwordRepository)
                       ->saveWithUserId(session, password);
        if (password != nullptr) {
          return password;
        } else {
          throw NotSavedEntityException(
              fmt::v9::format("PasswordService: user={} cannot saved", userId));
        }
      } else {
        throw DuplicatedEntityException(fmt::v9::format(
            "PasswordService: user={} already in Password", userId));
      }
    } catch (const DuplicatedEntityException &e) {
      serverLogger->error(e.what());
      throw;
    } catch (const NotSavedEntityException &e) {
      serverLogger->error(e.what());
      throw;
    } catch (const std::exception &e) {
      auto msg = fmt::v9::format("PasswordService : {}", e.what());
      serverLogger->error(msg);
      throw ServiceException(msg);
    }
  }

  bool removeUserPw(mysqlx::Session &session, uint64_t userId) {
    try {
      auto password =
          std::dynamic_pointer_cast<dao::PasswordRepository>(passwordRepository)
              ->findByUserId(session, userId);
      if (password != nullptr) {
        if (passwordRepository->remove(session, password)) {
          return true;
        } else {
          throw NotRemovedEntityException(fmt::v9::format(
              "PasswordService: user={} cannot be removed", userId));
        }

      } else {
        throw NotFoundEntityException(fmt::v9::format(
            "PasswordService: user={} not in Password", userId));
      }
    } catch (const NotFoundEntityException &e) {
      serverLogger->error(e.what());
      throw;
    } catch (const NotRemovedEntityException &e) {
      serverLogger->error(e.what());
      throw;
    } catch (const std::exception &e) {
      auto msg = fmt::v9::format("PasswordService : {}", e.what());
      serverLogger->error(msg);
      throw ServiceException(msg);
    }
  }

private:
  static std::shared_ptr<PasswordService> instance;
  static std::mutex createMutex;

  RP passwordRepository;
  RP companyRepository;
  RP userRepository;
  uint64_t saltLength;
  PasswordService() = delete;
};

std::shared_ptr<PasswordService> PasswordService::instance = nullptr;
std::mutex PasswordService::createMutex{};
} // namespace chat::service
