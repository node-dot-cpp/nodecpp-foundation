#-------------------------------------------------------------------------------------------
# Copyright (c) 2020, OLogN Technologies AG
#-------------------------------------------------------------------------------------------
cmake_minimum_required(VERSION 3.0)

if(POLICY CMP0054)
	cmake_policy(SET CMP0054 NEW)
endif()

if(POLICY CMP0077)
	cmake_policy(SET CMP0077 NEW)
endif()


project(libfoundation CXX)

include(CTest)

set(CMAKE_CXX_STANDARD 17)

set (CMAKE_MODULE_PATH "${CMAKE_MODULE_PATH};${CMAKE_CURRENT_SOURCE_DIR}/tools")
include(CommonCppFlags)



#-------------------------------------------------------------------------------------------
# Library definition
#-------------------------------------------------------------------------------------------
add_library(foundation STATIC
	src/cpu_exceptions_translator.cpp
	src/internal_msg.cpp
	src/log.cpp
	src/nodecpp_assert.cpp
	src/page_allocator.cpp
	src/safe_memory_error.cpp
	src/stack_info.cpp
	src/std_error.cpp
	src/tagged_ptr_impl.cpp
	)

target_compile_definitions(foundation PUBLIC NODECPP_NOT_USING_IIBMALLOC)

target_include_directories(foundation PUBLIC include)

if (CMAKE_SYSTEM_NAME STREQUAL "Windows")
	target_compile_options(foundation PUBLIC /EHa)
	# remove the default compiler option to avoid warning
	string( REPLACE "/EHsc" "" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}" )

elseif (CMAKE_SYSTEM_NAME STREQUAL "Linux")

	target_compile_options(foundation PUBLIC -fexceptions -fnon-call-exceptions)
#	target_link_options(foundation INTERFACE -rdynamic)
	set_target_properties(foundation PROPERTIES LINK_FLAGS -rdynamic)

elseif (CMAKE_SYSTEM_NAME STREQUAL "Darwin")
	target_compile_definitions(foundation PUBLIC NODECPP_NO_STACK_INFO_IN_EXCEPTIONS)

	target_compile_options(foundation PUBLIC -fexceptions -fnon-call-exceptions)
#	target_link_options(foundation INTERFACE -rdynamic)
	set_target_properties(foundation PROPERTIES LINK_FLAGS -rdynamic)
endif()


if(NOT TARGET fmt)
	add_subdirectory(3rdparty/fmt)
endif()

target_link_libraries(foundation fmt::fmt)

if (UNIX)
	target_link_libraries(foundation pthread)
endif()

if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
	target_link_libraries(foundation dl)
endif()

#-------------------------------------------------------------------------------------------
# Tests 
#-------------------------------------------------------------------------------------------
if (CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR)
	set(FOUNDATION_TEST ON CACHE BOOL "Enable foundation tests")
else()
	set(FOUNDATION_TEST OFF CACHE BOOL "Enable foundation tests")
endif()

if (FOUNDATION_TEST)
	# only when not a subproject
	add_executable(test_foundation
		test/main.cpp
		test/test_seh.cpp
		test/samples/file_error.cpp
	)

	# we test for self move 
	if (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
	  target_compile_options(test_foundation PRIVATE -Wno-self-move)
	endif()


	target_link_libraries(test_foundation foundation)

	if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
		add_test(Run_test_foundation test_foundation)
	endif()
endif()
