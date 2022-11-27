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
    if (module::secure::verifyUserInput(role)) {
      return findAllBy(
          session, fmt::v9::format("role='{}' AND room_id={}", role, roomId));
    } else {
      const auto msg = fmt::v9::format(
          "ParticipantRepository: role={} is invalid format", role);
      repoLogger->error(msg);
      throw EntityException(msg);
    }
  }

  R save(mysqlx::Session &session, E entity) override {
    std::lock_guard<std::mutex> lock(sessionMutex);
    try {
      auto tableInsert =
          getTable(session, tableName).insert("room_id", "user_id", "role");
      auto participant = std::dynamic_pointer_cast<Participant>(entity);
      if (!module::secure::verifyUserInput(participant->getRole())) {
        const auto msg =
            fmt::v9::format("ParticipantRepository: role={} is invalid format",
                            participant->getRole());
        repoLogger->error(msg);
        throw EntityException(msg);
      }
      const auto row =
          mysqlx::Row(participant->getRoomId(), participant->getUserId(),
                      participant->getRole());
      const auto result = tableInsert.values(row).execute();
      return findById(session, result.getAutoIncrementValue());
    } catch (const std::exception &e) {
      auto msg = fmt::v9::format("ParticipantRepository: {}", e.what());
      repoLogger->error(msg);
      throw EntityException(msg);
    }
  }

  R update(mysqlx::Session &session, E entity) override {
    std::lock_guard<std::mutex> lock(sessionMutex);
    try {
      auto tableUpdate = getTable(session, tableName).update();
      auto participant = std::dynamic_pointer_cast<Participant>(entity);
      if (!module::secure::verifyUserInput(participant->getRole())) {
        const auto msg =
            fmt::v9::format("ParticipantRepository: role={} is invalid format",
                            participant->getRole());
        repoLogger->error(msg);
        throw EntityException(msg);
      }
      const auto condition =
          fmt::v9::format("participant_id={}", participant->getId());
      const auto result =
          tableUpdate.set("room_id", participant->getRoomId())
              .set("user_id", participant->getUserId())
              .set("role", participant->getRole())
              .set("last_modified_at",
                   module::convertToLocalTimeString(module::getCurrentTime()))
              .where(condition)
              .execute();
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
      auto tableRemove = getTable(session, tableName).remove();
      auto participant = std::dynamic_pointer_cast<Participant>(entity);
      const auto condition =
          fmt::v9::format("participant_id={}", participant->getId());
      const auto result = tableRemove.where(condition).execute();
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
      auto tableSelect =
          getTable(session, tableName)
              .select("room_id", "user_id", "role", "participant_id",
                      getUnixTimestampFormatter("created_at"),
                      getUnixTimestampFormatter("last_modified_at"));
      auto result = tableSelect.where(condition).execute();

      if (result.count() > 1) {
        const auto msg = fmt::v9::format(
            "ParticipantRepository : more than one rows are selected");
        throw EntityException(msg);
      }
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
      auto tableSelect =
          getTable(session, tableName)
              .select("room_id", "user_id", "role", "participant_id",
                      getUnixTimestampFormatter("created_at"),
                      getUnixTimestampFormatter("last_modified_at"));
      auto result = tableSelect.where(condition).execute();

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