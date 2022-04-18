cmake_minimum_required(VERSION 3.0)
project(ddshop_cpp)

set(CMAKE_CXX_STANDARD 14)

find_package(OpenSSL REQUIRED)
find_package(spdlog REQUIRED)
find_package(nlohmann_json REQUIRED)

add_executable(${PROJECT_NAME}
        src/ddshop/address.cpp
        src/ddshop/cart.cpp
        src/ddshop/order.cpp
        src/ddshop/reserve_time.cpp
        src/ddshop/session_impl.cpp
        src/ddshop/user.cpp
        src/ddshop/dispatcher_impl.cpp
        src/notification/bark_impl.cpp
        src/main.cpp)
target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_LIST_DIR}/include)
target_link_libraries(${PROJECT_NAME} PRIVATE OpenSSL::SSL OpenSSL::Crypto pthread spdlog::spdlog nlohmann_json::nlohmann_json)
target_compile_definitions(${PROJECT_NAME} PRIVATE CPPHTTPLIB_OPENSSL_SUPPORT)