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
      set(OpenCV_HASH SHA256=974b7eaef7aed295dc3d15007fdf100b80ddaf518525f7d7edbea49826cb4794)
    else()
      set(OpenCV_URL "${OpenCV_BASEURL}/opencv-macos-${OpenCV_VERSION}-Release.tar.gz")
      set(OpenCV_HASH SHA256=e84953e5f063ad34ac27f3e1e13cd9863a57633b7d848608eaac1f3ebcc8dcda)
    endif()
  elseif(MSVC)
    if(OpenCV_BUILD_TYPE STREQUAL Debug)
      set(OpenCV_URL "${OpenCV_BASEURL}/opencv-windows-${OpenCV_VERSION}-Debug.zip")
      set(OpenCV_HASH SHA256=fdb054493781705160929f3ea5ac69468835d6bdf8632c03a7a0df672a8eed5f)
    else()
      set(OpenCV_URL "${OpenCV_BASEURL}/opencv-windows-${OpenCV_VERSION}-Release.zip")
      set(OpenCV_HASH SHA256=cf1f9e95d753537249170c83c0e7133eeaac1191a4831c23a634390d72a7d185)
    endif()
  else()
    if(OpenCV_BUILD_TYPE STREQUAL Debug)
      set(OpenCV_URL "${OpenCV_BASEURL}/opencv-linux-${OpenCV_VERSION}-Debug.tar.gz")
      set(OpenCV_HASH SHA256=b5eb5097fcd9a735e58ba91a25d28f95fe2bb46752382f766a9671147420618f)
    else()
      set(OpenCV_URL "${OpenCV_BASEURL}/opencv-linux-${OpenCV_VERSION}-Release.tar.gz")
      set(OpenCV_HASH SHA256=0052c4944ce2a7eff1df6e8f7505e094030ec28a063dead0cc51798de9701e6f)
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
              ${opencv_SOURCE_DIR}/x64/vc17/staticlib/zlib.lib)
  target_include_directories(OpenCV SYSTEM INTERFACE ${opencv_SOURCE_DIR}/include)
else()
  target_link_libraries(
    OpenCV INTERFACE ${opencv_SOURCE_DIR}/lib/libopencv_imgproc.a ${opencv_SOURCE_DIR}/lib/libopencv_core.a ${opencv_SOURCE_DIR}/lib/libopencv_imgcodecs.a
                     ${opencv_SOURCE_DIR}/lib/opencv4/3rdparty/libzlib.a)
  target_include_directories(OpenCV SYSTEM INTERFACE ${opencv_SOURCE_DIR}/include/opencv4)
endif()
