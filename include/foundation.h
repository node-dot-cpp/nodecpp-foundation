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
*     * Neither the name of the <organization> nor the
*       names of its contributors may be used to endorse or promote products
*       derived from this software without specific prior written permission.
*
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
* ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
* WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
* DISCLAIMED. IN NO EVENT SHALL <COPYRIGHT HOLDER> BE LIABLE FOR ANY
* DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
* (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
* LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
* ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
* (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
* SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
* -------------------------------------------------------------------------------*/

#ifndef NODECPP_FOUNDATION_H
#define NODECPP_FOUNDATION_H 

#if defined _MSC_VER
#define ALIGN(n)      __declspec(align(n))
#define NOINLINE      __declspec(noinline)
#define FORCE_INLINE	__forceinline
#elif defined __GNUC__
#define ALIGN(n)      __attribute__ ((aligned(n))) 
#define NOINLINE      __attribute__ ((noinline))
#define	FORCE_INLINE inline __attribute__((always_inline))
#else
#define	FORCE_INLINE inline
#define NOINLINE
//#define ALIGN(n)
#warning ALIGN, FORCE_INLINE and NOINLINE may not be properly defined
#endif

FORCE_INLINE
void* readVMT(void* p) { return *((void**)p); }

FORCE_INLINE
void restoreVMT(void* p, void* vpt) { *((void**)p) = vpt; }

FORCE_INLINE
std::pair<size_t, size_t> getVMPPos(void* p) { return std::make_pair( 0, sizeof(void*) ); }


FORCE_INLINE
bool isGuaranteedOnStack( void* ptr )
{
	int a;
	constexpr uintptr_t upperBitsMask = ~( CONTROL_BLOCK_SIZE - 1 );
//	printf( "   ---> isGuaranteedOnStack(%zd), &a = %zd (%s)\n", ((uintptr_t)(ptr)), ((uintptr_t)(&a)), ( ( ((uintptr_t)(ptr)) ^ ((uintptr_t)(&a)) ) & upperBitsMask ) == 0 ? "YES" : "NO" );
	return ( ( ((uintptr_t)(ptr)) ^ ((uintptr_t)(&a)) ) & upperBitsMask ) == 0;
}


struct Ptr2PtrWishFlags {
private:
	uintptr_t ptr;
public:
	void init( void* ptr_ ) { ptr = (uintptr_t)ptr_ & ~((uintptr_t)7); }
	void resetPtr( void* ptr_ ) { ptr = (ptr & 7) | ((uintptr_t)ptr_ & ~((uintptr_t)7)); }
	void* getPtr() const { return (void*)( ptr & ~((uintptr_t)7) ); }
	void setFlag(size_t pos) { assert( pos < 3); ptr |= ((uintptr_t)(1))<<pos; }
	void unsetFlag(size_t pos) { assert( pos < 3); ptr &= ~(((uintptr_t)(1))<<pos); }
	bool isFlag(size_t pos) const { assert( pos < 3); return (ptr & (((uintptr_t)(1))<<pos))>>pos; }
};
static_assert( sizeof(Ptr2PtrWishFlags) == 8 );

struct Ptr2PtrWishDataBase {
//private:
	uintptr_t ptr;
	static constexpr uintptr_t ptrMask_ = 0xFFFFFFFFFFF8ULL;
	static constexpr uintptr_t upperDataMask_ = ~(0xFFFFFFFFFFFFULL);
	static constexpr uintptr_t lowerDataMask_ = 0x7ULL;
	static constexpr uintptr_t upperDataMaskInData_ = 0x7FFF8ULL;
	static constexpr size_t upperDataSize_ = 16;
	static constexpr size_t lowerDataSize_ = 3;
	static constexpr size_t upperDataShift_ = 45;
	static constexpr size_t invalidData = ( lowerDataMask_ | upperDataMaskInData_ );
	static_assert ( (upperDataMaskInData_ << upperDataShift_ ) == upperDataMask_ );
	static_assert ( (ptrMask_ & upperDataMask_) == 0 );
	static_assert ( (ptrMask_ >> (upperDataShift_ + lowerDataSize_)) == 0 );
	static_assert ( (ptrMask_ & lowerDataMask_) == 0 );
	static_assert ( (upperDataMask_ & lowerDataMask_) == 0 );
	static_assert ( (ptrMask_ | upperDataMask_ | lowerDataMask_) == 0xFFFFFFFFFFFFFFFFULL );
public:
	void init( void* ptr_, size_t data ) { 
		assert( ( (uintptr_t)ptr_ & (~ptrMask_) ) == 0 ); 
		ptr = (uintptr_t)ptr_; 
		assert( data < (1<<(upperDataSize_+lowerDataSize_)) ); 
		ptr |= data & lowerDataMask_; 
		ptr |= (data & upperDataMaskInData_) << upperDataShift_; 
	}
	void* getPtr() const { return (void*)( ptr & ptrMask_ ); }
	size_t getData() const { return ( (ptr & upperDataMask_) >> 45 ) | (ptr & lowerDataMask_); }
	size_t get3bitBlockData() const { return ptr & lowerDataMask_; }
	uintptr_t getLargerBlockData() const { return ptr & upperDataMask_; }
	void updatePtr( void* ptr_ ) { 
		assert( ( (uintptr_t)ptr_ & (~ptrMask_) ) == 0 ); 
		ptr &= ~ptrMask_; 
		ptr |= (uintptr_t)ptr_; 
	}
	void updateData( size_t data ) { 
		assert( data < (1<<(upperDataSize_+lowerDataSize_)) ); 
		ptr &= ptrMask_; 
		ptr |= data & lowerDataMask_; 
		ptr |= (data & upperDataMaskInData_) << upperDataShift_; 
	}
	void update3bitBlockData( size_t data ) { 
		assert( data < 8 ); 
		ptr &= ~lowerDataMask_; 
		ptr |= data & lowerDataMask_; 
	}
	void updateLargerBlockData( size_t data ) { 
		assert( (data & ~upperDataMaskInData_) == 0 ); 
		ptr &= ~upperDataMaskInData_; 
		ptr |= data & lowerDataMask_; 
		ptr |= data & upperDataMask_; 
	}
	void swap( Ptr2PtrWishDataBase& other ) { uintptr_t tmp = ptr; ptr = other.ptr; other.ptr = tmp; }
};


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