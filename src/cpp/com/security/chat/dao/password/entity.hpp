#pragma once

#include "../base/entity.hpp"

#include "../../module/common.hpp"

#include <fmt/core.h>

#include <chrono>
#include <string>

namespace chat::dao {

class Password : public Base {
public:
  uint64_t getUserId() const { return this->userId; }
  void setUserId(uint64_t userId) { this->userId = userId; }

  uint64_t getCompanyId() const { return this->companyId; }
  void setCompanyId(uint64_t companyId) { this->companyId = companyId; }

  std::string getSalt() const { return this->salt; }
  void setSalt(std::string salt) { this->salt = salt; }

  std::string getHashedPw() const { return this->hashedPw; }
  void setHashedPw(std::string hashedPw) { this->hashedPw = hashedPw; }

  Password(uint64_t userId, std::string salt, std::string hashedPw,
           uint64_t companyId = -1, uint64_t id = -1, time_t createdAt = 0,
           time_t lastModifiedAt = 0)
      : Base(id, createdAt, lastModifiedAt), userId(userId), salt(salt),
        hashedPw(hashedPw), companyId(companyId) {}

  operator std::string() const {
    return fmt::v9::format(
        "Password(id={}, userId={}, companyId={}, hashedPw={}, salt={}, "
        "createdAt={}, lastModifiedAt={})",
        id, userId, companyId, hashedPw, salt,
        module::convertToLocalTimeString(createdAt),
        module::convertToLocalTimeString(lastModifiedAt));
  }

private:
  uint64_t userId;
  uint64_t companyId;
  std::string salt;
  std::string hashedPw;

  Password() = delete;
};
} // namespace chat::dao