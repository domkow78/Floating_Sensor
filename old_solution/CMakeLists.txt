# The following lines of boilerplate have to be in your project's
# CMakeLists in this exact order for cmake to work correctly
cmake_minimum_required(VERSION 3.5)

#https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-guides/build-system.html#optional-project-variables
#The set() commands should be placed after the cmake_minimum(...) line but before the include(...) line.

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DARCH_ESP32")
set(CMAKE_C_FLAGS 	"${CMAKE_C_FLAGS} 	-DARCH_ESP32")

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DVERBOSE_THR=0")
set(CMAKE_C_FLAGS 	"${CMAKE_C_FLAGS} 	-DVERBOSE_THR=0")

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DUSE_SHT")
set(CMAKE_C_FLAGS 	"${CMAKE_C_FLAGS} 	-DUSE_SHT")

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DUSE_BME")
set(CMAKE_C_FLAGS 	"${CMAKE_C_FLAGS} 	-DUSE_BME")

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DDEFERRED_SEND")
set(CMAKE_C_FLAGS 	"${CMAKE_C_FLAGS} 	-DDEFERRED_SEND")


include($ENV{IDF_PATH}/tools/cmake/project.cmake)
get_filename_component(ProjectId ${CMAKE_CURRENT_LIST_DIR} NAME)
string(REPLACE " " "_" ProjectId ${ProjectId})

idf_build_set_property(EXCLUDE_COMPONENTS "arduino")

project(${ProjectId})

idf_build_set_property(COMPILE_OPTIONS "-DUSE_BME" APPEND)
idf_build_set_property(COMPILE_OPTIONS "-DDEFERRED_SEND" APPEND)
idf_build_set_property(COMPILE_OPTIONS "-DVERBOSE_THR=0" APPEND)
idf_build_set_property(EXCLUDE_COMPONENTS "arduino")
