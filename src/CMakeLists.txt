set(echolib_SOURCES
  argument_parser.cpp
  echo_service.cpp
)

add_library(
  echolib
  OBJECT
  ${echolib_SOURCES}
)
target_include_directories(
  echolib
  PRIVATE
  ${INCLUDE_DIRS}
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
  $<TARGET_OBJECTS:echolib>
)

install(TARGETS echo-server
    RUNTIME
    COMPONENT Runtime
)
