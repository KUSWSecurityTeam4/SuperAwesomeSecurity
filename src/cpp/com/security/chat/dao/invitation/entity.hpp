#pragma once

#include "../base/entity.hpp"

#include "../../module/common.hpp"

#include <fmt/core.h>

#include <chrono>
#include <string>

namespace chat::dao {

class Invitation : public Base {
public:
  uint64_t getRoomId() const { return this->roomId; }
  void setRoomId(uint64_t roomId) { this->roomId = roomId; }

  uint64_t getUserId() const { return this->userId; }
  void setUserId(uint64_t userId) { this->userId = userId; }

  time_t getExpiredAt() const {
    return std::mktime(const_cast<std::tm *>(&this->expiredAt));
  }
  void setExpiredAt(std::tm expiredAt) { this->expiredAt = expiredAt; }

  std::string getPassword() const { return this->password; }
  void setPassword(std::string password) { this->password = password; }

  Invitation(uint64_t roomId, uint64_t userId, std::tm expiredAt,
             std::string password, uint64_t id = -1, time_t createdAt = 0,
             time_t lastModifiedAt = 0)
      : Base(id, createdAt, lastModifiedAt), roomId(roomId), userId(userId),
        expiredAt(expiredAt), password(password) {}

  operator std::string() const {
    return fmt::v9::format(
        "Invitation(id={}, roomId={}, userId={}, expiredAt={}, "
        "password={}, createdAt={}, lastModifiedAt={})",
        id, roomId, userId,
        module::convertToLocalTimeString(
            std::mktime(const_cast<std::tm *>(&expiredAt))),
        password, module::convertToLocalTimeString(createdAt),
        module::convertToLocalTimeString(lastModifiedAt));
  }

private:
  uint64_t roomId;
  uint64_t userId;
  std::tm expiredAt;
  std::string password;

  Invitation() = delete;
};
} // namespace chat::dao