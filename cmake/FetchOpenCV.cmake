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
      set(OpenCV_HASH SHA256=f77e6d7c41fe0ace971b180c5a7b2b535bb5a8098d4661f2ea3404aa49b93db7)
    else()
      set(OpenCV_URL "${OpenCV_BASEURL}/opencv-macos-${OpenCV_VERSION}-Release.tar.gz")
      set(OpenCV_HASH SHA256=7d376f2d4a904096c5acf4b87f38cc93f80d7cc09a9ba1c303c58a705cf93519)
    endif()
  elseif(MSVC)
    if(OpenCV_BUILD_TYPE STREQUAL Debug)
      set(OpenCV_URL "${OpenCV_BASEURL}/opencv-windows-${OpenCV_VERSION}-Debug.zip")
      set(OpenCV_HASH SHA256=bd5efb5cfeed0b61eae073e70898eb5b1160b1b31f32f35f5fa1ea18cddde71f)
    else()
      set(OpenCV_URL "${OpenCV_BASEURL}/opencv-windows-${OpenCV_VERSION}-Release.zip")
      set(OpenCV_HASH SHA256=9f7604f9958e2de8dd4de3b388a65bdbefb3070a141c74c14c4fbf98c7208ba8)
    endif()
  else()
    if(OpenCV_BUILD_TYPE STREQUAL Debug)
      set(OpenCV_URL "${OpenCV_BASEURL}/opencv-linux-${OpenCV_VERSION}-Debug.tar.gz")
      set(OpenCV_HASH SHA256=8496fb3a430fb0c7fb0563905751427eaa3ac1d1081ae66bb1785aa8029130dc)
    else()
      set(OpenCV_URL "${OpenCV_BASEURL}/opencv-linux-${OpenCV_VERSION}-Release.tar.gz")
      set(OpenCV_HASH SHA256=66f494787ab97dbfd9fbfa1ab21403552092c51ab5118329d19c3dd2f80eb98a)
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
