set(echolib_SOURCES
  argument_parser.cpp
  echo_server.cpp
)

add_library(
  echolib
  OBJECT
  ${echolib_SOURCES}
)
target_include_directories(
  echolib
  PUBLIC
  ${PROJECT_SOURCE_DIR}/include/
)
target_link_libraries(
  echolib
  PUBLIC
  asyncberk
  cppnet
  STDEXEC::stdexec
  spdlog::spdlog_header_only
)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_executable(
  echo-server
  main.cpp
)
target_include_directories(
  echo-server
  PRIVATE
  ${INCLUDE_DIRS}
)
target_link_libraries(
  echo-server
  PRIVATE
  echolib
)
install(TARGETS echo-server)
