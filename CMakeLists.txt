cmake_minimum_required(VERSION 3.28...3.30)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_CXX_STANDARD 20)

add_library(obs-scorecapture MODULE)


target_link_libraries(obs-scorecapture PRIVATE OBS::libobs OBS::websocket-api)

find_package(OpenCV REQUIRED COMPONENTS core imgproc imgcodecs)

find_package(Qt6 REQUIRED COMPONENTS Gui)
target_link_libraries(obs-scorecapture PRIVATE Qt::Gui)

if(OS_MACOS)
  find_package(Iconv REQUIRED)
elseif(OS_LINUX OR OS_FREEBSD OR OS_OPENBSD)
  find_package(Fontconfig REQUIRED)
endif()

target_link_libraries(obs-scorecapture PRIVATE "${OpenCV_LIBRARIES}")
target_include_directories(obs-scorecapture SYSTEM PUBLIC "${OpenCV_INCLUDE_DIRS}")

target_sources(obs-scorecapture PRIVATE 
  src/Preset.cpp
  src/SCFilter.cpp
  src/SCFilter-info.cpp
  src/SCFilter-properties.cpp
  src/SCSource.cpp
  src/SCSource-info.cpp
  src/SCSource-properties.cpp
  src/obs-utils.cpp
  src/plugin-main.c
  src/plugin-support.c
)

set_target_properties_obs(obs-scorecapture PROPERTIES FOLDER plugins PREFIX "")
