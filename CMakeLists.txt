
project(iwtftpd)
cmake_minimum_required(VERSION 2.6)

set(PRJ_MAJOR_VERSION "1")
set(PRJ_MINOR_VERSION "0")
set(PRJ_VERSION "${PRJ_MAJOR_VERSION}.${PRJ_MINOR_VERSION}")

set(TARGET_NAME iwtftpd)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/bin)
set(LIBPOPT popt)

set(SOURCE_FILES
  ${PROJECT_SOURCE_DIR}/src/datastore.c
  ${PROJECT_SOURCE_DIR}/src/iwtftpd.c
  ${PROJECT_SOURCE_DIR}/src/logging.c
  ${PROJECT_SOURCE_DIR}/src/tftp.c
  ${PROJECT_SOURCE_DIR}/src/util.c
  )

if(DEBUG)
  set(CMAKE_BUILD_TYPE "Debug")
  add_definitions(-DDEBUG)
  add_definitions(-g)
endif()

add_definitions(-DPROGRAM_VERSION=\"${PRJ_VERSION}\")
add_definitions(-W -Wall)

add_executable(${TARGET_NAME} ${SOURCE_FILES})
target_link_libraries(${TARGET_NAME} ${LIBPOPT})

