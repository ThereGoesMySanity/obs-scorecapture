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
      set(OpenCV_HASH SHA256=381fb123fd1008a763f1060d1c9120bed22b38aee073a27871ea9a2d3f65ade8)
    else()
      set(OpenCV_URL "${OpenCV_BASEURL}/opencv-macos-${OpenCV_VERSION}-Release.tar.gz")
      set(OpenCV_HASH SHA256=b93cf4613feff1121b8cb0dc81c5999e30f473818e02a05b7fc0d96972ced98c)
    endif()
  elseif(MSVC)
    if(OpenCV_BUILD_TYPE STREQUAL Debug)
      set(OpenCV_URL "${OpenCV_BASEURL}/opencv-windows-${OpenCV_VERSION}-Debug.zip")
      set(OpenCV_HASH SHA256=1361f0382d499d9c60fbb45f8b831825b616a10a8a1de83d34937e551bbbb5c3)
    else()
      set(OpenCV_URL "${OpenCV_BASEURL}/opencv-windows-${OpenCV_VERSION}-Release.zip")
      set(OpenCV_HASH SHA256=b72005f323185b683293c4eb46e2e1c6d62be6001d31da2cc5c86ec14adaa1cc)
    endif()
  else()
    if(OpenCV_BUILD_TYPE STREQUAL Debug)
      set(OpenCV_URL "${OpenCV_BASEURL}/opencv-linux-${OpenCV_VERSION}-Debug.tar.gz")
      set(OpenCV_HASH SHA256=69d0cb2a784a549c3ab805629125e2f4f765b247e99f3c37f1562dffefdbf2b7)
    else()
      set(OpenCV_URL "${OpenCV_BASEURL}/opencv-linux-${OpenCV_VERSION}-Release.tar.gz")
      set(OpenCV_HASH SHA256=02ae54d12ac710f9f11492700163481f2bddae7ade8af2dab06d8552e2c7f05e)
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
    INTERFACE ${opencv_SOURCE_DIR}/x64/vc17/staticlib/opencv_imgproc481.lib
              ${opencv_SOURCE_DIR}/x64/vc17/staticlib/opencv_core481.lib
              ${opencv_SOURCE_DIR}/x64/vc17/staticlib/zlib.lib)
  target_include_directories(OpenCV SYSTEM INTERFACE ${opencv_SOURCE_DIR}/include)
else()
  target_link_libraries(
    OpenCV INTERFACE ${opencv_SOURCE_DIR}/lib/libopencv_imgproc.a ${opencv_SOURCE_DIR}/lib/libopencv_core.a
                     ${opencv_SOURCE_DIR}/lib/opencv4/3rdparty/libzlib.a)
  target_include_directories(OpenCV SYSTEM INTERFACE ${opencv_SOURCE_DIR}/include/opencv4)
endif()
