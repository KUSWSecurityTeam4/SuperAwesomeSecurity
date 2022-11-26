#pragma once

#include "../base/entity.hpp"

#include "../../module/common.hpp"

#include <chrono>
#include <string>

namespace chat::dao {

class ServerSession : public Base {
public:
  using V = std::shared_ptr<Base>;
  V getValue() const { return this->value; }
  void setValue(V value) { this->value = value; }

  time_t getExpiredAt() const {
    return std::mktime(const_cast<std::tm *>(&this->expiredAt));
  }
  void setExpiredAt(std::tm expiredAt) { this->expiredAt = expiredAt; }

  std::string getToken() const { return this->token; }
  void setToken(std::string token) { this->token = token; }

  void setId(uint64_t id) { this->id = id; }

  ServerSession(V value, std::string token, std::tm expiredAt, uint64_t id = -1,
                time_t createdAt = 0, time_t lastModifiedAt = 0)
      : Base(id, createdAt, lastModifiedAt), value(value), token(token),
        expiredAt(expiredAt) {}

private:
  V value;
  std::tm expiredAt;
  std::string token;

  ServerSession() = delete;
};
} // namespace chat::dao