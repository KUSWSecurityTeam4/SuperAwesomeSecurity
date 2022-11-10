#pragma once

#include "base.hpp"

#include "../dao/server_session/entity.hpp"
#include "../dao/user/entity.hpp"

#include "../dto/response.hpp"

#include "../module/common.hpp"
#include "../module/exception.hpp"
using namespace chat::module::exception;

#include "../service/auth.hpp"
#include "../service/user.hpp"

#include <fmt/core.h>

#include <cpprest/http_headers.h>
#include <cpprest/http_listener.h>
#include <cpprest/http_msg.h>
#include <cpprest/uri.h>
#include <cpprest/uri_builder.h>

#include <algorithm>
#include <exception>
#include <functional>
#include <iterator>
#include <memory>
#include <string>
#include <thread>

namespace chat::controller {
class UserController : public BaseController {
public:
  static std::shared_ptr<UserController> getInstance(web::uri baseUri,
                                                     L serverLogger, CN conn) {
    std::lock_guard<std::mutex> lock(createMutex);
    if (instance == nullptr) {
      instance = std::make_shared<UserController>(baseUri, serverLogger, conn);
    }
    return instance;
  }
  UserController(web::uri baseUri, L serverLogger, CN conn)
      : BaseController(baseUri, serverLogger, conn),
        userService(service::UserService::getInstance(serverLogger, conn)) {
    web::uri_builder builder{baseUri};
    this->listenUri = builder.set_path("/users").to_uri();
    this->listener =
        web::http::experimental::listener::http_listener{this->listenUri};
  }

  static void handleGet(web::http::http_request request) {
    auto headers = request.headers();
    auto requestUri = request.absolute_uri();
    auto query = requestUri.query();
    auto path = requestUri.path();

    try {
      auto body = request.extract_json().get();

      auto sessionEntity = instance->authenticateAccess(body);

      if ((std::dynamic_pointer_cast<service::AuthService>(
               instance->authService)
               ->isUser(sessionEntity) == false) &&
          (std::dynamic_pointer_cast<service::AuthService>(
               instance->authService)
               ->isCompany(sessionEntity) == false)) {
        throw NotAuthorizedException(fmt::v9::format("not authorized"));
      }

      // main routine
      auto data = std::shared_ptr<dto::Data>{nullptr};
      auto splittedQuery = web::uri::split_query(query);
      auto splittedPath = web::uri::split_path(path);
      if ((splittedQuery.find("company") != splittedQuery.end()) &&
          (splittedPath.back() == "users")) {
        // /users?company=id
        uint64_t companyId = std::stoull(splittedQuery.find("company")->second);
        auto userList = std::dynamic_pointer_cast<service::UserService>(
                            instance->userService)
                            ->findAllInCompany(companyId);
        auto userDataList = std::list<dto::Data>{};

        std::transform(userList.begin(), userList.end(),
                       std::back_inserter(userDataList), [](E user) {
                         return dto::UserData(
                             *std::dynamic_pointer_cast<dao::User>(user));
                       });
        data = std::make_unique<dto::ArrayData>(userDataList);

      } else if (splittedQuery.find("company") == splittedQuery.end()) {
        // /users/id
        uint64_t userId = std::stoull(splittedPath.back());
        auto user = std::dynamic_pointer_cast<service::UserService>(
                        instance->userService)
                        ->findById(userId);
        data = std::make_unique<dto::UserData>(
            *std::dynamic_pointer_cast<dao::User>(user));
      } else {
        throw ControllerException(fmt::v9::format("not qualified uri"));
      }

      auto msg =
          fmt::v9::format("UserController[GET]({})", requestUri.to_string());
      auto logMsg = fmt::v9::format("{} : {}", msg, "ok");
      auto sendMsg = logMsg;

      instance->serverLogger->info(logMsg);
      request.reply(web::http::status_codes::OK,
                    dto::Response(dto::CODE::OK, sendMsg, *data).serialize());
    } catch (const NotAuthorizedException &e) {
      auto msg =
          fmt::v9::format("UserController[GET]({})", requestUri.to_string());
      auto logMsg = fmt::v9::format("{} : {}", msg, e.what());
      auto sendMsg = fmt::v9::format("{} : NOT_AUTHRIZED", msg);

      instance->serverLogger->error(logMsg);
      auto data = dto::ExceptionData(dto::CODE::UNAUTHORIZED, sendMsg);
      request.reply(
          web::http::status_codes::OK,
          dto::Response(dto::CODE::UNAUTHORIZED, sendMsg, data).serialize());
    } catch (const NotFoundEntityException &e) {
      auto msg =
          fmt::v9::format("UserController[GET]({})", requestUri.to_string());
      auto logMsg = fmt::v9::format("{} : {}", msg, e.what());
      auto sendMsg = fmt::v9::format("{} : NOT_FOUND", msg);

      instance->serverLogger->error(logMsg);
      auto data = dto::ExceptionData(dto::CODE::NOT_FOUND, sendMsg);
      request.reply(
          web::http::status_codes::OK,
          dto::Response(dto::CODE::NOT_FOUND, sendMsg, data).serialize());
    } catch (const std::exception &e) {
      auto msg =
          fmt::v9::format("UserController[GET]({})", requestUri.to_string());
      auto logMsg = fmt::v9::format("{} : {}", msg, e.what());
      auto sendMsg = fmt::v9::format("{} : UNEXPECTED_ERROR", msg);

      instance->serverLogger->error(logMsg);
      auto data = dto::ExceptionData(dto::CODE::UNEXPECTED, sendMsg);
      request.reply(
          web::http::status_codes::OK,
          dto::Response(dto::CODE::UNEXPECTED, sendMsg, data).serialize());
    }
  }

  static void handleUpdate(web::http::http_request request) {
    /**
     * this-user or company only
     *
     * /users/id
     * header
     *  - session-id
     *  - session-token
     * body
     *  - name
     *  - email
     *  - role
     *  - password
     *
     * response - userInfo
     */
    auto headers = request.headers();
    auto requestUri = request.absolute_uri();
    auto path = requestUri.path();

    try {
      auto body = request.extract_json().get();
      uint64_t userId = std::stoull(web::uri::split_path(path).back());
      //권한 검증
      auto sessionEntity = instance->authenticateAccess(body);

      if ((std::dynamic_pointer_cast<service::AuthService>(
               instance->authService)
               ->isThisUser(sessionEntity, userId) == false) &&
          (std::dynamic_pointer_cast<service::AuthService>(
               instance->authService)
               ->isCompany(sessionEntity) == false)) {
        throw NotAuthorizedException(fmt::v9::format("not authorized"));
      }

      // main routine
      auto name = bodyAt(body, "name");
      auto email = bodyAt(body, "email");
      auto role = bodyAt(body, "role");
      auto password = bodyAt(body, "password");

      auto user =
          std::dynamic_pointer_cast<service::UserService>(instance->userService)
              ->update(userId, name, role, email, password);
      auto data = dto::UserData(*std::dynamic_pointer_cast<dao::User>(user));

      auto msg =
          fmt::v9::format("UserController[UPDATE]({})", requestUri.to_string());
      auto logMsg = fmt::v9::format("{} : {}", msg, "ok");
      auto sendMsg = logMsg;

      instance->serverLogger->info(logMsg);
      request.reply(web::http::status_codes::OK,
                    dto::Response(dto::CODE::OK, sendMsg, data).serialize());
    } catch (const NotAuthorizedException &e) {
      auto msg =
          fmt::v9::format("UserController[UPDATE]({})", requestUri.to_string());
      auto logMsg = fmt::v9::format("{} : {}", msg, e.what());
      auto sendMsg = fmt::v9::format("{} : NOT_AUTHRIZED", msg);

      instance->serverLogger->error(logMsg);
      auto data = dto::ExceptionData(dto::CODE::UNAUTHORIZED, sendMsg);
      request.reply(
          web::http::status_codes::OK,
          dto::Response(dto::CODE::UNAUTHORIZED, sendMsg, data).serialize());
    } catch (const NotFoundEntityException &e) {
      auto msg =
          fmt::v9::format("UserController[UPDATE]({})", requestUri.to_string());
      auto logMsg = fmt::v9::format("{} : {}", msg, e.what());
      auto sendMsg = fmt::v9::format("{} : NOT_FOUND", msg);

      instance->serverLogger->error(logMsg);
      auto data = dto::ExceptionData(dto::CODE::NOT_FOUND, sendMsg);
      request.reply(
          web::http::status_codes::OK,
          dto::Response(dto::CODE::NOT_FOUND, sendMsg, data).serialize());
    } catch (const DuplicatedEntityException &e) {
      auto msg =
          fmt::v9::format("UserController[UPDATE]({})", requestUri.to_string());
      auto logMsg = fmt::v9::format("{} : {}", msg, e.what());
      auto sendMsg = fmt::v9::format("{} : DUPLICATED", msg);

      instance->serverLogger->error(logMsg);
      auto data = dto::ExceptionData(dto::CODE::DUPLICATED, sendMsg);
      request.reply(
          web::http::status_codes::OK,
          dto::Response(dto::CODE::DUPLICATED, sendMsg, data).serialize());
    } catch (const NotUpdatedEntityException &e) {
      auto msg =
          fmt::v9::format("UserController[UPDATE]({})", requestUri.to_string());
      auto logMsg = fmt::v9::format("{} : {}", msg, e.what());
      auto sendMsg = fmt::v9::format("{} : NOT_UPDATED", msg);

      instance->serverLogger->error(logMsg);
      auto data = dto::ExceptionData(dto::CODE::NOT_UPDATED, sendMsg);
      request.reply(
          web::http::status_codes::OK,
          dto::Response(dto::CODE::NOT_UPDATED, sendMsg, data).serialize());
    } catch (const std::exception &e) {
      auto msg =
          fmt::v9::format("UserController[UPDATE]({})", requestUri.to_string());
      auto logMsg = fmt::v9::format("{} : {}", msg, e.what());
      auto sendMsg = fmt::v9::format("{} : UNEXPECTED_ERROR", msg);

      instance->serverLogger->error(logMsg);
      auto data = dto::ExceptionData(dto::CODE::UNEXPECTED, sendMsg);
      request.reply(
          web::http::status_codes::OK,
          dto::Response(dto::CODE::UNEXPECTED, sendMsg, data).serialize());
    }
  }

  static void handleSave(web::http::http_request request) {
    /**
     * company only
     *
     * /users
     * header
     *  - session-id
     *  - session-token
     * body
     *  - name
     *  - email
     *  - role
     *  - password
     */
    auto headers = request.headers();
    auto requestUri = request.absolute_uri();

    try {
      auto body = request.extract_json().get();
      uint64_t companyId = -1;

      //권한 검증
      auto sessionEntity = instance->authenticateAccess(body);
      if (std::dynamic_pointer_cast<service::AuthService>(instance->authService)
              ->isCompany(sessionEntity) == false) {
        throw NotAuthorizedException(fmt::v9::format("not authorized"));
      }
      // Find companyId
      companyId = sessionEntity->getId();

      // main routine
      auto name = bodyAt(body, "name");
      auto email = bodyAt(body, "email");
      auto role = bodyAt(body, "role");
      auto password = bodyAt(body, "password");

      auto user =
          std::dynamic_pointer_cast<service::UserService>(instance->userService)
              ->save(name, companyId, role, email, password);
      auto data = dto::UserData(*std::dynamic_pointer_cast<dao::User>(user));

      auto msg =
          fmt::v9::format("UserController[SAVE]({})", requestUri.to_string());
      auto logMsg = fmt::v9::format("{} : {}", msg, "ok");
      auto sendMsg = logMsg;

      instance->serverLogger->info(logMsg);
      request.reply(web::http::status_codes::OK,
                    dto::Response(dto::CODE::OK, sendMsg, data).serialize());
    } catch (const NotAuthorizedException &e) {
      auto msg =
          fmt::v9::format("UserController[SAVE]({})", requestUri.to_string());
      auto logMsg = fmt::v9::format("{} : {}", msg, e.what());
      auto sendMsg = fmt::v9::format("{} : NOT_AUTHRIZED", msg);

      instance->serverLogger->error(logMsg);
      auto data = dto::ExceptionData(dto::CODE::UNAUTHORIZED, sendMsg);
      request.reply(
          web::http::status_codes::OK,
          dto::Response(dto::CODE::UNAUTHORIZED, sendMsg, data).serialize());
    } catch (const DuplicatedEntityException &e) {
      auto msg =
          fmt::v9::format("UserController[SAVE]({})", requestUri.to_string());
      auto logMsg = fmt::v9::format("{} : {}", msg, e.what());
      auto sendMsg = fmt::v9::format("{} : DUPLICATED", msg);

      instance->serverLogger->error(logMsg);
      auto data = dto::ExceptionData(dto::CODE::DUPLICATED, sendMsg);
      request.reply(
          web::http::status_codes::OK,
          dto::Response(dto::CODE::DUPLICATED, sendMsg, data).serialize());
    } catch (const NotSavedEntityException &e) {
      auto msg =
          fmt::v9::format("UserController[SAVE]({})", requestUri.to_string());
      auto logMsg = fmt::v9::format("{} : {}", msg, e.what());
      auto sendMsg = fmt::v9::format("{} : NOT_SAVED", msg);

      instance->serverLogger->error(logMsg);
      auto data = dto::ExceptionData(dto::CODE::NOT_SAVED, sendMsg);
      request.reply(
          web::http::status_codes::OK,
          dto::Response(dto::CODE::NOT_SAVED, sendMsg, data).serialize());
    } catch (const std::exception &e) {
      auto msg =
          fmt::v9::format("UserController[SAVE]({})", requestUri.to_string());
      auto logMsg = fmt::v9::format("{} : {}", msg, e.what());
      auto sendMsg = fmt::v9::format("{} : UNEXPECTED_ERROR", msg);

      instance->serverLogger->error(logMsg);
      auto data = dto::ExceptionData(dto::CODE::UNEXPECTED, sendMsg);
      request.reply(
          web::http::status_codes::OK,
          dto::Response(dto::CODE::UNEXPECTED, sendMsg, data).serialize());
    }
  }

  static void handleDelete(web::http::http_request request) {
    /**
     * company or this-user only
     *
     * /users/id
     * header
     *  - session-id
     *  - session-token
     */
    /**
     * company only
     *
     * /users
     * header
     *  - session-id
     *  - session-token
     * body
     *  - name
     *  - email
     *  - role
     *  - password
     */
    auto headers = request.headers();
    auto requestUri = request.absolute_uri();
    auto path = requestUri.path();

    try {
      auto body = request.extract_json().get();
      uint64_t userId = std::stoull(web::uri::split_path(path).back());

      //권한 검증
      auto sessionEntity = instance->authenticateAccess(body);

      if ((std::dynamic_pointer_cast<service::AuthService>(
               instance->authService)
               ->isCompany(sessionEntity) == false) &&
          (std::dynamic_pointer_cast<service::AuthService>(
               instance->authService)
               ->isThisUser(sessionEntity, userId) == false)) {
        throw NotAuthorizedException(fmt::v9::format("not authorized"));
      }

      // main routine
      std::dynamic_pointer_cast<service::UserService>(instance->userService)
          ->remove(userId);

      auto msg =
          fmt::v9::format("UserController[DELETE]({})", requestUri.to_string());
      auto logMsg = fmt::v9::format("{} : {}", msg, "ok");
      auto sendMsg = logMsg;

      auto data = dto::MsgData(sendMsg);

      instance->serverLogger->info(logMsg);
      request.reply(web::http::status_codes::OK,
                    dto::Response(dto::CODE::OK, sendMsg, data).serialize());
    } catch (const NotAuthorizedException &e) {
      auto msg =
          fmt::v9::format("UserController[DELETE]({})", requestUri.to_string());
      auto logMsg = fmt::v9::format("{} : {}", msg, e.what());
      auto sendMsg = fmt::v9::format("{} : NOT_AUTHRIZED", msg);

      instance->serverLogger->error(logMsg);
      auto data = dto::ExceptionData(dto::CODE::UNAUTHORIZED, sendMsg);
      request.reply(
          web::http::status_codes::OK,
          dto::Response(dto::CODE::UNAUTHORIZED, sendMsg, data).serialize());
    } catch (const NotRemovedEntityException &e) {
      auto msg =
          fmt::v9::format("UserController[DELETE]({})", requestUri.to_string());
      auto logMsg = fmt::v9::format("{} : {}", msg, e.what());
      auto sendMsg = fmt::v9::format("{} : NOT_REMOVED", msg);

      instance->serverLogger->error(logMsg);
      auto data = dto::ExceptionData(dto::CODE::NOT_REMOVED, sendMsg);
      request.reply(
          web::http::status_codes::OK,
          dto::Response(dto::CODE::NOT_REMOVED, sendMsg, data).serialize());
    } catch (const NotFoundEntityException &e) {
      auto msg =
          fmt::v9::format("UserController[DELETE]({})", requestUri.to_string());
      auto logMsg = fmt::v9::format("{} : {}", msg, e.what());
      auto sendMsg = fmt::v9::format("{} : NOT_FOUND", msg);

      instance->serverLogger->error(logMsg);
      auto data = dto::ExceptionData(dto::CODE::NOT_FOUND, sendMsg);
      request.reply(
          web::http::status_codes::OK,
          dto::Response(dto::CODE::NOT_FOUND, sendMsg, data).serialize());
    } catch (const std::exception &e) {
      auto msg =
          fmt::v9::format("UserController[DELETE]({})", requestUri.to_string());
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
    std::function<void(web::http::http_request)> getHandler =
        &UserController::handleGet;
    std::function<void(web::http::http_request)> updateHandler =
        &UserController::handleUpdate;
    std::function<void(web::http::http_request)> saveHandler =
        &UserController::handleSave;
    std::function<void(web::http::http_request)> deleteHandler =
        &UserController::handleDelete;

    listener.support(web::http::methods::GET, getHandler);
    listener.support(web::http::methods::PATCH, updateHandler);
    listener.support(web::http::methods::POST, saveHandler);
    listener.support(web::http::methods::DEL, deleteHandler);

    try {
      listener.open()
          .then([this]() {
            serverLogger->info(fmt::v9::format("UserController : Listening {}",
                                               this->listenUri.to_string()));
          })
          .wait();
      while (true) {
        std::this_thread::yield();
      }
    } catch (const std::exception &e) {
      auto msg = fmt::v9::format("UserController : {}", e.what());
      serverLogger->error(msg);
      throw ControllerException(msg);
    }
  }

private:
  static std::shared_ptr<UserController> instance;
  static std::mutex createMutex;

  web::http::experimental::listener::http_listener listener;
  web::uri listenUri;
  SV userService;
  UserController() = delete;
};

std::shared_ptr<UserController> UserController::instance = nullptr;
std::mutex UserController::createMutex{};
} // namespace chat::controller