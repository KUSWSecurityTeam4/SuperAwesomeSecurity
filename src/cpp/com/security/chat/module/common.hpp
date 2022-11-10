#pragma once

// macro U turn off
// <cpprest/details/basic_types.h - 87>
#define _TURN_OFF_PLATFORM_STRING

#include <cpprest/uri.h>

#include <arpa/inet.h>
#include <chrono>
#include <ctime>
#include <net/if.h>
#include <string.h>
#include <string>
#include <sys/ioctl.h>

namespace chat::module {

std::string convertToLocalTimeString(time_t time) {
  char localTime[20];
  strftime(localTime, 20, "%Y-%m-%d %H:%M:%S", std::localtime(&time));
  return localTime;
}

std::tm convertToLocalTimeTM(time_t time) { return *std::localtime(&time); }

time_t getCurrentTime() {
  auto now = std::chrono::system_clock::now();
  std::time_t time = std::chrono::system_clock::to_time_t(now);
  return time;
}

std::string getIpAddr(std::string interface) {
  struct ifreq ifr;
  char ipstr[40];
  int s;

  s = socket(AF_INET, SOCK_DGRAM, 0);
  strncpy(ifr.ifr_name, interface.c_str(), IFNAMSIZ);

  if (ioctl(s, SIOCGIFADDR, &ifr) < 0) {
    return "";
  } else {
    inet_ntop(AF_INET, ifr.ifr_addr.sa_data + 2, ipstr,
              sizeof(struct sockaddr));
    return ipstr;
  }
}

web::uri buildUri(std::string scheme, std::string host, std::string port = "",
                  std::string userInfo = "", std::string path = "") {
  web::uri_builder uriBuilder{};
  uriBuilder.set_scheme(scheme);
  uriBuilder.set_host(host);

  if (port.length() > 0) {
    uriBuilder.set_port(port);
  }
  if (userInfo.length() > 0) {
    uriBuilder.set_user_info(userInfo);
  }
  if (path.length() > 0) {
    uriBuilder.set_path(path);
  }
  return uriBuilder.to_uri();
}

std::string trim(std::string original, char c = '"') {
  if (original.front() == c) {
    original.erase(original.begin());
  }
  if (original.back() == c) {
    original.erase(original.end() - 1);
  }
  return original;
}

bool isNumber(const std::string &str) {
  return str.find_first_not_of("0123456789") == std::string::npos;
}
} // namespace chat::module
