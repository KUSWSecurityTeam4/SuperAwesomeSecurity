#pragma once

#include <exception>
#include <string>

#include <fmt/core.h>

namespace chat::module::exception {

std::string makeExceptionMSG(uint8_t code, std::string cls, std::string msg) {
  return fmt::v9::format("code[{}] : class[{}] :msg[{}]", code, cls, msg);
}

class BusinessException : public std::runtime_error {
public:
  BusinessException(std::string msg) : std::runtime_error(msg) {}
};

class SecurityException : public BusinessException {
public:
  SecurityException(std::string msg) : BusinessException(msg) {}
};

class NotImplementedException : public BusinessException {
public:
  NotImplementedException(std::string msg) : BusinessException(msg) {}
};

class EntityException : public BusinessException {
public:
  EntityException(std::string msg) : BusinessException(msg) {}
};

class DuplicatedEntityException : public EntityException {
public:
  DuplicatedEntityException(std::string msg) : EntityException(msg) {}
};

class NotFoundEntityException : public EntityException {
public:
  NotFoundEntityException(std::string msg) : EntityException(msg) {}
};

class NotSavedEntityException : public EntityException {
public:
  NotSavedEntityException(std::string msg) : EntityException(msg) {}
};

class NotUpdatedEntityException : public EntityException {
public:
  NotUpdatedEntityException(std::string msg) : EntityException(msg) {}
};

class NotRemovedEntityException : public EntityException {
public:
  NotRemovedEntityException(std::string msg) : EntityException(msg) {}
};

class ServiceException : public BusinessException {
public:
  ServiceException(std::string msg) : BusinessException(msg) {}
};

class ControllerException : public BusinessException {
public:
  ControllerException(std::string msg) : BusinessException(msg) {}
};

class NotAuthorizedException : public ControllerException {
public:
  NotAuthorizedException(std::string msg) : ControllerException(msg) {}
};
} // namespace chat::module::exception
