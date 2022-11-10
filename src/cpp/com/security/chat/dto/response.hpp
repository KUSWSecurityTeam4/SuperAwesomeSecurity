#pragma once

#include "../dao/all.hpp"

#include <cpprest/json.h>

#include <algorithm>
#include <initializer_list>
#include <iterator>
#include <list>
#include <string>
#include <vector>

namespace chat::dto {

enum CODE {
  OK = 200,
  UNAUTHORIZED = 401,
  NOT_FOUND = 404,
  DUPLICATED = 405,
  NOT_UPDATED = 406,
  NOT_SAVED = 407,
  NOT_REMOVED = 408,
  UNEXPECTED = 500
};

using Serializable =
    std::vector<std::pair<utility::string_t, web::json::value>>;

class Data {
public:
  virtual web::json::value serialize() const {
    return web::json::value::object(data);
  }

protected:
  Data() = default;
  Serializable data;
};

class Response {
  // Int를 모두 string 형식으로 보낸다(Dart에서 bigInt로 처리하는 불편함을
  // 해결하기 위해
public:
  Response(CODE code, std::string msg, Data data)
      : code(code), msg(msg), data(data) {}

  web::json::value serialize() const {
    Serializable value{};
    value.emplace_back("code", web::json::value::string(std::to_string(code)));
    value.emplace_back("message", web::json::value::string(msg));
    value.emplace_back("data", data.serialize());
    return web::json::value::object(value);
  }

private:
  CODE code;
  std::string msg;
  Data data;
  Response() = delete;
};

class CompanyData : public Data {
public:
  CompanyData(dao::Company &company) {
    Serializable companyData{};
    companyData.emplace_back(
        "id", web::json::value::string(std::to_string(company.getId())));
    companyData.emplace_back("name",
                             web::json::value::string(company.getName()));
    data.emplace_back("company", web::json::value::object(companyData));
  }
};

class UserData : public Data {
public:
  UserData(dao::User &user) {
    Serializable userData{};
    userData.emplace_back(
        "id", web::json::value::string(std::to_string(user.getId())));
    userData.emplace_back("companyId", web::json::value::string(std::to_string(
                                           user.getCompanyId())));
    userData.emplace_back("name", web::json::value::string(user.getName()));
    userData.emplace_back("email", web::json::value::string(user.getEmail()));
    userData.emplace_back("role", web::json::value::string(user.getRole()));

    data.emplace_back("user", web::json::value::object(userData));
  }
};

class ParticipantData : public Data {
public:
  ParticipantData(dao::Participant &participant) {
    Serializable participantData{};
    participantData.emplace_back(
        "id", web::json::value::string(std::to_string(participant.getId())));
    participantData.emplace_back(
        "roomId",
        web::json::value::string(std::to_string(participant.getRoomId())));
    participantData.emplace_back(
        "userId",
        web::json::value::string(std::to_string(participant.getUserId())));
    participantData.emplace_back(
        "role", web::json::value::string(participant.getRole()));
    data.emplace_back("participant", web::json::value::object(participantData));
  }
};

class RoomData : public Data {
public:
  RoomData(dao::Room &room) {
    Serializable roomData{};
    roomData.emplace_back(
        "id", web::json::value::string(std::to_string(room.getId())));
    roomData.emplace_back("name", web::json::value::string(room.getName()));
    data.emplace_back("room", web::json::value::object(roomData));
  }
};

class InvitationData : public Data {
public:
  InvitationData(dao::Invitation &invitation) {
    Serializable invitationData;
    invitationData.emplace_back(
        "id", web::json::value::string(std::to_string(invitation.getId())));
    invitationData.emplace_back(
        "roomId",
        web::json::value::string(std::to_string(invitation.getRoomId())));
    invitationData.emplace_back(
        "userId",
        web::json::value::string(std::to_string(invitation.getUserId())));
    invitationData.emplace_back(
        "expiredAt", web::json::value::string(module::convertToLocalTimeString(
                         invitation.getExpiredAt())));
    data.emplace_back("invitation", web::json::value::object(invitationData));
  }
};

class ServerSessionData : public Data {
public:
  ServerSessionData(dao::ServerSession &serverSession) {
    Serializable serverSessionData;
    serverSessionData.emplace_back(
        "id", web::json::value::string(std::to_string(serverSession.getId())));
    serverSessionData.emplace_back(
        "token", web::json::value::string(serverSession.getToken()));
    data.emplace_back("session", web::json::value::object(serverSessionData));
  }
};

class ExceptionData : public Data {
public:
  ExceptionData(CODE code, std::string msg) {
    Serializable exceptionData;
    exceptionData.emplace_back("code",
                               web::json::value::string(std::to_string(code)));
    exceptionData.emplace_back("msg", web::json::value::string(msg));
    data.emplace_back("exception", web::json::value::object(exceptionData));
  }
};

class ArrayData : public Data {
public:
  ArrayData(std::initializer_list<Data> array) {
    std::vector<web::json::value> arrayData;
    std::transform(array.begin(), array.end(), std::back_inserter(arrayData),
                   [](Data data) { return data.serialize(); });
    data.emplace_back("array", web::json::value::array(arrayData));
  }
  ArrayData(std::list<Data> array) {
    std::vector<web::json::value> arrayData;
    std::transform(array.begin(), array.end(), std::back_inserter(arrayData),
                   [](Data data) { return data.serialize(); });
    data.emplace_back("array", web::json::value::array(arrayData));
  }
  ArrayData(std::vector<Data> array) {
    std::vector<web::json::value> arrayData;
    std::transform(array.begin(), array.end(), std::back_inserter(arrayData),
                   [](Data data) { return data.serialize(); });
    data.emplace_back("array", web::json::value::array(arrayData));
  }
};

class MsgData : public Data {
public:
  MsgData(std::string msg) {
    data.emplace_back("message", web::json::value::string(msg));
  }
};
} // namespace chat::dto