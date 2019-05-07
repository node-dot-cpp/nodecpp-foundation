cmake_minimum_required(VERSION 3.0)
project(libfoundation)
set(PROJ_NAME foundation)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Release")
    message(STATUS "Build type not specified: defaulting to release")
endif()


file(GLOB SOURCE_LIB 	"src/log.cpp",
					 	"src/cpu_exceptions_translator.cpp",
					 	"src/std_error.cpp",
					 	"src/safe_memory_error.cpp",
					 	"src/tagged_ptr_impl.cpp"
					 	)

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


add_library(${PROJ_NAME} STATIC ${SOURCE_LIB})

file(GLOB SOURCE_EXE  "test/main.cpp", "test/test_seh.cpp" , "test/samples/file_error.cpp" )
add_executable(test.bin EXCLUDE_FROM_ALL ${SOURCE_EXE})

target_link_libraries(test.bin ${PROJ_NAME})

set_target_properties( ${PROJ_NAME} 
  PROPERTIES
  ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/build/lib"
  LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/build/lib"
)

set_target_properties(test.bin
  PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/test/build"
)

if (CMAKE_BUILD_TYPE STREQUAL  "Debug")
    message("Debug mode")
	set_target_properties(
	    ${PROJ_NAME}
	    PROPERTIES
	        OUTPUT_NAME "${PROJ_NAME}_debug"
	        ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/build/debug/lib"
  			LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/build/debug/lib"
	)
	set_target_properties(
    	test.bin
    	PROPERTIES
        	OUTPUT_NAME "test_debug.bin"
        	RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/test/build/debug"
	)
else()
	message("Release mode")
	set_target_properties(
	    ${PROJ_NAME}
	    PROPERTIES
	    	OUTPUT_NAME "${PROJ_NAME}"
	    	ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/build/release/lib"
  			LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/build/release/lib"
	 )
 	set_target_properties(
    	test.bin
    	PROPERTIES
       		OUTPUT_NAME "test.bin"
       		RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/test/build/release"
	 )
endif ( CMAKE_BUILD_TYPE STREQUAL  "Debug")

