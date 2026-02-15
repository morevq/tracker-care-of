#include <iostream>
#include "config.h"
#include "api-server.h"

int main() {
    try {
        auto& config = tracker_api::Config::getInstance();
        tracker_api::ApiServer server(config);
        server.run();
    }
    catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
