cmake_minimum_required (VERSION 2.8.11 FATAL_ERROR)
project (HoneycombViewer)

set(EXECUTABLE_OUTPUT_PATH ${CMAKE_SOURCE_DIR}/bin)
link_directories (${CMAKE_LIBRARY_OUTPUT_DIRECTORY})

set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")

cmake_policy (SET CMP0020 NEW) # link qtmain by default on win32 platform

find_package (Qt5Widgets REQUIRED)
find_package (Qt5OpenGL REQUIRED)
find_package (OpenGL REQUIRED)
find_package (OpenCL)
#find_package (GLU)

include_directories(${Qt5OpenGL_INCLUDE_DIRS})
include_directories(${OPENCL_INCLUDE_DIRS})
include_directories(${CMAKE_SOURCE_DIR}/Ext/include)

set(CMAKE_LIBRARY_PATH ${CMAKE_LIBRARY_PATH} ${CMAKE_SOURCE_DIR}/Ext/lib/x86)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set (CMAKE_AUTOMOC ON)

file( GLOB HoneycombViewer_SRC ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)
file( GLOB HoneycombViewer_HDD ${CMAKE_CURRENT_SOURCE_DIR}/*.h)

source_group(Src FILES ${HoneycombViewer_SRC})
source_group(Headers FILES ${HoneycombViewer_HDD})

add_executable(HoneycombViewer ${HoneycombViewer_SRC} ${HoneycombViewer_HDD})
target_link_libraries (HoneycombViewer ${OPENGL_LIBRARIES} ${_OPENCL_64_LIBRARIES})
qt5_use_modules (HoneycombViewer Gui Widgets OpenGL)


    