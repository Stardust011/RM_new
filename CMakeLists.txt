cmake_minimum_required(VERSION 3.26)
project(RM)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O2 -pthread")

# opencv
if (WIN32)
    message(FATAL_ERROR "Can't build under Windows")
    set(OpenCV_DIR "H:\\opencv\\build\\install\\x64\\vc17\\lib")    # 设置opencv路径
else ()
endif ()
find_package(OpenCV REQUIRED)
include_directories(${OpenCV_INCLUDE_DIRS})
link_libraries(${OpenCV_LIBS})

# ncurses
# sudo apt-get install libncurses5-dev
find_package(Curses REQUIRED)
include_directories(${CURSES_INCLUDE_DIR})
# 输出CURSES_INCLUDE_DIR
#message(STATUS "CURSES_INCLUDE_DIR: ${CURSES_INCLUDE_DIR}")
link_libraries(${CURSES_LIBRARY})

# CUDA
#find_package(CUDA)
#if (${CUDA_FOUND})
#    set(CUDA_SOURCE_PROPERTY_FORMAT OBJ)
#    set(CUDA_SEPARABLE_COMPILATION ON)
#    include_directories(${CUDA_INCLUDE_DIRS})
#    set(CUDA_PROPAGATE_HOST_FLAGS OFF)
#    set(CUDA_NVCC_FLAGS -gencode arch=compute_20,code=sm_20;-G;-g)#根据具体GPU更改参数
#    #arch=compute_30,code=sm_30
#    #arch=compute_35,code=sm_35
#    #arch=compute_50,code=[sm_50,compute_50]
#    #arch=compute_52,code=[sm_52,compute_52]
#    #SET(CUDA_NVCC_FLAGS ${CUDA_NVCC_FLAGS};-gencode arch=compute_61,code=sm_61;-std=c++11;-O3;-G;-g)
#    link_directories($ENV{CUDA_PATH}/lib/x64)
#    CUDA_ADD_EXECUTABLE(cv_2022 ${CURRENT_HEADERS} ${CURRENT_SOURCES})
#else (${CUDA_FOUND})
#    MESSAGE(STATUS "Skip CUDA!")
#endif (${CUDA_FOUND})

# 项目文件
if (WIN32)
    file(GLOB CURRENT_HEADERS Inc/*.h Inc/*.hpp Inc/*.cuh)
    file(GLOB CURRENT_SOURCES Src/*.cpp Src/*.cu)
    include_directories(win-unix/)
    file(GLOB WIN_UNIX win-unix/*.h win-unix/*.hpp)
else()
    file(GLOB CURRENT_HEADERS Inc/*.h Inc/*.hpp Inc/*.cuh)
    file(GLOB CURRENT_SOURCES Src/*.cpp Src/*.h Src/*.cu )
endif()
source_group("Include" FILES ${CURRENT_HEADERS})
source_group("Source" FILES ${CURRENT_SOURCES})

include_directories(./)
include_directories(Inc/)
include_directories(Src/)

# 引入库
# WzSerialPortPlus 串口库
add_library(WzSerialPortPlus STATIC Lib/WzSerialPortPlus/WzSerialPortPlus.cpp )
target_include_directories(WzSerialPortPlus PUBLIC "Lib/WzSerialPortPlus")

# yaml-cpp yaml解析库
include_directories(./Lib/yaml-cpp)
find_library(YAMLCPP libyaml-cpp.a ${CMAKE_SOURCE_DIR}/Lib/yaml-cpp)

add_executable(RM main.cpp ${CURRENT_HEADERS} ${CURRENT_SOURCES} ${WIN_UNIX})
target_link_libraries(RM WzSerialPortPlus)
target_link_libraries(RM ${YAMLCPP})
