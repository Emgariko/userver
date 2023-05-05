#include <userver/clients/dns/component.hpp>
#include <userver/testsuite/testsuite_support.hpp>

#include <userver/utest/using_namespace_userver.hpp>

#include <userver/components/minimal_server_component_list.hpp>
#include <userver/server/handlers/http_handler_base.hpp>
#include <userver/utils/daemon_run.hpp>

#include <userver/storages/postgres/cluster.hpp>
#include <userver/storages/postgres/component.hpp>

#include <fmt/format.h>
#include <userver/formats/json.hpp>

#include "message.hpp"

namespace samples::pg {

class MessagesService final : public server::handlers::HttpHandlerBase {
 public:
  static constexpr std::string_view kName = "handler-chat";

  MessagesService(const components::ComponentConfig& config,
           const components::ComponentContext& context);

  std::string HandleRequestThrow(
      const server::http::HttpRequest& request,
      server::request::RequestContext&) const override;

 private:
  std::string GetValue(const server::http::HttpRequest& request) const;
  std::string PostValue(const server::http::HttpRequest& request) const;
//  std::string DeleteValue(std::string_view key) const;

  userver::storages::postgres::ClusterPtr pg_cluster_;
};

}  // namespace samples::pg

namespace samples::pg {

MessagesService::MessagesService(const components::ComponentConfig& config,
                   const components::ComponentContext& context)
    : HttpHandlerBase(config, context),
      pg_cluster_(
          context.FindComponent<components::Postgres>("message-database")
              .GetCluster()) {
  constexpr auto kDropTable = "DROP TABLE IF EXISTS messages";
  constexpr auto kCreateTable = R"~(
      CREATE TABLE IF NOT EXISTS messages (
        id SERIAL PRIMARY KEY,
        author VARCHAR NOT NULL,
        text VARCHAR NOT NULL,
        datetime TIMESTAMPTZ NOT NULL
      )
    )~";
  constexpr auto kDropMessageV1Type = "DROP TYPE IF EXISTS message_v1";
  constexpr auto kCreateMessageV1Type = R"~(
      CREATE TYPE message_v1 AS (
        id integer,
        author VARCHAR,
        text VARCHAR,
        datetime TIMESTAMPTZ
      )
    )~";

  using storages::postgres::ClusterHostType;
  pg_cluster_->Execute(ClusterHostType::kMaster, kDropTable);
  pg_cluster_->Execute(ClusterHostType::kMaster, kCreateTable);
  pg_cluster_->Execute(ClusterHostType::kMaster, kDropMessageV1Type);
  pg_cluster_->Execute(ClusterHostType::kMaster, kCreateMessageV1Type);
}

std::string MessagesService::HandleRequestThrow(
    const server::http::HttpRequest& request,
    server::request::RequestContext&) const {

    switch (request.GetMethod()) {
      case server::http::HttpMethod::kGet:
        return GetValue(request);
      case server::http::HttpMethod::kPost:
        return PostValue(request);
//      case server::http::HttpMethod::kDelete:
//        return DeleteValue(key);
      default:
        throw server::handlers::ClientError(server::handlers::ExternalBody{
            fmt::format("Unsupported method {}", request.GetMethod())});
    }
//  const auto& key = request.GetArg("key");
//  if (key.empty()) {
//    throw server::handlers::ClientError(
//        server::handlers::ExternalBody{"No 'key' query argument"});
//  }
//
//  switch (request.GetMethod()) {
//    case server::http::HttpMethod::kGet:
//      return GetValue(key, request);
//    case server::http::HttpMethod::kPost:
//      return PostValue(key, request);
//    case server::http::HttpMethod::kDelete:
//      return DeleteValue(key);
//    default:
//      throw server::handlers::ClientError(server::handlers::ExternalBody{
//          fmt::format("Unsupported method {}", request.GetMethod())});
//  }
}

const storages::postgres::Query kSelectValues{
    "SELECT id, author, text, datetime FROM messages "
    "WHERE id > $1 "
    "ORDER BY id",
    storages::postgres::Query::Name{"sample_select_values"},
};

std::string MessagesService::GetValue(const server::http::HttpRequest& request) const {
    // exclusive
  const auto& from_id = std::stoi(request.GetArg("from_id"));

  const auto& new_msgs = pg_cluster_->Execute(storages::postgres::ClusterHostType::kSlave, kSelectValues, from_id).AsContainer<std::vector<model::message::Message>>(storages::postgres::kRowTag);


//  std::string res = "";
//
//  for (const auto& msg: new_msgs) {
//      res += msg.text + ", ";
//  }
//
//  return res;

  formats::json::ValueBuilder result(formats::json::Type::kObject);
  formats::json::ValueBuilder messages_json(formats::json::Type::kArray);
  for (const auto& msg: new_msgs) {
      formats::json::Value msg_json = msg.ToJsonValue();
      messages_json.PushBack(std::move(msg_json));
  }
  result["messages"] = messages_json.ExtractValue();

//  for (const auto& name : request.ArgNames()) {
//      formats::json::ValueBuilder values_json(formats::json::Type::kArray);
//      for (const auto& value : request.GetArgVector(name))
//        values_json.PushBack(value);
//
//      result[name] = values_json;
//  }
  return ToString(result.ExtractValue());
}

const storages::postgres::Query kInsertMessage{
    "INSERT INTO messages (author, text, datetime) "
    "VALUES ($1, $2, now()) "
    "RETURNING id;",
    storages::postgres::Query::Name{"sample_insert_value"},
};

std::string MessagesService::PostValue(const server::http::HttpRequest& request) const {
  const auto& author = request.GetArg("author");

  const auto& message = formats::json::FromString(request.RequestBody())["message"].As<std::string>();

  const auto inserted_message_id = pg_cluster_->Execute(
      storages::postgres::ClusterHostType::kMaster, kInsertMessage, author, message).AsSingleRow<int>();

  return std::to_string(inserted_message_id);
}

}  // namespace samples::pg

/// [Postgres service sample - main]
int main(int argc, char* argv[]) {
  const auto component_list =
      components::MinimalServerComponentList()
          .Append<samples::pg::MessagesService>()
          .Append<components::Postgres>("message-database")
          .Append<components::TestsuiteSupport>()
          .Append<clients::dns::Component>();
  return utils::DaemonMain(argc, argv, component_list);

  // :TODO:
  // 1) chat handlers
  // 2) dynamic_conf fix
  //
}
/// [Postgres service sample - main]
