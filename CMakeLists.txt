#-------------------------------------------------------------------------------------------
# Copyright (c) 2020, OLogN Technologies AG
#-------------------------------------------------------------------------------------------
cmake_minimum_required(VERSION 3.0)
project(libfoundation CXX)
set(CMAKE_CXX_STANDARD 17)
include(CTest)

#-------------------------------------------------------------------------------------------
# Defines
#-------------------------------------------------------------------------------------------
add_definitions(-D_CRT_SECURE_NO_WARNINGS)
add_definitions(-D_CRT_NONSTDC_NO_WARNINGS)

#add_definitions(-DNODECPP_CUSTOM_LOG_PROCESSING=\\\"${PROJECT_SOURCE_DIR}/test/my_logger.h\\\")

#-------------------------------------------------------------------------------------------
# Compiler Flags
#-------------------------------------------------------------------------------------------
if(MSVC)
	 add_compile_options(/W3)
	 add_compile_options(/EHa)
#	 add_compile_options(/MD)
else()
	#enable_testing()
	add_compile_options( -Wall )
	add_compile_options(-fexceptions)
	add_compile_options(-fnon-call-exceptions)
	#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -fexceptions -fnon-call-exceptions")
endif()

#-------------------------------------------------------------------------------------------
# Library definition
#-------------------------------------------------------------------------------------------
add_library(foundation STATIC
	src/cpu_exceptions_translator.cpp 
	src/log.cpp 
	src/nodecpp_assert.cpp 
	src/page_allocator.cpp 
	src/safe_memory_error.cpp 
	src/std_error.cpp 
	src/tagged_ptr_impl.cpp 
	)

#-------------------------------------------------------------------------------------------
# Export Include Directories
#-------------------------------------------------------------------------------------------
target_include_directories(foundation
  PUBLIC include
  )


#-------------------------------------------------------------------------------------------
# Package Dependencies 
#-------------------------------------------------------------------------------------------
add_subdirectory(3rdparty/fmt)

target_link_libraries(foundation fmt::fmt)
if (UNIX)
	target_link_libraries(foundation pthread)
endif()


#-------------------------------------------------------------------------------------------
# Tests 
#-------------------------------------------------------------------------------------------
add_executable(test_foundation
	test/main.cpp
	test/test_seh.cpp
	test/samples/file_error.cpp
)

target_link_libraries(test_foundation foundation)
add_test(Run_test_foundation test_foundation)
