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
    if (instance == nullptr) {
      instance = std::make_shared<RoomRepository>(repoLogger);
    }
    return instance;
  }

  RoomRepository(L repoLogger) : BaseRepository(repoLogger){};

  R findById(mysqlx::Session &session, uint64_t id) override {
    return findBy(session, fmt::v9::format("room_id={}", id));
  }

  R findByName(mysqlx::Session &session, std::string name) {
    return findBy(session, fmt::v9::format("name='{}'", name));
  }
  std::list<R> findAll(mysqlx::Session &session) {
    return findAllBy(session, fmt::v9::format("true"));
  }

  R save(mysqlx::Session &session, E entity) override {
    std::lock_guard<std::mutex> lock(sessionMutex);
    try {
      auto room = std::dynamic_pointer_cast<Room>(entity);
      time_t deletedAt = room->getDeletedAt();
      auto query = fmt::v9::format(
          "INSERT INTO chat_room(name, deleted_at) VALUES('{}', '{}')",
          room->getName(),
          module::convertToLocalTimeString(room->getDeletedAt()));
      auto stmt = session.sql(query);
      auto result = stmt.execute();
      return findByName(session, room->getName());
    } catch (const std::exception &e) {
      auto msg = fmt::v9::format("RoomRepository: {}", e.what());
      repoLogger->error(msg);
      throw EntityException(msg);
    }
  }
  R update(mysqlx::Session &session, E entity) override {
    std::lock_guard<std::mutex> lock(sessionMutex);
    try {
      auto room = std::dynamic_pointer_cast<Room>(entity);
      auto query = fmt::v9::format(
          "UPDATE chat_room SET name='{}', deleted_at='{}', "
          "last_modified_at=NOW() WHERE room_id={}",
          room->getName(),
          module::convertToLocalTimeString(room->getDeletedAt()),
          room->getId());

      auto stmt = session.sql(query);
      auto result = stmt.execute();

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
      auto room = std::dynamic_pointer_cast<Room>(entity);
      auto query = fmt::v9::format("DELETE FROM chat_room WHERE room_id={}",
                                   room->getId());
      auto stmt = session.sql(query);
      auto result = stmt.execute();
      return true;
    } catch (const std::exception &e) {
      auto msg = fmt::v9::format("RoomRepository: {}", e.what());
      repoLogger->error(msg);
      throw EntityException(msg);
    }
  }

private:
  static std::shared_ptr<RoomRepository> instance;
  RoomRepository() = delete;

  R findBy(mysqlx::Session &session, std::string condition) {
    try {
      auto query = fmt::v9::format(
          "SELECT name, {}, room_id, {}, {} FROM "
          "chat_room WHERE {}",
          getUnixTimestampFormatter("deleted_at"),
          getUnixTimestampFormatter("created_at"),
          getUnixTimestampFormatter("last_modified_at"), condition);
      auto stmt = session.sql(query);
      auto result = stmt.execute();

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
      auto query = fmt::v9::format(
          "SELECT name, {}, room_id, {}, {} FROM "
          "chat_room WHERE {}",
          getUnixTimestampFormatter("deleted_at"),
          getUnixTimestampFormatter("created_at"),
          getUnixTimestampFormatter("last_modified_at"), condition);
      auto stmt = session.sql(query);
      auto result = stmt.execute();

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
} // namespace chat::dao