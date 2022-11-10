#pragma once

#include "../dao/participant/entity.hpp"
#include "../dao/participant/repository.hpp"
#include "../dao/room/entity.hpp"
#include "../dao/room/repository.hpp"

#include "../module/all.hpp"
using namespace chat::module::exception;

#include "base.hpp"

#include <mysqlx/xdevapi.h>

#include <fmt/core.h>

#include <exception>
#include <list>
#include <memory>
#include <mutex>
#include <string>

namespace chat::service {

class RoomService : public BaseService {
public:
  static std::shared_ptr<RoomService> getInstance(L serverLogger, CN conn) {
    std::lock_guard<std::mutex> lock(createMutex);
    if (instance == nullptr) {
      instance = std::make_shared<RoomService>(serverLogger, conn);
    }
    return instance;
  }

  RoomService(L serverLogger, CN conn)
      : BaseService(serverLogger, conn),
        roomRepository(dao::RoomRepository::getInstance(serverLogger)),
        participantRepository(
            dao::ParticipantRepository::getInstance(serverLogger)) {}

  R findById(uint64_t roomId) {
    auto session = std::unique_ptr<mysqlx::Session>(nullptr);
    try {
      session = std::make_unique<mysqlx::Session>(conn->client->getSession());
      session->startTransaction();
      auto room = roomRepository->findById(*session, roomId);
      if (room != nullptr) {
        session->commit();
        return room;
      } else {
        throw NotFoundEntityException(
            fmt::v9::format("RoomService: id={} not in Room", roomId));
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
      auto msg = fmt::v9::format("RoomService : {}", e.what());
      serverLogger->error(msg);
      throw ServiceException(msg);
    }
  }

  R findByName(std::string roomName) {
    auto session = std::unique_ptr<mysqlx::Session>(nullptr);
    try {
      session = std::make_unique<mysqlx::Session>(conn->client->getSession());
      session->startTransaction();
      auto room = std::dynamic_pointer_cast<dao::RoomRepository>(roomRepository)
                      ->findByName(*session, roomName);
      if (room != nullptr) {
        session->commit();
        return room;
      } else {
        throw NotFoundEntityException(
            fmt::v9::format("RoomService: name={} not in Room", roomName));
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
      auto msg = fmt::v9::format("RoomService : {}", e.what());
      serverLogger->error(msg);
      throw ServiceException(msg);
    }
  }

  std::list<R> findAll() {
    auto session = std::unique_ptr<mysqlx::Session>(nullptr);
    try {
      session = std::make_unique<mysqlx::Session>(conn->client->getSession());
      session->startTransaction();
      auto roomList =
          std::dynamic_pointer_cast<dao::RoomRepository>(roomRepository)
              ->findAll(*session);
      if (roomList.size() > 0) {
        session->commit();
        return roomList;
      } else {
        throw NotFoundEntityException(
            fmt::v9::format("RoomService: nothing in Room"));
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
      auto msg = fmt::v9::format("RoomService : {}", e.what());
      serverLogger->error(msg);
      throw ServiceException(msg);
    }
  }

  R findAllInCompany() {
    throw NotImplementedException(
        fmt::v9::format("RoomService: findALlInCompany not implemented"));
  }

  R findHost(uint64_t roomId) {
    /**
     * Only one host is permitted
     */
    auto session = std::unique_ptr<mysqlx::Session>(nullptr);
    try {
      session = std::make_unique<mysqlx::Session>(conn->client->getSession());
      session->startTransaction();
      auto roomList =
          std::dynamic_pointer_cast<dao::RoomRepository>(roomRepository)
              ->findAll(*session);

      auto host = std::dynamic_pointer_cast<dao::ParticipantRepository>(
                      participantRepository)
                      ->findAllByRoleInRoom(*session,
                                            dao::Participant::convertToString(
                                                dao::Participant::TYPE::HOST),
                                            roomId);
      if (host.size() > 0) {
        // 'cause each room has only one host, just return one element
        return host.front();
      } else {
        throw NotFoundEntityException("RoomService: host not in Room");
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
      auto msg = fmt::v9::format("RoomService : {}", e.what());
      serverLogger->error(msg);
      throw ServiceException(msg);
    }
  }

  std::list<R> findAllGuestInRoom(uint64_t roomId) {
    auto session = std::unique_ptr<mysqlx::Session>(nullptr);
    try {
      session = std::make_unique<mysqlx::Session>(conn->client->getSession());
      session->startTransaction();
      auto roomList =
          std::dynamic_pointer_cast<dao::RoomRepository>(roomRepository)
              ->findAll(*session);

      auto guestList =
          std::dynamic_pointer_cast<dao::ParticipantRepository>(
              participantRepository)
              ->findAllByRoleInRoom(*session,
                                    dao::Participant::convertToString(
                                        dao::Participant::TYPE::GUEST),
                                    roomId);
      return guestList;

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
      auto msg = fmt::v9::format("RoomService : {}", e.what());
      serverLogger->error(msg);
      throw ServiceException(msg);
    }
  }

  R save(uint64_t companyId, std::string name) {
    /**
     * Name is CK
     * Future work. each room belongs in company
     */
    auto session = std::unique_ptr<mysqlx::Session>(nullptr);
    try {
      session = std::make_unique<mysqlx::Session>(conn->client->getSession());
      session->startTransaction();
      auto room = std::dynamic_pointer_cast<dao::RoomRepository>(roomRepository)
                      ->findByName(*session, name);
      if (room == nullptr) {
        room = std::make_unique<dao::Room>(name);

        room = roomRepository->save(*session, room);
        if (room != nullptr) {
          session->commit();
          return room;
        } else {
          throw NotSavedEntityException(
              fmt::v9::format("RoomService: name={} cannot be saved", name));
        }
      } else {
        throw DuplicatedEntityException(
            fmt::v9::format("RoomService: name={} already in Room", name));
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
      auto msg = fmt::v9::format("RoomService : {}", e.what());
      serverLogger->error(msg);
      throw ServiceException(msg);
    }
  }

  R update(uint64_t roomId, std::string name) {
    /**
     * Name is CK. So, Name MUST not be duplicated
     */
    auto session = std::unique_ptr<mysqlx::Session>(nullptr);
    try {
      session = std::make_unique<mysqlx::Session>(conn->client->getSession());
      session->startTransaction();
      auto room = std::dynamic_pointer_cast<dao::RoomRepository>(roomRepository)
                      ->findById(*session, roomId);
      if (room != nullptr) {
        if (std::dynamic_pointer_cast<dao::RoomRepository>(roomRepository)
                ->findByName(*session, name) == nullptr) {

          std::dynamic_pointer_cast<dao::Room>(room)->setName(name);
          room = roomRepository->update(*session, room);

          if (room != nullptr) {
            session->commit();
            return room;
          } else {
            throw NotUpdatedEntityException(fmt::v9::format(
                "RoomService: id={} cannot be updated", roomId));
          }
        } else {
          throw DuplicatedEntityException(
              fmt::v9::format("RoomService: name={} already in Room", name));
        }
      } else {
        throw NotFoundEntityException(
            fmt::v9::format("RoomService: id={} not in Room", roomId));
      }
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
      auto msg = fmt::v9::format("RoomService : {}", e.what());
      serverLogger->error(msg);
      throw ServiceException(msg);
    }
  }

  bool remove(uint64_t roomId) {
    /**
     * Whether guests exist in room or not, if room is rmoved, all participants
     * in room also deleted
     *
     * Participant has roomId for FK. So, first remove participants and then
     * remove room
     */
    auto session = std::unique_ptr<mysqlx::Session>(nullptr);
    try {
      session = std::make_unique<mysqlx::Session>(conn->client->getSession());
      session->startTransaction();
      auto room = roomRepository->findById(*session, roomId);
      if (room != nullptr) {
        for (auto guest : findAllGuestInRoom(roomId)) {
          participantRepository->remove(*session, guest);
        }
        auto host = findHost(roomId);
        if (host != nullptr) {
          if (participantRepository->remove(*session, host) == false) {
            throw NotRemovedEntityException(fmt::v9::format(
                "RoomService: cannot remove host in Room(id={})", roomId));
          }
        }
        if (roomRepository->remove(*session, room)) {
          session->commit();
          return true;
        } else {
          throw NotRemovedEntityException(
              fmt::v9::format("RoomService: id={} cannot removed", roomId));
        }
      } else {
        throw NotFoundEntityException(
            fmt::v9::format("RoomService: id={} not in Room", roomId));
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
      auto msg = fmt::v9::format("RoomService : {}", e.what());
      serverLogger->error(msg);
      throw ServiceException(msg);
    }
  }

private:
  static std::shared_ptr<RoomService> instance;
  static std::mutex createMutex;

  RP roomRepository;
  RP participantRepository;
  RoomService() = delete;
};

std::shared_ptr<RoomService> RoomService::instance = nullptr;
std::mutex RoomService::createMutex{};
} // namespace chat::service
