/* -------------------------------------------------------------------------------
* Copyright (c) 2018, OLogN Technologies AG
* All rights reserved.
*
* Redistribution and use in source and binary forms, with or without
* modification, are permitted provided that the following conditions are met:
*     * Redistributions of source code must retain the above copyright
*       notice, this list of conditions and the following disclaimer.
*     * Redistributions in binary form must reproduce the above copyright
*       notice, this list of conditions and the following disclaimer in the
*       documentation and/or other materials provided with the distribution.
*     * Neither the name of the OLogN Technologies AG nor the
*       names of its contributors may be used to endorse or promote products
*       derived from this software without specific prior written permission.
*
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
* ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
* WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
* DISCLAIMED. IN NO EVENT SHALL OLogN Technologies AG BE LIABLE FOR ANY
* DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
* (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
* LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
* ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
* (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
* SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
* -------------------------------------------------------------------------------*/

#include <stdio.h>
#include <utility> // TODO: move it to a proper place

#include <foundation.h>
#include <nodecpp_assert.h>
#include "test.h"

void printPlatform()
{
#if defined NODECPP_CLANG
	nodecpp::default_log::info(  "Compiler: clang" );
#elif defined NODECPP_GCC
	nodecpp::default_log::info(  "Compiler: gcc" );
#elif defined NODECPP_MSVC
	nodecpp::default_log::info(  "Compiler: msvcv" );
#else
	nodecpp::default_log::info(  "Compiler: unknown" );
#endif

#if defined NODECPP_X64
	nodecpp::default_log::info(  "64 bit" );
#elif defined NODECPP_X86
	nodecpp::default_log::info(  "32 bit" );
#else
	nodecpp::default_log::info(  "unknown platform" );
#endif

#if defined NODECPP_LINUX
	nodecpp::default_log::info(  "OS: Linux" );
#elif (defined NODECPP_WINDOWS )
	nodecpp::default_log::info(  "OS: Windows" );
#else
	nodecpp::default_log::info(  "OS: unknown" );
#endif
	nodecpp::default_log::info(  "Minimum CPU page size: {} bytes", NODECPP_MINIMUM_CPU_PAGE_SIZE );
	nodecpp::default_log::info(  "Minimum Zero Guard page size: {} bytes", NODECPP_MINIMUM_ZERO_GUARD_PAGE_SIZE );
}

void fnWithAssertion(int i)
{
	NODECPP_ASSERT( nodecpp::foundation::module_id, nodecpp::assert::AssertLevel::critical, i>0, "i = {}", i );
}

void fnWithAssertion2(int i)
{
	NODECPP_ASSERT( nodecpp::foundation::module_id, nodecpp::assert::AssertLevel::critical, i>0 );
}

#include "../include/std_error.h"
#include "../include/safe_memory_error.h"
#include "samples/file_error.h"

int fnThatThrows( int n )
{
	if ( n > 0 )
		return n;
	switch ( n )
	{
		case - 1: throw nodecpp::error::file_error(nodecpp::error::FILE_EXCEPTION::dne, "some_file" ); break;
		case -2: throw nodecpp::error::bad_address; break;
		/*case -3: throw nodecpp::error::memory_error_memory_access_violation__; break;
		case -4: throw nodecpp::error::memory_error_null_pointer_access__; break;
		case -5: throw nodecpp::error::memory_error_out_of_bound__; break;*/
	}
	return 1;
}

void fnThatCatches()
{
	int ret = fnThatThrows(1);
	nodecpp::default_log::info( "fnThatThrows(1) = {}", ret);
	ret = 0;
	try
	{
		ret = fnThatThrows(-1);
		nodecpp::default_log::info( "fnThatThrows(-1): OK, ret = {}", ret);
	}
	catch (nodecpp::error::error e)
	{
		nodecpp::default_log::info( "error caught; e.name = {}, e.description = {}", e.name().c_str(), e.description().c_str() );
		ret = 0;
	}
	try
	{
		ret = fnThatThrows(-2);
		nodecpp::default_log::info( "fnThatThrows(-1): OK, ret = {}", ret);
	}
	catch (nodecpp::error::error e)
	{
		nodecpp::default_log::info( "error caught; e.name = {}, e.description = {}", e.name().c_str(), e.description().c_str() );
		if ( e == nodecpp::error::bad_address )
			nodecpp::default_log::info( "error comparison: OK" );
		else
			nodecpp::default_log::info( "error comparison: FAILED" );
		if ( e == nodecpp::error::zero_pointer_access )
			nodecpp::default_log::info( "error comparison: OK" );
		else
			nodecpp::default_log::info( "error comparison: FAILED" );
		ret = 0;
	}
	nodecpp::default_log::info( "fnThatCatches(), ret = {}", ret);
}

template<class TestStructT, bool secondPtr = false>
void testPtrStructsWithZombieProperty_(int* dummy)
{
	bool OK = true;
	TestStructT optimizedPtrWithZombiePproperty;

	if constexpr( secondPtr )
		optimizedPtrWithZombiePproperty.init();
	else
		optimizedPtrWithZombiePproperty.init(nullptr);

	NODECPP_ASSERT( nodecpp::foundation::module_id, nodecpp::assert::AssertLevel::critical, !optimizedPtrWithZombiePproperty.is_zombie() );
	try { optimizedPtrWithZombiePproperty.get_dereferencable_ptr(); OK = false; } catch ( ... ) {}
	NODECPP_ASSERT( nodecpp::foundation::module_id, nodecpp::assert::AssertLevel::critical, OK );
	try { optimizedPtrWithZombiePproperty.get_ptr(); } catch ( ... ) { OK = false; }
	NODECPP_ASSERT( nodecpp::foundation::module_id, nodecpp::assert::AssertLevel::critical, OK );

	if constexpr( secondPtr )
		optimizedPtrWithZombiePproperty.init(dummy, dummy, 17);
	else
		optimizedPtrWithZombiePproperty.init(dummy);
	NODECPP_ASSERT( nodecpp::foundation::module_id, nodecpp::assert::AssertLevel::critical, !optimizedPtrWithZombiePproperty.is_zombie() );
	try { optimizedPtrWithZombiePproperty.get_dereferencable_ptr(); } catch ( ... ) { OK = false; }
	NODECPP_ASSERT( nodecpp::foundation::module_id, nodecpp::assert::AssertLevel::critical, OK );
	try { optimizedPtrWithZombiePproperty.get_ptr(); } catch ( ... ) { OK = false; }
	NODECPP_ASSERT( nodecpp::foundation::module_id, nodecpp::assert::AssertLevel::critical, OK );

	optimizedPtrWithZombiePproperty.set_zombie();
	NODECPP_ASSERT( nodecpp::foundation::module_id, nodecpp::assert::AssertLevel::critical, optimizedPtrWithZombiePproperty.is_zombie() );
	try { optimizedPtrWithZombiePproperty.get_ptr(); OK = false; } catch ( ... ) {}
	NODECPP_ASSERT( nodecpp::foundation::module_id, nodecpp::assert::AssertLevel::critical, OK );
	try { optimizedPtrWithZombiePproperty.get_dereferencable_ptr(); OK = false; } catch ( ... ) {}
	NODECPP_ASSERT( nodecpp::foundation::module_id, nodecpp::assert::AssertLevel::critical, OK );

	if constexpr( secondPtr )
	{
		NODECPP_ASSERT( nodecpp::foundation::module_id, nodecpp::assert::AssertLevel::critical, optimizedPtrWithZombiePproperty.is_zombie() );
		try { optimizedPtrWithZombiePproperty.get_allocated_ptr(); OK = false; } catch ( ... ) {}
		NODECPP_ASSERT( nodecpp::foundation::module_id, nodecpp::assert::AssertLevel::critical, OK );
		try { optimizedPtrWithZombiePproperty.get_dereferencable_allocated_ptr(); OK = false; } catch ( ... ) {}
		NODECPP_ASSERT( nodecpp::foundation::module_id, nodecpp::assert::AssertLevel::critical, OK );
	}
}

void testPtrStructsWithZombieProperty()
{
	int* dummy = new int; // allocated

	testPtrStructsWithZombieProperty_<nodecpp::platform::ptrwithdatastructsdefs::optimized_ptr_with_zombie_property_>( dummy );
	testPtrStructsWithZombieProperty_<nodecpp::platform::ptrwithdatastructsdefs::generic_ptr_with_zombie_property_>( dummy );

	// NOTE:
	//     for practical purposes allocated_ptr_and_ptr_and_data_and_flags< dataminsize, nflags > should be used;
	//     Platform-dependent code below is provided for direct testing purposes only
	//     to test both implementations, if current platform allows (that is, 64 bit)
#ifdef NODECPP_X64
	testPtrStructsWithZombieProperty_<nodecpp::platform::ptrwithdatastructsdefs::optimized_allocated_ptr_and_ptr_and_data_and_flags_64_<32,1>, true>( dummy );
#endif
	testPtrStructsWithZombieProperty_<nodecpp::platform::ptrwithdatastructsdefs::generic_allocated_ptr_and_ptr_and_data_and_flags_<32,1>, true>( dummy );

	delete dummy;
}

int main(int argc, char *argv[])
{
	nodecpp::Log log;
	log.level = nodecpp::LogLevel::info;
	log.add( stdout );
	for ( size_t i=0; i<2000; ++i )
		log.warning( "whatever warning # {}", i );
	nodecpp::logging_impl::currentLog = &log;
	for ( size_t i=0; i<2000; ++i )
		nodecpp::default_log::warning( "whatever warning # {}", 2000+i );

	printPlatform();
	testSEH();

	fnThatCatches(); //return 0;
	fnWithAssertion(1);
	try
	{
		fnWithAssertion(0);
	}
	catch (...)
	{
		nodecpp::default_log::info( "error cought!");
	}
	try
	{
		fnWithAssertion2(0);
	}
	catch (...)
	{
		nodecpp::default_log::info( "error cought!");
	}


	const char* testMsg = "some long message";
	int fake = 17;
	nodecpp::default_log::info( "[1] Hi! msg = \'{}\', fake = {} <end>", testMsg, fake );

	testPtrStructsWithZombieProperty();

	nodecpp::default_log::info( "about to exit...                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                         " );

    return 0;
}
