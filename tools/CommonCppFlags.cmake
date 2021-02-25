#-------------------------------------------------------------------------------------------
# Copyright (c) 2020, OLogN Technologies AG
#-------------------------------------------------------------------------------------------

#-------------------------------------------------------------------------------------------
# Compiler Flags
#
# mb: This file only takes care of warnings compiler flags
#-------------------------------------------------------------------------------------------

if (MSVC)
  # remove the default exception flag, so project has to be explicit about it
  string( REPLACE "/EHsc" "" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}" )

  add_definitions(-D_CRT_SECURE_NO_WARNINGS)
  
  add_compile_options( /W3 )
  add_compile_options( /wd4267 ) # conversion, possible loss of data


  if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    # emulating clang-cl specifics

    add_compile_options( -Wno-unused-variable )
    add_compile_options( -Wno-unused-parameter )
    add_compile_options( -Wno-empty-body )
    add_compile_options( -Wno-unknown-pragmas )
  endif()
else ()

  add_compile_options( -Wall )
  add_compile_options( -Wextra )
  add_compile_options( -Wno-unused-variable )
  add_compile_options( -Wno-unused-parameter )
  add_compile_options( -Wno-empty-body )
endif()

