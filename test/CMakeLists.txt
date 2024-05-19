find_package(GTest REQUIRED)
message(STATUS "GTEST_INCLUDE_DIR: ${GTEST_INCLUDE_DIR}")
message(STATUS "GTEST_LIBRARIES: ${GTEST_LIBRARIES}")

# configure build of googletest
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
set(BUILD_GMOCK OFF CACHE BOOL "" FORCE)

add_executable(
    unit_tests
		main.cpp
		config_test.cpp
		prometheus_test.cpp
)

#target_include_directories(unit_tests PUBLIC ${GTEST_INCLUDE_DIR})
target_link_libraries(unit_tests PUBLIC ${GTEST_LIBRARIES})
target_link_libraries(unit_tests PRIVATE lib-tetra-receiver gmock)

install(TARGETS unit_tests DESTINATION bin)
