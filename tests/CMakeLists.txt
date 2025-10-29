include(GoogleTest)

set(TEST_NAMES
  test_argument_parser
  test_generator
  test_mock_sendmsg
  test_tcp_echo_static_mock_getpeername
  test_tcp_echo_static
  test_tcp_echo
  test_udp_echo
)

foreach(TEST_NAME IN LISTS TEST_NAMES)
  add_executable(
    ${TEST_NAME}
    ${TEST_NAME}.cpp
  )

  target_include_directories(${TEST_NAME}
    PRIVATE
    "${INCLUDE_DIRS}"
  )

  target_link_libraries(
    ${TEST_NAME}
    PRIVATE
    echolib
    GTest::gtest_main
  )

  if(ECHO_ENABLE_COVERAGE)
    target_link_libraries(${TEST_NAME} PRIVATE gcov)
  endif()

  gtest_discover_tests(${TEST_NAME})
endforeach()
