#pragma once

#include "base.hpp"

#include "../dao/company/entity.hpp"
#include "../dao/server_session/entity.hpp"

#include "../dto/response.hpp"

#include "../module/common.hpp"
#include "../module/exception.hpp"
using namespace chat::module::exception;

#include "../service/auth.hpp"
#include "../service/company.hpp"

#include <fmt/core.h>

#include <cpprest/http_headers.h>
#include <cpprest/http_listener.h>
#include <cpprest/http_msg.h>
#include <cpprest/uri.h>
#include <cpprest/uri_builder.h>

#include <exception>
#include <functional>
#include <memory>
#include <string>
#include <thread>

namespace chat::controller {
class CompanyController : public BaseController {
public:
  static std::shared_ptr<CompanyController>
  getInstance(web::uri baseUri, L serverLogger, CN conn, CONFIG config) {
    std::lock_guard<std::mutex> lock(createMutex);
    if (instance == nullptr) {
      instance = std::make_shared<CompanyController>(baseUri, serverLogger,
                                                     conn, config);
    }
    return instance;
  }
  CompanyController(web::uri baseUri, L serverLogger, CN conn, CONFIG config)
      : BaseController(baseUri, serverLogger, conn, config),
        companyService(
            service::CompanyService::getInstance(serverLogger, conn)) {
    web::uri_builder builder{baseUri};
    builder.set_path("/company");
    this->listenUri = builder.to_uri();
    this->listener = web::http::experimental::listener::http_listener{
        this->listenUri, *config};
  }
  static void handleGet(web::http::http_request request) {
    // /company/id <- login한 모든 사용자는 접근 가능
    auto headers = request.headers();
    auto requestUri = request.absolute_uri();
    auto path = requestUri.path();
    auto splitedPath = web::http::uri::split_path(path);

    try {
      auto body = request.extract_json().get();

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
      uint64_t companyId = std::stoull(splitedPath.back());
      auto company = std::dynamic_pointer_cast<service::CompanyService>(
                         instance->companyService)
                         ->findById(companyId);
      auto msg =
          fmt::v9::format("CompanyController[GET]({})", requestUri.to_string());
      auto logMsg = fmt::v9::format("{} : {}", msg, "ok");
      auto sendMsg = logMsg;

      instance->serverLogger->info(logMsg);
      auto data =
          dto::CompanyData(*std::dynamic_pointer_cast<dao::Company>(company));
      request.reply(web::http::status_codes::OK,
                    dto::Response(dto::CODE::OK, sendMsg, data).serialize());
    } catch (const NotAuthorizedException &e) {
      auto msg =
          fmt::v9::format("CompanyController[GET]({})", requestUri.to_string());
      auto logMsg = fmt::v9::format("{} : {}", msg, e.what());
      auto sendMsg = fmt::v9::format("{} : NOT_AUTHRIZED", msg);

      instance->serverLogger->error(logMsg);
      auto data = dto::ExceptionData(dto::CODE::UNAUTHORIZED, sendMsg);
      request.reply(
          web::http::status_codes::OK,
          dto::Response(dto::CODE::UNAUTHORIZED, sendMsg, data).serialize());
    } catch (const NotFoundEntityException &e) {
      auto msg =
          fmt::v9::format("CompanyController[GET]({})", requestUri.to_string());
      auto logMsg = fmt::v9::format("{} : {}", msg, e.what());
      auto sendMsg = fmt::v9::format("{} : NOT_FOUND", msg);

      instance->serverLogger->error(logMsg);
      auto data = dto::ExceptionData(dto::CODE::NOT_FOUND, sendMsg);
      request.reply(
          web::http::status_codes::OK,
          dto::Response(dto::CODE::NOT_FOUND, sendMsg, data).serialize());
    } catch (const std::exception &e) {
      auto msg =
          fmt::v9::format("CompanyController[GET]({})", requestUri.to_string());
      auto logMsg = fmt::v9::format("{} : {}", msg, e.what());
      auto sendMsg = fmt::v9::format("{} : UNEXPECTED_ERROR", msg);

      instance->serverLogger->error(logMsg);
      auto data = dto::ExceptionData(dto::CODE::UNEXPECTED, sendMsg);
      request.reply(
          web::http::status_codes::OK,
          dto::Response(dto::CODE::UNEXPECTED, sendMsg, data).serialize());
    }
  }

  static void handlePatch(web::http::http_request request) {
    //해당하는 company만 접근 가능!
    // /company/id + body에 name: [name], pw: [pw]
    auto headers = request.headers();
    auto requestUri = request.absolute_uri();
    auto path = requestUri.path();
    auto splitedPath = web::http::uri::split_path(path);

    try {
      uint64_t companyId = std::stoull(splitedPath.back());
      auto body = request.extract_json().get();

      //권한 검증
      auto sessionEntity = instance->authenticateAccess(body);

      // Authorization
      if (std::dynamic_pointer_cast<service::AuthService>(instance->authService)
                  ->isCompany(sessionEntity) == false ||
          sessionEntity->getId() != companyId) {
        throw NotAuthorizedException(fmt::v9::format("not authorized"));
      }

      // main routine
      auto companyName = bodyAt(body, "name");
      auto companyPw = bodyAt(body, "password");

      auto company = std::dynamic_pointer_cast<service::CompanyService>(
                         instance->companyService)
                         ->updateName(companyId, companyName);
      company = std::dynamic_pointer_cast<service::CompanyService>(
                    instance->companyService)
                    ->updatePw(companyId, companyPw);

      auto msg = fmt::v9::format("CompanyController[PATCH]({})",
                                 requestUri.to_string());
      auto logMsg = fmt::v9::format("{} : {}", msg, "ok");
      auto sendMsg = logMsg;

      instance->serverLogger->info(logMsg);
      auto data =
          dto::CompanyData(*std::dynamic_pointer_cast<dao::Company>(company));
      request.reply(web::http::status_codes::OK,
                    dto::Response(dto::CODE::OK, sendMsg, data).serialize());
    } catch (const NotAuthorizedException &e) {
      auto msg = fmt::v9::format("CompanyController[PATCH]({})",
                                 requestUri.to_string());
      auto logMsg = fmt::v9::format("{} : {}", msg, e.what());
      auto sendMsg = fmt::v9::format("{} : NOT_AUTHRIZED", msg);

      instance->serverLogger->error(logMsg);
      auto data = dto::ExceptionData(dto::CODE::UNAUTHORIZED, sendMsg);
      request.reply(
          web::http::status_codes::OK,
          dto::Response(dto::CODE::UNAUTHORIZED, sendMsg, data).serialize());
    } catch (const NotFoundEntityException &e) {
      auto msg = fmt::v9::format("CompanyController[PATCH]({})",
                                 requestUri.to_string());
      auto logMsg = fmt::v9::format("{} : {}", msg, e.what());
      auto sendMsg = fmt::v9::format("{} : NOT_FOUND", msg);

      instance->serverLogger->error(logMsg);
      auto data = dto::ExceptionData(dto::CODE::NOT_FOUND, sendMsg);
      request.reply(
          web::http::status_codes::OK,
          dto::Response(dto::CODE::NOT_FOUND, sendMsg, data).serialize());
    } catch (const DuplicatedEntityException &e) {
      auto msg = fmt::v9::format("CompanyController[PATCH]({})",
                                 requestUri.to_string());
      auto logMsg = fmt::v9::format("{} : {}", msg, e.what());
      auto sendMsg = fmt::v9::format("{} : DUPLICATED", msg);

      instance->serverLogger->error(logMsg);
      auto data = dto::ExceptionData(dto::CODE::DUPLICATED, sendMsg);
      request.reply(
          web::http::status_codes::OK,
          dto::Response(dto::CODE::DUPLICATED, sendMsg, data).serialize());
    } catch (const std::exception &e) {
      auto msg = fmt::v9::format("CompanyController[PATCH]({})",
                                 requestUri.to_string());
      auto logMsg = fmt::v9::format("{} : {}", msg, e.what());
      auto sendMsg = fmt::v9::format("{} : UNEXPECTED_ERROR", msg);

      instance->serverLogger->error(logMsg);
      auto data = dto::ExceptionData(dto::CODE::NOT_UPDATED, sendMsg);
      request.reply(
          web::http::status_codes::OK,
          dto::Response(dto::CODE::NOT_UPDATED, sendMsg, data).serialize());
    }
  }

  void listen() override {
    std::function<void(web::http::http_request)> getHandler =
        &CompanyController::handleGet;
    std::function<void(web::http::http_request)> patchHandler =
        &CompanyController::handlePatch;

    listener.support(web::http::methods::GET, getHandler);
    listener.support(web::http::methods::PATCH, patchHandler);

    try {
      listener.open()
          .then([this]() {
            serverLogger->info(
                fmt::v9::format("CompanyController : Listening {}",
                                this->listenUri.to_string()));
          })
          .wait();
      while (true) {
        std::this_thread::yield();
      }
    } catch (const std::exception &e) {
      auto msg = fmt::v9::format("CompanyController : {}", e.what());
      serverLogger->error(msg);
      throw ControllerException(msg);
    }
  }

private:
  static std::shared_ptr<CompanyController> instance;
  static std::mutex createMutex;

  web::http::experimental::listener::http_listener listener;
  web::uri listenUri;
  SV companyService;
  CompanyController() = delete;
};

std::shared_ptr<CompanyController> CompanyController::instance = nullptr;
std::mutex CompanyController::createMutex{};
} // namespace chat::controller