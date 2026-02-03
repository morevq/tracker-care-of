cmake_minimum_required(VERSION 3.25)

if(NOT DEFINED CMAKE_TOOLCHAIN_FILE)
    if(DEFINED ENV{VCPKG_ROOT})
        set(CMAKE_TOOLCHAIN_FILE "$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake" CACHE STRING "")
    else()
        message(FATAL_ERROR "VCPKG_ROOT environment variable is not set!")
    endif()
endif()

project(tracker_care_of LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(fmt CONFIG REQUIRED)
find_package(PostgreSQL REQUIRED)
find_package(unofficial-argon2 CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)
find_package(cpr CONFIG REQUIRED)
find_package(Crow CONFIG REQUIRED)


add_library(tracker_common STATIC
    libs/common/src/env-parser.cpp
)
target_include_directories(tracker_common
    PUBLIC
        ${CMAKE_SOURCE_DIR}/libs/common/include
)

add_library(tracker_crypto STATIC
    libs/crypto/src/password-hasher.cpp
)
target_include_directories(tracker_crypto
    PUBLIC
        ${CMAKE_SOURCE_DIR}/libs/crypto/include
)
target_link_libraries(tracker_crypto
    PUBLIC 
        unofficial::argon2::libargon2
        tracker_common
)

add_library(tracker_core STATIC
    libs/core/src/models/patient.cpp
)
target_include_directories(tracker_core
    PUBLIC
        ${CMAKE_SOURCE_DIR}/libs/core/include
)
target_link_libraries(tracker_core
    PUBLIC
        tracker_common
        fmt::fmt
)

add_library(tracker_db STATIC
    libs/db/src/postgres-db.cpp
    libs/db/src/repositories/anamnesis-repository.cpp
    libs/db/src/repositories/patient-repository.cpp
    libs/db/src/repositories/user-repository.cpp
    libs/db/src/repositories/water-repository.cpp
    libs/core/src/usecases/auth-service.cpp
)
target_include_directories(tracker_db
    PUBLIC
        ${CMAKE_SOURCE_DIR}/libs/db/include
)
target_link_libraries(tracker_db
    PUBLIC
        tracker_core
        tracker_crypto
        PostgreSQL::PostgreSQL
)


add_executable(tracker_cli
    apps/cli/src/main.cpp
    apps/cli/src/api-client.cpp
    apps/cli/src/add-anamnesis-ui.cpp
    apps/cli/src/add-patient-ui.cpp
    apps/cli/src/console-utils.cpp
    apps/cli/src/show-anamnesis-ui.cpp
    apps/cli/src/date-utils.cpp
    apps/cli/src/patient-table-loader.cpp
    apps/cli/src/auth-handler.cpp
)
target_include_directories(tracker_cli
    PRIVATE
        ${CMAKE_SOURCE_DIR}/apps/cli/include
)
target_link_libraries(tracker_cli
    PRIVATE
        tracker_common
        tracker_core
        cpr::cpr
        nlohmann_json::nlohmann_json
        fmt::fmt
)


add_executable(tracker_api
    apps/api/src/main.cpp
    apps/api/src/controllers/anamnesis-controller.cpp
    apps/api/src/controllers/auth-controller.cpp
    apps/api/src/controllers/patient-controller.cpp
    apps/api/src/controllers/water-controller.cpp
    apps/api/src/middleware/auth-middleware.cpp
)
target_include_directories(tracker_api
    PRIVATE
        ${CMAKE_SOURCE_DIR}/apps/api/include
)
target_link_libraries(tracker_api
    PRIVATE
        tracker_db
        fmt::fmt
        nlohmann_json::nlohmann_json
)

set_target_properties(tracker_cli PROPERTIES
    VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
)
set_target_properties(tracker_api PROPERTIES
    VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
)

# Set tracker_api as the default startup project
set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT tracker_api)