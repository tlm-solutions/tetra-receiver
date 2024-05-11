find_package(GTest REQUIRED)
message(STATUS "GTEST_INCLUDE_DIR: ${GTEST_INCLUDE_DIR}")
message(STATUS "GTEST_LIBRARIES: ${GTEST_LIBRARIES}")

# configure build of googletest
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
set(BUILD_GMOCK OFF CACHE BOOL "" FORCE)

add_executable(
    unit_tests
		main.cpp
		mockup_test.cpp
)

target_include_directories(unit_tests PUBLIC ${GTEST_INCLUDE_DIR})
target_link_libraries(unit_tests PUBLIC ${GTEST_LIBRARIES})

# automatic discovery of unit tests
include(GoogleTest)
gtest_discover_tests(unit_tests
  PROPERTIES
    LABELS "unit"
  DISCOVERY_TIMEOUT  # how long to wait (in seconds) before crashing
    240
  )
