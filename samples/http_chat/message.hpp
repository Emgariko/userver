#pragma once

#include "userver/storages/postgres/io/io_fwd.hpp"
#include "userver/storages/postgres/io/pg_types.hpp"

#include <userver/formats/json/value.hpp>

namespace model::message {

struct Message {
  int id;
  std::string author;
  std::string text;
  userver::storages::postgres::TimePointTz datetime;

  formats::json::Value ToJsonValue() const {
    const auto res = formats::json::MakeObject(
        "id", id,
        "author", author,
        "text", text,
        "datetime", datetime.GetUnderlying()
    );

    return res;
  }
};

} // model::message

namespace userver::storages::postgres::io {
// This specialization MUST go to the header together with the mapped type
template <>
struct CppToUserPg<model::message::Message> {
  static constexpr DBTypeName postgres_name = "public.message_v1";
};
}  // namespace storages::postgres::io