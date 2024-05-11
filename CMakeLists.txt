cmake_minimum_required(VERSION 3.22)

add_executable(tetra-receiver
               src/main.cpp)

target_compile_options(tetra-receiver PUBLIC -std=c++11 -Wall)

include_directories(src)

find_package(Gnuradio "3.8" REQUIRED)
find_package(Boost REQUIRED)
find_package(Volk REQUIRED)
find_package(gnuradio-osmosdr REQUIRED)
find_package(cxxopts REQUIRED)

include_directories(${GNURADIO_ALL_INCLUDE_DIRS})

target_link_libraries(tetra-receiver log4cpp gnuradio-digital gnuradio-analog gnuradio-filter gnuradio-blocks gnuradio-fft gnuradio-runtime gnuradio-pmt volk gnuradio-osmosdr)

enable_testing()

add_subdirectory(test)

install(TARGETS tetra-receiver DESTINATION bin)
