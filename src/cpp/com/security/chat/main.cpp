
#include "controller/all.hpp"
#include "module/all.hpp"

#include <mysqlx/xdevapi.h>

#include <fmt/core.h>
#include <spdlog/logger.h>
#include <spdlog/sinks/file_sinks.h>

#include <cpprest/uri.h>

#include <cstdio>
#include <cstdlib>
#include <exception>
#include <memory>
#include <string>
#include <thread>

using namespace chat;

int main(int argc, char **argv) {
  if (argc != 3) {
    fprintf(stderr, "Run with a specific network interface & port. example) "
                    "./run.out ens4 9000\n");
    exit(1);
  }

  auto ipAddress = module::getIpAddr(argv[1]);
  auto port = argv[2];

  auto dbUser = std::getenv("DB_USER");
  auto dbHost = std::getenv("DB_HOST");
  auto dbName = std::getenv("DB_NAME");
  auto dbPassword = std::getenv("DB_PASSWORD");

  auto apiUri = module::buildUri("http", ipAddress, port);
  auto dbUri = module::buildUri("mysqlx", dbHost, "",
                                std::string(dbUser) + ":" + dbPassword, dbName);

  auto logFile = std::getenv("LOG_FILE");

  auto serverLogger = std::make_shared<spdlog::logger>(
      "SECURE_CHAT_SERVER_LOGGER",
      std::unique_ptr<spdlog::sinks::sink>(
          new spdlog::sinks::simple_file_sink_st(logFile, true)));

  auto connection = chat::module::Connection::getInstance(
      mysqlx::ClientSettings(dbUri.to_string()));

  serverLogger->info(fmt::v9::format("apiUri : {}", apiUri.to_string()));

  try {
    auto companyController = controller::CompanyController::getInstance(
        apiUri, serverLogger, connection);
    auto authController = controller::AuthController::getInstance(
        apiUri, serverLogger, connection);
    auto userController = controller::UserController::getInstance(
        apiUri, serverLogger, connection);

    auto roomController = controller::RoomController::getInstance(
        apiUri, serverLogger, connection);

    auto participantController = controller::ParticipantController::getInstance(
        apiUri, serverLogger, connection);

    auto invitationController = controller::InvitationController::getInstance(
        apiUri, serverLogger, connection);

    auto companyThread =
        std::thread(&controller::CompanyController::listen,
                    std::dynamic_pointer_cast<controller::CompanyController>(
                        companyController));

    auto authThread = std::thread(
        &controller::AuthController::listen,
        std::dynamic_pointer_cast<controller::AuthController>(authController));

    auto userThread = std::thread(
        &controller::UserController::listen,
        std::dynamic_pointer_cast<controller::UserController>(userController));

    auto roomThread = std::thread(
        &controller::RoomController::listen,
        std::dynamic_pointer_cast<controller::RoomController>(roomController));

    auto participantThread = std::thread(
        &controller::ParticipantController::listen,
        std::dynamic_pointer_cast<controller::ParticipantController>(
            participantController));

    auto invitationThread =
        std::thread(&controller::InvitationController::listen,
                    std::dynamic_pointer_cast<controller::InvitationController>(
                        invitationController));

    companyThread.join();
    authThread.join();
    userThread.join();
    roomThread.join();
    participantThread.join();
    invitationThread.join();

  } catch (const std::exception &e) {
    serverLogger->error(e.what());
  }

  return 0;
}
