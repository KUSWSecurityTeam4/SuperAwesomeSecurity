#pragma once

#include "../dao/base/entity.hpp"

#include "../dto/response.hpp"

#include "../module/common.hpp"
#include "../module/connection.hpp"
#include "../module/exception.hpp"

#include "../service/auth.hpp"
#include "../service/base.hpp"

#include <spdlog/logger.h>

#include <cpprest/http_listener.h>
#include <cpprest/json.h>
#include <cpprest/uri.h>

#include <memory>
#include <string.h>
#include <vector>

namespace chat::controller {
class BaseController {
public:
  using R = std::shared_ptr<dto::Response>;
  using E = std::shared_ptr<dao::Base>;
  using L = std::shared_ptr<spdlog::logger>;
  using CN = std::shared_ptr<module::Connection>;
  using SV = std::shared_ptr<service::BaseService>;
  using CONFIG = web::http::experimental::listener::http_listener_config;

  virtual void listen() = 0;

protected:
  CN conn;
  L serverLogger;
  SV authService;
  web::uri baseUri;
  CONFIG config;

  static std::string bodyAt(const web::json::value &body, std::string key) {
    auto value = module::trim(body.at(key).serialize(), '"');
    return value;
  }

  E authenticateAccess(web::json::value body) {
    if (body.has_field("session-id") && body.has_field("session-token")) {
      auto rawSessionId = bodyAt(body, "session-id");

      if (module::isNumber(rawSessionId) == false) {
        throw NotAuthorizedException(fmt::v9::format("not authorized"));
      }

      uint64_t sessionId = std::stoull(rawSessionId);
      auto sessionToken = bodyAt(body, "session-token");

      if (std::dynamic_pointer_cast<service::AuthService>(authService)
              ->verifyToken(sessionId, sessionToken) == false) {
        throw NotAuthorizedException(fmt::v9::format("not authorized"));
      }
      auto session =
          std::dynamic_pointer_cast<service::AuthService>(authService)
              ->getSession(sessionId);
      auto entity =
          std::dynamic_pointer_cast<dao::ServerSession>(session)->getValue();
      return entity;

    } else {
      throw NotAuthorizedException(fmt::v9::format("not authorized"));
    }
  }

  BaseController(web::uri baseUri, L serverLogger, CN conn, CONFIG &config)
      : serverLogger(serverLogger), conn(conn), config(config),
        authService(service::AuthService::getInstance(serverLogger, conn)) {}
  BaseController() = delete;
};
} // namespace chat::controller