#pragma once

#include <userver/components/tcp_acceptor_base.hpp>
#include <userver/server/http/http_method.hpp>
#include <userver/server/http/http_status.hpp>


#include <userver/logging/log.hpp>

USERVER_NAMESPACE_BEGIN

namespace ws_server {

// namespace unetwork::util {
struct string_hash {
  using hash_type = std::hash<std::string_view>;
  using is_transparent = void;
  size_t operator()(const char* str) const { return hash_type{}(str); }
  size_t operator()(std::string_view str) const { return hash_type{}(str); }
  size_t operator()(std::string const& str) const { return hash_type{}(str); }
};

template <typename T>
using string_map =
    std::unordered_map<std::string, T, string_hash, std::equal_to<>>;
//}  // namespace unetwork::util

using userver::server::http::HttpMethod;
using userver::server::http::HttpStatus;

using Headers = string_map<std::string>;

struct Request {
  std::string url;
  HttpMethod method;
  Headers headers;
  std::vector<std::byte> content;

  const userver::engine::io::Sockaddr* client_address;
  bool keepalive = false;
};

struct Response {
  HttpStatus status = HttpStatus::kOk;
  Headers headers;
  std::vector<std::byte> content;
  std::string_view content_type;
  bool keepalive = false;
  std::function<void()> post_send_cb;
  std::function<void(std::unique_ptr<userver::engine::io::RwBase>)>
      upgrade_connection;
};

struct WsHandlerBase {
  WsHandlerBase(std::unique_ptr<engine::io::RwBase>&& socket): socket_(std::move(socket)) {
    LOG_INFO() << "Upgrade has been performed";


  }

 private:
  std::unique_ptr<engine::io::RwBase> socket_;
};

struct WsServer : public components::TcpAcceptorBase {
  WsServer(const components::ComponentConfig& config,
           const components::ComponentContext& context): TcpAcceptorBase(config, context) {

    LOG_INFO() << "Ws server constructor has been called";
  }

  static constexpr std::string_view kName = "ws-server";

  /*struct WebSocket : public engine::io::RwBase {
    std::string Read();

   private:
    engine::io::Socket socket_;
  };*/

  const static size_t buffer_size = 512;

  void ProcessSocket(engine::io::Socket&& sock) override final;

  Response HandleRequest(const Request& request);

  std::optional<WsHandlerBase> handler;
};

} // namespace ws_server

USERVER_NAMESPACE_END
