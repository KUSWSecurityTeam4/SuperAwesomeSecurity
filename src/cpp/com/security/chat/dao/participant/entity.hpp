#pragma once

#include "../base/entity.hpp"

#include "../../module/common.hpp"

#include <fmt/core.h>

#include <chrono>
#include <string>

namespace chat::dao {

class Participant : public Base {
public:
  enum class TYPE : uint8_t { HOST, GUEST, INVALID };
  static std::string convertToString(TYPE type) {
    switch (type) {
    case TYPE::HOST:
      return "host";
    case TYPE::GUEST:
      return "guest";
    case TYPE::INVALID:
      return "invalid";
    }
    return "invalid";
  }
  static TYPE convertToType(std::string type) {
    if (type == "host") {
      return TYPE::HOST;
    } else if (type == "guest") {
      return TYPE::GUEST;
    } else {
      return TYPE::INVALID;
    }
  }

  uint64_t getRoomId() const { return this->roomId; }
  void setRoomId(uint64_t roomId) { this->roomId = roomId; }

  uint64_t getUserId() const { return this->userId; }
  void setUserId(uint64_t userId) { this->userId = userId; }

  std::string getRole() const { return this->role; }
  void setRole(std::string role) { this->role = role; }

  Participant(uint64_t roomId, uint64_t userId, std::string role,
              uint64_t id = -1, time_t createdAt = 0, time_t lastModifiedAt = 0)
      : Base(id, createdAt, lastModifiedAt), roomId(roomId), userId(userId),
        role(role) {}

  operator std::string() const {
    return fmt::v9::format("Participant(id={}, roomId={}, userId={}, role={}, "
                           "createdAt={}, lastModifiedAt={})",
                           id, roomId, userId, role,
                           module::convertToLocalTimeString(createdAt),
                           module::convertToLocalTimeString(lastModifiedAt));
  }

private:
  uint64_t roomId;
  uint64_t userId;
  std::string role;

  Participant() = delete;
};
} // namespace chat::dao