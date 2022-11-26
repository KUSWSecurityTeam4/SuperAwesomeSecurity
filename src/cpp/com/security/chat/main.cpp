
#include "controller/all.hpp"

#include "module/all.hpp"

#include <mysqlx/xdevapi.h>

#include <fmt/core.h>
#include <spdlog/logger.h>
#include <spdlog/sinks/file_sinks.h>

#include <cpprest/json.h>
#include <cpprest/uri.h>

#include <cstdio>
#include <cstdlib>
#include <exception>
#include <fstream>
#include <memory>
#include <string>
#include <thread>

using namespace chat;

int main(int argc, char **argv) {
  if (argc != 3) {
    fprintf(stderr,
            "\n\nRun with a specific network interface & port. example) "
            "./run.out ens4 9000\n\n");
    exit(1);
  }

  auto ipAddress = module::getIpAddr(argv[1]);
  auto port = argv[2];

  auto apiUri = module::buildUri("https", ipAddress, port);
  std::system("pwd");
  std::ifstream ifs{"resources/secret/config.json"};
  auto config = web::json::value::object();

  if (ifs.fail()) {
    fprintf(stderr, "\n\nSecret Config File Not Opened\n\n");
    exit(1);
  }
  ifs >> config;
  ifs.close();

  if (!config.has_field("timezone")) {
    fprintf(stderr, "\n\nTimezone Not Exist\n\n");
    exit(1);
  }
  const auto tz = config.at("timezone").serialize();
  setenv("TZ", tz.c_str(), 1);

  if (!config.has_field("database")) {
    fprintf(stderr, "\n\nDatabase Config Not Exist\n\n");
  }
  const auto dbConfig = config.at("database");
  if (!dbConfig.has_field("name") || !dbConfig.has_field("host") ||
      !dbConfig.has_field("user") || !dbConfig.has_field("password")) {
    fprintf(stderr, "\n\nDatabase Field Not Exist\n\n");
    exit(1);
  }
  const auto dbName = dbConfig.at("name").serialize();
  const auto dbHost = dbConfig.at("host").serialize();
  const auto dbUser = dbConfig.at("user").serialize();
  const auto dbPassword = dbConfig.at("password").serialize();

  auto dbUri = module::buildUri("mysqlx", dbHost, "",
                                std::string(dbUser) + ":" + dbPassword, dbName);

  auto connection = chat::module::Connection::getInstance(
      mysqlx::ClientSettings(dbUri.to_string()));

  if (!config.has_field("log")) {
    fprintf(stderr, "\n\nLog Field Not Exist\n\n");
    exit(1);
  }
  const auto logFile = config.at("log").serialize();

  auto serverLogger = std::make_shared<spdlog::logger>(
      "SECURE_CHAT_SERVER_LOGGER",
      std::unique_ptr<spdlog::sinks::sink>(
          new spdlog::sinks::simple_file_sink_st(logFile, true)));

  if (!config.has_field("ssl")) {
    fprintf(stderr, "\n\nSsl Config Not Exist\n\n");
    exit(1);
  }
  const auto sslConfig = config.at("ssl");

  if (!sslConfig.has_field("crt") || !sslConfig.has_field("key") ||
      !sslConfig.has_field("pem")) {
    fprintf(stderr, "\n\nSsl Field Not Exist\n\n");
    exit(1);
  }
  const auto sslCrtPath = sslConfig.at("crt").serialize();
  const auto sslKeyPath = sslConfig.at("key").serialize();
  const auto sslDhPath = sslConfig.at("pem").serialize();

  auto &&ssl = module::secure::configSSL(sslKeyPath, sslCrtPath, sslDhPath);

  serverLogger->info(fmt::v9::format("apiUri : {}", apiUri.to_string()));

  try {
    auto companyController = controller::CompanyController::getInstance(
        apiUri, serverLogger, connection, ssl);
    auto authController = controller::AuthController::getInstance(
        apiUri, serverLogger, connection, ssl);
    auto userController = controller::UserController::getInstance(
        apiUri, serverLogger, connection, ssl);

    auto roomController = controller::RoomController::getInstance(
        apiUri, serverLogger, connection, ssl);

    auto participantController = controller::ParticipantController::getInstance(
        apiUri, serverLogger, connection, ssl);

    auto invitationController = controller::InvitationController::getInstance(
        apiUri, serverLogger, connection, ssl);

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
