#pragma once

#include "base.hpp"

#include "../dao/company/entity.hpp"
#include "../dao/server_session/entity.hpp"
#include "../dao/user/entity.hpp"

#include "../dto/response.hpp"

#include "../module/common.hpp"
#include "../module/exception.hpp"
#include "../module/secure.hpp"
using namespace chat::module::exception;

#include "../service/auth.hpp"

#include <fmt/core.h>

#include <cpprest/http_headers.h>
#include <cpprest/http_listener.h>
#include <cpprest/http_msg.h>
#include <cpprest/uri.h>
#include <cpprest/uri_builder.h>

#include <atomic>
#include <exception>
#include <functional>
#include <memory>
#include <string>
#include <thread>

namespace chat::controller {
class AuthController : public BaseController {
public:
  static std::shared_ptr<AuthController>
  getInstance(web::uri baseUri, L serverLogger, CN conn, CONFIG &config) {
    std::lock_guard<std::mutex> lock(createMutex);
    if (instance == nullptr) {
      instance =
          std::make_shared<AuthController>(baseUri, serverLogger, conn, config);
    }
    return instance;
  }
  AuthController(web::uri baseUri, L serverLogger, CN conn, CONFIG &config)
      : BaseController(baseUri, serverLogger, conn, config) {
    web::uri_builder baseBuilder{baseUri};

    this->loginUri = baseBuilder.set_path("/auth/login").to_uri();
    this->loginListener = web::http::experimental::listener::http_listener{
        this->loginUri, config};

    auto logoutConfig = CONFIG{config};
    this->logoutUri = baseBuilder.set_path("/auth/logout").to_uri();
    this->logoutListener = web::http::experimental::listener::http_listener{
        this->logoutUri, config};
  }

  static void handleLogin(web::http::http_request request) {
    /**
     * header : application/json
     * query : type=[company|user]
     * body :
     *  type=company : name=[name], password=[password]
     *  type=user : email=[email], password=[password]
     *
     * response : session & [user|company]
     */

    auto headers = request.headers();
    auto requestUri = request.absolute_uri();
    auto query = requestUri.query();

    try {
      // main routine
      auto body = request.extract_json().get();
      auto splitedQueries = web::uri::split_query(query);
      auto typeIter = splitedQueries.find("type");
      if (typeIter == splitedQueries.end()) {
        throw ControllerException(fmt::v9::format("not specified type"));
      }

      E session{nullptr};

      auto type = typeIter->second;
      if (type == "company") {
        auto name = bodyAt(body, "name");
        auto password = bodyAt(body, "password");

        session = std::dynamic_pointer_cast<service::AuthService>(
                      instance->authService)
                      ->loginOfCompany(name, password);

      } else if (type == "user") {
        auto email = bodyAt(body, "email");
        auto password = bodyAt(body, "password");
        session = std::dynamic_pointer_cast<service::AuthService>(
                      instance->authService)
                      ->loginOfUser(email, password);
      } else {
        throw ControllerException(fmt::v9::format("not specified type"));
      }
      auto entity =
          std::dynamic_pointer_cast<dao::ServerSession>(session)->getValue();

      auto msg =
          fmt::v9::format("AuthController[LOGIN]({})", requestUri.to_string());
      auto logMsg = fmt::v9::format("{} : {}", msg, "ok");
      auto sendMsg = logMsg;

      instance->serverLogger->info(logMsg);

      auto entityData = std::shared_ptr<dto::Data>(nullptr);
      if (type == "company") {
        entityData = std::make_unique<dto::CompanyData>(
            *std::dynamic_pointer_cast<dao::Company>(entity));
      } else {
        entityData = std::make_unique<dto::UserData>(
            *std::dynamic_pointer_cast<dao::User>(entity));
      }

      auto sessionData = dto::ServerSessionData(
          *std::dynamic_pointer_cast<dao::ServerSession>(session));

      request.reply(web::http::status_codes::OK,
                    dto::Response(dto::CODE::OK, sendMsg,
                                  dto::ArrayData({*entityData, sessionData}))
                        .serialize());
    } catch (const NotFoundEntityException &e) {
      auto msg =
          fmt::v9::format("AuthController[LOGIN]({})", requestUri.to_string());
      auto logMsg = fmt::v9::format("{} : {}", msg, e.what());
      auto sendMsg = fmt::v9::format("{} : NOT_FOUND", msg);

      instance->serverLogger->error(logMsg);
      auto data = dto::ExceptionData(dto::CODE::NOT_FOUND, sendMsg);
      request.reply(
          web::http::status_codes::OK,
          dto::Response(dto::CODE::NOT_FOUND, sendMsg, data).serialize());

    } catch (const NotSavedEntityException &e) {
      auto msg =
          fmt::v9::format("AuthController[LOGIN]({})", requestUri.to_string());
      auto logMsg = fmt::v9::format("{} : {}", msg, e.what());
      auto sendMsg = fmt::v9::format("{} : SESSION_SAVE_FAILURE", msg);

      instance->serverLogger->error(logMsg);
      auto data = dto::ExceptionData(dto::CODE::NOT_SAVED, sendMsg);
      request.reply(
          web::http::status_codes::OK,
          dto::Response(dto::CODE::NOT_SAVED, sendMsg, data).serialize());

    } catch (const std::exception &e) {
      auto msg =
          fmt::v9::format("AuthController[LOGIN]({})", requestUri.to_string());
      auto logMsg = fmt::v9::format("{} : {}", msg, e.what());
      auto sendMsg = fmt::v9::format("{} : UNEXPECTED_ERROR", msg);

      instance->serverLogger->error(logMsg);
      auto data = dto::ExceptionData(dto::CODE::UNEXPECTED, sendMsg);
      request.reply(
          web::http::status_codes::OK,
          dto::Response(dto::CODE::UNEXPECTED, sendMsg, data).serialize());
    }
  }

  static void handleLogout(web::http::http_request request) {
    /**
     * header : application/json
     *  - session-id : [id]
     *  - session-token : [token]
     *
     * body :
     *  - type=[company|user]
     *  - id=[id]
     *
     * response : msg
     */

    auto headers = request.headers();
    auto requestUri = request.absolute_uri();

    try {
      auto body = request.extract_json().get();

      // 권한 검증
      auto sessionEntity = instance->authenticateAccess(body);

      auto type = bodyAt(body, "type");
      uint64_t entityId = std::stoull(bodyAt(body, "id"));

      if (type == "company") {
        if (std::dynamic_pointer_cast<service::AuthService>(
                instance->authService)
                    ->isCompany(sessionEntity) == false ||
            sessionEntity->getId() != entityId) {
          throw NotAuthorizedException(fmt::v9::format("not authorized"));
        }
      } else if (type == "user") {
        if (std::dynamic_pointer_cast<service::AuthService>(
                instance->authService)
                ->isThisUser(sessionEntity, entityId) == false) {
          throw NotAuthorizedException(fmt::v9::format("not authorized"));
        }
      } else {
        throw NotAuthorizedException(fmt::v9::format("not authorized"));
      }

      // main routine
      uint64_t sessionId = std::stoull(bodyAt(body, "session-id"));
      if (std::dynamic_pointer_cast<service::AuthService>(instance->authService)
              ->logout(sessionId)) {
        auto msg = fmt::v9::format("AuthController[LOGOUT]({})",
                                   requestUri.to_string());
        auto logMsg = fmt::v9::format("{} : {}", msg, "ok");
        auto sendMsg = logMsg;

        instance->serverLogger->info(logMsg);
        request.reply(
            web::http::status_codes::OK,
            dto::Response(dto::CODE::OK, sendMsg, dto::MsgData(sendMsg))
                .serialize());
      } else {
        throw ControllerException(
            fmt::v9::format("sessionId={} cannot logout", sessionId));
      }
    } catch (const NotFoundEntityException &e) {
      auto msg =
          fmt::v9::format("AuthController[LOGOUT]({})", requestUri.to_string());
      auto logMsg = fmt::v9::format("{} : {}", msg, e.what());
      auto sendMsg = fmt::v9::format("{} : NOT_FOUND", msg);

      instance->serverLogger->error(logMsg);
      auto data = dto::ExceptionData(dto::CODE::NOT_FOUND, sendMsg);
      request.reply(
          web::http::status_codes::OK,
          dto::Response(dto::CODE::NOT_FOUND, sendMsg, data).serialize());

    } catch (const NotRemovedEntityException &e) {
      auto msg =
          fmt::v9::format("AuthController[LOGOUT]({})", requestUri.to_string());
      auto logMsg = fmt::v9::format("{} : {}", msg, e.what());
      auto sendMsg = fmt::v9::format("{} : SESSION_REMOVE_FAILURE", msg);

      instance->serverLogger->error(logMsg);
      auto data = dto::ExceptionData(dto::CODE::NOT_REMOVED, sendMsg);
      request.reply(
          web::http::status_codes::OK,
          dto::Response(dto::CODE::NOT_REMOVED, sendMsg, data).serialize());

    } catch (const std::exception &e) {
      auto msg =
          fmt::v9::format("AuthController[LOGIN]({})", requestUri.to_string());
      auto logMsg = fmt::v9::format("{} : {}", msg, e.what());
      auto sendMsg = fmt::v9::format("{} : UNEXPECTED_ERROR", msg);

      instance->serverLogger->error(logMsg);
      auto data = dto::ExceptionData(dto::CODE::UNEXPECTED, sendMsg);
      request.reply(
          web::http::status_codes::OK,
          dto::Response(dto::CODE::UNEXPECTED, sendMsg, data).serialize());
    }
  }

  void listen() override {
    std::function<void(web::http::http_request)> loginHandler =
        &AuthController::handleLogin;
    std::function<void(web::http::http_request)> logoutHandler =
        &AuthController::handleLogout;

    loginListener.support(web::http::methods::POST, loginHandler);
    logoutListener.support(web::http::methods::DEL, logoutHandler);

    try {
      loginListener.open()
          .then([this]() {
            serverLogger->info(fmt::v9::format("AuthController : Listening {} ",
                                               loginUri.to_string()));
          })
          .wait();

      logoutListener.open()
          .then([this]() {
            serverLogger->info(fmt::v9::format("AuthController : Listening {}",
                                               logoutUri.to_string()));
          })
          .wait();
      while (true) {
        std::this_thread::yield();
      }
    } catch (const std::exception &e) {
      auto msg = fmt::v9::format("AuthController : {}", e.what());
      serverLogger->error(msg);
      throw ControllerException(msg);
    }
  }

private:
  static std::shared_ptr<AuthController> instance;
  static std::mutex createMutex;

  web::http::experimental::listener::http_listener loginListener;
  web::http::experimental::listener::http_listener logoutListener;
  web::uri loginUri;
  web::uri logoutUri;
  AuthController() = delete;
};

std::shared_ptr<AuthController> AuthController::instance = nullptr;
std::mutex AuthController::createMutex{};
} // namespace chat::controller