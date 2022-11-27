#pragma once

#include "../base/entity.hpp"
#include "../base/repository.hpp"
#include "./entity.hpp"

#include "../../module/all.hpp"
using namespace chat::module::exception;

#include <mysqlx/xdevapi.h>

#include <fmt/core.h>

#include <exception>
#include <memory>
#include <mutex>
#include <string>

namespace chat::dao {

class InvitationRepository : public BaseRepository {
public:
  static std::shared_ptr<InvitationRepository> getInstance(L repoLogger) {
    std::lock_guard<std::mutex> lock(createMutex);
    if (instance == nullptr) {
      instance = std::make_shared<InvitationRepository>(repoLogger);
    }
    return instance;
  }

  InvitationRepository(L repoLogger)
      : BaseRepository(repoLogger, "invitation"){};

  R findById(mysqlx::Session &session, uint64_t id) override {
    return findBy(session, fmt::v9::format("invitation_id={}", id));
  }

  R findByRoomId(mysqlx::Session &session, uint64_t roomId) {
    return findBy(session, fmt::v9::format("room_id={}", roomId));
  }

  R findByUserId(mysqlx::Session &session, uint64_t userId) {
    return findBy(session, fmt::v9::format("user_id={}", userId));
  }

  R findByUserIdInRoom(mysqlx::Session &session, uint64_t userId,
                       uint64_t roomId) {
    return findBy(session,
                  fmt::v9::format("user_id={} AND room_id={}", userId, roomId));
  }

  R save(mysqlx::Session &session, E entity) override {
    std::lock_guard<std::mutex> lock(sessionMutex);
    try {
      auto tableInsert =
          getTable(session, tableName)
              .insert("room_id", "user_id", "expired_at", "password");
      auto invitation = std::dynamic_pointer_cast<Invitation>(entity);
      const auto row = mysqlx::Row(
          invitation->getRoomId(), invitation->getUserId(),
          module::convertToLocalTimeString(invitation->getExpiredAt()),
          invitation->getPassword());
      const auto result = tableInsert.values(row).execute();
      return findById(session, result.getAutoIncrementValue());
    } catch (const std::exception &e) {
      auto msg = fmt::v9::format("InvitationRepository: {}", e.what());
      repoLogger->error(msg);
      throw EntityException(msg);
    }
  }

  R update(mysqlx::Session &session, E entity) override {
    std::lock_guard<std::mutex> lock(sessionMutex);
    try {
      auto tableUpdate = getTable(session, tableName).update();
      auto invitation = std::dynamic_pointer_cast<Invitation>(entity);
      const auto condition =
          fmt::v9::format("invitation_id={}", invitation->getId());
      const auto result =
          tableUpdate.set("room_id", invitation->getRoomId())
              .set("user_id", invitation->getUserId())
              .set("expired_at",
                   module::convertToLocalTimeString(invitation->getExpiredAt()))
              .set("password", invitation->getPassword())
              .set("last_modified_at",
                   module::convertToLocalTimeString(module::getCurrentTime()))
              .where(condition)
              .execute();
      return findById(session, invitation->getId());
    } catch (const std::exception &e) {
      auto msg = fmt::v9::format("InvitationRepository: {}", e.what());
      repoLogger->error(msg);
      throw EntityException(msg);
    }
  }

  bool remove(mysqlx::Session &session, E entity) override {
    std::lock_guard<std::mutex> lock(sessionMutex);
    try {
      auto tableRemove = getTable(session, tableName).remove();
      auto invitation = std::dynamic_pointer_cast<Invitation>(entity);
      const auto condition =
          fmt::v9::format("invitation_id={}", invitation->getId());
      const auto result = tableRemove.where(condition).execute();
      return true;
    } catch (const std::exception &e) {
      auto msg = fmt::v9::format("InvitationRepository: {}", e.what());
      repoLogger->error(msg);
      throw EntityException(msg);
    }
  }

private:
  static std::shared_ptr<InvitationRepository> instance;
  static std::mutex createMutex;
  InvitationRepository() = delete;

  R findBy(mysqlx::Session &session, std::string condition) {
    try {
      auto tableSelect =
          getTable(session, tableName)
              .select("room_id", "user_id",
                      getUnixTimestampFormatter("expired_at"), "password",
                      "invitation_id", getUnixTimestampFormatter("created_at"),
                      getUnixTimestampFormatter("last_modified_at"));
      auto result = tableSelect.where(condition).execute();

      if (result.count() != 1) {
        const auto msg = fmt::v9::format(
            "InvitationRepository : more than one rows are selected");
        throw EntityException(msg);
      }
      auto row = result.fetchOne();
      if (row.isNull() != true) {
        auto entity = R(new Invitation(
            uint64_t(row.get(0)), uint64_t(row.get(1)),
            module::convertToLocalTimeTM(convertToTimeT(row.get(2))),
            std::string(row.get(3)), uint64_t(row.get(4)),
            convertToTimeT(row.get(5)), convertToTimeT(row.get(6))));
        return entity;
      } else {
        return nullptr;
      }
    } catch (const std::exception &e) {
      auto msg = fmt::v9::format("InvitationRepository: {}", e.what());
      repoLogger->error(msg);
      throw EntityException(msg);
    }
  }
};

std::shared_ptr<InvitationRepository> InvitationRepository::instance = nullptr;
std::mutex InvitationRepository::createMutex{};
} // namespace chat::dao