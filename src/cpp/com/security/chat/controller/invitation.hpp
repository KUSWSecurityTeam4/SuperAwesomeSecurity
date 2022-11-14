#pragma once

#include "base.hpp"

#include "../dao/invitation/entity.hpp"
#include "../dao/server_session/entity.hpp"

#include "../dto/response.hpp"

#include "../module/common.hpp"
#include "../module/exception.hpp"
using namespace chat::module::exception;

#include "../service/auth.hpp"
#include "../service/invitation.hpp"
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
class InvitationController : public BaseController {
public:
  static std::shared_ptr<InvitationController>
  getInstance(web::uri baseUri, L serverLogger, CN conn, CONFIG &config) {
    std::lock_guard<std::mutex> lock(createMutex);
    if (instance == nullptr) {
      instance = std::make_shared<InvitationController>(baseUri, serverLogger,
                                                        conn, config);
    }
    return instance;
  }
  InvitationController(web::uri baseUri, L serverLogger, CN conn,
                       CONFIG &config)
      : BaseController(baseUri, serverLogger, conn, config),
        invitationService(
            service::InvitationService::getInstance(serverLogger, conn)),
        participantService(
            service::ParticipantService::getInstance(serverLogger, conn)) {

    web::uri_builder builder{baseUri};
    this->listenUri = builder.set_path("/invitations").to_uri();
    this->listener = web::http::experimental::listener::http_listener{
        this->listenUri, config};
  }

  static void handleInvitation(web::http::http_request request) {
    /**
     * host나 company가 초대!
     *
     * /invitations?type=[request | register]
     *
     * request : 방을 입장하기 위한 코드를 요청
     * register : 방에 들어갈 때 해당 코드를 입력
     *
     * body :
     *  room-id
     *  user-id
     *  password <- register의 경우
     */
    auto headers = request.headers();
    auto requestUri = request.absolute_uri();
    auto path = requestUri.path();
    auto query = requestUri.query();
    auto splittedPath = web::uri::split_path(path);
    auto splittedQuery = web::uri::split_query(query);

    try {
      auto body = request.extract_json().get();

      //권한 검증
      auto sessionEntity = instance->authenticateAccess(body);

      // Authorization

      if ((body.has_field("room-id") == false) ||
          (body.has_field("user-id") == false)) {
        throw ControllerException(
            "not qualified body: user-id or room-id don't exist");
      }
      if ((module::isNumber(bodyAt(body, "room-id")) == false) ||
          (module::isNumber(bodyAt(body, "user-id")) == false)) {
        throw ControllerException(
            "not qualified body: user-id or room-id aren't number");
      }
      uint64_t userId = std::stoull(bodyAt(body, "user-id"));
      uint64_t roomId = std::stoull(bodyAt(body, "room-id"));

      if ((std::dynamic_pointer_cast<service::AuthService>(
               instance->authService)
               ->isCompany(sessionEntity) == false) &&
          ((std::dynamic_pointer_cast<service::AuthService>(
                instance->authService)
                ->isHost(sessionEntity, roomId) == false))) {
        throw NotAuthorizedException(fmt::v9::format("not authorized"));
      }

      if ((splittedPath.size() != 1) ||
          (splittedQuery.find("type") == splittedQuery.end())) {
        throw ControllerException(fmt::v9::format("not qualified uri"));
      }

      auto data = std::shared_ptr<dto::Data>{nullptr};
      auto type = splittedQuery.find("type")->second;
      if (type == "request") {
        data = handleRequest(userId, roomId, body);

      } else if (type == "register") {
        data = handleRegister(userId, roomId, body);

      } else {
        throw ControllerException(fmt::v9::format("type({}) not permited"));
      }
      auto msg =
          fmt::v9::format("InvitationController({})", requestUri.to_string());
      auto logMsg = fmt::v9::format("{} : {}", msg, "ok");
      auto sendMsg = logMsg;

      instance->serverLogger->info(logMsg);
      request.reply(web::http::status_codes::OK,
                    dto::Response(dto::CODE::OK, sendMsg, *data).serialize());

    } catch (const NotAuthorizedException &e) {
      auto msg =
          fmt::v9::format("InvitationController({})", requestUri.to_string());
      auto logMsg = fmt::v9::format("{} : {}", msg, e.what());
      auto sendMsg = fmt::v9::format("{} : NOT_AUTHRIZED", msg);

      instance->serverLogger->error(logMsg);
      auto data = dto::ExceptionData(dto::CODE::UNAUTHORIZED, sendMsg);
      request.reply(
          web::http::status_codes::OK,
          dto::Response(dto::CODE::UNAUTHORIZED, sendMsg, data).serialize());

    } catch (const NotFoundEntityException &e) {
      auto msg =
          fmt::v9::format("InvitationController({})", requestUri.to_string());
      auto logMsg = fmt::v9::format("{} : {}", msg, e.what());
      auto sendMsg = fmt::v9::format("{} : NOT_FOUND", msg);

      instance->serverLogger->error(logMsg);
      auto data = dto::ExceptionData(dto::CODE::NOT_FOUND, sendMsg);
      request.reply(
          web::http::status_codes::OK,
          dto::Response(dto::CODE::NOT_FOUND, sendMsg, data).serialize());

    } catch (const NotSavedEntityException &e) {
      auto msg =
          fmt::v9::format("InvitationController({})", requestUri.to_string());
      auto logMsg = fmt::v9::format("{} : {}", msg, e.what());
      auto sendMsg = fmt::v9::format("{} : NOT_SAVED", msg);

      instance->serverLogger->error(logMsg);
      auto data = dto::ExceptionData(dto::CODE::NOT_SAVED, sendMsg);
      request.reply(
          web::http::status_codes::OK,
          dto::Response(dto::CODE::NOT_SAVED, sendMsg, data).serialize());

    } catch (const NotRemovedEntityException &e) {
      auto msg =
          fmt::v9::format("InvitationController({})", requestUri.to_string());
      auto logMsg = fmt::v9::format("{} : {}", msg, e.what());
      auto sendMsg = fmt::v9::format("{} : NOT_REMOVED", msg);

      instance->serverLogger->error(logMsg);
      auto data = dto::ExceptionData(dto::CODE::NOT_REMOVED, sendMsg);
      request.reply(
          web::http::status_codes::OK,
          dto::Response(dto::CODE::NOT_REMOVED, sendMsg, data).serialize());

    } catch (const DuplicatedEntityException &e) {
      auto msg =
          fmt::v9::format("InvitationController({})", requestUri.to_string());
      auto logMsg = fmt::v9::format("{} : {}", msg, e.what());
      auto sendMsg = fmt::v9::format("{} : DUPLICATED", msg);

      instance->serverLogger->error(logMsg);
      auto data = dto::ExceptionData(dto::CODE::DUPLICATED, sendMsg);
      request.reply(
          web::http::status_codes::OK,
          dto::Response(dto::CODE::DUPLICATED, sendMsg, data).serialize());

    } catch (const std::exception &e) {
      auto msg =
          fmt::v9::format("InvitationController({})", requestUri.to_string());
      auto logMsg = fmt::v9::format("{} : {}", msg, e.what());
      auto sendMsg = fmt::v9::format("{} : UNEXPECTED_ERROR", msg);

      instance->serverLogger->error(logMsg);
      auto data = dto::ExceptionData(dto::CODE::UNEXPECTED, sendMsg);
      request.reply(
          web::http::status_codes::OK,
          dto::Response(dto::CODE::UNEXPECTED, sendMsg, data).serialize());
    }
  }

  static std::unique_ptr<dto::Data>
  handleRequest(uint64_t userId, uint64_t roomId, web::json::value body) {
    // 방에 있는지 확인하기
    // response : invitationInfo

    try {
      std::dynamic_pointer_cast<service::ParticipantService>(
          instance->participantService)
          ->findByUserIdInRoom(userId, roomId);
      throw DuplicatedEntityException(
          fmt::v9::format("user({}) already in room({})", userId, roomId));
    } catch (const NotFoundEntityException &e) {
      auto invitation = std::dynamic_pointer_cast<service::InvitationService>(
                            instance->invitationService)
                            ->save(userId, roomId);
      std::dynamic_pointer_cast<service::InvitationService>(
          instance->invitationService)
          ->sendEmail(invitation->getId());

      return std::make_unique<dto::InvitationData>(
          *std::dynamic_pointer_cast<dao::Invitation>(invitation));
    }
  }

  static std::unique_ptr<dto::Data>
  handleRegister(uint64_t userId, uint64_t roomId, web::json::value body) {
    /**
     * 사용자가 요청 후, 입장할 때(방 관리자가 직접 등록하는 경우는 제외!) <-
     * 처음만 하면 된당(participant 등록까지만)
     *
     *  /invitations?type=register
     *
     * response : participantInfo
     */
    auto receivedPw = bodyAt(body, "password");
    auto invitation = std::dynamic_pointer_cast<service::InvitationService>(
                          instance->invitationService)
                          ->findByUserIdInRoom(userId, roomId);

    auto isCorrect = std::dynamic_pointer_cast<service::InvitationService>(
                         instance->invitationService)
                         ->compare(userId, roomId, receivedPw);
    if (isCorrect) {
      // register - guest로!
      auto participant = std::dynamic_pointer_cast<service::ParticipantService>(
                             instance->participantService)
                             ->save(roomId, userId, "guest");
      return std::make_unique<dto::ParticipantData>(
          *std::dynamic_pointer_cast<dao::Participant>(participant));
    } else {
      throw NotSavedEntityException("password isn't correct");
    }
  }

  void listen() override {
    std::function<void(web::http::http_request)> invitationhandler =
        &InvitationController::handleInvitation;

    listener.support(web::http::methods::POST, invitationhandler);
    try {
      listener.open()
          .then([this]() {
            serverLogger->info(
                fmt::v9::format("InvitationController : Listening {}",
                                this->listenUri.to_string()));
          })
          .wait();
      while (true) {
        std::this_thread::yield();
      }
    } catch (const std::exception &e) {
      auto msg = fmt::v9::format("InvitationController : {}", e.what());
      serverLogger->error(msg);
      throw ControllerException(msg);
    }
  }

private:
  static std::shared_ptr<InvitationController> instance;
  static std::mutex createMutex;

  web::http::experimental::listener::http_listener listener;
  web::uri listenUri;
  SV invitationService;
  SV participantService;
  InvitationController() = delete;
};

std::shared_ptr<InvitationController> InvitationController::instance = nullptr;
std::mutex InvitationController::createMutex{};
} // namespace chat::controller