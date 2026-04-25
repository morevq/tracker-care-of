#include "component-list.hpp"

#include <userver/utils/daemon_run.hpp>

int main(int argc, char* argv[]) {
    auto component_list = MakeComponentList();
    return userver::utils::DaemonMain(argc, argv, component_list);
}
