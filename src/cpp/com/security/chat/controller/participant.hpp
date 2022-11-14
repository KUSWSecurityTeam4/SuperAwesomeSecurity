#pragma once

#include "base.hpp"

#include "../dao/participant/entity.hpp"
#include "../dao/server_session/entity.hpp"

#include "../dto/response.hpp"

#include "../module/common.hpp"
#include "../module/exception.hpp"
using namespace chat::module::exception;

#include "../service/auth.hpp"
#include "../service/participant.hpp"

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
class ParticipantController : public BaseController {
public:
  static std::shared_ptr<ParticipantController>
  getInstance(web::uri baseUri, L serverLogger, CN conn, CONFIG &config) {
    std::lock_guard<std::mutex> lock(createMutex);
    if (instance == nullptr) {
      instance = std::make_shared<ParticipantController>(baseUri, serverLogger,
                                                         conn, config);
    }
    return instance;
  }
  ParticipantController(web::uri baseUri, L serverLogger, CN conn,
                        CONFIG &config)
      : BaseController(baseUri, serverLogger, conn, config),
        participantService(
            service::ParticipantService::getInstance(serverLogger, conn)) {
    web::uri_builder builder{baseUri};
    this->listenUri = builder.set_path("/participants").to_uri();
    this->listener = web::http::experimental::listener::http_listener{
        this->listenUri, config};
  }

  static void handleGet(web::http::http_request request) {
    /**
     * 모든 사용자 가능
     * /participants?room=id or /participants/id
     * header:
     *  - session-id
     *  - session-token
     *
     * response:
     *  - participantInfo or participantArray
     */
    auto headers = request.headers();
    auto requestUri = request.absolute_uri();
    auto query = requestUri.query();
    auto path = requestUri.path();

    try {
      auto body = request.extract_json().get();
      //권한 검증
      auto sessionEntity = instance->authenticateAccess(body);

      // Authorization
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
      if ((splittedQuery.find("room") != splittedQuery.end()) &&
          (splittedPath.size() == 1)) {
        // /participants?room=id
        uint64_t roomId = std::stoull(splittedQuery.find("room")->second);
        auto participantList =
            std::dynamic_pointer_cast<service::ParticipantService>(
                instance->participantService)
                ->findAllInRoom(roomId);

        auto participantDataList = std::list<dto::Data>{};

        std::transform(
            participantList.begin(), participantList.end(),
            std::back_inserter(participantDataList), [](E user) {
              return dto::ParticipantData(
                  *std::dynamic_pointer_cast<dao::Participant>(user));
            });
        data = std::make_unique<dto::ArrayData>(participantDataList);

      } else if ((splittedPath.size() == 2) &&
                 module::isNumber(splittedPath.back())) {
        // /participants/id
        uint64_t participantId = std::stoull(splittedPath.back());
        auto participant =
            std::dynamic_pointer_cast<service::ParticipantService>(
                instance->participantService)
                ->findById(participantId);
        data = std::make_unique<dto::ParticipantData>(
            *std::dynamic_pointer_cast<dao::Participant>(participant));
      } else {
        throw ControllerException(fmt::v9::format("not qualified uri"));
      }

      auto msg = fmt::v9::format("ParticipantController[GET]({})",
                                 requestUri.to_string());
      auto logMsg = fmt::v9::format("{} : {}", msg, "ok");
      auto sendMsg = logMsg;

      instance->serverLogger->info(logMsg);
      request.reply(web::http::status_codes::OK,
                    dto::Response(dto::CODE::OK, sendMsg, *data).serialize());

    } catch (const NotAuthorizedException &e) {
      auto msg = fmt::v9::format("ParticipantController[GET]({})",
                                 requestUri.to_string());
      auto logMsg = fmt::v9::format("{} : {}", msg, e.what());
      auto sendMsg = fmt::v9::format("{} : NOT_AUTHRIZED", msg);

      instance->serverLogger->error(logMsg);
      auto data = dto::ExceptionData(dto::CODE::UNAUTHORIZED, sendMsg);
      request.reply(
          web::http::status_codes::OK,
          dto::Response(dto::CODE::UNAUTHORIZED, sendMsg, data).serialize());
    } catch (const NotFoundEntityException &e) {
      auto msg = fmt::v9::format("ParticipantController[GET]({})",
                                 requestUri.to_string());
      auto logMsg = fmt::v9::format("{} : {}", msg, e.what());
      auto sendMsg = fmt::v9::format("{} : NOT_FOUND", msg);

      instance->serverLogger->error(logMsg);
      auto data = dto::ExceptionData(dto::CODE::NOT_FOUND, sendMsg);
      request.reply(
          web::http::status_codes::OK,
          dto::Response(dto::CODE::NOT_FOUND, sendMsg, data).serialize());
    } catch (const std::exception &e) {
      auto msg = fmt::v9::format("ParticipantController[GET]({})",
                                 requestUri.to_string());
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
    // TODO
  }

  static void handleSave(web::http::http_request request) {
    /**
     * host or company만 가능
     * /participants?room=id
     *
     * headers:
     *  - session-id
     *  - session-token
     * body:
     *  - user-id
     *  - role : [host | guest]
     *
     * response:
     *  - participantInfo
     */
    auto headers = request.headers();
    auto requestUri = request.absolute_uri();
    auto query = requestUri.query();
    auto path = requestUri.path();

    try {
      auto body = request.extract_json().get();
      auto splittedPath = web::uri::split_path(path);
      auto splittedQuery = web::uri::split_query(query);

      if ((splittedPath.size() != 1) ||
          (splittedQuery.find("room") == splittedQuery.end()) ||
          (module::isNumber(splittedQuery.find("room")->second) == false)) {
        throw ControllerException(fmt::v9::format("not qualified uri"));
      }
      uint64_t roomId = std::stoull(splittedQuery.find("room")->second);

      //권한 검증
      auto sessionEntity = instance->authenticateAccess(body);

      if ((std::dynamic_pointer_cast<service::AuthService>(
               instance->authService)
               ->isHost(sessionEntity, roomId) == false) &&
          (std::dynamic_pointer_cast<service::AuthService>(
               instance->authService)
               ->isCompany(sessionEntity) == false)) {
        throw NotAuthorizedException(fmt::v9::format("not authorized"));
      }

      // main routine
      uint64_t userId = std::stoull(bodyAt(body, "user-id"));
      auto role = bodyAt(body, "role");

      auto participant = std::dynamic_pointer_cast<service::ParticipantService>(
                             instance->participantService)
                             ->save(roomId, userId, role);

      auto data = dto::ParticipantData(
          *std::dynamic_pointer_cast<dao::Participant>(participant));

      auto msg = fmt::v9::format("ParticipantController[SAVE]({})",
                                 requestUri.to_string());
      auto logMsg = fmt::v9::format("{} : {}", msg, "ok");
      auto sendMsg = logMsg;

      instance->serverLogger->info(logMsg);
      request.reply(web::http::status_codes::OK,
                    dto::Response(dto::CODE::OK, sendMsg, data).serialize());

    } catch (const NotAuthorizedException &e) {
      auto msg = fmt::v9::format("ParticipantController[SAVE]({})",
                                 requestUri.to_string());
      auto logMsg = fmt::v9::format("{} : {}", msg, e.what());
      auto sendMsg = fmt::v9::format("{} : NOT_AUTHRIZED", msg);

      instance->serverLogger->error(logMsg);
      auto data = dto::ExceptionData(dto::CODE::UNAUTHORIZED, sendMsg);
      request.reply(
          web::http::status_codes::OK,
          dto::Response(dto::CODE::UNAUTHORIZED, sendMsg, data).serialize());

    } catch (const NotSavedEntityException &e) {
      auto msg = fmt::v9::format("ParticipantController[SAVE]({})",
                                 requestUri.to_string());
      auto logMsg = fmt::v9::format("{} : {}", msg, e.what());
      auto sendMsg = fmt::v9::format("{} : NOT_SAVED", msg);

      instance->serverLogger->error(logMsg);
      auto data = dto::ExceptionData(dto::CODE::NOT_SAVED, sendMsg);
      request.reply(
          web::http::status_codes::OK,
          dto::Response(dto::CODE::NOT_SAVED, sendMsg, data).serialize());

    } catch (const NotFoundEntityException &e) {
      auto msg = fmt::v9::format("ParticipantController[SAVE]({})",
                                 requestUri.to_string());
      auto logMsg = fmt::v9::format("{} : {}", msg, e.what());
      auto sendMsg = fmt::v9::format("{} : NOT_FOUND", msg);

      instance->serverLogger->error(logMsg);
      auto data = dto::ExceptionData(dto::CODE::NOT_FOUND, sendMsg);
      request.reply(
          web::http::status_codes::OK,
          dto::Response(dto::CODE::NOT_FOUND, sendMsg, data).serialize());

    } catch (const DuplicatedEntityException &e) {
      auto msg = fmt::v9::format("ParticipantController[SAVE]({})",
                                 requestUri.to_string());
      auto logMsg = fmt::v9::format("{} : {}", msg, e.what());
      auto sendMsg = fmt::v9::format("{} : DUPLICATED", msg);

      instance->serverLogger->error(logMsg);
      auto data = dto::ExceptionData(dto::CODE::DUPLICATED, sendMsg);
      request.reply(
          web::http::status_codes::OK,
          dto::Response(dto::CODE::DUPLICATED, sendMsg, data).serialize());

    } catch (const std::exception &e) {
      auto msg = fmt::v9::format("ParticipantController[SAVE]({})",
                                 requestUri.to_string());
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
     * host or company만 가능
     * /participants/id?room=id
     *
     * headers:
     *  - session-id
     *  - session-token
     *
     * response:
     *  msg
     */
    auto headers = request.headers();
    auto requestUri = request.absolute_uri();
    auto query = requestUri.query();
    auto path = requestUri.path();

    try {
      auto body = request.extract_json().get();
      auto splittedPath = web::uri::split_path(path);
      auto splittedQuery = web::uri::split_query(query);

      if ((splittedPath.size() != 2) ||
          (module::isNumber(splittedPath.back()) == false) ||
          (splittedQuery.find("room") == splittedQuery.end()) ||
          (module::isNumber(splittedQuery.find("room")->second) == false)) {

        throw ControllerException(fmt::v9::format("not qualified uri"));
      }
      uint64_t roomId = std::stoull(splittedQuery.find("room")->second);

      //권한 검증
      auto sessionEntity = instance->authenticateAccess(body);

      // Authorization
      if ((std::dynamic_pointer_cast<service::AuthService>(
               instance->authService)
               ->isHost(sessionEntity, roomId) == false) &&
          (std::dynamic_pointer_cast<service::AuthService>(
               instance->authService)
               ->isCompany(sessionEntity) == false)) {
        throw NotAuthorizedException(fmt::v9::format("not authorized"));
      }

      // main routine
      uint64_t participantId = std::stoull(splittedPath.back());

      std::dynamic_pointer_cast<service::ParticipantService>(
          instance->participantService)
          ->remove(participantId);

      auto msg = fmt::v9::format("ParticipantController[DELETE]({})",
                                 requestUri.to_string());
      auto logMsg = fmt::v9::format("{} : {}", msg, "ok");
      auto sendMsg = logMsg;

      auto data = dto::MsgData(sendMsg);

      instance->serverLogger->info(logMsg);
      request.reply(web::http::status_codes::OK,
                    dto::Response(dto::CODE::OK, sendMsg, data).serialize());

    } catch (const NotAuthorizedException &e) {
      auto msg = fmt::v9::format("ParticipantController[DELETE]({})",
                                 requestUri.to_string());
      auto logMsg = fmt::v9::format("{} : {}", msg, e.what());
      auto sendMsg = fmt::v9::format("{} : NOT_AUTHRIZED", msg);

      instance->serverLogger->error(logMsg);
      auto data = dto::ExceptionData(dto::CODE::UNAUTHORIZED, sendMsg);
      request.reply(
          web::http::status_codes::OK,
          dto::Response(dto::CODE::UNAUTHORIZED, sendMsg, data).serialize());

    } catch (const NotRemovedEntityException &e) {
      auto msg = fmt::v9::format("ParticipantController[DELETE]({})",
                                 requestUri.to_string());
      auto logMsg = fmt::v9::format("{} : {}", msg, e.what());
      auto sendMsg = fmt::v9::format("{} : NOT_REMOVED", msg);

      instance->serverLogger->error(logMsg);
      auto data = dto::ExceptionData(dto::CODE::NOT_REMOVED, sendMsg);
      request.reply(
          web::http::status_codes::OK,
          dto::Response(dto::CODE::NOT_REMOVED, sendMsg, data).serialize());

    } catch (const NotFoundEntityException &e) {
      auto msg = fmt::v9::format("ParticipantController[DELETE]({})",
                                 requestUri.to_string());
      auto logMsg = fmt::v9::format("{} : {}", msg, e.what());
      auto sendMsg = fmt::v9::format("{} : NOT_FOUND", msg);

      instance->serverLogger->error(logMsg);
      auto data = dto::ExceptionData(dto::CODE::NOT_FOUND, sendMsg);
      request.reply(
          web::http::status_codes::OK,
          dto::Response(dto::CODE::NOT_FOUND, sendMsg, data).serialize());

    } catch (const std::exception &e) {
      auto msg = fmt::v9::format("ParticipantController[DELETE]({})",
                                 requestUri.to_string());
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
        &ParticipantController::handleGet;
    std::function<void(web::http::http_request)> updateHandler =
        &ParticipantController::handleUpdate;
    std::function<void(web::http::http_request)> saveHandler =
        &ParticipantController::handleSave;
    std::function<void(web::http::http_request)> deleteHandler =
        &ParticipantController::handleDelete;

    listener.support(web::http::methods::GET, getHandler);
    // listener.support(web::http::methods::PATCH, updateHandler);
    listener.support(web::http::methods::POST, saveHandler);
    listener.support(web::http::methods::DEL, deleteHandler);

    try {
      listener.open()
          .then([this]() {
            serverLogger->info(
                fmt::v9::format("ParticipantController : Listening {}",
                                this->listenUri.to_string()));
          })
          .wait();
      while (true) {
        std::this_thread::yield();
      }
    } catch (const std::exception &e) {
      auto msg = fmt::v9::format("ParticipantController : {}", e.what());
      serverLogger->error(msg);
      throw ControllerException(msg);
    }
  }

private:
  static std::shared_ptr<ParticipantController> instance;
  static std::mutex createMutex;

  web::http::experimental::listener::http_listener listener;
  web::uri listenUri;
  SV participantService;
  ParticipantController() = delete;
};

std::shared_ptr<ParticipantController> ParticipantController::instance =
    nullptr;
std::mutex ParticipantController::createMutex{};
} // namespace chat::controller