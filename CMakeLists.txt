cmake_minimum_required(VERSION 3.25)
project(tracker-care-of LANGUAGES C CXX)

if(NOT DEFINED CMAKE_TOOLCHAIN_FILE)
    if(DEFINED ENV{VCPKG_ROOT})
        set(CMAKE_TOOLCHAIN_FILE "$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake" CACHE STRING "")
    else()
        message(FATAL_ERROR "VCPKG_ROOT environment variable is not set!")
    endif()
endif()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(fmt CONFIG REQUIRED)
find_package(PostgreSQL REQUIRED)
find_package(unofficial-argon2 CONFIG REQUIRED)

add_executable(main
    main.cpp
    db/postgre-db.cpp
    include/crypto/password-hasher.cpp
    include/env/env-parser.cpp
    models/patient.cpp
    repositories/anamnesis-repository.cpp
    repositories/patient-repository.cpp
    repositories/user-repository.cpp
    repositories/water-repository.cpp
    services/auth-service.cpp
    ui/add-anamnesis-ui.cpp
    ui/add-patient-ui.cpp
    ui/console-utils.cpp
    ui/show-anamnesis-ui.cpp)

target_link_libraries(main
    PRIVATE fmt::fmt
    PostgreSQL::PostgreSQL
    unofficial::argon2::libargon2
)

set_target_properties(main PROPERTIES
    VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
)

target_include_directories(main PRIVATE ${CMAKE_SOURCE_DIR}/include)