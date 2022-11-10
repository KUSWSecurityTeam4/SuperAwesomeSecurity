#pragma once

#include "exception.hpp"
using namespace chat::module::exception;

#include <algorithm>
#include <functional>
#include <limits>
#include <random>
#include <string>
#include <vector>

namespace chat::module::secure {

std::string hash(std::string password, std::string salt) {
  constexpr uint8_t pepper = 10;
  static std::hash<std::string> hash{};

  auto hashedPw = password + salt;
  for (auto i = 0; i < pepper; ++i) {
    hashedPw = std::to_string(hash(hashedPw + salt));
  }
  return hashedPw;
}
bool compare(std::string receivedPw, std::string storedPw, std::string salt) {
  constexpr uint8_t pepper = 10;
  static std::hash<std::string> hash{};

  auto hashedReceivedPw = receivedPw + salt;
  for (auto i = 0; i < pepper; ++i) {
    hashedReceivedPw = std::to_string(hash(hashedReceivedPw + salt));
  }
  return hashedReceivedPw == storedPw;
}

uint64_t generateRandomNumber() {
  std::random_device rd;
  std::mt19937_64 gen(rd());
  std::uniform_int_distribution<uint64_t> distrib(
      std::numeric_limits<uint64_t>::min(),
      std::numeric_limits<uint64_t>::max());
  return distrib(gen);
}

std::string generateFixedLengthCode(uint64_t length) {
  static constexpr char charset[] = {
      '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C',
      'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P',
      'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z', 'a', 'b', 'c',
      'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o', 'p',
      'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z'};

  std::default_random_engine rng(std::random_device{}());
  std::uniform_int_distribution<> dist(0, sizeof(charset) - 1);
  auto randchar = [&dist, &rng]() { return charset[dist(rng)]; };
  std::string str(length, 0);
  std::generate_n(str.begin(), length, randchar);
  return str;
}
} // namespace chat::module::secure
