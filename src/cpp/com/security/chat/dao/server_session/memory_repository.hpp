#pragma once

#include "../base/entity.hpp"
#include "../base/repository.hpp"
#include "./entity.hpp"

#include "../../module/all.hpp"
using namespace chat::module::exception;

#include <fmt/core.h>

#include <mysqlx/xdevapi.h>

#include <chrono>
#include <exception>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>

namespace chat::dao {

class ServerSessionRepository : public BaseRepository {
public:
  using K = uint64_t;
  using V = std::shared_ptr<ServerSession>;

  static std::shared_ptr<ServerSessionRepository> getInstance(L repoLogger) {
    std::lock_guard<std::mutex> lock(createMutex);
    if (instance == nullptr) {
      instance = std::make_shared<ServerSessionRepository>(repoLogger);
    }
    return instance;
  }

  ServerSessionRepository(L repoLogger) : BaseRepository(repoLogger), db{} {};

  R findById(mysqlx::Session &session, uint64_t id) override {
    try {
      auto serverSession = this->db.find(id);

      if (serverSession != this->db.end()) {
        auto entity = serverSession->second;
        return entity;
      } else {
        return nullptr;
      }
    } catch (const std::exception &e) {
      auto msg = fmt::v9::format("ServerSessionRepository : {}", e.what());
      repoLogger->error(msg);
      throw EntityException(msg);
    }
  }

  R save(mysqlx::Session &session, E entity) override {
    std::lock_guard<std::mutex> lock(sessionMutex);
    try {
      auto serverSession = std::dynamic_pointer_cast<ServerSession>(entity);
      auto key = module::secure::generateRandomNumber();
      while (findById(session, key) != nullptr) {
        key = module::secure::generateRandomNumber();
      }
      // The key is The id of entity
      serverSession->setId(key);
      auto &&[it, inserted] = this->db.insert({key, serverSession});

      if (inserted) {
        return it->second;
      } else {
        return nullptr;
      }
    } catch (const std::exception &e) {
      auto msg = fmt::v9::format("ServerSessionRepository : {}", e.what());
      repoLogger->error(msg);
      throw EntityException(msg);
    }
  }

  R update(mysqlx::Session &session, E entity) override {
    std::lock_guard<std::mutex> lock(sessionMutex);
    try {
      auto serverSession = std::dynamic_pointer_cast<ServerSession>(entity);
      auto &&[it, updated] =
          this->db.insert_or_assign(serverSession->getId(), serverSession);
      if (updated) {
        return it->second;
      } else {
        return nullptr;
      }
    } catch (const std::exception &e) {
      auto msg = fmt::v9::format("ServerSessionRepository : {}", e.what());
      repoLogger->error(msg);
      throw EntityException(msg);
    }
  }

  bool remove(mysqlx::Session &session, E entity) override {
    std::lock_guard<std::mutex> lock(sessionMutex);
    try {
      auto serverSession = std::dynamic_pointer_cast<ServerSession>(entity);
      auto erased = this->db.erase(serverSession->getId());
      if (erased != 0) {
        return true;
      } else {
        return false;
      }
    } catch (const std::exception &e) {
      auto msg = fmt::v9::format("ServerSessionRepository : {}", e.what());
      repoLogger->error(msg);
      throw EntityException(msg);
    }
  }

private:
  static std::shared_ptr<ServerSessionRepository> instance;
  static std::mutex createMutex;
  ServerSessionRepository() = delete;

  std::unordered_map<K, V> db;
};

std::shared_ptr<ServerSessionRepository> ServerSessionRepository::instance =
    nullptr;
std::mutex ServerSessionRepository::createMutex{};
} // namespace chat::dao