cmake_minimum_required(VERSION 2.8)
project(PvRec)

# Version informations -----------------------------------------------------

set(PVREC_VERSION_MAJOR 1)
set(PVREC_VERSION_MINOR 0)
set(PVREC_VERSION_RELEASE 0)
set(PVREC_VERSION_STRING "${PVREC_VERSION_MAJOR}.${PVREC_VERSION_MINOR}.${PVREC_VERSION_RELEASE}")
configure_file(src/version.h.in ${CMAKE_CURRENT_BINARY_DIR}/version.h)

# Find libraries -----------------------------------------------------------

set(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake)

find_package(CFITSIO)
if(CFITSIO_FOUND)
    include_directories(${CFITSIO_INCLUDE_DIR})
    link_directories(${CFITSIO_LINK_DIRECTORIES})
else()
    message(FATAL_ERROR "CFITSIO not found")
endif()

find_package(PROSILICA)
if(PROSILICA_FOUND)
    # Defines needed for PvApi.h
    set(PROSILICA_OS LINUX CACHE STRING "The target operating sytem, see PvAPI.h")
    set(PROSILICA_CPU x86 CACHE STRING "The target cpu type, see PvAPI.h")
    mark_as_advanced(PROSILICA_OS PROSILICA_CPU)
    add_definitions(-D_${PROSILICA_OS} -D_${PROSILICA_CPU})
    include_directories(${PROSILICA_INCLUDE_DIR})
    link_directories(${PROSILICA_LINK_DIRECTORIES})
else()
    message(FATAL_ERROR "Prosilica API not found")
endif()

# Build --------------------------------------------------------------------

include_directories(${CMAKE_CURRENT_BINARY_DIR})

set(PvRec_SRCS
    src/pvrec.cpp
    src/recorder.cpp
    src/fitswriter.cpp
    src/pvutils.cpp
    src/cmdopts.cpp
)

add_executable(pvrec ${PvRec_SRCS})
target_link_libraries(pvrec
    ${PROSILICA_LIBRARIES}
    ${CFITSIO_LIBRARIES}
)
