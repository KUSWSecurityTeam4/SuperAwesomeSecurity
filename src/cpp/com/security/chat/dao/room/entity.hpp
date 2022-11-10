#pragma once

#include "../base/entity.hpp"

#include "../../module/common.hpp"

#include <fmt/core.h>

#include <chrono>
#include <ctime>
#include <string>

namespace chat::dao {

class Room : public Base {
public:
  std::string getName() const { return this->name; }
  void setName(std::string name) { this->name = name; }

  time_t getDeletedAt() const {
    return std::mktime(const_cast<std::tm *>(&this->deletedAt));
  }
  void setDeletedAt(std::tm deletedAt) { this->deletedAt = deletedAt; }

  Room(std::string name, std::tm deletedAt = std::tm{}, uint64_t id = -1,
       time_t createdAt = 0, time_t lastModifiedAt = 0)
      : Base(id, createdAt, lastModifiedAt), name(name), deletedAt(deletedAt) {}

  operator std::string() const {

    return fmt::v9::format(
        "Room(id={}, name={}, deletedAt={}, createdAt={}, lastModifiedAt={})",
        id, name,
        module::convertToLocalTimeString(
            std::mktime(const_cast<std::tm *>(&deletedAt))),
        module::convertToLocalTimeString(createdAt),
        module::convertToLocalTimeString(lastModifiedAt));
  }

private:
  std::string name;
  std::tm deletedAt; // 'deletedAt = 0'은 초깃값(1970-01-01)

  Room() = delete;
};
} // namespace chat::dao