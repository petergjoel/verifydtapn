cmake_minimum_required(VERSION 3.14)
cmake_policy(SET CMP0048 NEW)
cmake_policy(SET CMP0069 NEW)

set(CMAKE_CXX_STANDARD 17)
if (NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif (NOT CMAKE_BUILD_TYPE)

if (CMAKE_BUILD_TYPE MATCHES Release)
    if (NOT APPLE)
    	set(CMAKE_INTERPROCEDURAL_OPTIMIZATION TRUE)
    endif ()
endif ()
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

set(VERIFYPN_VERSION 3.4)
add_compile_definitions(VERIFYDTAPN_VERSION=\"${VERIFYDTAPN_VERSION}\")

option(VERIFYDTAPN_Static "Link libraries statically" ON)
option(VERIFYDTAPN_GetDependencies "Fetch external dependencies from web." ON)

project(verifydtapn VERSION ${VERIFYPN_VERSION} LANGUAGES CXX C)

if (VERIFYDTAPN_Static)
    set(BUILD_SHARED_LIBS OFF)
    if (NOT APPLE)
        set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static")
    endif (NOT APPLE)
else (VERIFYDTAPN_Static)
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static-libgcc -static-libstdc++")
endif (VERIFYDTAPN_Static)

set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -Wall -O3 -DNDEBUG")

set(CMAKE_EXE_LINKER_FLAGS_RELEASE "${CMAKE_EXE_LINKER_FLAGS_RELEASE} -O3 -DNDEBUG")
if (VERIFYPN_Static AND NOT APPLE)
    set(CMAKE_EXE_LINKER_FLAGS_RELEASE "${CMAKE_EXE_LINKER_FLAGS_RELEASE} -static")
endif ()

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

if (UNIX AND NOT APPLE)
    set(ARCH_TYPE "linux64")
elseif (APPLE)
    set(ARCH_TYPE "osx64")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mmacosx-version-min=11.2 -m64 ")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -dynamic -mmacosx-version-min=11.2 -std=c++14 -m64 ")
else ()
    set(ARCH_TYPE "win64")
endif ()


find_package(FLEX 2.6.0 REQUIRED)
find_package(BISON 3.5.0 REQUIRED)

if (VERIFYDTAPN_GetDependencies)
    # setup for external imports
    include(ExternalProject)
    set(EXTERNAL_INSTALL_LOCATION ${CMAKE_BINARY_DIR}/external)

    ExternalProject_add(rapidxml-ext
            URL https://downloads.sourceforge.net/project/rapidxml/rapidxml/rapidxml%201.13/rapidxml-1.13.zip
            URL_HASH SHA512=6c10583e6631ccdb0217d0a5381172cb4c1046226de6ef1acf398d85e81d145228e14c3016aefcd7b70a1db8631505b048d8b4f5d4b0dbf1811d2482eefdd265
            BUILD_COMMAND ""
            CONFIGURE_COMMAND mkdir -p ${CMAKE_BINARY_DIR}/external/include
            INSTALL_COMMAND cd ../rapidxml-ext && ${CMAKE_COMMAND} -E copy rapidxml.hpp rapidxml_iterators.hpp rapidxml_print.hpp rapidxml_utils.hpp ${EXTERNAL_INSTALL_LOCATION}/include
            )

    ExternalProject_add(unfoldtacpn-ext 
        GIT_REPOSITORY https://github.com/TAPAAL/unfoldTACPN.git
        GIT_TAG main 
	CMAKE_ARGS -DCMAKE_TOOLCHAIN_FILE=${CMAKE_TOOLCHAIN_FILE} -DCMAKE_C_COMPILER=${CMAKE_C_COMPILER} -DCMAKE_CXX_COMPILER=${CMAKE_CXX_COMPILER}  -DCMAKE_INSTALL_PREFIX=${EXTERNAL_INSTALL_LOCATION} -DBUILD_TESTING=OFF -DCMAKE_BUILD_TYPE=Release -DFLEX_EXECUTABLE=${FLEX_EXECUTABLE} -DBISON_EXECUTABLE=${BISON_EXECUTABLE}
    )
    include_directories(${EXTERNAL_INSTALL_LOCATION}/include)

endif (VERIFYDTAPN_GetDependencies)

set(Boost_USE_STATIC_LIBS ON)
set(Boost_USE_RELEASE_LIBS ON)
set(Boost_USE_MULTITHREADED OFF)
find_package(Boost 1.66 REQUIRED)
include_directories(${Boost_INCLUDE_DIR})

include_directories(include)
add_subdirectory(${CMAKE_SOURCE_DIR}/src/)
