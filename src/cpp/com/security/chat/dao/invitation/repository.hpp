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
  InvitationRepository(L repoLogger) : BaseRepository(repoLogger){};

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
      auto invitation = std::dynamic_pointer_cast<Invitation>(entity);
      auto query = fmt::v9::format(
          "INSERT INTO invitation(room_id, user_id, "
          "expired_at, password) VALUES({}, {}, '{}', '{}')",
          invitation->getRoomId(), invitation->getUserId(),
          module::convertToLocalTimeString(invitation->getExpiredAt()),
          invitation->getPassword());

      auto stmt = session.sql(query);
      auto result = stmt.execute();

      return findBy(
          session,
          fmt::v9::format(
              "room_id={} AND user_id={} AND password='{}' AND expired_at='{}'",
              invitation->getRoomId(), invitation->getUserId(),
              invitation->getPassword(),
              module::convertToLocalTimeString(invitation->getExpiredAt())));
    } catch (const std::exception &e) {
      auto msg = fmt::v9::format("InvitationRepository: {}", e.what());
      repoLogger->error(msg);
      throw EntityException(msg);
    }
  }

  R update(mysqlx::Session &session, E entity) override {
    std::lock_guard<std::mutex> lock(sessionMutex);
    try {
      auto invitation = std::dynamic_pointer_cast<Invitation>(entity);
      auto query = fmt::v9::format(
          "UPDATE invitation SET room_id={}, user_id={}, "
          "expired_at='{}', password='{}', "
          "last_modified_at=NOW() WHERE invitation_id={}",
          invitation->getRoomId(), invitation->getUserId(),
          module::convertToLocalTimeString(invitation->getExpiredAt()),
          invitation->getPassword(), invitation->getId());

      auto stmt = session.sql(query);
      auto result = stmt.execute();
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
      auto invitation = std::dynamic_pointer_cast<Invitation>(entity);
      auto query = fmt::v9::format(
          "DELETE FROM invitation WHERE invitation_id={}", invitation->getId());
      auto stmt = session.sql(query);
      auto result = stmt.execute();
      return true;
    } catch (const std::exception &e) {
      auto msg = fmt::v9::format("InvitationRepository: {}", e.what());
      repoLogger->error(msg);
      throw EntityException(msg);
    }
  }

private:
  InvitationRepository() = delete;

  R findBy(mysqlx::Session &session, std::string condition) {
    try {
      auto query = fmt::v9::format(
          "SELECT room_id, user_id, {}, password, invitation_id, {}, {} "
          "FROM invitation WHERE {}",
          getUnixTimestampFormatter("expired_at"),
          getUnixTimestampFormatter("created_at"),
          getUnixTimestampFormatter("last_modified_at"), condition);
      auto stmt = session.sql(query);
      auto result = stmt.execute();

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
} // namespace chat::dao