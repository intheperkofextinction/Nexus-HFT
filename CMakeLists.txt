cmake_minimum_required(VERSION 3.20)
project(NexusHFT LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_compile_options(-O3 -march=native -Wall -Wextra -pthread)

# Find system libpcap library
find_package(PkgConfig REQUIRED)
pkg_check_modules(PCAP REQUIRED libpcap)

include_directories(include ${PCAP_INCLUDE_DIRS})

# Core Executable
add_executable(nexus_hft src/main.cpp)
target_link_libraries(nexus_hft PRIVATE pthread ${PCAP_LIBRARIES})

# Benchmarks
add_executable(benchmark_phase1 tests/benchmark_phase1.cpp)
target_link_libraries(benchmark_phase1 PRIVATE pthread)

add_executable(benchmark_phase2 tests/benchmark_phase2.cpp)
target_link_libraries(benchmark_phase2 PRIVATE pthread)

add_executable(benchmark_phase3 tests/benchmark_phase3.cpp)
target_link_libraries(benchmark_phase3 PRIVATE pthread)

add_executable(benchmark_phase4 tests/benchmark_phase4.cpp)
target_link_libraries(benchmark_phase4 PRIVATE pthread)

# PCAP Generator & Benchmark Targets
add_executable(generate_pcap tests/generate_pcap.cpp)

add_executable(benchmark_pcap tests/benchmark_pcap.cpp)
target_link_libraries(benchmark_pcap PRIVATE pthread ${PCAP_LIBRARIES})

# Module 1 Target
add_executable(benchmark_resiliency tests/benchmark_resiliency.cpp)
target_link_libraries(benchmark_resiliency PRIVATE pthread)

# Module 2 Target
add_executable(benchmark_matching tests/benchmark_matching.cpp)
target_link_libraries(benchmark_matching PRIVATE pthread)

# Module 4 Target
add_executable(benchmark_percentiles tests/benchmark_percentiles.cpp)
target_link_libraries(benchmark_percentiles PRIVATE pthread)
