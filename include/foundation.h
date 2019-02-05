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

#ifndef NODECPP_FOUNDATION_H
#define NODECPP_FOUNDATION_H 

#include "platform_base.h"
#include "log.h"
#include "ptr_with_data_structs_impl.h"

namespace nodecpp::foundation
{
	constexpr uint64_t module_id = 0;
}

namespace nodecpp::platform { 

#ifndef assert // TODO: replace by our own means ASAP
#define assert(x) 
#endif//X64

#ifdef NODECPP_X64
#define nodecpp_memory_size_bits 47
#else
#define nodecpp_memory_size_bits 31
#endif

#ifdef NODECPP_USE_GENERIC_STRUCTS // unconditionally
#define NODECPP_DECLARE_PTR_STRUCTS_AS_GENERIC
#else // depending on a platform
#ifdef NODECPP_X64
#define NODECPP_DECLARE_PTR_STRUCTS_AS_OPTIMIZED
#else
#define NODECPP_DECLARE_PTR_STRUCTS_AS_GENERIC
#endif NODECPP_X64
#endif // NODECPP_USE_GENERIC_STRUCTS

#if defined NODECPP_DECLARE_PTR_STRUCTS_AS_OPTIMIZED
template< int nflags >
using allocated_ptr_with_flags = ::nodecpp::platform::ptrwithdatastructsdefs::optimized_struct_allocated_ptr_with_flags_<nflags>;
#elif defined NODECPP_DECLARE_PTR_STRUCTS_AS_GENERIC
template< int nflags >
using allocated_ptr_with_flags = ::nodecpp::platform::ptrwithdatastructsdefs::generic_struct_allocated_ptr_with_flags_<nflags>;
#else
#error Unsupported configuration
#endif // NODECPP_DECLARE_PTR_STRUCTS_AS_OPTIMIZED vs. NODECPP_DECLARE_PTR_STRUCTS_AS_GENERIC

#if defined NODECPP_DECLARE_PTR_STRUCTS_AS_OPTIMIZED
template< int masksize, int nflags >
using allocated_ptr_with_mask_and_flags = ::nodecpp::platform::ptrwithdatastructsdefs::optimized_struct_allocated_ptr_with_mask_and_flags_64_<masksize, nflags>;
#elif defined NODECPP_DECLARE_PTR_STRUCTS_AS_GENERIC
template< int masksize, int nflags >
using allocated_ptr_with_mask_and_flags = ::nodecpp::platform::ptrwithdatastructsdefs::generic_struct_allocated_ptr_with_mask_and_flags_<masksize, nflags>;
#else
#error Unsupported configuration
#endif // NODECPP_DECLARE_PTR_STRUCTS_AS_OPTIMIZED vs. NODECPP_DECLARE_PTR_STRUCTS_AS_GENERIC


template< int dataminsize, int nflags >
struct reference_impl__allocated_ptr_and_ptr_and_data_and_flags {
	// space-ineffective implenetation for testing purposes
	static_assert(nflags <= 3); // current needs
	static_assert(dataminsize <= 32);
private:
	void* ptr;
	void* allocptr;
	size_t data;
	size_t flags;

public:
	static constexpr size_t max_data = ((size_t)1 << dataminsize ) - 1;

	void init() { ptr = 0; allocptr = 0; data = 0; flags = 0; }
	void init( size_t data_ ) { init(); data = data_; }
	void init( void* ptr_, void* allocptr_, size_t data_ ) {
		ptr = ptr_; 
		allocptr = allocptr_;
		data = data_;
		flags = 0;
	}

	void set_ptr( void* ptr_ ) { ptr = ptr_; }
	void* get_ptr() const { return ptr; }
	void set_allocated_ptr( void* ptr_ ) { allocptr = ptr_; }
	void* get_allocated_ptr() const { return allocptr; }

	template<int pos>
	void set_flag() { static_assert( pos < nflags); flags |= ((uintptr_t)(1))<<pos; }
	template<int pos>
	void unset_flag() { static_assert( pos < nflags); flags &= ~(((uintptr_t)(1))<<pos); }
	template<int pos>
	bool has_flag() const { static_assert( pos < nflags); return (flags & (((uintptr_t)(1))<<pos)) != 0; }

	size_t get_data() const { return data; }
	void set_data( size_t data_ ) { 
		assert( data_ <= max_data ); 
		data = data_; 
	}
};

#ifdef NODECPP_DECLARE_PTR_STRUCTS_AS_OPTIMIZED
template< int dataminsize, int nflags >
using allocated_ptr_and_ptr_and_data_and_flags = ::nodecpp::platform::ptrwithdatastructsdefs::optimized_struct_allocated_ptr_and_ptr_and_data_and_flags_64_<dataminsize, nflags>;
#elif defined NODECPP_DECLARE_PTR_STRUCTS_AS_GENERIC
template< int masksize, int nflags >
using allocated_ptr_with_mask_and_flags = ::nodecpp::platform::ptrwithdatastructsdefs::generic_struct_allocated_ptr_with_mask_and_flags_<masksize, nflags>;
#else
#error Unsupported configuration
#endif // NODECPP_DECLARE_PTR_STRUCTS_AS_OPTIMIZED

}//nodecpp:platform

#ifndef assert // TODO: replace by our own means ASAP
#define assert(x) 
#endif

#if defined _MSC_VER
class SE_Exception  
{  
private:  
    unsigned int nSE;  
public:  
    SE_Exception() {}  
    SE_Exception( unsigned int n ) : nSE( n ) {}  
    ~SE_Exception() {}  
    unsigned int getSeNumber() { return nSE; }  
};  

#elif defined __GNUC__

#else

#endif

#endif // NODECPP_FOUNDATION_H
