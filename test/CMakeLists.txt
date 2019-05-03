cmake_minimum_required(VERSION 3.0)
project(foundation_test)
set(PROJ_NAME test_foundation)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Release")
    message(STATUS "Build type not specified: defaulting to release.")
endif()

file(GLOB SOURCE_EXE  "*.cpp" , "samples/*.cpp" )


add_subdirectory(${PROJECT_SOURCE_DIR}/../ build)
include_directories(../include ../3rdparty/fmt/include ../src  ${PROJECT_SOURCE_DIR}/samples)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED on)
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -DDEBUG")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -DNDEBUG")
set(CMAKE_CXX_FLAGS " ${CMAKE_CXX_FLAGS}  -O2   -DNODECPP_CUSTOM_LOG_PROCESSING=\\\"${PROJECT_SOURCE_DIR}/my_logger.h\\\" " )

if(MSVC OR MSYS OR MINGW)
	 add_definitions(/W3)
	 add_definitions(/EHa)
else()
	add_definitions( -Wall )
	add_definitions(-fexceptions)
	add_definitions(-fnon-call-exceptions)
	#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -fexceptions -fnon-call-exceptions")
endif()

add_executable(test_foundation ${SOURCE_EXE})
target_link_libraries(test_foundation foundation )

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
endif ( CMAKE_BUILD_TYPE STREQUAL   "Debug")
