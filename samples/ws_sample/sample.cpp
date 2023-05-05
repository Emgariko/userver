#include <userver/components/minimal_component_list.hpp>
#include <userver/utils/daemon_run.hpp>
#include <userver/ws/ws_server.h>

int main(int argc, char* argv[]) {
  const auto component_list =
      userver::components::MinimalComponentList().Append<userver::ws_server::WsServer>();
  return userver::utils::DaemonMain(argc, argv, component_list);
}