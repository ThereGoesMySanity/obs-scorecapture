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
      set(OpenCV_HASH SHA256=88785fe76e7b1387158eb604b3f66847167f6f19f7ccfd3c6d44c7da17ec770a)
    else()
      set(OpenCV_URL "${OpenCV_BASEURL}/opencv-macos-${OpenCV_VERSION}-Release.tar.gz")
      set(OpenCV_HASH SHA256=47f7b6ade1c2f086e332d4f7bbe88a4672f9a738fd1557db61cfc28eb02c9140)
    endif()
  elseif(MSVC)
    if(OpenCV_BUILD_TYPE STREQUAL Debug)
      set(OpenCV_URL "${OpenCV_BASEURL}/opencv-windows-${OpenCV_VERSION}-Debug.zip")
      set(OpenCV_HASH SHA256=61ed880ee34c1f3798e38910301d45069317a0d7f401890fb614c65492da105e)
    else()
      set(OpenCV_URL "${OpenCV_BASEURL}/opencv-windows-${OpenCV_VERSION}-Release.zip")
      set(OpenCV_HASH SHA256=8bb008ab21077183369b04661e7ed106bc02b77500cf2e81a104da9036d1dcea)
    endif()
  else()
    if(OpenCV_BUILD_TYPE STREQUAL Debug)
      set(OpenCV_URL "${OpenCV_BASEURL}/opencv-linux-${OpenCV_VERSION}-Debug.tar.gz")
      set(OpenCV_HASH SHA256=3cc9166c5e067a5732217a9b3dd3ac657b0ae95d27db3587820a4005b084a3be)
    else()
      set(OpenCV_URL "${OpenCV_BASEURL}/opencv-linux-${OpenCV_VERSION}-Release.tar.gz")
      set(OpenCV_HASH SHA256=ea79e87b50c1e2fc614f0a351ecc5d4bc658cec731c4b0b9267a90c1e9d045a4)
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
