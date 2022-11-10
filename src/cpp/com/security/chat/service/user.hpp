#pragma once

#include "../dao/user/entity.hpp"
#include "../dao/user/repository.hpp"

#include "../module/all.hpp"
using namespace chat::module::exception;

#include "base.hpp"
#include "company.hpp"
#include "password.hpp"

#include <mysqlx/xdevapi.h>

#include <fmt/core.h>

#include <exception>
#include <list>
#include <memory>
#include <mutex>
#include <string>

namespace chat::service {

class UserService : public BaseService {
public:
  static std::shared_ptr<UserService> getInstance(L serverLogger, CN conn) {
    std::lock_guard<std::mutex> lock(createMutex);
    if (instance == nullptr) {
      instance = std::make_shared<UserService>(serverLogger, conn);
    }
    return instance;
  }
  UserService(L serverLogger, CN conn)
      : BaseService(serverLogger, conn),
        userRepository(dao::UserRepository::getInstance(serverLogger)),
        companyService(CompanyService::getInstance(serverLogger, conn)),
        passwordService(PasswordService::getInstance(serverLogger, conn)) {}

  R findById(uint64_t userId) {
    auto session = std::unique_ptr<mysqlx::Session>(nullptr);
    try {
      session = std::make_unique<mysqlx::Session>(conn->client->getSession());
      session->startTransaction();
      auto user = userRepository->findById(*session, userId);
      if (user != nullptr) {
        session->commit();
        return user;
      } else {
        throw NotFoundEntityException(
            fmt::v9::format("UserService: id={} not in User", userId));
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
      auto msg = fmt::v9::format("UserService : {}", e.what());
      serverLogger->error(msg);
      throw ServiceException(msg);
    }
  }

  std::list<R> findByName(std::string userName) {
    auto session = std::unique_ptr<mysqlx::Session>(nullptr);
    try {
      session = std::make_unique<mysqlx::Session>(conn->client->getSession());
      session->startTransaction();
      auto userList =
          std::dynamic_pointer_cast<dao::UserRepository>(userRepository)
              ->findByName(*session, userName);
      if (userList.size() > 0) {
        session->commit();
        return userList;
      } else {
        throw NotFoundEntityException(
            fmt::v9::format("UserService: name={} not in User", userName));
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
      auto msg = fmt::v9::format("UserService : {}", e.what());
      serverLogger->error(msg);
      throw ServiceException(msg);
    }
  }

  R findByEmail(std::string email) {
    auto session = std::unique_ptr<mysqlx::Session>(nullptr);
    try {
      session = std::make_unique<mysqlx::Session>(conn->client->getSession());
      session->startTransaction();
      auto user = std::dynamic_pointer_cast<dao::UserRepository>(userRepository)
                      ->findByEmail(*session, email);
      if (user != nullptr) {
        session->commit();
        return user;
      } else {
        throw NotFoundEntityException(
            fmt::v9::format("UserService: email={} not in User", email));
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
      auto msg = fmt::v9::format("UserService : {}", e.what());
      serverLogger->error(msg);
      throw ServiceException(msg);
    }
  }

  std::list<R> findAllByRoleInCompany(uint64_t companyId, std::string role) {
    auto session = std::unique_ptr<mysqlx::Session>(nullptr);
    try {
      session = std::make_unique<mysqlx::Session>(conn->client->getSession());
      session->startTransaction();
      auto userList =
          std::dynamic_pointer_cast<dao::UserRepository>(userRepository)
              ->findAllByRoleInCompany(*session, role, companyId);
      if (userList.size() > 0) {
        session->commit();
        return userList;
      } else {
        throw NotFoundEntityException(
            fmt::v9::format("UserService: role={} AND company={} not in User",
                            role, companyId));
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
      auto msg = fmt::v9::format("UserService : {}", e.what());
      serverLogger->error(msg);
      throw ServiceException(msg);
    }
  }
  std::list<R> findAllInCompany(uint64_t companyId) {
    auto session = std::unique_ptr<mysqlx::Session>(nullptr);
    try {
      session = std::make_unique<mysqlx::Session>(conn->client->getSession());
      session->startTransaction();
      auto userList =
          std::dynamic_pointer_cast<dao::UserRepository>(userRepository)
              ->findAllByCompanyId(*session, companyId);
      if (userList.size() > 0) {
        session->commit();
        return userList;
      } else {
        throw NotFoundEntityException(
            fmt::v9::format("UserService: company={} not in User", companyId));
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
      auto msg = fmt::v9::format("UserService : {}", e.what());
      serverLogger->error(msg);
      throw ServiceException(msg);
    }
  }

  R save(std::string name, uint64_t companyId, std::string role,
         std::string email, std::string pw) {
    auto session = std::unique_ptr<mysqlx::Session>(nullptr);
    try {
      session = std::make_unique<mysqlx::Session>(conn->client->getSession());
      session->startTransaction();

      auto company = companyService->findById(companyId);

      if (std::dynamic_pointer_cast<dao::UserRepository>(userRepository)
              ->findByEmail(*session, email) == nullptr) {

        R user = std::make_shared<dao::User>(companyId, name, role, email);
        user = userRepository->save(*session, user);
        if (user != nullptr) {
          passwordService->saveWithUserId(*session, user->getId(), pw);
          session->commit();
          return user;
        } else {
          throw NotSavedEntityException(
              fmt::v9::format("UserService: name={}, companyId={}, role={}, "
                              "email={} cannot saved",
                              name, companyId, email));
        }
      } else {
        throw DuplicatedEntityException(
            fmt::v9::format("UserService: email={} alread in User", email));
      }

    } catch (const DuplicatedEntityException &e) {
      if (session != nullptr) {
        session->rollback();
      }
      serverLogger->error(e.what());
      throw;
    } catch (const NotSavedEntityException &e) {
      if (session != nullptr) {
        session->rollback();
      }
      serverLogger->error(e.what());
      throw;
    } catch (const std::exception &e) {
      if (session != nullptr) {
        session->rollback();
      }
      auto msg = fmt::v9::format("UserService : {}", e.what());
      serverLogger->error(msg);
      throw ServiceException(msg);
    }
  }

  R update(uint64_t userId, std::string name, std::string role,
           std::string email, std::string pw) {

    auto session = std::unique_ptr<mysqlx::Session>(nullptr);
    try {
      session = std::make_unique<mysqlx::Session>(conn->client->getSession());
      session->startTransaction();

      auto user = std::dynamic_pointer_cast<dao::User>(
          userRepository->findById(*session, userId));
      if (user != nullptr) {
        if (std::dynamic_pointer_cast<dao::UserRepository>(userRepository)
                ->findByEmail(*session, email) == nullptr) {

          user->setName(name);
          user->setRole(role);
          user->setEmail(email);

          user = std::dynamic_pointer_cast<dao::User>(
              userRepository->update(*session, user));
          passwordService->updateUserPw(*session, userId, pw);
          if (user != nullptr) {
            session->commit();
            return user;
          } else {
            throw NotUpdatedEntityException(fmt::v9::format(
                "UserService: id={} cannot be updated", userId));
          }
        } else {
          throw DuplicatedEntityException(
              fmt::v9::format("UserService: email={} already in User", email));
        }
      } else {
        throw NotFoundEntityException(
            fmt::v9::format("UserService: id={} not in User", userId));
      }

    } catch (const DuplicatedEntityException &e) {
      if (session != nullptr) {
        session->rollback();
      }
      serverLogger->error(e.what());
      throw;

    } catch (const NotFoundEntityException &e) {
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
      auto msg = fmt::v9::format("UserService : {}", e.what());
      serverLogger->error(msg);
      throw ServiceException(msg);
    }
  }

  bool remove(uint64_t userId) {
    auto session = std::unique_ptr<mysqlx::Session>(nullptr);
    try {
      session = std::make_unique<mysqlx::Session>(conn->client->getSession());
      session->startTransaction();

      auto user = userRepository->findById(*session, userId);
      if (user != nullptr) {
        if (passwordService->removeUserPw(*session, userId)) {
          userRepository->remove(*session, user);
          session->commit();
          return true;
        } else {
          throw NotRemovedEntityException(
              fmt::v9::format("UserService: id={} cannot be removed", userId));
        }
      } else {
        throw NotFoundEntityException(
            fmt::v9::format("UserService: id={} not in User", userId));
      }
    } catch (const NotFoundEntityException &e) {
      if (session != nullptr) {
        session->rollback();
      }
      serverLogger->error(e.what());
      throw;
    } catch (const NotRemovedEntityException &e) {
      if (session != nullptr) {
        session->rollback();
      }
      serverLogger->error(e.what());
      throw;
    } catch (const std::exception &e) {
      if (session != nullptr) {
        session->rollback();
      }
      auto msg = fmt::v9::format("UserService : {}", e.what());
      serverLogger->error(msg);
      throw ServiceException(msg);
    }
  }

private:
  static std::shared_ptr<UserService> instance;
  static std::mutex createMutex;

  RP userRepository;
  std::shared_ptr<CompanyService> companyService;
  std::shared_ptr<PasswordService> passwordService;

  UserService() = delete;
};

std::shared_ptr<UserService> UserService::instance = nullptr;
std::mutex UserService::createMutex{};
} // namespace chat::service
