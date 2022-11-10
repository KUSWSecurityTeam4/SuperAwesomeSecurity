#pragma once

#include "../base/entity.hpp"

#include "../../module/common.hpp"

#include <fmt/core.h>

#include <chrono>
#include <string>

namespace chat::dao {

class Company : public Base {
public:
  std::string getName() const { return this->name; }
  void setName(std::string name) { this->name = name; }

  Company(std::string name, uint64_t id = -1, time_t createdAt = 0,
          time_t lastModifiedAt = 0)
      : Base(id, createdAt, lastModifiedAt), name(name) {}

  operator std::string() const {
    return fmt::v9::format(
        "Company(id={}, name={}, createdAt={}, lastModifiedAt={})", id, name,
        module::convertToLocalTimeString(createdAt),
        module::convertToLocalTimeString(lastModifiedAt));
  }

private:
  std::string name;
  Company() = delete;
};
} // namespace chat::dao