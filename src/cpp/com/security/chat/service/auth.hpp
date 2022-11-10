#pragma once

#include "../dao/company/entity.hpp"
#include "../dao/password/entity.hpp"
#include "../dao/server_session/entity.hpp"
#include "../dao/server_session/memory_repository.hpp"
#include "../dao/user/entity.hpp"

#include "../module/all.hpp"
using namespace chat::module::exception;

#include "base.hpp"
#include "company.hpp"
#include "password.hpp"
#include "room.hpp"
#include "user.hpp"

#include <mysqlx/xdevapi.h>

#include <fmt/core.h>

#include <cstdlib>
#include <ctime>
#include <exception>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>

namespace chat::service {

class AuthService : public BaseService {
public:
  static std::shared_ptr<AuthService> getInstance(L serverLogger, CN conn) {
    std::lock_guard<std::mutex> lock(createMutex);
    if (instance == nullptr) {
      instance = std::make_shared<AuthService>(serverLogger, conn);
    }
    return instance;
  }
  AuthService(L serverLogger, CN conn, uint64_t tokenLength = 512)
      : BaseService(serverLogger, conn),
        serverSessionRepository(
            dao::ServerSessionRepository::getInstance(serverLogger)),
        userService(UserService::getInstance(serverLogger, conn)),
        companyService(CompanyService::getInstance(serverLogger, conn)),
        passwordService(PasswordService::getInstance(serverLogger, conn)),
        roomService(RoomService::getInstance(serverLogger, conn)),
        tokenLength(tokenLength) {}

  R getSession(uint64_t sessionId) {
    auto session = std::unique_ptr<mysqlx::Session>(nullptr);
    try {
      session = std::make_unique<mysqlx::Session>(conn->client->getSession());
      session->startTransaction();

      auto serverSession =
          serverSessionRepository->findById(*session, sessionId);
      if (serverSession != nullptr) {
        session->commit();
        return serverSession;
      } else {
        throw NotFoundEntityException(fmt::v9::format(
            "AuthService : id={} not in ServerSession", sessionId));
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
      auto msg = fmt::v9::format("AutoService : {}", e.what());
      serverLogger->error(msg);
      throw ServiceException(msg);
    }
  }

  bool verifyToken(uint64_t sessionId, std::string token) {
    auto session = std::unique_ptr<mysqlx::Session>(nullptr);
    try {
      session = std::make_unique<mysqlx::Session>(conn->client->getSession());
      session->startTransaction();

      auto serverSession = std::dynamic_pointer_cast<dao::ServerSession>(
          serverSessionRepository->findById(*session, sessionId));

      if (serverSession == nullptr) {
        throw NotFoundEntityException(fmt::v9::format(
            "AuthService : id={} not in ServerSession", sessionId));
      }
      auto storedToken = serverSession->getToken();
      session->commit();

      if (token == storedToken) {
        return true;
      } else {
        return false;
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
      auto msg = fmt::v9::format("AutoService : {}", e.what());
      serverLogger->error(msg);
      throw ServiceException(msg);
    }
  }

  bool deleteIfExpired(uint64_t sessionId) {
    throw NotImplementedException(
        fmt::v9::format("AuthService : deleteIfExpired is not implemented"));
  }

  bool isCompany(E entity) {
    return std::dynamic_pointer_cast<dao::Company>(entity) != nullptr;
  }

  bool isUser(E entity) {
    return std::dynamic_pointer_cast<dao::User>(entity) != nullptr;
  }

  bool isHost(E entity, uint64_t roomId) {
    try {
      if (isUser(entity)) {
        auto host = roomService->findHost(roomId);
        return std::dynamic_pointer_cast<dao::Participant>(host)->getUserId() ==
               entity->getId();
      } else {
        return false;
      }
    } catch (const std::exception &e) {
      auto msg = fmt::v9::format("AutoService : {}", e.what());
      serverLogger->error(msg);
      throw ServiceException(msg);
    }
  }

  bool isThisUser(E entity, uint64_t userId) {
    if (isUser(entity)) {
      return entity->getId() == userId;
    } else {
      return false;
    }
  }

  R loginOfCompany(std::string name, std::string pw) {
    /**
     * Multiple sessions of one entity are permitted
     */
    auto session = std::unique_ptr<mysqlx::Session>(nullptr);
    try {
      session = std::make_unique<mysqlx::Session>(conn->client->getSession());
      session->startTransaction();

      auto company = companyService->findByName(name);
      if (passwordService->compareWithCompanyPw(company->getId(), pw)) {
        auto token = module::secure::generateFixedLengthCode(tokenLength);

        R serverSession = std::make_shared<dao::ServerSession>(company, token);
        serverSession = serverSessionRepository->save(*session, serverSession);

        if (serverSession != nullptr) {
          session->commit();
          return serverSession;
        } else {
          throw NotSavedEntityException(fmt::v9::format(
              "AuthService: company(name={}) cannot login", name));
        }
      } else {
        throw NotSavedEntityException(fmt::v9::format(
            "AuthService: company(name={}) has different password", name));
      }
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
      auto msg = fmt::v9::format("AutoService : {}", e.what());
      serverLogger->error(msg);
      throw ServiceException(msg);
    }
  }

  R loginOfUser(std::string email, std::string pw) {
    /**
     * Multiple sessions of one entity are permitted
     */
    auto session = std::unique_ptr<mysqlx::Session>(nullptr);
    try {
      session = std::make_unique<mysqlx::Session>(conn->client->getSession());
      session->startTransaction();

      auto user = userService->findByEmail(email);
      if (passwordService->compareWithUserPw(user->getId(), pw)) {
        auto token = module::secure::generateFixedLengthCode(tokenLength);

        R serverSession = std::make_shared<dao::ServerSession>(user, token);
        serverSession = serverSessionRepository->save(*session, serverSession);
        if (serverSession != nullptr) {
          session->commit();
          return serverSession;
        } else {
          throw NotSavedEntityException(fmt::v9::format(
              "AuthService: user(email={}) cannot login", email));
        }
      } else {
        throw NotSavedEntityException(fmt::v9::format(
            "AuthService: user(email={}) has different password", email));
      }
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
      auto msg = fmt::v9::format("AutoService : {}", e.what());
      serverLogger->error(msg);
      throw ServiceException(msg);
    }
  }

  bool logout(uint64_t sessionId) {
    auto session = std::unique_ptr<mysqlx::Session>(nullptr);
    try {
      session = std::make_unique<mysqlx::Session>(conn->client->getSession());
      session->startTransaction();

      auto serverSession =
          serverSessionRepository->findById(*session, sessionId);
      if (serverSession != nullptr) {
        if (serverSessionRepository->remove(*session, serverSession)) {
          session->commit();
          return true;
        } else {
          throw NotRemovedEntityException(fmt::v9::format(
              "AuthService : id={} cannot be removed", sessionId));
        }
      } else {
        throw NotFoundEntityException(fmt::v9::format(
            "AuthService : id={} not in serverSession", sessionId));
      }
    } catch (const NotRemovedEntityException &e) {
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
    } catch (const std::exception &e) {
      if (session != nullptr) {
        session->rollback();
      }
      auto msg = fmt::v9::format("AutoService : {}", e.what());
      serverLogger->error(msg);
      throw ServiceException(msg);
    }
  }

private:
  static std::shared_ptr<AuthService> instance;
  static std::mutex createMutex;

  RP serverSessionRepository;
  std::shared_ptr<UserService> userService;
  std::shared_ptr<CompanyService> companyService;
  std::shared_ptr<PasswordService> passwordService;
  std::shared_ptr<RoomService> roomService;
  const uint64_t tokenLength;

  AuthService() = delete;
};

std::shared_ptr<AuthService> AuthService::instance = nullptr;
std::mutex AuthService::createMutex{};
} // namespace chat::service
