#-------------------------------------------------------------------------------------------
# Copyright (c) 2020, OLogN Technologies AG
#-------------------------------------------------------------------------------------------
cmake_minimum_required(VERSION 3.0)
project(libfoundation CXX)

include(CTest)

set (CMAKE_MODULE_PATH "${CMAKE_MODULE_PATH};${CMAKE_CURRENT_SOURCE_DIR}/tools")
include(CommonCppFlags)


#-------------------------------------------------------------------------------------------
# Defines
#-------------------------------------------------------------------------------------------

#add_definitions(-DNODECPP_CUSTOM_LOG_PROCESSING=\\\"${PROJECT_SOURCE_DIR}/test/my_logger.h\\\")


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

if (MSVC AND (CMAKE_CXX_COMPILER_ID STREQUAL "Clang"))
  # emulating clang-cl specifics
  target_compile_options(test_foundation PRIVATE -Wno-self-move)
endif()


target_link_libraries(test_foundation foundation)
add_test(Run_test_foundation test_foundation)
