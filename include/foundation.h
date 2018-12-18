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

namespace nodecpp::platform { 

#ifndef assert // TODO: replace by our own means ASAP
#define assert(x) 
#endif//X64

#ifdef NODECPP_X64
#define nodecpp_memory_size_bits 47
#else
#define nodecpp_memory_size_bits 31
#endif
	
#ifdef NODECPP_X64
template< int nflags >
struct allocated_ptr_with_flags {
	static_assert(nflags > 0 && nflags <= 2);//don't need more than 2 ATM
private:
	static constexpr uintptr_t lowerDataMask_ = 0x7ULL;
	uintptr_t ptr;
public:
	void init( void* ptr_ ) { 
		assert( ( (uintptr_t)ptr_ & lowerDataMask_ ) == 0 ); 
		ptr = (uintptr_t)ptr_ & ~lowerDataMask_; 
	}
	void set_ptr( void* ptr_ ) { ptr = (ptr & lowerDataMask_) | ((uintptr_t)ptr_ & ~lowerDataMask_); }
	void* get_ptr() const { return (void*)( ptr & ~lowerDataMask_ ); }
	template<int pos>
	void set_flag() { static_assert( pos >= 0 && pos < nflags); ptr |= ((uintptr_t)(1))<<pos; }
	template<int pos>
	void unset_flag() { static_assert( pos >= 0 && pos < nflags); ptr &= ~(((uintptr_t)(1))<<pos); }
	template<int pos>
	bool has_flag() const { static_assert( pos >= 0 && pos < nflags); return (ptr & (((uintptr_t)(1))<<pos)) != 0; }
};
static_assert( sizeof(allocated_ptr_with_flags<1>) == 8 );
static_assert( sizeof(allocated_ptr_with_flags<2>) == 8 );
#else
template< int nflags >
struct allocated_ptr_with_flags {
	static_assert(nflags > 0 && nflags <= 2);//don't need more than 2 ATM
private:
	void* ptr;
	uint8_t flags;
public:
	void init( void* ptr_ ) { ptr = ptr_; flags = 0;}
	void set_ptr( void* ptr_ ) { ptr = ptr_; }
	void* get_ptr() const { return ptr; }
	template<int pos>
	void set_flag() { static_assert( pos >= 0 && pos < nflags ); flags |= (uint8_t(1))<<pos; }
	template<int pos>
	void unset_flag() { static_assert( pos >= 0 && pos < nflags ); flags &= ~(((uint8_t)(1))<<pos); }
	template<int pos>
	bool has_flag() const { static_assert( pos >= 0 && pos < nflags ); return (flags & (((uint8_t)(1))<<pos)) != 0; }
};
#endif//X64

template< int masksize, int nflags >
struct reference_impl__allocated_ptr_with_mask_and_flags {
	static_assert(nflags <= 2);
	static_assert(masksize <= 3);
private:
	void* ptr;
	size_t flags;
	size_t mask;
	static constexpr uintptr_t ptrMask_ = 0xFFFFFFFFFFF8ULL;
public:
	void init() { ptr = 0; flags = 0;}
	void init( void* ptr_ ) { 
		assert( ( (uintptr_t)ptr_ & (~ptrMask_) ) == 0 ); 
		ptr = ptr_; 
		flags = 0;
	}
	void set_ptr( void* ptr_ ) { assert( ( (uintptr_t)ptr_ & (~ptrMask_) ) == 0 ); ptr = ptr_; }
	void* get_ptr() const { return ptr; }
	template<int pos>
	void set_flag() { static_assert( pos >= 0 && pos < nflags); flags |= ((uintptr_t)(1))<<pos; }
	template<int pos>
	void unset_flag() { static_assert( pos >= 0 && pos < nflags); flags &= ~(((uintptr_t)(1))<<pos); }
	template<int pos>
	bool has_flag() const { static_assert( pos >= 0 && pos < nflags); return (flags & (((uintptr_t)(1))<<pos)) != 0; }
	size_t get_mask() { return mask; }
	void set_mask( size_t mask_ ) { assert( mask < (1<<masksize)); mask = mask_; }
};
#ifdef NODECPP_X64
template< int masksize, int nflags >
struct allocated_ptr_with_mask_and_flags {
	static_assert(nflags <= 2);
	static_assert(masksize <= 3);
private:
	uintptr_t ptr;
	static constexpr uintptr_t ptrMask_ = 0xFFFFFFFFFFF8ULL;
	static constexpr uintptr_t upperDataMask_ = ~(0xFFFFFFFFFFFFULL);
	static constexpr uintptr_t upperDataOffset_ = 48;
	static constexpr uintptr_t lowerDataMask_ = 0x7ULL;
	static constexpr size_t upperDataSize_ = 16;
	static constexpr size_t lowerDataSize_ = 3;
	static_assert ( (ptrMask_ & upperDataMask_) == 0 );
	static_assert ( (ptrMask_ >> upperDataOffset_) == 0 );
	static_assert ( (ptrMask_ & lowerDataMask_) == 0 );
	static_assert ( (upperDataMask_ & lowerDataMask_) == 0 );
	static_assert ( (ptrMask_ | upperDataMask_ | lowerDataMask_) == 0xFFFFFFFFFFFFFFFFULL );
public:
	void init() { ptr = 0; }
	void init( void* ptr_ ) { 
		assert( ( (uintptr_t)ptr_ & (~ptrMask_) ) == 0 );
		ptr = (uintptr_t)ptr_; 
	}
	void set_ptr( void* ptr_ ) { assert( ((uintptr_t)ptr_ & ~ptrMask_) == 0 ); ptr = (ptr & lowerDataMask_) | ((uintptr_t)ptr_ & ~lowerDataMask_); }
	void* get_ptr() const { return (void*)( ptr & ptrMask_ ); }
	template<int pos>
	void set_flag() { static_assert( pos >= 0 && pos < nflags); ptr |= ((uintptr_t)(1))<<upperDataOffset_; }
	template<int pos>
	void unset_flag() { static_assert( pos >= 0 && pos < nflags); ptr &= ~(((uintptr_t)(1))<<upperDataOffset_); }
	template<int pos>
	bool has_flag() const { static_assert( pos >= 0 && pos < nflags); return (ptr & (((uintptr_t)(1))<<upperDataOffset_)) != 0; }
	size_t get_mask() { return ptr & lowerDataMask_; }
	void set_mask( size_t mask ) { assert( mask < (1<<masksize)); ptr = (ptr & ~lowerDataMask_) | mask; }
};
#else
template< int masksize, int nflags >
struct allocated_ptr_with_mask_and_flags {
	static_assert(nflags <= 3);
private:
	void* ptr;
	uint8_t flags;
	uint8_t mask;
public:
	void init() { ptr = 0; flags = 0; mask = 0;}
	void init( void* ptr_ ) { ptr = ptr_; flags = 0; mask = 0;}
	void set_ptr( void* ptr_ ) { ptr = ptr_; }
	void* get_ptr() const { return ptr; }
	template<int pos>
	void set_flag() { static_assert( pos < nflags ); flags |= (uint8_t(1))<<pos; }
	template<int pos>
	void unset_flag() { static_assert( pos < nflags ); flags &= ~(((uint8_t)(1))<<pos); }
	template<int pos>
	bool has_flag() const { static_assert( pos < nflags ); return (flags & (((uint8_t)(1))<<pos)) != 0; }

	size_t get_mask() { return mask & ((1<<masksize)-1); }
	void set_mask( size_t mask_ ) { assert( mask < (1<<masksize)); mask = (uint8_t)mask_; }
};
#endif//X64

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

#ifdef NODECPP_X64
template< int dataminsize, int nflags >
struct allocated_ptr_and_ptr_and_data_and_flags {
	// implementation notes:
	// for allocptr low (due to allocation alignment) and upper bits are available
	// for ptr, only upper bits are available
	// flags are stored as lowest position of low bits of allocptr following, if possible, by bits of data
	// data consists of upper bits of ptr, remaining low bits of allocated ptr, and upper bits of allocated ptr, in this order from lower to higher
	static_assert(nflags <= 1); // current needs
	static_assert(dataminsize <= 32);
private:
	uintptr_t ptr;
	uintptr_t allocptr;
	static constexpr uintptr_t allocptrMask_ = 0xFFFFFFFFFFF8ULL;
	static constexpr uintptr_t ptrMask_ = 0xFFFFFFFFFFFFULL;
	static constexpr uintptr_t upperDataMaskInPointer_ = ~(0xFFFFFFFFFFFFULL);
	static constexpr uintptr_t lowerDataMaskInPointer_ = 0x7ULL;
	static constexpr uintptr_t allocPtrFlagsMask_ = (1 << nflags) - 1;
	static constexpr size_t upperDataSize_ = 16;
	static constexpr size_t lowerDataSize_ = 3;
	static constexpr uintptr_t ptrPartMaskInData_ = 0xFFFFULL;
	static constexpr uintptr_t allocptrLowerPartMaskInData_ = ((1ULL<<(lowerDataSize_ - nflags))-1) << upperDataSize_;
	static constexpr uintptr_t allocptrUpperPartMaskInData_ = 0xFFFFULL << (upperDataSize_ + lowerDataSize_ - nflags);
	static constexpr uintptr_t allocptrLowerPartOffsetInData_ = upperDataSize_;
	static constexpr uintptr_t allocptrUpperPartOffsetInData_ = upperDataSize_ + nflags;
	static constexpr size_t upperDataBitOffsetInPointers_ = 48;
	static_assert ( (allocptrMask_ & upperDataMaskInPointer_) == 0 );
	static_assert ( (allocptrMask_ & lowerDataMaskInPointer_) == 0 );
	static_assert ( (upperDataMaskInPointer_ & lowerDataMaskInPointer_) == 0 );
	static_assert ( (allocptrMask_ | upperDataMaskInPointer_ | lowerDataMaskInPointer_) == 0xFFFFFFFFFFFFFFFFULL );
	static_assert ( (ptrMask_ | upperDataMaskInPointer_) == 0xFFFFFFFFFFFFFFFFULL );
	static_assert ( (ptrMask_ & upperDataMaskInPointer_) == 0x0ULL );

public:
	static constexpr size_t max_data = ((size_t)1 << dataminsize ) - 1;

	void init() { ptr = 0; allocptr = 0;}
	void init( size_t data ) { init(); set_data( data ); }
	void init( void* ptr_, void* allocptr_, size_t data ) { 
		assert( ((uintptr_t)ptr_ & ~ptrMask_) == 0 ); 
		assert( ((uintptr_t)allocptr_ & ~allocptrMask_) == 0 ); 
		ptr = (uintptr_t)ptr_ & ptrMask_; 
		allocptr = (uintptr_t)allocptr_ & allocptrMask_; 
		set_data( data ); 
	}

	void set_ptr( void* ptr_ ) { assert( ((uintptr_t)ptr_ & ~ptrMask_) == 0 ); ptr = (ptr & upperDataMaskInPointer_) | ((uintptr_t)ptr_ & ptrMask_); }
	void* get_ptr() const { return (void*)( ptr & ptrMask_ ); }
	void set_allocated_ptr( void* ptr_ ) { assert( ((uintptr_t)ptr_ & ~allocptrMask_) == 0 ); allocptr = (allocptr & ~allocptrMask_) | ((uintptr_t)ptr_ & allocptrMask_); }
	void* get_allocated_ptr() const { return (void*)( allocptr & allocptrMask_ ); }

	template<int pos>
	void set_flag() { static_assert( pos < nflags); allocptr |= ((uintptr_t)(1))<<pos; }
	template<int pos>
	void unset_flag() { static_assert( pos < nflags); allocptr &= ~(((uintptr_t)(1))<<pos); }
	template<int pos>
	bool has_flag() const { static_assert( pos < nflags); return (allocptr & (((uintptr_t)(1))<<pos)) != 0; }

	size_t get_data() const { 
		constexpr size_t shiftSizeForAllocPtr = 64 - upperDataSize_ - upperDataSize_ - nflags;
		constexpr size_t shiftSizeForPtr = 64 - upperDataSize_;
		size_t ret = ( allocptr & upperDataMaskInPointer_ ) >> shiftSizeForAllocPtr;
		ret |= ( ptr & upperDataMaskInPointer_ ) >> shiftSizeForPtr;
		constexpr size_t remainingInAllocPtr = lowerDataSize_ - nflags;
		if constexpr (remainingInAllocPtr > 0)
		{
			constexpr size_t remainingDataMaskInAllocPtr = ((1 << remainingInAllocPtr ) - 1) << nflags;
			ret |= (allocptr & remainingDataMaskInAllocPtr) << ( upperDataSize_ - nflags );
			return ret;
		}
		else
		{
			static_assert( shiftSizeForAllocPtr == 64 - upperDataSize_ - upperDataSize_ );
			return ret;
		}
	}
	void set_data( size_t data ) { 
		assert( data <= max_data ); 
		ptr &= ptrMask_; 
		ptr |= (data & ptrPartMaskInData_) << upperDataBitOffsetInPointers_;
		allocptr &= allocptrMask_ | allocPtrFlagsMask_;
		allocptr |= ( (data & allocptrUpperPartMaskInData_) >> allocptrUpperPartOffsetInData_ ) << upperDataBitOffsetInPointers_;
		allocptr |= ( (data & allocptrLowerPartMaskInData_) >> allocptrLowerPartOffsetInData_ ) << nflags;
	}
};
#else
template< int dataminsize, int nflags >
struct allocated_ptr_and_ptr_and_data_and_flags : public reference_impl__allocated_ptr_and_ptr_and_data_and_flags<dataminsize, nflags> {};
#endif//X64

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
