project(evdipp)
cmake_minimum_required(VERSION 2.8.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_INSTALL_PREFIX stage)
set(CMAKE_INSTALL_RPATH "${evdipp_BINARY_DIR}/stage/lib")
#INCLUDE_DIRECTORIES(stage/lib)
MESSAGE(STATUS "This is BINARY dir ${evdipp_BINARY_DIR}")
#set(LD_LIBRARY_PATH "${evdipp_BINARY_DIR}/stage/lib")

include(ExternalProject)
ExternalProject_Add(github_evdi
 GIT_REPOSITORY https://github.com/DisplayLink/evdi.git
 GIT_TAG v1.12.0
 BUILD_IN_SOURCE 1
 CONFIGURE_COMMAND ""
 BUILD_COMMAND cd library && sed -i "368,388d" evdi_lib.c && make
 INSTALL_COMMAND ""
)

ExternalProject_Get_Property(github_evdi SOURCE_DIR)
set(LIBEVDI_DIR ${SOURCE_DIR}/library)
install(FILES ${LIBEVDI_DIR}/libevdi.so DESTINATION lib RENAME libevdi.so.0)
install(FILES ${LIBEVDI_DIR}/evdi_lib.h DESTINATION include)

add_library(libevdi STATIC IMPORTED)
set_target_properties(libevdi PROPERTIES
  IMPORTED_LOCATION ${LIBEVDI_DIR}/libevdi.so
)
add_dependencies(libevdi github_evdi)

include_directories(${LIBEVDI_DIR})

# usbdisp

include(ExternalProject)
ExternalProject_Add(usbdisp
 SOURCE_DIR ../libusbdisp # the . directory is in where you type the command
 BUILD_IN_SOURCE 1
 CONFIGURE_COMMAND ""
 BUILD_COMMAND make libdisp
 INSTALL_COMMAND ""
)

ExternalProject_Get_Property(usbdisp SOURCE_DIR)
set(LIBDISP_DIR ${SOURCE_DIR}/lib)
install(FILES ${LIBDISP_DIR}/libusbdisp.so DESTINATION lib RENAME libusbdisp.so.0)
install(FILES ${LIBDISP_DIR}/../xrandr.h DESTINATION include)
install(FILES ${LIBDISP_DIR}/../usbDisplay.h DESTINATION include)

add_library(libusbdisp STATIC IMPORTED)
set_target_properties(libusbdisp PROPERTIES
  IMPORTED_LOCATION ${LIBDISP_DIR}/libusbdisp.so
)
add_dependencies(libusbdisp usbdisp)

include_directories(${LIBDISP_DIR})
include_directories(${LIBDISP_DIR}/../)


add_subdirectory(libevdipp)

add_subdirectory(example)
