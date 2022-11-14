#pragma once

#include <cpprest/http_listener.h>
#include <string>

namespace chat::controller::ssl {
decltype(auto) configSSL(std::string keyPath, std::string crtPath,
                         std::string dhPath) {
  auto config = web::http::experimental::listener::http_listener_config{};
  config.set_ssl_context_callback([=](boost::asio::ssl::context &ctx) {
    ctx.set_options(boost::asio::ssl::context::default_workarounds |
                    boost::asio::ssl::context::no_sslv2 |
                    boost::asio::ssl::context::no_tlsv1 |
                    boost::asio::ssl::context::no_tlsv1_1 |
                    boost::asio::ssl::context::single_dh_use);

    ctx.use_certificate_chain_file(crtPath);
    ctx.use_private_key_file(keyPath, boost::asio::ssl::context::pem);
    ctx.use_tmp_dh_file(dhPath);
  });
  config.set_timeout(utility::seconds(10));

  return config;
}
} // namespace chat::controller::ssl