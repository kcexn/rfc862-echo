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
  echo
  main.cpp
)
target_include_directories(
  echo
  PRIVATE
  ${INCLUDE_DIRS}
)
target_link_libraries(
  echo
  PRIVATE
  $<TARGET_OBJECTS:echolib>
)
