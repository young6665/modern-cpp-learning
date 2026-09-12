cmake_minimum_required(VERSION 3.14...3.22)

project(GreeterStandalone LANGUAGES CXX)

# --- Import tools ----

include(../cmake/tools.cmake)

# ---- Dependencies ----

include(../cmake/CPM.cmake)

CPMAddPackage(
  GITHUB_REPOSITORY jarro2783/cxxopts
  VERSION 3.3.1
  OPTIONS "CXXOPTS_BUILD_EXAMPLES NO" "CXXOPTS_BUILD_TESTS NO" "CXXOPTS_ENABLE_INSTALL YES"
)

CPMAddPackage(NAME Greeter SOURCE_DIR ${CMAKE_CURRENT_LIST_DIR}/..)

# ---- Create executables ----

add_executable(${PROJECT_NAME} source/main.cpp)
add_executable(TcpServer source/tcp_server.cpp)
add_executable(TcpClient source/tcp_client.cpp)

set_target_properties(
  ${PROJECT_NAME}
  PROPERTIES
    CXX_STANDARD 17
    OUTPUT_NAME "Greeter"
)

set_target_properties(
  TcpServer
  TcpClient
  PROPERTIES
    CXX_STANDARD 17
)

target_link_libraries(
  ${PROJECT_NAME}
  PRIVATE
    Greeter::Greeter
    cxxopts
)

target_link_libraries(TcpServer PRIVATE ws2_32)
target_link_libraries(TcpClient PRIVATE ws2_32)

if(MSVC)
  target_compile_options(${PROJECT_NAME} PRIVATE /utf-8)
  target_compile_options(TcpServer PRIVATE /utf-8)
  target_compile_options(TcpClient PRIVATE /utf-8)
endif()

