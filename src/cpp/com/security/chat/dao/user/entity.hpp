#pragma once

#include "../base/entity.hpp"

#include "../../module/common.hpp"

#include <fmt/core.h>

#include <chrono>
#include <string>

namespace chat::dao {
class User : public Base {
public:
  uint64_t getCompanyId() const { return this->companyId; }
  void setCompanyId(uint64_t companyId) { this->companyId = companyId; }

  std::string getName() const { return this->name; }
  void setName(std::string name) { this->name = name; }

  std::string getRole() const { return this->role; }
  void setRole(std::string role) { this->role = role; }

  std::string getEmail() const { return this->email; }
  void setEmail(std::string email) { this->email = email; }

  User(uint64_t companyId, std::string name, std::string role,
       std::string email, uint64_t id = -1, time_t createdAt = 0,
       time_t lastModifiedAt = 0)
      : Base(id, createdAt, lastModifiedAt), companyId(companyId), name(name),
        role(role), email(email) {}

  operator std::string() const {

    return fmt::v9::format(
        "User(id={}, companyId={}, name={}, role={}, email={}, createdAt={}, "
        "lastModifiedAt={})",
        id, companyId, name, role, email,
        module::convertToLocalTimeString(createdAt),
        module::convertToLocalTimeString(lastModifiedAt));
  }

private:
  uint64_t companyId;
  std::string name;
  std::string role; // ex) Boss, Developer ...
  std::string email;

  User() = delete;
};
} // namespace chat::dao