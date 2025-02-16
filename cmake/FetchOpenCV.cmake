include(FetchContent)

set(CUSTOM_OPENCV_URL
    ""
    CACHE STRING "URL of a downloaded OpenCV static library tarball")

set(CUSTOM_OPENCV_HASH
    ""
    CACHE STRING "Hash of a downloaded OpenCV staitc library tarball")

if(CUSTOM_OPENCV_URL STREQUAL "")
  set(USE_PREDEFINED_OPENCV ON)
else()
  message(STATUS "Using custom OpenCV: ${CUSTOM_OPENCV_URL}")
  if(CUSTOM_OPENCV_HASH STREQUAL "")
    message(
      FATAL_ERROR "CUSTOM_OPENCV_HASH not found. Both of CUSTOM_OPENCV_URL and CUSTOM_OPENCV_HASH must be present!")
  else()
    set(USE_PREDEFINED_OPENCV OFF)
  endif()
endif()

if(USE_PREDEFINED_OPENCV)
  set(OpenCV_VERSION "v4.11.0")
  set(OpenCV_BASEURL "https://github.com/ThereGoesMySanity/build-dep-opencv/releases/download/${OpenCV_VERSION}")

  if("${CMAKE_BUILD_TYPE}" STREQUAL Release OR "${CMAKE_BUILD_TYPE}" STREQUAL RelWithDebInfo)
    set(OpenCV_BUILD_TYPE Release)
  else()
    set(OpenCV_BUILD_TYPE Debug)
  endif()

  if(APPLE)
    if(OpenCV_BUILD_TYPE STREQUAL Debug)
      set(OpenCV_URL "${OpenCV_BASEURL}/opencv-macos-${OpenCV_VERSION}-Debug.tar.gz")
      set(OpenCV_HASH SHA256=984a5e08716424db3a77fbcb2293164cf856c064c95461f9f0e0f939faed5526)
    else()
      set(OpenCV_URL "${OpenCV_BASEURL}/opencv-macos-${OpenCV_VERSION}-Release.tar.gz")
      set(OpenCV_HASH SHA256=cd6b26e0296c64e36d2d11966bba1f9b519c88c05358fde543a42bd0c5e1ce2a)
    endif()
  elseif(MSVC)
    if(OpenCV_BUILD_TYPE STREQUAL Debug)
      set(OpenCV_URL "${OpenCV_BASEURL}/opencv-windows-${OpenCV_VERSION}-Debug.zip")
      set(OpenCV_HASH SHA256=5e1273253fc53ffe2602ea82c5d444f4de5413e79c2131abaacd73e874929f0a)
    else()
      set(OpenCV_URL "${OpenCV_BASEURL}/opencv-windows-${OpenCV_VERSION}-Release.zip")
      set(OpenCV_HASH SHA256=4095cf39338fc08e762d323c6f7b6505c1c09fff89989486f57a490bd51399be)
    endif()
  else()
    if(OpenCV_BUILD_TYPE STREQUAL Debug)
      set(OpenCV_URL "${OpenCV_BASEURL}/opencv-linux-${OpenCV_VERSION}-Debug.tar.gz")
      set(OpenCV_HASH SHA256=c6c6c27e6a1bb6acde6c11a8384658be4bcafb519820b6cf639617f5d876bf27)
    else()
      set(OpenCV_URL "${OpenCV_BASEURL}/opencv-linux-${OpenCV_VERSION}-Release.tar.gz")
      set(OpenCV_HASH SHA256=8433f776a0d7ea7494d62f9b41f556f4bd99db9dee042272d7cb15234f3bd998)
    endif()
  endif()
else()
  set(OpenCV_URL "${CUSTOM_OPENCV_URL}")
  set(OpenCV_HASH "${CUSTOM_OPENCV_HASH}")
endif()

FetchContent_Declare(
  opencv
  URL ${OpenCV_URL}
  URL_HASH ${OpenCV_HASH})
FetchContent_MakeAvailable(opencv)

add_library(OpenCV INTERFACE)
if(MSVC)
  target_link_libraries(
    OpenCV
    INTERFACE ${opencv_SOURCE_DIR}/x64/vc17/staticlib/opencv_imgproc4110.lib
              ${opencv_SOURCE_DIR}/x64/vc17/staticlib/opencv_imgcodecs4110.lib
              ${opencv_SOURCE_DIR}/x64/vc17/staticlib/opencv_core4110.lib
              ${opencv_SOURCE_DIR}/x64/vc17/staticlib/zlib.lib
              ${opencv_SOURCE_DIR}/x64/vc17/staticlib/libpng.lib)
  target_include_directories(OpenCV INTERFACE ${opencv_SOURCE_DIR}/include)
else()
  target_link_libraries(
    OpenCV INTERFACE ${opencv_SOURCE_DIR}/lib/libopencv_imgproc.a ${opencv_SOURCE_DIR}/lib/libopencv_core.a ${opencv_SOURCE_DIR}/lib/libopencv_imgcodecs.a
                     ${opencv_SOURCE_DIR}/lib/opencv4/3rdparty/libzlib.a
                     ${opencv_SOURCE_DIR}/lib/opencv4/3rdparty/liblibpng.a)
  target_include_directories(OpenCV INTERFACE ${opencv_SOURCE_DIR}/include/opencv4)
endif()
