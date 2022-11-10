#pragma once

#include "../dao/invitation/entity.hpp"
#include "../dao/invitation/repository.hpp"

#include "../module/all.hpp"
using namespace chat::module::exception;

#include "base.hpp"
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
#include <thread>

namespace chat::service {

class InvitationService : public BaseService {
public:
  static std::shared_ptr<InvitationService> getInstance(L serverLogger,
                                                        CN conn) {
    std::lock_guard<std::mutex> lock(createMutex);
    if (instance == nullptr) {
      instance = std::make_shared<InvitationService>(serverLogger, conn);
    }
    return instance;
  }
  InvitationService(L serverLogger, CN conn)
      : BaseService(serverLogger, conn),
        invitationRepository(
            dao::InvitationRepository::getInstance(serverLogger)),
        userService(UserService::getInstance(serverLogger, conn)),
        roomService(RoomService::getInstance(serverLogger, conn)) {}

  R findById(uint64_t invitationId) {
    auto session = std::unique_ptr<mysqlx::Session>(nullptr);
    try {
      session = std::make_unique<mysqlx::Session>(conn->client->getSession());
      session->startTransaction();

      auto invitation = invitationRepository->findById(*session, invitationId);
      if (invitation != nullptr) {
        session->commit();
        return invitation;
      } else {
        throw NotFoundEntityException(fmt::v9::format(
            "InvitationService : id={} not in Invitation", invitationId));
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
      auto msg = fmt::v9::format("InvitationService : {}", e.what());
      serverLogger->error(msg);
      throw ServiceException(msg);
    }
  }

  R findByUserIdInRoom(uint64_t userId, uint64_t roomId) {
    auto session = std::unique_ptr<mysqlx::Session>(nullptr);
    try {
      session = std::make_unique<mysqlx::Session>(conn->client->getSession());
      session->startTransaction();

      auto invitation = std::dynamic_pointer_cast<dao::InvitationRepository>(
                            invitationRepository)
                            ->findByUserIdInRoom(*session, userId, roomId);

      if (invitation != nullptr) {
        session->commit();
        return invitation;
      } else {
        throw NotFoundEntityException(fmt::v9::format(
            "InvitationService : userId={} AND roomId={} not in Invitation",
            userId, roomId));
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
      auto msg = fmt::v9::format("InvitationService : {}", e.what());
      serverLogger->error(msg);
      throw ServiceException(msg);
    }
  }

  bool removeIfExpired(mysqlx::Session &session, uint64_t invitationId) {
    try {
      auto invitation = invitationRepository->findById(session, invitationId);
      if (invitation != nullptr) {
        if (std::dynamic_pointer_cast<dao::Invitation>(invitation)
                ->getExpiredAt() < module::getCurrentTime()) {

          if (invitationRepository->remove(session, invitation)) {
            return true;
          } else {
            throw NotRemovedEntityException(fmt::v9::format(
                "InvitationService : id={} cannot be removed", invitationId));
          }
        } else {
          // expired > current : Don't expire invitation!
          return false;
        }
      } else {
        throw NotFoundEntityException(fmt::v9::format(
            "InvitationService : id={} not in Invitation", invitationId));
      }
    } catch (const NotRemovedEntityException &e) {
      serverLogger->error(e.what());
      throw;
    } catch (const NotFoundEntityException &e) {
      serverLogger->error(e.what());
      throw;
    } catch (const std::exception &e) {
      auto msg = fmt::v9::format("InvitationService : {}", e.what());
      serverLogger->error(msg);
      throw ServiceException(msg);
    }
  }

  bool compare(uint64_t userId, uint64_t roomId, std::string receivedPw) {
    /**
     * If invitation is expired, remove & return false
     * If password is correct, remove & return true
     * If password is incorrect, don't remove & return false
     */

    auto session = std::unique_ptr<mysqlx::Session>(nullptr);
    try {
      session = std::make_unique<mysqlx::Session>(conn->client->getSession());
      session->startTransaction();

      auto invitation = std::dynamic_pointer_cast<dao::InvitationRepository>(
                            invitationRepository)
                            ->findByUserIdInRoom(*session, userId, roomId);

      if (invitation == nullptr) {
        throw NotFoundEntityException(fmt::v9::format(
            "InvitationService : userId={} AND roomId={} not in Invitation",
            userId, roomId));
      }
      if (removeIfExpired(*session, invitation->getId()) == true) {
        throw NotFoundEntityException(
            fmt::v9::format("InvitationService : userId={} AND roomId={} not "
                            "in Invitation(Expired)",
                            userId, roomId));
      }
      auto storedPw =
          std::dynamic_pointer_cast<dao::Invitation>(invitation)->getPassword();

      if (storedPw == receivedPw) {
        if (invitationRepository->remove(*session, invitation)) {
          session->commit();
          return true;
        } else {
          throw NotRemovedEntityException(fmt::v9::format(
              "InvitationService : userId={} AND roomId={} cannot "
              "be removed",
              userId, roomId));
        }
      } else {
        session->rollback();
        return false;
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
      auto msg = fmt::v9::format("InvitationService : {}", e.what());
      serverLogger->error(msg);
      throw ServiceException(msg);
    }
  }

  R save(uint64_t userId, uint64_t roomId) {
    /**
     * ExpiredAt : current + 30min
     * If userId & roomId already in invitation, don't re-generate invitation
     * Length = 8
     */
    constexpr auto codeLength = 8;
    constexpr time_t timeOffset = 1800l;

    auto session = std::unique_ptr<mysqlx::Session>(nullptr);
    try {
      session = std::make_unique<mysqlx::Session>(conn->client->getSession());
      session->startTransaction();

      auto invitation = std::dynamic_pointer_cast<dao::InvitationRepository>(
                            invitationRepository)
                            ->findByUserIdInRoom(*session, userId, roomId);
      if (invitation == nullptr) {
        auto pw = module::secure::generateFixedLengthCode(codeLength);
        auto expiredAt = static_cast<time_t>(module::getCurrentTime() +
                                             timeOffset); // 1800초 = 30분!
        invitation = std::make_unique<dao::Invitation>(
            roomId, userId, module::convertToLocalTimeTM(expiredAt), pw);
        invitation = invitationRepository->save(*session, invitation);

        if (invitation != nullptr) {
          session->commit();
          return invitation;
        } else {
          throw NotSavedEntityException(fmt::v9::format(
              "InvitationService : userId={} AND roomId={} cannot be saved",
              userId, roomId));
        }
      } else {
        throw DuplicatedEntityException(fmt::v9::format(
            "InvitationService : userId={} AND roomId={} already in Invitation",
            userId, roomId));
      }
    } catch (const NotSavedEntityException &e) {
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

    } catch (const std::exception &e) {
      if (session != nullptr) {
        session->rollback();
      }
      auto msg = fmt::v9::format("InvitationService : {}", e.what());
      serverLogger->error(msg);
      throw ServiceException(msg);
    }
  }

  void sendEmail(uint64_t inviatationId) {
    /**
     * Use postfix
     * Postfix runs in docker container named by "postfix"
     */
    auto session = std::unique_ptr<mysqlx::Session>(nullptr);
    try {
      session = std::make_unique<mysqlx::Session>(conn->client->getSession());
      session->startTransaction();

      auto invitation = invitationRepository->findById(*session, inviatationId);

      if (invitation != nullptr) {
        auto code = std::dynamic_pointer_cast<dao::Invitation>(invitation)
                        ->getPassword();
        auto userId =
            std::dynamic_pointer_cast<dao::Invitation>(invitation)->getUserId();
        auto roomId =
            std::dynamic_pointer_cast<dao::Invitation>(invitation)->getRoomId();
        auto user =
            std::dynamic_pointer_cast<dao::User>(userService->findById(userId));
        auto room =
            std::dynamic_pointer_cast<dao::Room>(roomService->findById(roomId));

        auto expiredAt = std::dynamic_pointer_cast<dao::Invitation>(invitation)
                             ->getExpiredAt();

        auto title = "[Secure Chat Service]";

        auto msg = fmt::v9::format("Welcome, {}\n"
                                   "Room {} invites you\n"
                                   "Your verified Code is {}\n"
                                   "ExpiredAt: {} (KST/Seoul)\n",
                                   user->getName(), room->getName(), code,
                                   module::convertToLocalTimeString(expiredAt));

        auto cmd = fmt::v9::format(
            "docker exec -it postfix bash -c 'echo \"{}\" | mail -s \"{}\" {}'",
            msg, title, user->getEmail());

        // Run with another thread
        std::thread(system, cmd.c_str()).join();

      } else {
        throw NotFoundEntityException(fmt::v9::format(
            "InvitationService : id={} not in Invitation", inviatationId));
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
      auto msg = fmt::v9::format("InvitationService : {}", e.what());
      serverLogger->error(msg);
      throw ServiceException(msg);
    }
  }

private:
  static std::shared_ptr<InvitationService> instance;
  static std::mutex createMutex;

  RP invitationRepository;
  std::shared_ptr<UserService> userService;
  std::shared_ptr<RoomService> roomService;

  InvitationService() = delete;
};

std::shared_ptr<InvitationService> InvitationService::instance = nullptr;
std::mutex InvitationService::createMutex{};
} // namespace chat::service
