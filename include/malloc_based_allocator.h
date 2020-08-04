/* -------------------------------------------------------------------------------
* Copyright (c) 2020, OLogN Technologies AG
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

#ifdef NODECPP_WINDOWS

#include <malloc.h>
#define MALLOC_BASED_ALIGNED_ALLOC( size, alignment ) _aligned_malloc( size, alignment )
#define MALLOC_BASED_ALIGNED_FREE( ptr ) ::_aligned_free( ptr )

#elif defined NODECPP_LINUX && (defined NODECPP_GCC || defined NODECPP_CLANG)

#include <stdlib.h>
#define MALLOC_BASED_ALIGNED_ALLOC( size, alignment ) ::aligned_alloc( size, alignment )
#define MALLOC_BASED_ALIGNED_FREE( ptr ) ::free( ptr )

#else
#error not implemented (add known cases when possible)
#endif

namespace nodecpp {

	struct StdRawAllocator
	{
		static constexpr size_t guaranteed_alignment = NODECPP_MAX_SUPPORTED_ALIGNMENT_FOR_NEW;
		template<size_t alignment = 0> 
		static NODECPP_FORCEINLINE void* allocate( size_t allocSize ) { 
			static_assert( alignment <= guaranteed_alignment );
			if constexpr ( alignment <= NODECPP_GUARANTEED_MALLOC_ALIGNMENT )
				return ::malloc( allocSize );
			else
				return MALLOC_BASED_ALIGNED_ALLOC( allocSize, alignment );
		}
		template<size_t alignment = 0> 
		static NODECPP_FORCEINLINE void deallocate( void* ptr) {
			static_assert( alignment <= guaranteed_alignment );
			if constexpr ( alignment <= NODECPP_GUARANTEED_MALLOC_ALIGNMENT )
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
