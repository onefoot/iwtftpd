
project(iwtftpd)
cmake_minimum_required(VERSION 2.6)

set(PRJ_MAJOR_VERSION "1")
set(PRJ_MINOR_VERSION "0")
set(PRJ_VERSION "${PRJ_MAJOR_VERSION}.${PRJ_MINOR_VERSION}")
message(${PRJ_VERSION})

set(TARGET_NAME iwtftpd)
set(CMAKE_C_FLAGS "-W -Wall")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/bin)
set(LIBPOPT popt)

set(SOURCES
  ${PROJECT_BINARY_DIR}/../datastore.c
  ${PROJECT_BINARY_DIR}/../iwtftpd.c
  ${PROJECT_BINARY_DIR}/../logging.c
  ${PROJECT_BINARY_DIR}/../tftp.c
  ${PROJECT_BINARY_DIR}/../util.c
  )

if(DEBUG)
  set(CMAKE_BUILD_TYPE "Debug")
  add_definitions(-DDEBUG)
endif()

add_definitions(-DPROGRAM_VERSION=\"${PRJ_VERSION}\")

add_executable(${TARGET_NAME} ${SOURCES})
target_link_libraries(${TARGET_NAME} ${LIBPOPT})

