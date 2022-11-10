#pragma once

#include <chrono>
#include <memory>

namespace chat::dao {

class Base {
public:
  uint64_t getId() const { return this->id; }

  time_t getCreatedAt() const { return this->createdAt; }

  time_t getLastModifiedAt() const { return this->lastModifiedAt; }

protected:
  Base(uint64_t id = -1, time_t createdAt = 0, time_t lastModifiedAt = 0)
      : id(id), createdAt(createdAt), lastModifiedAt(lastModifiedAt) {}

  uint64_t id;
  time_t createdAt;
  time_t lastModifiedAt;

  Base() = delete;

  // For polymorphism, at lease one virtual method is required.
  virtual ~Base() = default;
};
} // namespace chat::dao
