#pragma once

#include "../dao/participant/entity.hpp"
#include "../dao/participant/repository.hpp"

#include "../module/all.hpp"
using namespace chat::module::exception;

#include "base.hpp"
#include "room.hpp"
#include "user.hpp"

#include <mysqlx/xdevapi.h>

#include <fmt/core.h>

#include <exception>
#include <list>
#include <memory>
#include <mutex>
#include <set>
#include <string>

namespace chat::service {

class ParticipantService : public BaseService {
public:
  static std::shared_ptr<ParticipantService> getInstance(L serverLogger,
                                                         CN conn) {
    std::lock_guard<std::mutex> lock(createMutex);
    if (instance == nullptr) {
      instance = std::make_shared<ParticipantService>(serverLogger, conn);
    }
    return instance;
  }
  ParticipantService(L serverLogger, CN conn)
      : BaseService(serverLogger, conn),
        participantRepository(
            dao::ParticipantRepository::getInstance(serverLogger)),
        roomService(RoomService::getInstance(serverLogger, conn)),
        userService(UserService::getInstance(serverLogger, conn)) {}

  R findById(uint64_t participantId) {
    auto session = std::unique_ptr<mysqlx::Session>(nullptr);
    try {
      session = std::make_unique<mysqlx::Session>(conn->client->getSession());
      session->startTransaction();
      auto participant =
          participantRepository->findById(*session, participantId);
      if (participant != nullptr) {
        session->commit();
        return participant;
      } else {
        throw NotFoundEntityException(fmt::v9::format(
            "ParticipantService: id={} not in Participant", participantId));
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
      auto msg = fmt::v9::format("ParticipantService : {}", e.what());
      serverLogger->error(msg);
      throw ServiceException(msg);
    }
  }

  R findByUserIdInRoom(uint64_t userId, uint64_t roomId) {
    auto session = std::unique_ptr<mysqlx::Session>(nullptr);
    try {
      session = std::make_unique<mysqlx::Session>(conn->client->getSession());
      session->startTransaction();
      auto participant = std::dynamic_pointer_cast<dao::ParticipantRepository>(
                             participantRepository)
                             ->findByUserIdInRoom(*session, userId, roomId);
      if (participant != nullptr) {
        session->commit();
        return participant;
      } else {
        throw NotFoundEntityException(fmt::v9::format(
            "ParticipantService: user={} AND room={} not in Participant",
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
      auto msg = fmt::v9::format("ParticipantService : {}", e.what());
      serverLogger->error(msg);
      throw ServiceException(msg);
    }
  }

  std::list<R> findAllInRoom(uint64_t roomId) {
    auto session = std::unique_ptr<mysqlx::Session>(nullptr);
    try {
      session = std::make_unique<mysqlx::Session>(conn->client->getSession());
      session->startTransaction();
      auto participantList =
          std::dynamic_pointer_cast<dao::ParticipantRepository>(
              participantRepository)
              ->findAllInRoom(*session, roomId);
      if (participantList.size() > 0) {
        session->commit();
        return participantList;
      } else {
        throw NotFoundEntityException(fmt::v9::format(
            "ParticipantService: room={} not in Participant", roomId));
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
      auto msg = fmt::v9::format("ParticipantService : {}", e.what());
      serverLogger->error(msg);
      throw ServiceException(msg);
    }
  }

  R save(uint64_t roomId, uint64_t userId, std::string role) {
    auto session = std::unique_ptr<mysqlx::Session>(nullptr);
    try {
      session = std::make_unique<mysqlx::Session>(conn->client->getSession());
      session->startTransaction();

      // room & user Service throw NotFoundEntityException if entity doesn't
      // exist
      roomService->findById(roomId);
      userService->findById(userId);

      auto participant = std::dynamic_pointer_cast<dao::ParticipantRepository>(
                             participantRepository)
                             ->findByUserIdInRoom(*session, userId, roomId);
      if (participant != nullptr) {
        throw DuplicatedEntityException(fmt::v9::format(
            "ParticipantService: user={} already in room={}", userId, roomId));
      }

      if (isCorrectRole(role) == false) {
        throw NotSavedEntityException(
            fmt::v9::format("ParticipantService: not support role={}", role));
      }

      auto roleType = dao::Participant::convertToType(role);

      if (roleType == dao::Participant::TYPE::HOST) {
        // Only one host is permitted in each room
        if (std::dynamic_pointer_cast<dao::ParticipantRepository>(
                participantRepository)
                ->findAllByRoleInRoom(
                    *session, dao::Participant::convertToString(roleType),
                    roomId)
                .size() > 0) {
          throw NotSavedEntityException(fmt::v9::format(
              "ParticipantService: host already in room={}", roomId));
        }
      }

      participant = std::make_unique<dao::Participant>(roomId, userId, role);
      participant = participantRepository->save(*session, participant);

      if (participant != nullptr) {
        session->commit();
        return participant;
      } else {
        throw NotSavedEntityException(fmt::v9::format(
            "ParticipantService: userId={}, roomId={} cannot be saved", userId,
            roomId));
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
      auto msg = fmt::v9::format("ParticipantService : {}", e.what());
      serverLogger->error(msg);
      throw ServiceException(msg);
    }
  }

  bool remove(uint64_t participantId) {
    /**
     * Host cannot be removed
     * Host is removed when the room is removed
     */
    auto session = std::unique_ptr<mysqlx::Session>(nullptr);
    try {
      session = std::make_unique<mysqlx::Session>(conn->client->getSession());
      session->startTransaction();
      auto participant =
          participantRepository->findById(*session, participantId);

      if (participant == nullptr) {
        throw NotFoundEntityException(fmt::v9::format(
            "ParticipantService: id={} not in Participant", participantId));
      }

      uint64_t roomId =
          std::dynamic_pointer_cast<dao::Participant>(participant)->getRoomId();
      auto host = roomService->findHost(roomId);
      if (host->getId() == participant->getId()) {
        throw NotRemovedEntityException(
            fmt::v9::format("ParticipantService: id={} cannot be removed(host)",
                            participantId));
      }

      if (participantRepository->remove(*session, participant)) {
        session->commit();
        return true;
      } else {
        throw NotRemovedEntityException(fmt::v9::format(
            "ParticipantService: id={} cannot be removed", participantId));
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
      auto msg = fmt::v9::format("ParticipantService : {}", e.what());
      serverLogger->error(msg);
      throw ServiceException(msg);
    }
  }

  bool isCorrectRole(const std::string &role) const {
    auto roleType = dao::Participant::convertToType(role);
    return roleType != dao::Participant::TYPE::INVALID;
  }

private:
  static std::shared_ptr<ParticipantService> instance;
  static std::mutex createMutex;

  RP participantRepository;
  std::shared_ptr<RoomService> roomService;
  std::shared_ptr<UserService> userService;

  ParticipantService() = delete;
};

std::shared_ptr<ParticipantService> ParticipantService::instance = nullptr;
std::mutex ParticipantService::createMutex{};
} // namespace chat::service
