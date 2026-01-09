cmake_minimum_required(VERSION 3.25)
project(tracker-care-of LANGUAGES C CXX)

if(NOT DEFINED CMAKE_TOOLCHAIN_FILE)
    set(CMAKE_TOOLCHAIN_FILE "C:/Users/user/Documents/visual-studio-libraries/vcpkg/scripts/buildsystems/vcpkg.cmake"
        CACHE STRING "")
endif()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(fmt CONFIG REQUIRED)
find_package(PostgreSQL REQUIRED)
find_package(unofficial-argon2 CONFIG REQUIRED)

add_executable(main
    main.cpp

    services/auth-service.cpp

    db/postgre-db.cpp
    db/env-parser.cpp

    repositories/patient-repository.cpp
)

target_link_libraries(main
    PRIVATE fmt::fmt
    PostgreSQL::PostgreSQL
    unofficial::argon2::libargon2
)

