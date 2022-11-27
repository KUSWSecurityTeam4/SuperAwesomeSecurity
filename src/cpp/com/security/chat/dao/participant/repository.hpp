#pragma once

#include "../base/entity.hpp"
#include "../base/repository.hpp"
#include "./entity.hpp"

#include "../../module/all.hpp"
using namespace chat::module::exception;

#include <mysqlx/xdevapi.h>

#include <fmt/core.h>

#include <algorithm>
#include <exception>
#include <iterator>
#include <list>
#include <memory>
#include <mutex>
#include <string>

namespace chat::dao {

class ParticipantRepository : public BaseRepository {
public:
  static std::shared_ptr<ParticipantRepository> getInstance(L repoLogger) {
    std::lock_guard<std::mutex> lock(createMutex);
    if (instance == nullptr) {
      instance = std::make_shared<ParticipantRepository>(repoLogger);
    }
    return instance;
  }

  ParticipantRepository(L repoLogger)
      : BaseRepository(repoLogger, "room_participant"){};

  R findById(mysqlx::Session &session, uint64_t id) override {
    return findBy(session, fmt::v9::format("participant_id={}", id));
  }

  R findByUserIdInRoom(mysqlx::Session &session, uint64_t userId,
                       uint64_t roomId) {
    return findBy(session,
                  fmt::v9::format("user_id={} AND room_id={}", userId, roomId));
  }

  std::list<R> findAllInRoom(mysqlx::Session &session, uint64_t roomId) {
    return findAllBy(session, fmt::v9::format("room_id={}", roomId));
  }

  std::list<R> findAllByRoleInRoom(mysqlx::Session &session, std::string role,
                                   uint64_t roomId) {
    return findAllBy(session,
                     fmt::v9::format("role='{}' AND room_id={}", role, roomId));
  }

  R save(mysqlx::Session &session, E entity) override {
    std::lock_guard<std::mutex> lock(sessionMutex);
    try {
      auto participant = std::dynamic_pointer_cast<Participant>(entity);
      auto query =
          fmt::v9::format("INSERT INTO room_participant(room_id, user_id, "
                          "role) VALUES({}, {}, '{}')",
                          participant->getRoomId(), participant->getUserId(),
                          participant->getRole());
      auto stmt = session.sql(query);
      auto result = stmt.execute();
      return findBy(session,
                    fmt::v9::format("room_id={} AND user_id={} AND role='{}'",
                                    participant->getRoomId(),
                                    participant->getUserId(),
                                    participant->getRole()));
    } catch (const std::exception &e) {
      auto msg = fmt::v9::format("ParticipantRepository: {}", e.what());
      repoLogger->error(msg);
      throw EntityException(msg);
    }
  }
  R update(mysqlx::Session &session, E entity) override {
    std::lock_guard<std::mutex> lock(sessionMutex);
    try {
      auto participant = std::dynamic_pointer_cast<Participant>(entity);
      auto query = fmt::v9::format(
          "UPDATE room_participant SET room_id={}, user_id={}, "
          "role='{}', last_modified_at=NOW() WHERE participant_id={}",
          participant->getRoomId(), participant->getUserId(),
          participant->getRole(), participant->getId());
      auto stmt = session.sql(query);
      auto result = stmt.execute();
      return findById(session, participant->getId());
    } catch (const std::exception &e) {
      auto msg = fmt::v9::format("ParticipantRepository: {}", e.what());
      repoLogger->error(msg);
      throw EntityException(msg);
    }
  }

  bool remove(mysqlx::Session &session, E entity) override {
    std::lock_guard<std::mutex> lock(sessionMutex);
    try {
      auto participant = std::dynamic_pointer_cast<Participant>(entity);
      auto query = fmt::v9::format(
          "DELETE FROM room_participant WHERE participant_id={}",
          participant->getId());
      auto stmt = session.sql(query);
      auto result = stmt.execute();
      return true;
    } catch (const std::exception &e) {
      auto msg = fmt::v9::format("ParticipantRepository: {}", e.what());
      repoLogger->error(msg);
      throw EntityException(msg);
    }
  }

private:
  static std::shared_ptr<ParticipantRepository> instance;
  static std::mutex createMutex;
  ParticipantRepository() = delete;

  R findBy(mysqlx::Session &session, std::string condition) {
    try {
      auto query = fmt::v9::format(
          "SELECT room_id, user_id, role, participant_id, {}, {} FROM "
          "room_participant WHERE {}",
          getUnixTimestampFormatter("created_at"),
          getUnixTimestampFormatter("last_modified_at"), condition);
      auto stmt = session.sql(query);
      auto result = stmt.execute();

      auto row = result.fetchOne();
      if (row.isNull() != true) {
        auto entity = R(new Participant(
            uint64_t(row.get(0)), uint64_t(row.get(1)), std::string(row.get(2)),
            uint64_t(row.get(3)), convertToTimeT(row.get(4)),
            convertToTimeT(row.get(5))));
        return entity;
      } else {
        return nullptr;
      }
    } catch (const std::exception &e) {
      auto msg = fmt::v9::format("ParticipantRepository: {}", e.what());
      repoLogger->error(msg);
      throw EntityException(msg);
    }
  }

  std::list<R> findAllBy(mysqlx::Session &session, std::string condition) {
    try {
      auto query = fmt::v9::format(
          "SELECT room_id, user_id, role, participant_id, {}, {} FROM "
          "room_participant WHERE {}",
          getUnixTimestampFormatter("created_at"),
          getUnixTimestampFormatter("last_modified_at"), condition);
      auto stmt = session.sql(query);
      auto result = stmt.execute();

      auto rawList = result.fetchAll();
      auto filteredRawList = std::list<mysqlx::Row>{};
      std::copy_if(rawList.begin(), rawList.end(),
                   std::back_inserter(filteredRawList),
                   [](mysqlx::Row row) { return row.isNull() != true; });

      auto participantList = std::list<R>{};
      std::transform(filteredRawList.begin(), filteredRawList.end(),
                     std::back_inserter(participantList), [](mysqlx::Row row) {
                       return R(new Participant(
                           uint64_t(row.get(0)), uint64_t(row.get(1)),
                           std::string(row.get(2)), uint64_t(row.get(3)),
                           convertToTimeT(row.get(4)),
                           convertToTimeT(row.get(5))));
                     });
      return participantList;
    } catch (const std::exception &e) {
      auto msg = fmt::v9::format("ParticipantRepository: {}", e.what());
      repoLogger->error(msg);
      throw EntityException(msg);
    }
  }
};

std::shared_ptr<ParticipantRepository> ParticipantRepository::instance =
    nullptr;
std::mutex ParticipantRepository::createMutex{};
} // namespace chat::dao