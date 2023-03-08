/* -------------------------------------------------------------------------------
* Copyright (c) 2020-2021, OLogN Technologies AG
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

#ifndef MAALLOC_BASED_ALLOCATOR_H
#define MAALLOC_BASED_ALLOCATOR_H

#include "platform_base.h"
#include "allocator_template.h"


#if defined(NODECPP_ARM64)

#if defined(NODECPP_MAC) || defined(NODECPP_ANDROID)
// mb: aligned_alloc on Mac on M1 is always returning null
// on Android is not even there
// also it seems that default malloc alignment is rather high
// so temporarily disabling aligned_alloc
#include <stdlib.h>
#define MALLOC_BASED_ALIGNED_ALLOC( size, alignment ) ::malloc( size )
#define MALLOC_BASED_ALIGNED_FREE( ptr ) ::free( ptr )

#else
#error not implemented (add known cases when possible)
#endif

#elif defined(NODECPP_X86) || defined(NODECPP_X64)

#if defined(NODECPP_WINDOWS)

#include <malloc.h>
#define MALLOC_BASED_ALIGNED_ALLOC( size, alignment ) _aligned_malloc( size, alignment )
#define MALLOC_BASED_ALIGNED_FREE( ptr ) ::_aligned_free( ptr )

#elif defined(NODECPP_LINUX) || defined(NODECPP_MAC)

#include <stdlib.h>
#define MALLOC_BASED_ALIGNED_ALLOC( size, alignment ) ::aligned_alloc( size, alignment )
#define MALLOC_BASED_ALIGNED_FREE( ptr ) ::free( ptr )

#elif defined(NODECPP_ANDROID)
// mb: aligned_alloc doesn't exist on Android
// but it seems that default malloc alignment is rather high
// so temporarily disabling aligned_alloc
#include <stdlib.h>
#define MALLOC_BASED_ALIGNED_ALLOC( size, alignment ) ::malloc( size )
#define MALLOC_BASED_ALIGNED_FREE( ptr ) ::free( ptr )

#else
#error not implemented (add known cases when possible)
#endif

#else
#error not implemented (add known cases when possible)
#endif


namespace nodecpp {

	struct StdRawAllocator
	{
		static constexpr bool objectRequired = false;
		static constexpr size_t guaranteed_alignment = NODECPP_MAX_SUPPORTED_ALIGNMENT_FOR_NEW;
// NOTE: with MS NODECPP_MAX_SUPPORTED_ALIGNMENT_FOR_NEW is known tobe the same as malloc supports, and malloc()/free() pair is used for both 'new' operator and this allocator; 
//       for greater values both 'new' operator and this allocator will use _aligned_malloc()/_aligned_free() pair
//       For Linux/Mac with GCC and CLANG a counterpart to alloc() and aligned_alloc() is always free()
//       This may be useful when allocation is done by this allocator, and deallocation is done inside 3rd party lib with standard means
#ifdef NODECPP_WINDOWS
		static constexpr size_t guaranteed_malloc_alignment = __STDCPP_DEFAULT_NEW_ALIGNMENT__;
#elif ( defined(NODECPP_LINUX) || defined(NODECPP_MAC) || defined(NODECPP_ANDROID)) && (defined(NODECPP_GCC) || defined(NODECPP_CLANG))
		static constexpr size_t guaranteed_malloc_alignment = NODECPP_GUARANTEED_MALLOC_ALIGNMENT;
#else
#error not implemented (add known cases when possible)
#endif
		template<size_t alignment = 0> 
		static NODECPP_FORCEINLINE void* allocate( size_t allocSize ) { 
			static_assert( alignment <= guaranteed_alignment );
			void* ptr;
			if constexpr ( alignment <= guaranteed_malloc_alignment )
				ptr = ::malloc( allocSize );
			else
				ptr = MALLOC_BASED_ALIGNED_ALLOC( allocSize, alignment );
				
			if(ptr)
				return ptr;
			throw std::bad_alloc();
		}
		template<size_t alignment = 0> 
		static NODECPP_FORCEINLINE void deallocate( void* ptr) {
			static_assert( alignment <= guaranteed_alignment );
			if constexpr ( alignment <= guaranteed_malloc_alignment )
				::free( ptr );
			else
				MALLOC_BASED_ALIGNED_FREE( ptr );
		}
	};

	template<class _Ty>
	using stdallocator = selective_allocator<StdRawAllocator, _Ty>;
	template< class T1, class T2 >
	bool operator==( const stdallocator<T1>& lhs, const stdallocator<T2>& rhs ) noexcept { return true; }
	template< class T1, class T2 >
	bool operator!=( const stdallocator<T1>& lhs, const stdallocator<T2>& rhs ) noexcept { return false; }

} //namespace nodecpp

#endif // MAALLOC_BASED_ALLOCATOR_H
