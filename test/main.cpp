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
	nodecpp::log::default_log::info( nodecpp::log::ModuleID(nodecpp::foundation_module_id), "Compiler: clang" );
#elif defined NODECPP_GCC
	nodecpp::log::default_log::info( nodecpp::log::ModuleID(nodecpp::foundation_module_id), "Compiler: gcc" );
#elif defined NODECPP_MSVC
	nodecpp::log::default_log::info( nodecpp::log::ModuleID(nodecpp::foundation_module_id), "Compiler: msvcv" );
#else
	nodecpp::log::default_log::info( nodecpp::log::ModuleID(nodecpp::foundation_module_id), "Compiler: unknown" );
#endif

#if defined NODECPP_X64
	nodecpp::log::default_log::info( nodecpp::log::ModuleID(nodecpp::foundation_module_id), "64 bit" );
#elif defined NODECPP_X86
	nodecpp::log::default_log::info( nodecpp::log::ModuleID(nodecpp::foundation_module_id), "32 bit" );
#else
	nodecpp::log::default_log::info( nodecpp::log::ModuleID(nodecpp::foundation_module_id), "unknown platform" );
#endif

#if defined NODECPP_LINUX
	nodecpp::log::default_log::info( nodecpp::log::ModuleID(nodecpp::foundation_module_id), "OS: Linux" );
#elif (defined NODECPP_WINDOWS )
	nodecpp::log::default_log::info( nodecpp::log::ModuleID(nodecpp::foundation_module_id), "OS: Windows" );
#else
	nodecpp::log::default_log::info( nodecpp::log::ModuleID(nodecpp::foundation_module_id), "OS: unknown" );
#endif
	nodecpp::log::default_log::info( nodecpp::log::ModuleID(nodecpp::foundation_module_id), "Minimum CPU page size: {} bytes", NODECPP_MINIMUM_CPU_PAGE_SIZE );
	nodecpp::log::default_log::info( nodecpp::log::ModuleID(nodecpp::foundation_module_id), "Minimum Zero Guard page size: {} bytes", NODECPP_MINIMUM_ZERO_GUARD_PAGE_SIZE );
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
	nodecpp::log::default_log::info( nodecpp::log::ModuleID(nodecpp::foundation_module_id),"fnThatThrows(1) = {}", ret);
	ret = 0;
	try
	{
		ret = fnThatThrows(-1);
		nodecpp::log::default_log::info( nodecpp::log::ModuleID(nodecpp::foundation_module_id),"fnThatThrows(-1): OK, ret = {}", ret);
	}
	catch (nodecpp::error::error e)
	{
		nodecpp::log::default_log::info( nodecpp::log::ModuleID(nodecpp::foundation_module_id),"error caught; e.name = {}, e.description = {}", e.name().c_str(), e.description().c_str() );
		ret = 0;
	}
	try
	{
		ret = fnThatThrows(-2);
		nodecpp::log::default_log::info( nodecpp::log::ModuleID(nodecpp::foundation_module_id),"fnThatThrows(-1): OK, ret = {}", ret);
	}
	catch (nodecpp::error::error e)
	{
		nodecpp::log::default_log::info( nodecpp::log::ModuleID(nodecpp::foundation_module_id),"error caught; e.name = {}, e.description = {}", e.name().c_str(), e.description().c_str() );
		if ( e == nodecpp::error::bad_address )
			nodecpp::log::default_log::info( nodecpp::log::ModuleID(nodecpp::foundation_module_id),"error comparison: OK" );
		else
			nodecpp::log::default_log::info( nodecpp::log::ModuleID(nodecpp::foundation_module_id),"error comparison: FAILED" );
		if ( e == nodecpp::error::zero_pointer_access )
			nodecpp::log::default_log::info( nodecpp::log::ModuleID(nodecpp::foundation_module_id),"error comparison: OK" );
		else
			nodecpp::log::default_log::info( nodecpp::log::ModuleID(nodecpp::foundation_module_id),"error comparison: FAILED" );
		ret = 0;
	}
	nodecpp::log::default_log::info( nodecpp::log::ModuleID(nodecpp::foundation_module_id),"fnThatCatches(), ret = {}", ret);
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
	testPtrStructsWithZombieProperty_<nodecpp::platform::ptrwithdatastructsdefs::optimized_allocated_ptr_and_ptr_and_data_and_flags_64_<3,32,1>, true>( dummy );
#endif
	testPtrStructsWithZombieProperty_<nodecpp::platform::ptrwithdatastructsdefs::generic_allocated_ptr_and_ptr_and_data_and_flags_<2,32,1>, true>( dummy );

	delete dummy;
}

#include <internal_msg.h>
void testVectorOfPages()
{
	nodecpp::platform::internal_msg::InternalMsg imsg;
	constexpr size_t maxSz = 0x1000;
	uint64_t* buff = new uint64_t[maxSz];
	uint64_t ctr1 = 0;

	for ( size_t i=1; i<=maxSz; ++i )
	{
		for ( size_t j=0; j<i; j++ )
			buff[j] = ctr1++;
		imsg.append( buff, i * sizeof( uint64_t) );
	}
	delete [] buff;

	NODECPP_ASSERT( nodecpp::foundation::module_id, nodecpp::assert::AssertLevel::critical, imsg.size() == ctr1 * sizeof( uint64_t), "{:x} vs. {:x}", imsg.size(), ctr1 * sizeof( uint64_t) );
	nodecpp::log::default_log::info( nodecpp::log::ModuleID(nodecpp::foundation_module_id), "ctr1 = {:x}, msg.size() = {:x}", ctr1 * sizeof( uint64_t), imsg.size() );

	{
		uint64_t ctr2 = 0;
		nodecpp::platform::internal_msg::InternalMsg::ReadIter it = imsg.getReadIter();
		size_t available = it.availableSize();
		while ( available )
		{
			const uint64_t* ptr = reinterpret_cast<const uint64_t*>(it.read(available));
			available /= sizeof( uint64_t );
			for ( size_t i=0; i<available; ++i )
			{
				uint64_t val = ptr[i];
				NODECPP_ASSERT( nodecpp::foundation::module_id, nodecpp::assert::AssertLevel::critical, val == ctr2, "{} vs. {}", val, ctr2 );
				++ctr2;
			}
			available = it.availableSize();
		}
		NODECPP_ASSERT( nodecpp::foundation::module_id, nodecpp::assert::AssertLevel::critical, ctr1 == ctr2, "{:x} vs. {:x}", ctr1, ctr2 );
		nodecpp::log::default_log::info( nodecpp::log::ModuleID(nodecpp::foundation_module_id), "ctr1 = {:x}, ctr2 = {:x}", ctr1, ctr2 );
	}

	nodecpp::platform::internal_msg::InternalMsg imsg1( std::move( imsg ) );


	NODECPP_ASSERT( nodecpp::foundation::module_id, nodecpp::assert::AssertLevel::critical, imsg.size() == 0, "indeed: {:x}", imsg.size() );
	nodecpp::log::default_log::info( nodecpp::log::ModuleID(nodecpp::foundation_module_id), "after move msg.size() = {:x}", ctr1, imsg.size() );
	NODECPP_ASSERT( nodecpp::foundation::module_id, nodecpp::assert::AssertLevel::critical, imsg1.size() == ctr1 * sizeof( uint64_t), "{:x} vs. {:x}", imsg1.size(), ctr1 * sizeof( uint64_t) );
	nodecpp::log::default_log::info( nodecpp::log::ModuleID(nodecpp::foundation_module_id), "ctr1 * sizeof( uint64_t) = {:x}, msg.size() = {:x}", ctr1 * sizeof( uint64_t), imsg1.size() );

	{
		uint64_t ctr2 = 0;
		auto it = imsg1.getReadIter();
		size_t available = it.availableSize();
		while ( available )
		{
			const uint64_t* ptr = reinterpret_cast<const uint64_t*>(it.read(available));
			available /= sizeof( uint64_t );
			for ( size_t i=0; i<available; ++i )
			{
				uint64_t val = ptr[i];
				NODECPP_ASSERT( nodecpp::foundation::module_id, nodecpp::assert::AssertLevel::critical, val == ctr2, "{} vs. {}", val, ctr2 );
				++ctr2;
			}
			available = it.availableSize();
		}
		NODECPP_ASSERT( nodecpp::foundation::module_id, nodecpp::assert::AssertLevel::critical, ctr1 == ctr2, "{:x} vs. {:x}", ctr1, ctr2 );
		nodecpp::log::default_log::info( nodecpp::log::ModuleID(nodecpp::foundation_module_id), "ctr1 = {:x}, ctr2 = {:x}", ctr1, ctr2 );
	}

	imsg1 = std::move( imsg1 );

	{
		uint64_t ctr2 = 0;
		auto it = imsg1.getReadIter();
		size_t available = it.availableSize();
		while ( available )
		{
			const uint64_t* ptr = reinterpret_cast<const uint64_t*>(it.read(available));
			available /= sizeof( uint64_t );
			for ( size_t i=0; i<available; ++i )
			{
				uint64_t val = ptr[i];
				NODECPP_ASSERT( nodecpp::foundation::module_id, nodecpp::assert::AssertLevel::critical, val == ctr2, "{} vs. {}", val, ctr2 );
				++ctr2;
			}
			available = it.availableSize();
		}
		NODECPP_ASSERT( nodecpp::foundation::module_id, nodecpp::assert::AssertLevel::critical, ctr1 == ctr2, "{:x} vs. {:x}", ctr1, ctr2 );
		nodecpp::log::default_log::info( nodecpp::log::ModuleID(nodecpp::foundation_module_id), "ctr1 = {:x}, ctr2 = {:x}", ctr1, ctr2 );
	}

	{
		uint64_t ctr2 = 0;
		auto it = imsg.getReadIter();
		size_t available = it.availableSize();
		while ( available )
		{
			const uint64_t* ptr = reinterpret_cast<const uint64_t*>(it.read(available));
			available /= sizeof( uint64_t );
			for ( size_t i=0; i<available; ++i )
			{
				uint64_t val = ptr[i];
				NODECPP_ASSERT( nodecpp::foundation::module_id, nodecpp::assert::AssertLevel::critical, val == ctr2, "{} vs. {}", val, ctr2 );
				++ctr2;
			}
			available = it.availableSize();
		}
		NODECPP_ASSERT( nodecpp::foundation::module_id, nodecpp::assert::AssertLevel::critical, 0 == ctr2, "indeed: {:x}", ctr2 );
		nodecpp::log::default_log::info( nodecpp::log::ModuleID(nodecpp::foundation_module_id), "ctr1 = {:x}, ctr2 = {:x}", ctr1, ctr2 );
	}

	{
		NODECPP_ASSERT( nodecpp::foundation::module_id, nodecpp::assert::AssertLevel::critical, imsg1.size() != 0 );
		size_t inisz = imsg1.size();
		auto iptr = imsg1.convertToPointer();
		NODECPP_ASSERT( nodecpp::foundation::module_id, nodecpp::assert::AssertLevel::critical, imsg1.size() == 0 );
		nodecpp::platform::internal_msg::InternalMsg imsg4;
		imsg4.restoreFromPointer( iptr );
		NODECPP_ASSERT( nodecpp::foundation::module_id, nodecpp::assert::AssertLevel::critical, imsg4.size() == inisz, "{} vs. {}", imsg4.size(), inisz );

		uint64_t ctr2 = 0;
		auto it = imsg4.getReadIter();
		size_t available = it.availableSize();
		while ( available )
		{
			const uint64_t* ptr = reinterpret_cast<const uint64_t*>(it.read(available));
			available /= sizeof( uint64_t );
			for ( size_t i=0; i<available; ++i )
			{
				uint64_t val = ptr[i];
				NODECPP_ASSERT( nodecpp::foundation::module_id, nodecpp::assert::AssertLevel::critical, val == ctr2, "{} vs. {}", val, ctr2 );
				++ctr2;
			}
			available = it.availableSize();
		}
		NODECPP_ASSERT( nodecpp::foundation::module_id, nodecpp::assert::AssertLevel::critical, ctr1 == ctr2, "{:x} vs. {:x}", ctr1, ctr2 );
		nodecpp::log::default_log::info( nodecpp::log::ModuleID(nodecpp::foundation_module_id), "ctr1 = {:x}, ctr2 = {:x}", ctr1, ctr2 );

		imsg1 = std::move( imsg4 );

		NODECPP_ASSERT( nodecpp::foundation::module_id, nodecpp::assert::AssertLevel::critical, imsg4.size() == 0 );
		auto iptr4 = imsg4.convertToPointer();
		NODECPP_ASSERT( nodecpp::foundation::module_id, nodecpp::assert::AssertLevel::critical, iptr4 == nullptr );
		imsg4.restoreFromPointer( iptr4 );
		NODECPP_ASSERT( nodecpp::foundation::module_id, nodecpp::assert::AssertLevel::critical, imsg4.size() == 0 );
	}
}

/*#include <allocator_template.h>
struct LargeAndAligned
{
	alignas(32) uint8_t basemem[ 72 ];
	uintptr_t dummy;
};*/
int main(int argc, char *argv[])
{
	/*static_assert( std::alignment_of_v<LargeAndAligned> == 32 );
	printf( "sizeof(LargeAndAligned) = %zd\n", sizeof( LargeAndAligned ) );

	std::allocator<LargeAndAligned> a2;
	LargeAndAligned* p2 = a2.allocate(1024);
	printf( "p = 0x%zx\n", (size_t)p2 );
	a2.deallocate(p2, 1024);

	nodecpp::stdallocator<LargeAndAligned> stdalloc;
	LargeAndAligned* p = stdalloc.allocate(1024);
	printf( "p = 0x%zx\n", (size_t)p );
	stdalloc.deallocate(p, 1024);

	return 0;

	printf( "char: sizeof() = %zd, alignment = %zd\n", sizeof(char), alignof(char) );
	printf( "short: sizeof() = %zd, alignment = %zd\n", sizeof(short), alignof(short) );
	printf( "int: sizeof() = %zd, alignment = %zd\n", sizeof(int), alignof(int) );
	printf( "double: sizeof() = %zd, alignment = %zd\n", sizeof(double), alignof(double) );
	printf( "void*: sizeof() = %zd, alignment = %zd\n", sizeof(void*), alignof(void*) );
	char* chp[1024];
	for ( size_t i=0; i<1024; ++i )
	{
//		chp[i] = new char;
		chp[i] = (char*)(malloc(1));
		printf( "chp[%zd] = 0x%zx\n", i, (size_t)(chp[i]) );
		if ( (size_t)(chp[i]) &0xF )
			printf( "GOT LOW!!!\n" );
	}
	return 0;*/


	nodecpp::log::Log log;
	log.level = nodecpp::log::LogLevel::info;
	log.add( stdout );

	for ( size_t i=0; i<2000; ++i )
		log.warning( "whatever warning # {}", i );
	nodecpp::logging_impl::currentLog = &log;

	for ( size_t i=0; i<2000; ++i )
		nodecpp::log::default_log::warning( nodecpp::log::ModuleID(nodecpp::foundation_module_id), "whatever warning # {}", 2000+i );

	testVectorOfPages();
//	return 0;

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
		nodecpp::log::default_log::info( nodecpp::log::ModuleID(nodecpp::foundation_module_id),"error cought!");
	}
	try
	{
		fnWithAssertion2(0);
	}
	catch (...)
	{
		nodecpp::log::default_log::info( nodecpp::log::ModuleID(nodecpp::foundation_module_id),"error cought!");
	}


	const char* testMsg = "some long message";
	int fake = 17;
	nodecpp::log::default_log::info( nodecpp::log::ModuleID(nodecpp::foundation_module_id),"[1] Hi! msg = \'{}\', fake = {} <end>", testMsg, fake );

	testPtrStructsWithZombieProperty();

	nodecpp::log::default_log::log( nodecpp::log::ModuleID(nodecpp::foundation_module_id), nodecpp::log::LogLevel::fatal, "about to exit..." );

    return 0;
}
