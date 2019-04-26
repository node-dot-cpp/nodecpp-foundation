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

#ifndef TAGGED_PTR_IMPL_H
#define TAGGED_PTR_IMPL_H

#include "platform_base.h"
#include "foundation_module.h"
#include "nodecpp_assert.h"
#include "safe_memory_error.h"

namespace nodecpp::platform::ptrwithdatastructsdefs { 

///////  ptr_with_zombie_property

struct optimized_ptr_with_zombie_property_ {
private:
	static constexpr uintptr_t zombie_indicator = nodecpp_guaranteed_malloc_alignment;
	void* ptr = nullptr;
	// means to keep mainstream branch clean
	[[noreturn]] NODECPP_NOINLINE void throwNullptrOrZombieAccess() const;
	[[noreturn]] NODECPP_NOINLINE void throwZombieAccess() const;
public:
	optimized_ptr_with_zombie_property_() {}
	optimized_ptr_with_zombie_property_( const optimized_ptr_with_zombie_property_& other ) = delete;
	optimized_ptr_with_zombie_property_& operator =( const optimized_ptr_with_zombie_property_& other ) = delete;
	optimized_ptr_with_zombie_property_( optimized_ptr_with_zombie_property_&& other ) = delete;
	optimized_ptr_with_zombie_property_& operator =( optimized_ptr_with_zombie_property_&& other ) = delete;
	void copy_from( const optimized_ptr_with_zombie_property_& other ) {
		if ( NODECPP_LIKELY( ((uintptr_t)(ptr)) != zombie_indicator ) && NODECPP_LIKELY( ((uintptr_t)(other.ptr)) != zombie_indicator ) )
			ptr = other.ptr; 
		else
			throwZombieAccess();
	}
	void move_from( optimized_ptr_with_zombie_property_&& other ) {
		if ( NODECPP_LIKELY( ((uintptr_t)(other.ptr)) != zombie_indicator ) && NODECPP_LIKELY( ((uintptr_t)(other.ptr)) != zombie_indicator ) )
		{
			ptr = other.ptr; 
			other.ptr = nullptr;
		}
		else
			throwZombieAccess();
	}
	void swap( optimized_ptr_with_zombie_property_& other ) {
		if ( NODECPP_LIKELY( ((uintptr_t)(ptr)) != zombie_indicator ) && NODECPP_LIKELY( ((uintptr_t)(other.ptr)) != zombie_indicator ) )
		{
			void* tmp = ptr;
			ptr = other.ptr; 
			other.ptr = tmp;
		}
		else
			throwZombieAccess();
	}

	void init( void* ptr_ ) { ptr = ptr_; }
	void set_zombie() { ptr = (void*)zombie_indicator; }
	bool is_zombie() const { return ((uintptr_t)ptr) == zombie_indicator; }
	void* get_dereferencable_ptr() const { 
		if ( NODECPP_LIKELY( ((uintptr_t)ptr) > zombie_indicator ) )
			return ptr; 
		else
			throwNullptrOrZombieAccess();
	}
	void* get_ptr() const { 
		if ( NODECPP_LIKELY( ((uintptr_t)ptr) != zombie_indicator ) ) 
			return ptr; 
		else
			throwZombieAccess();
	}
};

struct generic_ptr_with_zombie_property_ {
private:
	void* ptr = nullptr;
	bool isZombie = false;
	// means to keep mainstream branch clean
	[[noreturn]] NODECPP_NOINLINE void throwNullptrOrZombieAccess() const;
	[[noreturn]] NODECPP_NOINLINE void throwZombieAccess() const;
public:
	generic_ptr_with_zombie_property_() {}
	generic_ptr_with_zombie_property_( const generic_ptr_with_zombie_property_& other ) = delete;
	generic_ptr_with_zombie_property_& operator =( const generic_ptr_with_zombie_property_& other ) = delete;
	generic_ptr_with_zombie_property_( generic_ptr_with_zombie_property_&& other ) = delete;
	generic_ptr_with_zombie_property_& operator =( generic_ptr_with_zombie_property_&& other ) = delete;
	void copy_from( const generic_ptr_with_zombie_property_& other ) {
		if ( !isZombie && !other.isZombie )
			ptr = other.ptr; 
		else
			throwZombieAccess();
	}
	void move_from( generic_ptr_with_zombie_property_&& other ) {
		if ( !isZombie && !other.isZombie )
		{
			ptr = other.ptr; 
			other.ptr = nullptr;
		}
		else
			throwZombieAccess();
	}
	void swap( generic_ptr_with_zombie_property_& other ) {
		if ( !isZombie && !other.isZombie )
		{
			void* tmp = ptr;
			ptr = other.ptr; 
			other.ptr = tmp;
		}
		else
			throwZombieAccess();
	}

	void init( void* ptr_ ) { ptr = ptr_; isZombie = false;}
	void set_zombie() { isZombie = true; }
	bool is_zombie() const { return isZombie; }
	void* get_dereferencable_ptr() const { 
		if ( NODECPP_LIKELY( !(isZombie || ptr == nullptr) ) )
			return ptr;
		else
			throwNullptrOrZombieAccess();
	}
	void* get_ptr() const { 
		if ( NODECPP_LIKELY( !isZombie ) ) 
			return ptr; 
		else
			throwZombieAccess();
	}
};

///////  allocated_ptr_with_flags

template< int nflags >
struct optimized_allocated_ptr_with_flags_ {
	static_assert(nflags > 0 && (1<<nflags) <= nodecpp_guaranteed_malloc_alignment);//don't need more than 2 ATM
private:
	static constexpr uintptr_t lowerDataMask_ = nodecpp_guaranteed_malloc_alignment - 1;
	uintptr_t ptr;
public:
	void init( void* ptr_ ) { 
		NODECPP_ASSERT( nodecpp::foundation::module_id, nodecpp::assert::AssertLevel::critical, ( (uintptr_t)ptr_ & lowerDataMask_ ) == 0 ); 
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
#ifdef NODECPP_X64
static_assert( sizeof(optimized_allocated_ptr_with_flags_<1>) == 8 );
static_assert( sizeof(optimized_allocated_ptr_with_flags_<2>) == 8 );
#endif

template< int nflags >
struct generic_allocated_ptr_with_flags_ {
	static_assert(nflags > 0 && (1<<nflags) <= nodecpp_guaranteed_malloc_alignment);//don't need more than 2 ATM
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


///////  allocated_ptr_with_mask_and_flags

template< int masksize, int nflags >
struct generic_allocated_ptr_with_mask_and_flags_ {
#ifdef NODECPP_X64
	static_assert(nflags <= 3);
	static constexpr uintptr_t ptrMask_ = 0xFFFFFFFFFFF8ULL;
#else
	static_assert(nflags <= 2);
	static constexpr uintptr_t ptrMask_ = 0xFFFFFFFC;
#endif
	static_assert(masksize <= 3);
private:
	void* ptr;
	uint8_t flags;
	uint8_t mask;
public:
	void init() { ptr = 0; flags = 0; mask = 0;}
	void init( void* ptr_ ) { 
		NODECPP_ASSERT( nodecpp::foundation::module_id, nodecpp::assert::AssertLevel::regular, ( (uintptr_t)ptr_ & (~ptrMask_) ) == 0 ); 
		ptr = ptr_; 
		flags = 0;
		mask = 0;
	}
	void set_ptr( void* ptr_ ) { 
		NODECPP_ASSERT( nodecpp::foundation::module_id, nodecpp::assert::AssertLevel::regular, ( (uintptr_t)ptr_ & (~ptrMask_) ) == 0 ); 
		ptr = ptr_; 
	}
	void* get_ptr() const { return ptr; }
	template<int pos>
	void set_flag() { static_assert( pos >= 0 && pos < nflags); flags |= ((uintptr_t)(1))<<pos; }
	template<int pos>
	void unset_flag() { static_assert( pos >= 0 && pos < nflags); flags &= ~(((uintptr_t)(1))<<pos); }
	template<int pos>
	bool has_flag() const { static_assert( pos >= 0 && pos < nflags); return (flags & (((uintptr_t)(1))<<pos)) != 0; }
	size_t get_mask() const { return mask; }
	void set_mask( size_t mask_ ) { NODECPP_ASSERT( nodecpp::foundation::module_id, nodecpp::assert::AssertLevel::regular, mask < (1<<masksize)); mask = (uint8_t)mask_; }
};

#ifdef NODECPP_X64
template< int masksize, int nflags >
struct optimized_allocated_ptr_with_mask_and_flags_64_ {
//	static constexpr uintptr_t zombie_indicator = nodecpp_guaranteed_malloc_alignment;
	static_assert((1<<nflags) <= nodecpp_guaranteed_malloc_alignment);
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
		NODECPP_ASSERT( nodecpp::foundation::module_id, nodecpp::assert::AssertLevel::critical, ( (uintptr_t)ptr_ & (~ptrMask_) ) == 0 );
		ptr = (uintptr_t)ptr_; 
	}
	void set_ptr( void* ptr_ ) { NODECPP_ASSERT( nodecpp::foundation::module_id, nodecpp::assert::AssertLevel::critical, ((uintptr_t)ptr_ & ~ptrMask_) == 0 ); ptr = (ptr & lowerDataMask_) | ((uintptr_t)ptr_ & ~lowerDataMask_); }
	void* get_ptr() const { return (void*)( ptr & ptrMask_ ); }
	template<int pos>
	void set_flag() { static_assert( pos >= 0 && pos < nflags); ptr |= ((uintptr_t)(1))<<upperDataOffset_; }
	template<int pos>
	void unset_flag() { static_assert( pos >= 0 && pos < nflags); ptr &= ~(((uintptr_t)(1))<<upperDataOffset_); }
	template<int pos>
	bool has_flag() const { static_assert( pos >= 0 && pos < nflags); return (ptr & (((uintptr_t)(1))<<upperDataOffset_)) != 0; }
	size_t get_mask() const { return ptr & lowerDataMask_; }
	void set_mask( size_t mask ) { NODECPP_ASSERT( nodecpp::foundation::module_id, nodecpp::assert::AssertLevel::critical, mask < (1<<masksize)); ptr = (ptr & ~lowerDataMask_) | mask; }
};
#else
template< int masksize, int nflags >
using optimized_allocated_ptr_with_mask_and_flags_64_ = generic_allocated_ptr_with_mask_and_flags_<masksize, nflags>; // TODO: consider writing optimized version for other platforms
#endif // NODECPP_X64


///////  allocated_ptr_and_ptr_and_data_and_flags

template< int dataminsize, int nflags >
struct generic_allocated_ptr_and_ptr_and_data_and_flags_ {
	// space-ineffective implenetation for testing purposes
	static_assert((1<<nflags) <= nodecpp_guaranteed_malloc_alignment); // current needs
	static_assert(dataminsize <= 32);
	static_assert(dataminsize >= 0);
private:
	void* ptr;
	void* allocptr;
	size_t data;
	uint16_t flags;
	bool isZombie;

	// means to keep mainstream branch clean
	[[noreturn]] NODECPP_NOINLINE void throwNullptrOrZombieAccess() const;
	[[noreturn]] NODECPP_NOINLINE void throwZombieAccess() const;

public:
	static constexpr size_t max_data = dataminsize < 32 ? (size_t)(((uint64_t)1 << dataminsize ) - 1) : 0xFFFFFFFF;

	generic_allocated_ptr_and_ptr_and_data_and_flags_() {init();}
	generic_allocated_ptr_and_ptr_and_data_and_flags_( const generic_allocated_ptr_and_ptr_and_data_and_flags_& other ) = delete;
	generic_allocated_ptr_and_ptr_and_data_and_flags_& operator =( const generic_allocated_ptr_and_ptr_and_data_and_flags_& other ) = delete;
	generic_allocated_ptr_and_ptr_and_data_and_flags_( generic_allocated_ptr_and_ptr_and_data_and_flags_&& other ) = delete;
	generic_allocated_ptr_and_ptr_and_data_and_flags_& operator =( generic_allocated_ptr_and_ptr_and_data_and_flags_&& other ) = delete;
	void copy_from( const generic_allocated_ptr_and_ptr_and_data_and_flags_& other ) {
		if ( !isZombie && !other.isZombie )
		{
			ptr = other.ptr;
			allocptr = other.allocptr;
			data = other.data;
			flags = other.flags;
		}
		else
			throwZombieAccess();
	}
	void move_from( generic_allocated_ptr_and_ptr_and_data_and_flags_&& other ) {
		if ( !isZombie && !other.isZombie )
		{
			ptr = other.ptr;
			allocptr = other.allocptr;
			data = other.data;
			flags = other.flags;
			other.init();
		}
		else
			throwZombieAccess();
	}
	void swap( generic_allocated_ptr_and_ptr_and_data_and_flags_& other ) {
		if ( !isZombie && !other.isZombie )
		{
			void* tmp = ptr;
			ptr = other.ptr; 
			other.ptr = tmp;
			tmp = allocptr;
			allocptr = other.allocptr; 
			other.allocptr = tmp;
			auto tmp_data = data;
			data = other.data;
			other.data = tmp_data;
			auto tmp_flags = flags;
			flags = other.flags;
			other.flags = tmp_flags;
		}
		else
			throwZombieAccess();
	}

	void init() { ptr = 0; allocptr = 0; data = 0; flags = 0; isZombie = false;}
	void init( size_t data_ ) { init(); data = data_; isZombie = false; }
	void init( void* ptr_, void* allocptr_, size_t data_ ) {
        NODECPP_ASSERT( nodecpp::foundation::module_id, nodecpp::assert::AssertLevel::critical, ((uintptr_t)allocptr_ & (nodecpp_guaranteed_malloc_alignment-1)) == 0 ); 
		NODECPP_ASSERT( nodecpp::foundation::module_id, nodecpp::assert::AssertLevel::critical, data_ <= max_data ); 
		ptr = ptr_; 
		allocptr = allocptr_;
		data = data_;
		flags = 0;
		isZombie = false;
	}

	void set_ptr( void* ptr_ ) { ptr = ptr_; }
	void* get_ptr() const { 
		if ( NODECPP_LIKELY( !isZombie ) )
			return ptr; 
		else
			throwZombieAccess(); 
	}
	void* get_dereferencable_ptr() const {
		if ( NODECPP_LIKELY( !(isZombie || ptr == nullptr) ) )
			return ptr; 
		else
			throwNullptrOrZombieAccess();
	}
	void set_allocated_ptr( void* allocptr_ ) { 
        NODECPP_ASSERT( nodecpp::foundation::module_id, nodecpp::assert::AssertLevel::critical, ((uintptr_t)allocptr_ & (nodecpp_guaranteed_malloc_alignment-1)) == 0 ); 
		allocptr = allocptr_; 
	}
	void* get_allocated_ptr() const { 
		if ( NODECPP_LIKELY( !isZombie ) ) 
			return allocptr;
		else
			throwZombieAccess(); 
	}
	void* get_dereferencable_allocated_ptr() const { 
		if ( NODECPP_LIKELY( !(isZombie || allocptr == nullptr) ) )
			return allocptr;
		else
			throwNullptrOrZombieAccess();
	}

	void set_zombie() { isZombie = true; }
	bool is_zombie() const { return isZombie; }

	template<int pos>
	void set_flag() { static_assert( pos < nflags); flags |= ((uintptr_t)(1))<<pos; }
	template<int pos>
	void unset_flag() { static_assert( pos < nflags); flags &= ~(((uintptr_t)(1))<<pos); }
	template<int pos>
	bool has_flag() const { static_assert( pos < nflags); return (flags & (((uintptr_t)(1))<<pos)) != 0; }

	size_t get_data() const { return data; }
	void set_data( size_t data_ ) { 
		NODECPP_ASSERT( nodecpp::foundation::module_id, nodecpp::assert::AssertLevel::critical, data_ <= max_data ); 
		data = data_; 
	}
};

template< int dataminsize, int nflags >
void generic_allocated_ptr_and_ptr_and_data_and_flags_< dataminsize, nflags >::throwNullptrOrZombieAccess() const {
	if ( isZombie )
		throw nodecpp::error::lately_detected_zombie_pointer_access; 
	else
		throw nodecpp::error::zero_pointer_access; 
}

template< int dataminsize, int nflags >
void generic_allocated_ptr_and_ptr_and_data_and_flags_< dataminsize, nflags >::throwZombieAccess() const {
	throw nodecpp::error::zero_pointer_access; 
}

#ifdef NODECPP_X64
template< int dataminsize, int nflags >
struct optimized_allocated_ptr_and_ptr_and_data_and_flags_64_ {
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
	static constexpr uintptr_t zombie_indicator = nodecpp_guaranteed_malloc_alignment;
	static_assert ( nodecpp_guaranteed_malloc_alignment >= 8 ); // this assumption is used in below definitions
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

	// means to keep mainstream branch clean
	[[noreturn]] NODECPP_NOINLINE void throwNullptrOrZombieAccess() const;
	[[noreturn]] NODECPP_NOINLINE void throwZombieAccess() const;

public:
	static constexpr size_t max_data = ((size_t)1 << dataminsize ) - 1;

	optimized_allocated_ptr_and_ptr_and_data_and_flags_64_() {init();}
	optimized_allocated_ptr_and_ptr_and_data_and_flags_64_( const optimized_allocated_ptr_and_ptr_and_data_and_flags_64_& other ) = delete;
	optimized_allocated_ptr_and_ptr_and_data_and_flags_64_& operator =( const optimized_allocated_ptr_and_ptr_and_data_and_flags_64_& other ) = delete;
	optimized_allocated_ptr_and_ptr_and_data_and_flags_64_( optimized_allocated_ptr_and_ptr_and_data_and_flags_64_&& other ) = delete;
	optimized_allocated_ptr_and_ptr_and_data_and_flags_64_& operator =( optimized_allocated_ptr_and_ptr_and_data_and_flags_64_&& other ) = delete;
	void copy_from( const optimized_allocated_ptr_and_ptr_and_data_and_flags_64_& other ) {
		if ( !NODECPP_LIKELY( (ptr & ptrMask_) == zombie_indicator ) && !NODECPP_LIKELY( (other.ptr & ptrMask_) == zombie_indicator ) )
		{
			ptr = other.ptr;
			allocptr = other.allocptr;
		}
		else
			throwZombieAccess();
	}
	void move_from( optimized_allocated_ptr_and_ptr_and_data_and_flags_64_&& other ) {
		if ( !NODECPP_LIKELY( (ptr & ptrMask_) == zombie_indicator ) && !NODECPP_LIKELY( (other.ptr & ptrMask_) == zombie_indicator ) )
		{
			ptr = other.ptr;
			allocptr = other.allocptr;
			other.init();
		}
		else
			throwZombieAccess();
	}
	void swap( optimized_allocated_ptr_and_ptr_and_data_and_flags_64_& other ) {
		if ( !NODECPP_LIKELY( (ptr & ptrMask_) == zombie_indicator ) && !NODECPP_LIKELY( (other.ptr & ptrMask_) == zombie_indicator ) )
		{
			void* tmp = ptr;
			ptr = other.ptr; 
			other.ptr = tmp;
			tmp = allocptr;
			allocptr = other.allocptr; 
			other.allocptr = tmp;
		}
		else
			throwZombieAccess();
	}

	void init() { ptr = 0; allocptr = 0;}
	void init( size_t data ) { init(); set_data( data ); }
	void init( void* ptr_, void* allocptr_, size_t data ) { 
        NODECPP_ASSERT( nodecpp::foundation::module_id, nodecpp::assert::AssertLevel::critical, ((uintptr_t)ptr_ & ~ptrMask_) == 0 ); 
        NODECPP_ASSERT( nodecpp::foundation::module_id, nodecpp::assert::AssertLevel::critical, ((uintptr_t)allocptr_ & ~allocptrMask_) == 0 ); 
		NODECPP_ASSERT( nodecpp::foundation::module_id, nodecpp::assert::AssertLevel::critical, data <= max_data ); 
		ptr = (uintptr_t)ptr_ & ptrMask_; 
		allocptr = (uintptr_t)allocptr_ & allocptrMask_; 
		set_data( data ); 
	}

	void set_ptr( void* ptr_ ) { 
		NODECPP_ASSERT( nodecpp::foundation::module_id, nodecpp::assert::AssertLevel::critical, ((uintptr_t)ptr_ & ~ptrMask_) == 0 ); 
		ptr = (ptr & upperDataMaskInPointer_) | ((uintptr_t)ptr_ & ptrMask_); 
	}
	void* get_ptr() const { 
		if ( NODECPP_LIKELY( (ptr & ptrMask_) != zombie_indicator ) )
			return (void*)( ptr & ptrMask_ ); 
		else
			throwZombieAccess();
	}
	void* get_dereferencable_ptr() const { 
		if ( NODECPP_LIKELY( (ptr & ptrMask_) > zombie_indicator ) )
			return (void*)(ptr & ptrMask_);
		else
			throwNullptrOrZombieAccess();
	}
	void set_allocated_ptr( void* ptr_ ) { 
		NODECPP_ASSERT( nodecpp::foundation::module_id, nodecpp::assert::AssertLevel::critical, ((uintptr_t)ptr_ & ~allocptrMask_) == 0 ); 
		allocptr = (allocptr & ~allocptrMask_) | ((uintptr_t)ptr_ & allocptrMask_); 
	}
	void* get_allocated_ptr() const { 
		if ( NODECPP_LIKELY( !is_zombie() ) ) 
			return (void*)( allocptr & allocptrMask_ ); 
		else
			throwZombieAccess(); 
	}
	void* get_dereferencable_allocated_ptr() const { 
		if ( NODECPP_LIKELY( !(is_zombie() || (allocptr & allocptrMask_) == 0) ) )
			return (void*)( allocptr & allocptrMask_ ); 
		else
			throwNullptrOrZombieAccess();
	}

	void set_zombie() { ptr = zombie_indicator; }
	bool is_zombie() const { return (ptr & ptrMask_) == zombie_indicator; }

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
		NODECPP_ASSERT( nodecpp::foundation::module_id, nodecpp::assert::AssertLevel::critical, data <= max_data ); 
		ptr &= ptrMask_; 
		ptr |= (data & ptrPartMaskInData_) << upperDataBitOffsetInPointers_;
		allocptr &= allocptrMask_ | allocPtrFlagsMask_;
		allocptr |= ( (data & allocptrUpperPartMaskInData_) >> allocptrUpperPartOffsetInData_ ) << upperDataBitOffsetInPointers_;
		allocptr |= ( (data & allocptrLowerPartMaskInData_) >> allocptrLowerPartOffsetInData_ ) << nflags;
	}
};

template< int dataminsize, int nflags >
void optimized_allocated_ptr_and_ptr_and_data_and_flags_64_< dataminsize, nflags >::throwZombieAccess() const {
	throw nodecpp::error::zero_pointer_access; 
}

template< int dataminsize, int nflags >
void optimized_allocated_ptr_and_ptr_and_data_and_flags_64_< dataminsize, nflags >::throwNullptrOrZombieAccess() const {
	if ( is_zombie() )
		throw nodecpp::error::zombie_pointer_access; 
	else
		throw nodecpp::error::zero_pointer_access;
}

#else
template< int dataminsize, int nflags >
using allocated_ptr_and_ptr_and_data_and_flags = generic_allocated_ptr_and_ptr_and_data_and_flags_<dataminsize, nflags>; // TODO: consider writing optimized version for other platforms
#endif // NODECPP_X64

} // nodecpp::platform::ptrwithdatastructsdefs


#endif // TAGGED_PTR_IMPL_H