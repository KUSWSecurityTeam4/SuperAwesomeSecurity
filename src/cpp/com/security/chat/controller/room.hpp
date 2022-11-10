#pragma once

#include "base.hpp"

#include "../dao/room/entity.hpp"
#include "../dao/server_session/entity.hpp"

#include "../dto/response.hpp"

#include "../module/common.hpp"
#include "../module/exception.hpp"
using namespace chat::module::exception;

#include "../service/auth.hpp"
#include "../service/participant.hpp"
#include "../service/room.hpp"

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
class RoomController : public BaseController {
public:
  static std::shared_ptr<RoomController> getInstance(web::uri baseUri,
                                                     L serverLogger, CN conn) {
    std::lock_guard<std::mutex> lock(createMutex);
    if (instance == nullptr) {
      instance = std::make_shared<RoomController>(baseUri, serverLogger, conn);
    }
    return instance;
  }
  RoomController(web::uri baseUri, L serverLogger, CN conn)
      : BaseController(baseUri, serverLogger, conn),
        roomService(service::RoomService::getInstance(serverLogger, conn)),
        participantService(
            service::ParticipantService::getInstance(serverLogger, conn)) {
    web::uri_builder builder{baseUri};
    this->listenUri = builder.set_path("/rooms").to_uri();
    this->listener =
        web::http::experimental::listener::http_listener(this->listenUri);
  }

  static void handleGet(web::http::http_request request) {
    /**
     * 모든 사용자 가능
     * /rooms/id or /rooms
     * header
     *  - session-id
     *  - session-token
     *
     * response - roomInfo or roomArray
     */
    auto headers = request.headers();
    auto requestUri = request.absolute_uri();
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
      auto splittedPath = web::uri::split_path(path);

      if (splittedPath.back() == "rooms") {
        auto roomList = std::dynamic_pointer_cast<service::RoomService>(
                            instance->roomService)
                            ->findAll();
        auto roomDataList = std::list<dto::Data>{};
        std::transform(roomList.begin(), roomList.end(),
                       std::back_inserter(roomDataList), [](E entity) {
                         return dto::RoomData(
                             *std::dynamic_pointer_cast<dao::Room>(entity));
                       });
        data = std::make_unique<dto::ArrayData>(roomDataList);

      } else if ((splittedPath.size() == 2) &&
                 module::isNumber(splittedPath.back())) {
        // /rooms/id
        uint64_t roomId = std::stoull(splittedPath.back());
        auto room = std::dynamic_pointer_cast<service::RoomService>(
                        instance->roomService)
                        ->findById(roomId);
        data = std::make_unique<dto::RoomData>(
            *std::dynamic_pointer_cast<dao::Room>(room));
      } else {
        throw ControllerException(fmt::v9::format("not qualified uri"));
      }
      auto msg =
          fmt::v9::format("RoomController[GET]({})", requestUri.to_string());
      auto logMsg = fmt::v9::format("{} : {}", msg, "ok");
      auto sendMsg = logMsg;

      instance->serverLogger->info(logMsg);
      request.reply(web::http::status_codes::OK,
                    dto::Response(dto::CODE::OK, sendMsg, *data).serialize());
    } catch (const NotAuthorizedException &e) {
      auto msg =
          fmt::v9::format("RoomController[GET]({})", requestUri.to_string());
      auto logMsg = fmt::v9::format("{} : {}", msg, e.what());
      auto sendMsg = fmt::v9::format("{} : NOT_AUTHRIZED", msg);

      instance->serverLogger->error(logMsg);
      auto data = dto::ExceptionData(dto::CODE::UNAUTHORIZED, sendMsg);
      request.reply(
          web::http::status_codes::OK,
          dto::Response(dto::CODE::UNAUTHORIZED, sendMsg, data).serialize());
    } catch (const NotFoundEntityException &e) {
      auto msg =
          fmt::v9::format("RoomController[GET]({})", requestUri.to_string());
      auto logMsg = fmt::v9::format("{} : {}", msg, e.what());
      auto sendMsg = fmt::v9::format("{} : NOT_FOUND", msg);

      instance->serverLogger->error(logMsg);
      auto data = dto::ExceptionData(dto::CODE::NOT_FOUND, sendMsg);
      request.reply(
          web::http::status_codes::OK,
          dto::Response(dto::CODE::NOT_FOUND, sendMsg, data).serialize());
    } catch (const std::exception &e) {
      auto msg =
          fmt::v9::format("RoomController[GET]({})", requestUri.to_string());
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
     * host거나, company만
     * /rooms/id
     * header
     *  - session-id
     *  - session-token
     *
     * body
     *  - name
     *
     * response - roomInfo
     */
    auto headers = request.headers();
    auto requestUri = request.absolute_uri();
    auto path = requestUri.path();

    try {
      auto body = request.extract_json().get();
      auto splittedPath = web::uri::split_path(path);
      if ((splittedPath.size() != 2) ||
          (module::isNumber(splittedPath.back()) == false)) {
        throw ControllerException(fmt::v9::format("not qualified uri"));
      }
      uint64_t roomId = std::stoull(splittedPath.back());

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
      auto name = bodyAt(body, "name");
      auto room =
          std::dynamic_pointer_cast<service::RoomService>(instance->roomService)
              ->update(roomId, name);
      auto data = dto::RoomData(*std::dynamic_pointer_cast<dao::Room>(room));

      auto msg =
          fmt::v9::format("RoomController[UPDATE]({})", requestUri.to_string());
      auto logMsg = fmt::v9::format("{} : {}", msg, "ok");
      auto sendMsg = logMsg;

      instance->serverLogger->info(logMsg);
      request.reply(web::http::status_codes::OK,
                    dto::Response(dto::CODE::OK, sendMsg, data).serialize());
    } catch (const NotAuthorizedException &e) {
      auto msg =
          fmt::v9::format("RoomController[UPDATE]({})", requestUri.to_string());
      auto logMsg = fmt::v9::format("{} : {}", msg, e.what());
      auto sendMsg = fmt::v9::format("{} : NOT_AUTHRIZED", msg);

      instance->serverLogger->error(logMsg);
      auto data = dto::ExceptionData(dto::CODE::UNAUTHORIZED, sendMsg);
      request.reply(
          web::http::status_codes::OK,
          dto::Response(dto::CODE::UNAUTHORIZED, sendMsg, data).serialize());
    } catch (const NotFoundEntityException &e) {
      auto msg =
          fmt::v9::format("RoomController[UPDATE]({})", requestUri.to_string());
      auto logMsg = fmt::v9::format("{} : {}", msg, e.what());
      auto sendMsg = fmt::v9::format("{} : NOT_FOUND", msg);

      instance->serverLogger->error(logMsg);
      auto data = dto::ExceptionData(dto::CODE::NOT_FOUND, sendMsg);
      request.reply(
          web::http::status_codes::OK,
          dto::Response(dto::CODE::NOT_FOUND, sendMsg, data).serialize());
    } catch (const NotUpdatedEntityException &e) {
      auto msg =
          fmt::v9::format("RoomController[UPDATE]({})", requestUri.to_string());
      auto logMsg = fmt::v9::format("{} : {}", msg, e.what());
      auto sendMsg = fmt::v9::format("{} : NOT_UPDATED", msg);

      instance->serverLogger->error(logMsg);
      auto data = dto::ExceptionData(dto::CODE::NOT_UPDATED, sendMsg);
      request.reply(
          web::http::status_codes::OK,
          dto::Response(dto::CODE::NOT_UPDATED, sendMsg, data).serialize());
    } catch (const DuplicatedEntityException &e) {
      auto msg =
          fmt::v9::format("RoomController[UPDATE]({})", requestUri.to_string());
      auto logMsg = fmt::v9::format("{} : {}", msg, e.what());
      auto sendMsg = fmt::v9::format("{} : DUPLICATED", msg);

      instance->serverLogger->error(logMsg);
      auto data = dto::ExceptionData(dto::CODE::DUPLICATED, sendMsg);
      request.reply(
          web::http::status_codes::OK,
          dto::Response(dto::CODE::DUPLICATED, sendMsg, data).serialize());
    } catch (const std::exception &e) {
      auto msg =
          fmt::v9::format("RoomController[UPDATE]({})", requestUri.to_string());
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
     * 모든 사용자 가능(company X)
     * /rooms
     * header
     *  - session-id
     *  - session-token
     *
     * body
     *  - name
     *
     * response - roomInfo
     */
    auto headers = request.headers();
    auto requestUri = request.absolute_uri();
    auto path = requestUri.path();

    try {
      auto body = request.extract_json().get();
      auto splittedPath = web::uri::split_path(path);
      if ((splittedPath.size() != 1)) {
        throw ControllerException(fmt::v9::format("not qualified uri"));
      }

      uint64_t companyId = -1;
      uint64_t userId = -1;

      //권한 검증
      auto sessionEntity = instance->authenticateAccess(body);

      // Authorization
      if (std::dynamic_pointer_cast<service::AuthService>(instance->authService)
              ->isUser(sessionEntity) == false) {
        throw NotAuthorizedException(fmt::v9::format("not authorized"));
      }
      // companyId 찾기
      companyId =
          std::dynamic_pointer_cast<dao::User>(sessionEntity)->getCompanyId();

      // userId 찾기
      userId = sessionEntity->getId();

      // main routine
      auto name = bodyAt(body, "name");
      auto room =
          std::dynamic_pointer_cast<service::RoomService>(instance->roomService)
              ->save(companyId, name);
      auto roomData =
          dto::RoomData(*std::dynamic_pointer_cast<dao::Room>(room));

      // set host
      auto host = std::dynamic_pointer_cast<service::ParticipantService>(
                      instance->participantService)
                      ->save(room->getId(), userId, "host");
      auto hostData = dto::ParticipantData(
          *std::dynamic_pointer_cast<dao::Participant>(host));
      auto data = dto::ArrayData({hostData, roomData});

      auto msg =
          fmt::v9::format("RoomController[SAVE]({})", requestUri.to_string());
      auto logMsg = fmt::v9::format("{} : {}", msg, "ok");
      auto sendMsg = logMsg;

      instance->serverLogger->info(logMsg);
      request.reply(web::http::status_codes::OK,
                    dto::Response(dto::CODE::OK, sendMsg, data).serialize());
    } catch (const NotAuthorizedException &e) {
      auto msg =
          fmt::v9::format("RoomController[SAVE]({})", requestUri.to_string());
      auto logMsg = fmt::v9::format("{} : {}", msg, e.what());
      auto sendMsg = fmt::v9::format("{} : NOT_AUTHRIZED", msg);

      instance->serverLogger->error(logMsg);
      auto data = dto::ExceptionData(dto::CODE::UNAUTHORIZED, sendMsg);
      request.reply(
          web::http::status_codes::OK,
          dto::Response(dto::CODE::UNAUTHORIZED, sendMsg, data).serialize());
    } catch (const NotSavedEntityException &e) {
      auto msg =
          fmt::v9::format("RoomController[SAVE]({})", requestUri.to_string());
      auto logMsg = fmt::v9::format("{} : {}", msg, e.what());
      auto sendMsg = fmt::v9::format("{} : NOT_SAVED", msg);

      instance->serverLogger->error(logMsg);
      auto data = dto::ExceptionData(dto::CODE::NOT_SAVED, sendMsg);
      request.reply(
          web::http::status_codes::OK,
          dto::Response(dto::CODE::NOT_SAVED, sendMsg, data).serialize());
    } catch (const DuplicatedEntityException &e) {
      auto msg =
          fmt::v9::format("RoomController[SAVE]({})", requestUri.to_string());
      auto logMsg = fmt::v9::format("{} : {}", msg, e.what());
      auto sendMsg = fmt::v9::format("{} : DUPLICATED", msg);

      instance->serverLogger->error(logMsg);
      auto data = dto::ExceptionData(dto::CODE::DUPLICATED, sendMsg);
      request.reply(
          web::http::status_codes::OK,
          dto::Response(dto::CODE::DUPLICATED, sendMsg, data).serialize());
    } catch (const std::exception &e) {
      auto msg =
          fmt::v9::format("RoomController[SAVE]({})", requestUri.to_string());
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
     * host거나 company만
     * /rooms/id
     * header
     *  - session-id
     *  - session-token
     *
     * response - msg
     */
    auto headers = request.headers();
    auto requestUri = request.absolute_uri();
    auto path = requestUri.path();

    try {
      auto body = request.extract_json().get();
      auto splittedPath = web::uri::split_path(path);
      if ((splittedPath.size() != 2) ||
          (module::isNumber(splittedPath.back()) == false)) {
        throw ControllerException(fmt::v9::format("not qualified uri"));
      }
      uint64_t roomId = std::stoull(splittedPath.back());

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
      std::dynamic_pointer_cast<service::RoomService>(instance->roomService)
          ->remove(roomId);
      auto msg =
          fmt::v9::format("RoomController[DELETE]({})", requestUri.to_string());
      auto logMsg = fmt::v9::format("{} : {}", msg, "ok");
      auto sendMsg = logMsg;

      auto data = dto::MsgData(sendMsg);
      instance->serverLogger->info(logMsg);
      request.reply(web::http::status_codes::OK,
                    dto::Response(dto::CODE::OK, sendMsg, data).serialize());
    } catch (const NotAuthorizedException &e) {
      auto msg =
          fmt::v9::format("RoomController[DELETE]({})", requestUri.to_string());
      auto logMsg = fmt::v9::format("{} : {}", msg, e.what());
      auto sendMsg = fmt::v9::format("{} : NOT_AUTHRIZED", msg);

      instance->serverLogger->error(logMsg);
      auto data = dto::ExceptionData(dto::CODE::UNAUTHORIZED, sendMsg);
      request.reply(
          web::http::status_codes::OK,
          dto::Response(dto::CODE::UNAUTHORIZED, sendMsg, data).serialize());
    } catch (const NotFoundEntityException &e) {
      auto msg =
          fmt::v9::format("RoomController[DELETE]({})", requestUri.to_string());
      auto logMsg = fmt::v9::format("{} : {}", msg, e.what());
      auto sendMsg = fmt::v9::format("{} : NOT_FOUND", msg);

      instance->serverLogger->error(logMsg);
      auto data = dto::ExceptionData(dto::CODE::NOT_FOUND, sendMsg);
      request.reply(
          web::http::status_codes::OK,
          dto::Response(dto::CODE::NOT_FOUND, sendMsg, data).serialize());
    } catch (const NotRemovedEntityException &e) {
      auto msg =
          fmt::v9::format("RoomController[DELETE]({})", requestUri.to_string());
      auto logMsg = fmt::v9::format("{} : {}", msg, e.what());
      auto sendMsg = fmt::v9::format("{} : NOT_REMOVED", msg);

      instance->serverLogger->error(logMsg);
      auto data = dto::ExceptionData(dto::CODE::NOT_REMOVED, sendMsg);
      request.reply(
          web::http::status_codes::OK,
          dto::Response(dto::CODE::NOT_REMOVED, sendMsg, data).serialize());
    } catch (const std::exception &e) {
      auto msg =
          fmt::v9::format("RoomController[DELETE]({})", requestUri.to_string());
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
        &RoomController::handleGet;
    std::function<void(web::http::http_request)> updateHandler =
        &RoomController::handleUpdate;
    std::function<void(web::http::http_request)> saveHandler =
        &RoomController::handleSave;
    std::function<void(web::http::http_request)> deleteHandler =
        &RoomController::handleDelete;

    listener.support(web::http::methods::GET, getHandler);
    listener.support(web::http::methods::PATCH, updateHandler);
    listener.support(web::http::methods::POST, saveHandler);
    listener.support(web::http::methods::DEL, deleteHandler);

    try {
      listener.open()
          .then([this]() {
            serverLogger->info(fmt::v9::format("RoomController : Listening {}",
                                               this->listenUri.to_string()));
          })
          .wait();
      while (true) {
        std::this_thread::yield();
      }
    } catch (const std::exception &e) {
      auto msg = fmt::v9::format("RoomController : {}", e.what());
      serverLogger->error(msg);
      throw ControllerException(msg);
    }
  }

private:
  static std::shared_ptr<RoomController> instance;
  static std::mutex createMutex;

  web::http::experimental::listener::http_listener listener;
  web::uri listenUri;
  SV roomService;
  SV participantService;
  RoomController() = delete;
};

std::shared_ptr<RoomController> RoomController::instance = nullptr;
std::mutex RoomController::createMutex{};
} // namespace chat::controller