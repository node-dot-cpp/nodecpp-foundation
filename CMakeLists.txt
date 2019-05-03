cmake_minimum_required(VERSION 3.0)
project(libfoundation)
set(PROJ_NAME foundation)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Release")
    message(STATUS "Build type not specified: defaulting to release")
endif()


file(GLOB SOURCE_LIB "src/*.cpp" )

set(SOURCE_LIB ${SOURCE_LIB} 3rdparty/fmt/src/format.cc  test/samples/file_error.cpp )
include_directories(include 3rdparty/fmt/include)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED on)
set(CMAKE_CXX_FLAGS_DEBUG " ${CMAKE_CXX_FLAGS_DEBUG} -DDEBUG")
set(CMAKE_CXX_FLAGS_RELEASE " ${CMAKE_CXX_FLAGS_RELEASE}  -DNDEBUG")
set(CMAKE_CXX_FLAGS  " ${CMAKE_CXX_FLAGS} -O2  -DNODECPP_CUSTOM_LOG_PROCESSING=\\\"${PROJECT_SOURCE_DIR}/test/my_logger.h\\\" " )

if(MSVC OR MSYS OR MINGW)
	 add_definitions(/W3)
	 add_definitions(/EHa)
else()
	add_definitions( -Wall )
	add_definitions(-fexceptions)
	add_definitions(-fnon-call-exceptions)
	#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -fexceptions -fnon-call-exceptions")
endif()

add_library(foundation STATIC ${SOURCE_LIB})

if (CMAKE_BUILD_TYPE STREQUAL  "Debug")
    message("Debug mode")
	set_target_properties(
	    ${PROJ_NAME}
	    PROPERTIES
	        OUTPUT_NAME "${PROJ_NAME}_debug"
	)
else()
	message("Release mode")
	set_target_properties(
	    ${PROJ_NAME}
	    PROPERTIES
	       OUTPUT_NAME "${PROJ_NAME}"
	 )
endif ( CMAKE_BUILD_TYPE STREQUAL  "Debug")

