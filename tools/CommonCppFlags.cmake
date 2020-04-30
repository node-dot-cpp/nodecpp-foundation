#-------------------------------------------------------------------------------------------
# Copyright (c) 2020, OLogN Technologies AG
#-------------------------------------------------------------------------------------------

#-------------------------------------------------------------------------------------------
# Compiler Flags
#-------------------------------------------------------------------------------------------

set(CMAKE_INTERPROCEDURAL_OPTIMIZATION TRUE)
set(CMAKE_CXX_STANDARD 17)

if (MSVC)
  # we need async exceptions on foundation
  string( REPLACE "/EHsc" "/EHa" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}" )

  add_definitions(-D_CRT_SECURE_NO_WARNINGS)
  add_definitions(-D_CRT_NONSTDC_NO_WARNINGS)
  
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
  	# if building release, use O2 instead of O3
  string(REPLACE "-O3" "-O2" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")

  add_compile_options( -Wall )
  add_compile_options( -Wextra )
  add_compile_options( -Wno-unused-variable )
  add_compile_options( -Wno-unused-parameter )
  add_compile_options( -Wno-empty-body )
  add_compile_options( -fexceptions )
  add_compile_options( -fnon-call-exceptions )
endif()

