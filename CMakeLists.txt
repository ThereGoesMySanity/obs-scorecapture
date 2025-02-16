cmake_minimum_required(VERSION 3.28...3.30)
set(CMAKE_OSX_DEPLOYMENT_TARGET 15.0)
set(CMAKE_OSX_ARCHITECTURES x86_64;arm64)

include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/common/bootstrap.cmake" NO_POLICY_SCOPE)

project(${_name} VERSION ${_version})

option(ENABLE_FRONTEND_API "Use obs-frontend-api for UI functionality" OFF)
option(ENABLE_QT "Use Qt functionality" OFF)

include(compilerconfig)
include(defaults)
include(helpers)

add_library(${CMAKE_PROJECT_NAME} MODULE)

find_package(libobs REQUIRED)
target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE OBS::libobs)
target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE ${CMAKE_SOURCE_DIR}/lib)

set(USE_SYSTEM_OPENCV
    OFF
    CACHE STRING "Use system OpenCV")

if(NOT OS_LINUX AND USE_SYSTEM_OPENCV)
  message(FATAL_ERROR "System OpenCV is only supported on Linux!")
endif()

if(USE_SYSTEM_OPENCV)
  find_package(OpenCV REQUIRED)
  target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE "${OpenCV_LIBRARIES}")
  target_include_directories(${CMAKE_PROJECT_NAME} PUBLIC "${OpenCV_INCLUDE_DIRS}")
else()
  include(cmake/FetchOpenCV.cmake)
endif()

find_package(Qt6 REQUIRED COMPONENTS Gui)
target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE Qt::Gui)
target_compile_options(
    ${CMAKE_PROJECT_NAME}
    PRIVATE $<$<C_COMPILER_ID:Clang,AppleClang>:-Wno-quoted-include-in-framework-header -Wno-comma>
  )

target_sources(${CMAKE_PROJECT_NAME} PRIVATE 
    src/Preset.cpp
    src/SCFilter.hpp
    src/SCFilter.cpp
    src/SCFilter-info.cpp
    src/SCFilter-properties.cpp
    src/SCSource.hpp
    src/SCSource.cpp
    src/SCSource-info.cpp
    src/SCSource-properties.cpp
    src/obs-utils.h
    src/obs-utils.cpp
    src/plugin-support.h
    src/plugin-main.c
    src/plugin-support.c
  )

set_target_properties_plugin(${CMAKE_PROJECT_NAME} PROPERTIES OUTPUT_NAME ${_name})
set_property(TARGET ${CMAKE_PROJECT_NAME} PROPERTY CXX_STANDARD 23)
