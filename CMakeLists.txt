cmake_minimum_required(VERSION 3.22)

add_executable(gnuradio-decoder-cpp
               src/main.cpp)

target_compile_options(gnuradio-decoder-cpp PUBLIC -std=c++11 -Wall)

include_directories(src)

find_package(Gnuradio "3.8" REQUIRED)
find_package(Boost REQUIRED)
find_package(Volk REQUIRED)
find_package(gnuradio-osmosdr REQUIRED)
find_package(cxxopts REQUIRED)

include_directories(${GNURADIO_ALL_INCLUDE_DIRS})

target_link_libraries(gnuradio-decoder-cpp log4cpp gnuradio-digital gnuradio-analog gnuradio-filter gnuradio-blocks gnuradio-fft gnuradio-runtime gnuradio-pmt volk gnuradio-osmosdr)

install(TARGETS gnuradio-decoder-cpp DESTINATION bin)
