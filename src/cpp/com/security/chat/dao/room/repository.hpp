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

class RoomRepository : public BaseRepository {
public:
  static std::shared_ptr<RoomRepository> getInstance(L repoLogger) {
    std::lock_guard<std::mutex> lock(createMutex);
    if (instance == nullptr) {
      instance = std::make_shared<RoomRepository>(repoLogger);
    }
    return instance;
  }

  RoomRepository(L repoLogger) : BaseRepository(repoLogger, "chat_room"){};

  R findById(mysqlx::Session &session, uint64_t id) override {
    return findBy(session, fmt::v9::format("room_id={}", id));
  }

  R findByName(mysqlx::Session &session, std::string name) {
    if (module::secure::verifyUserInput(name)) {
      return findBy(session, fmt::v9::format("name='{}'", name));
    } else {
      const auto msg =
          fmt::v9::format("RoomRepository: name={} is invalid format", name);
      repoLogger->error(msg);
      throw EntityException(msg);
    }
  }
  std::list<R> findAll(mysqlx::Session &session) {
    return findAllBy(session, fmt::v9::format("true"));
  }

  R save(mysqlx::Session &session, E entity) override {
    std::lock_guard<std::mutex> lock(sessionMutex);
    try {
      auto tableInsert =
          getTable(session, tableName).insert("name", "deleted_at");
      const auto room = std::dynamic_pointer_cast<Room>(entity);
      if (!module::secure::verifyUserInput(room->getName())) {
        const auto msg = fmt::v9::format(
            "RoomRepository: name={} is invalid format", room->getName());
        repoLogger->error(msg);
        throw EntityException(msg);
      }
      const auto row =
          mysqlx::Row(room->getName(),
                      module::convertToLocalTimeString(room->getDeletedAt()));
      const auto result = tableInsert.values(row).execute();
      return findById(session, result.getAutoIncrementValue());
    } catch (const std::exception &e) {
      auto msg = fmt::v9::format("RoomRepository: {}", e.what());
      repoLogger->error(msg);
      throw EntityException(msg);
    }
  }

  R update(mysqlx::Session &session, E entity) override {
    std::lock_guard<std::mutex> lock(sessionMutex);
    try {
      auto tableUpdate = getTable(session, tableName).update();
      const auto room = std::dynamic_pointer_cast<Room>(entity);
      if (!module::secure::verifyUserInput(room->getName())) {
        const auto msg = fmt::v9::format(
            "RoomRepository: name={} is invalid format", room->getName());
        repoLogger->error(msg);
        throw EntityException(msg);
      }
      const auto condition = fmt::v9::format("room_id={}", room->getId());
      const auto result =
          tableUpdate.set("name", room->getName())
              .set("deleted_at",
                   module::convertToLocalTimeString(room->getDeletedAt()))
              .set("last_modified_at",
                   module::convertToLocalTimeString(module::getCurrentTime()))
              .where(condition)
              .execute();
      return findById(session, room->getId());
    } catch (const std::exception &e) {
      auto msg = fmt::v9::format("RoomRepository: {}", e.what());
      repoLogger->error(msg);
      throw EntityException(msg);
    }
  }
  bool remove(mysqlx::Session &session, E entity) override {
    std::lock_guard<std::mutex> lock(sessionMutex);
    try {
      auto tableRemove = getTable(session, tableName).remove();
      const auto room = std::dynamic_pointer_cast<Room>(entity);
      const auto condition = fmt::v9::format("room_id={}", room->getId());
      const auto result = tableRemove.where(condition).execute();
      return true;
    } catch (const std::exception &e) {
      auto msg = fmt::v9::format("RoomRepository: {}", e.what());
      repoLogger->error(msg);
      throw EntityException(msg);
    }
  }

private:
  static std::shared_ptr<RoomRepository> instance;
  static std::mutex createMutex;
  RoomRepository() = delete;

  R findBy(mysqlx::Session &session, std::string condition) {
    try {
      auto tableSelect =
          getTable(session, tableName)
              .select("name", getUnixTimestampFormatter("deleted_at"),
                      "room_id", getUnixTimestampFormatter("created_at"),
                      getUnixTimestampFormatter("last_modified_at"));
      auto result = tableSelect.where(condition).execute();

      if (result.count() > 1) {
        const auto msg =
            fmt::v9::format("RoomRepository : more than one rows are selected");
        throw EntityException(msg);
      }
      auto row = result.fetchOne();

      if (row.isNull() != true) {
        auto entity =
            R(new Room(std::string(row.get(0)),
                       module::convertToLocalTimeTM(convertToTimeT(row.get(1))),
                       uint64_t(row.get(2)), convertToTimeT(row.get(3)),
                       convertToTimeT(row.get(4))));
        return entity;
      } else {
        return nullptr;
      }
    } catch (const std::exception &e) {
      auto msg = fmt::v9::format("RoomRepository: {}", e.what());
      repoLogger->error(msg);
      throw EntityException(msg);
    }
  }

  std::list<R> findAllBy(mysqlx::Session &session, std::string condition) {
    try {
      auto tableSelect =
          getTable(session, tableName)
              .select("name", getUnixTimestampFormatter("deleted_at"),
                      "room_id", getUnixTimestampFormatter("created_at"),
                      getUnixTimestampFormatter("last_modified_at"));
      auto result = tableSelect.where(condition).execute();

      auto rawList = result.fetchAll();
      auto filteredRawList = std::list<mysqlx::Row>{};
      std::copy_if(rawList.begin(), rawList.end(),
                   std::back_inserter(filteredRawList),
                   [](mysqlx::Row row) { return row.isNull() != true; });

      auto roomList = std::list<R>{};
      std::transform(
          filteredRawList.begin(), filteredRawList.end(),
          std::back_inserter(roomList), [](mysqlx::Row row) -> R {
            return R(new Room(
                std::string(row.get(0)),
                module::convertToLocalTimeTM(convertToTimeT(row.get(1))),
                uint64_t(row.get(2)), convertToTimeT(row.get(3)),
                convertToTimeT(row.get(4))));
          });
      return roomList;
    } catch (const std::exception &e) {
      auto msg = fmt::v9::format("RoomRepository: {}", e.what());
      repoLogger->error(msg);
      throw EntityException(msg);
    }
  }
};

std::shared_ptr<RoomRepository> RoomRepository::instance = nullptr;
std::mutex RoomRepository::createMutex{};
} // namespace chat::dao