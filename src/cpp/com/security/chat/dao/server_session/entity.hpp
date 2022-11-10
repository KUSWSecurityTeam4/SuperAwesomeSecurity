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

  std::string getToken() const { return this->token; }
  void setToken(std::string token) { this->token = token; }

  void setId(uint64_t id) { this->id = id; }

  ServerSession(V value, std::string token, uint64_t id = -1,
                time_t createdAt = 0, time_t lastModifiedAt = 0)
      : Base(id, createdAt, lastModifiedAt), value(value), token(token) {}

private:
  V value;
  std::string token;

  ServerSession() = delete;
};
} // namespace chat::dao