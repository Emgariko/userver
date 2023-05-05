#include <userver/ws/ws_server.h>

#include <userver/logging/log.hpp>

#include <http_parser.h>

#include <cryptopp/sha.h>
#include <userver/crypto/base64.hpp>

#include <fmt/format.h>

USERVER_NAMESPACE_BEGIN

namespace ws_server {

namespace {

HttpMethod ConvertHttpMethod(http_method method) {
  switch (method) {
    case HTTP_DELETE:
      return HttpMethod::kDelete;
    case HTTP_GET:
      return HttpMethod::kGet;
    case HTTP_HEAD:
      return HttpMethod::kHead;
    case HTTP_POST:
      return HttpMethod::kPost;
    case HTTP_PUT:
      return HttpMethod::kPut;
    case HTTP_CONNECT:
      return HttpMethod::kConnect;
    case HTTP_PATCH:
      return HttpMethod::kPatch;
    case HTTP_OPTIONS:
      return HttpMethod::kOptions;
    default:
      return HttpMethod::kUnknown;
  }
}

struct ParserState {
  Request curRequest;

  std::string headerField;
  bool complete_header = false;
  bool complete = false;

  http_parser parser;
  http_parser_settings settings;

  ParserState() {
    http_parser_init(&parser, HTTP_REQUEST);
    parser.data = this;

    http_parser_settings_init(&settings);
    settings.on_message_begin = http_on_message_begin;
    settings.on_url = http_on_url;
    settings.on_header_field = http_on_header_field;
    settings.on_header_value = http_on_header_value;
    settings.on_headers_complete = http_on_header_complete;
    settings.on_body = http_on_body;
    settings.on_message_complete = http_on_message_complete;
  }

  static int http_on_url(http_parser* parser, const char* at, size_t length) {
    auto self = static_cast<ParserState*>(parser->data);
    self->curRequest.url.append(at, length);
    return 0;
  }

  static int http_on_header_field(http_parser* parser, const char* at,
                                  size_t length) {
    auto self = static_cast<ParserState*>(parser->data);
    if (!self->complete_header)
      self->headerField.append(at, length);
    else {
      self->headerField.assign(at, length);
      self->complete_header = false;
    }
    return 0;
  }

  static int http_on_header_value(http_parser* parser, const char* at,
                                  size_t length) {
    auto self = static_cast<ParserState*>(parser->data);
    self->curRequest.headers[self->headerField].append(at, length);
    self->complete_header = true;
    return 0;
  }

  static int http_on_body(http_parser* parser, const char* at, size_t length) {
    auto self = static_cast<ParserState*>(parser->data);
    self->curRequest.content.insert(self->curRequest.content.end(),
                                    (const std::byte*)at,
                                    (const std::byte*)at + length);
    return 0;
  }

  static int http_on_message_begin(http_parser* parser) {
    auto self = static_cast<ParserState*>(parser->data);
    self->curRequest.method = ConvertHttpMethod((http_method)parser->method);
    self->complete = false;
    return 0;
  }

  static int http_on_header_complete(http_parser* parser) {
    auto self = static_cast<ParserState*>(parser->data);
    self->curRequest.keepalive = http_should_keep_alive(parser);
    return 0;
  }

  static int http_on_message_complete(http_parser* parser) {
    auto self = static_cast<ParserState*>(parser->data);
    self->complete = true;
    return 0;
  }
};

std::vector<std::byte> status_response(HttpStatus status) {
  std::string result_str = fmt::format(
      "HTTP/1.1 {} {}\r\nConnection: close\r\n\r\n", (int)status, status);

  std::vector<std::byte> result;
  result.reserve(result_str.size());
  result.insert(result.end(), (const std::byte*)result_str.data(),
                (const std::byte*)(result_str.data() + result_str.size()));

  return result;
}

std::vector<std::byte> serialize_response(const Response& response, const Request& request/*,
                                              bool allow_encoding*/) {
  std::vector<std::byte> content;
  std::string additionalHeaders;
  if (!response.content_type.empty()) {
    //    if (allow_encoding) {
    //      if (auto acceptEncodingIt = request.headers.find("Accept-Encoding");
    //          acceptEncodingIt != request.headers.end()) {
    //        if (EncodeResult er = encode({response.content.data(), response.content.size()},
    //                                     acceptEncodingIt->second);
    //            !er.encoded_data.empty()) {
    //          content = std::move(er.encoded_data);
    //          additionalHeaders += "Content-Encoding: ";
    //          additionalHeaders += er.encoding;
    //          additionalHeaders += "\r\n";
    //        }
    //      }
    //    }

    if (content.empty()) content = std::move(response.content);

    additionalHeaders += "Content-Type: ";
    additionalHeaders += response.content_type;
    additionalHeaders += "\r\n";
    additionalHeaders += "Content-Length: ";
    additionalHeaders += std::to_string(content.size());
    additionalHeaders += "\r\n";
  }
  if (!response.keepalive) additionalHeaders += "Connection: close\r\n";

  std::string head = "HTTP/1.1 ";
  head += std::to_string((int)response.status) + " " +
          ToString(response.status) + "\r\n";
  for (auto attrib : response.headers)
    head += attrib.first + ": " + attrib.second + "\r\n";
  head += additionalHeaders;
  head += "\r\n";

  std::vector<std::byte> result;
  result.reserve(head.size() + content.size());
  result.insert(result.end(), (const std::byte*)head.data(),
                (const std::byte*)(head.data() + head.size()));
  ;
  if (!content.empty())
    result.insert(result.end(), content.begin(), content.end());
  return result;
}

inline bool TestHeaderVal(const Headers& headers, const std::string& key,
                          std::optional<std::string_view> val) {
  auto it = headers.find(key);
  if (it == headers.end()) return false;

  if (val) return it->second == *val;
  else return true;
}

inline const std::string& GetOptKey(const Headers& headers, const std::string& key,
                                    const std::string& default_value) {
  auto it = headers.find(key);
  if (it != headers.end()) return it->second;
  return default_value;
}

std::string WebsocketSecAnswer(std::string_view sec_key) {
  // guid is taken from RFC https://datatracker.ietf.org/doc/html/rfc6455#section-1.3
  static const std::string_view websocket_guid = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";
  CryptoPP::byte webSocketRespKeySHA1[CryptoPP::SHA1::DIGESTSIZE];
  CryptoPP::SHA1 hash;
  hash.Update((const CryptoPP::byte*)sec_key.data(), sec_key.size());
  hash.Update((const CryptoPP::byte*)websocket_guid.data(), websocket_guid.size());
  hash.Final(webSocketRespKeySHA1);
  return userver::crypto::base64::Base64Encode(
      std::string_view((const char*)webSocketRespKeySHA1, 20));
}

inline void SendExactly(userver::engine::io::WritableBase* writable,
                        const std::vector<std::byte>& data,
                        userver::engine::Deadline deadline) {
  if (writable->WriteAll(data.data(), data.size(), deadline) != data.size())
    throw(userver::engine::io::IoException()
          << "Socket closed during transfer ");
}

struct HttpStatusException : public std::exception {
  HttpStatus status;
  HttpStatusException(HttpStatus s) : status(s) {}
};

}

void WsServer::ProcessSocket(engine::io::Socket&& sock) {
  engine::io::Sockaddr remoteAddr = sock.Getpeername();

  std::unique_ptr<engine::io::RwBase> io = std::make_unique<engine::io::Socket>(std::move(sock));

  std::array<std::byte, 1024> buf;
  ParserState parserState;
  parserState.curRequest.client_address = &remoteAddr;

  LOG_INFO() << "Processing socket___";

  while (!engine::current_task::ShouldCancel()) {
    size_t nread = io->ReadSome(buf.data(), buf.size(), {});
    if (nread > 0) {

      // TODO: remove it
      LOG_INFO() << fmt::format("Trying to parse HTTP request, read size {}: {}", nread, fmt::join(buf, ","));
      http_parser_execute(&parserState.parser, &parserState.settings, (const char*)buf.data(),
                          nread);
      if (parserState.parser.http_errno != 0) {
        LOG_WARNING() << "bad data in http request ";
        return;
      }

      if (parserState.complete) {
        parserState.complete = false;

        Response response;
        std::optional<HttpStatus> errorStatus;

        Request curRequest = std::move(parserState.curRequest);
        try {
          response = HandleRequest(curRequest);
        } catch (HttpStatusException const& e) {
          errorStatus = e.status;
          LOG_INFO() << "Status exception " << ToString(e.status);
        } catch (std::exception const& e) {
          LOG_WARNING() << "Exception in http handler " << e;
          errorStatus = HttpStatus::kInternalServerError;
        }

        if (errorStatus.has_value()) {
          std::vector<std::byte> respData = status_response(errorStatus.value());
          SendExactly(io.get(), respData, {});
          io.reset();
        } else {
          //            SendExactly(io.get(), serialize_response(response, curRequest, config.allow_encoding),
          //                        {});
          LOG_INFO() << "Sending response";
          SendExactly(io.get(), serialize_response(response, curRequest/*, config.allow_encoding*/ /*false*/),
                      {});
          LOG_INFO() << "Response sent";
          if (response.post_send_cb) response.post_send_cb();
          if (response.upgrade_connection)
          {
                          response.upgrade_connection(std::move(io));


            // move socket to handler


            return;
          }
          if (!response.keepalive) return;
        }
      } else {
        // TODO: remove it
        LOG_INFO() << "parserState.complete == false";
      }

    } else if (nread == 0)  // connection closed
      return;
  }
}

Response WsServer::HandleRequest(const Request& request) {
  // https://datatracker.ietf.org/doc/html/rfc6455#section-4.2.1

  LOG_INFO() << "Handling request";

  if (request.method != HttpMethod::kGet) {
    throw HttpStatusException(HttpStatus::kBadRequest);
  }

  if (!TestHeaderVal(request.headers, "Host", std::nullopt)) {
    throw HttpStatusException(HttpStatus::kBadRequest);
  }

  if (!TestHeaderVal(request.headers, "Upgrade", "websocket") ||
      !TestHeaderVal(request.headers, "Connection", "Upgrade")) {
    throw HttpStatusException(HttpStatus::kBadRequest);
  }

  const std::string& secWebsocketKey = GetOptKey(request.headers, "Sec-WebSocket-Key", "");
  if (secWebsocketKey.empty()) throw HttpStatusException(HttpStatus::kBadRequest);
  if (GetOptKey(request.headers, "Sec-WebSocket-Version", "") != "13") throw HttpStatusException(HttpStatus::kBadRequest);


  // https://datatracker.ietf.org/doc/html/rfc6455#section-4.2.2
  Response resp;
  resp.status = HttpStatus::kSwitchingProtocols;
  resp.headers["Connection"] = "Upgrade";
  resp.headers["Upgrade"] = "websocket";
  resp.headers["Sec-WebSocket-Accept"] = WebsocketSecAnswer(secWebsocketKey);
  resp.keepalive = true;
  resp.upgrade_connection =
      [this, secWebsocketKey_ = secWebsocketKey](std::unique_ptr<engine::io::RwBase>&& io) mutable {
//        this->ProcessConnection(std::make_shared<WebSocketConnectionImpl>(
//            std::move(io), std::move(headers), *remoteAddr, this->config));
//        this->handler.socket_ = std::move(io);
          if (!this->handler) {
            this->handler = WsHandlerBase();
          }
          this->handler->AddSocket(std::move(io), std::move(secWebsocketKey_));

          LOG_INFO() << "Upgrade has been performed";
      };
  return resp;
}

}  // ws_server

USERVER_NAMESPACE_END