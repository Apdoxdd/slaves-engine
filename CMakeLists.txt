cmake_minimum_required(VERSION 3.16)
project(slavesPool CXX)
set(CMAKE_CXX_STANDARD 23)


#libcurl
set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
set(BUILD_CURL_EXE OFF CACHE BOOL "" FORCE)
set(BUILD_TESTING OFF CACHE BOOL "" FORCE)
set(CURL_USE_OPENSSL ON CACHE BOOL "" FORCE)

add_subdirectory(third_party/curl EXCLUDE_FROM_ALL)
#libxml2
set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
set(LIBXML2_WITH_PYTHON OFF CACHE BOOL "" FORCE)
set(LIBXML2_WITH_ICONV OFF CACHE BOOL "" FORCE)
set(LIBXML2_WITH_LZMA OFF CACHE BOOL "" FORCE)
set(LIBXML2_WITH_ZLIB OFF CACHE BOOL "" FORCE)
set(LIBXML2_WITH_TESTS OFF CACHE BOOL "" FORCE)
set(LIBXML2_WITH_PROGRAMS OFF CACHE BOOL "" FORCE)

add_subdirectory(third_party/libxml2 EXCLUDE_FROM_ALL)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_executable(slavesPool src/main.cpp src/crawler.cpp src/slave.cpp src/utlis.cpp)
target_link_libraries(slavesPool PRIVATE CURL::libcurl LibXml2::LibXml2)
