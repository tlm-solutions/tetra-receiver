cmake_minimum_required(VERSION 3.22)
project(tetra-receiver)

SET(CMAKE_CXX_STANDARD 17)

#
# Find required libraries
#
find_package(Gnuradio "3.8" REQUIRED)
find_package(Boost REQUIRED)
find_package(Volk REQUIRED)
find_package(gnuradio-osmosdr REQUIRED)
find_package(cxxopts REQUIRED)
find_package(prometheus-cpp CONFIG REQUIRED)
find_package(GMP REQUIRED)

include_directories(${GNURADIO_ALL_INCLUDE_DIRS})

#
# Turn ON Testing
#
enable_testing()

#
# Configure the tetra-receiver library
#
add_library(lib-tetra-receiver
        src/config.cpp
        src/prometheus.cpp
        src/prometheus_gauge_populator.cpp
)

target_include_directories(lib-tetra-receiver PUBLIC include)

target_link_libraries(lib-tetra-receiver
        log4cpp
        volk
        gnuradio-osmosdr
        gnuradio-digital
        gnuradio-analog
        gnuradio-filter
        gnuradio-blocks
        gnuradio-fft
        gnuradio-runtime
        gnuradio-pmt
        prometheus-cpp::pull
        gmp
        #libgmp
)

#
# Build the tool
#
add_executable(tetra-receiver
        src/tetra-receiver.cpp
)

target_compile_options(tetra-receiver PUBLIC -std=c++17 -Wall)

target_link_libraries(tetra-receiver
  lib-tetra-receiver 
)

#
# Testing
#
add_subdirectory(test)

#
# Install tetra-receiver in bin folder
#
install(TARGETS tetra-receiver DESTINATION bin)
