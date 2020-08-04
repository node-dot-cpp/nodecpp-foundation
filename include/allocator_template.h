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

#ifndef ALLOCATOR_TEMPLATE_H
#define ALLOCATOR_TEMPLATE_H

#include "platform_base.h"

#if defined NODECPP_MSVC
#define NODISCARD _NODISCARD
#elif (defined NODECPP_GCC) || (defined NODECPP_CLANG)
#define NODISCARD [[nodiscard]]
#else
#define NODISCARD
#endif

namespace nodecpp {

	template<class RawAllocT, class _Ty>
	class selective_allocator
	{
		static constexpr size_t alignment4BigAlloc = 32;
		static_assert(2 * sizeof(void *) <= alignment4BigAlloc);

		size_t getByteSizeOfNElem(const size_t numOfElements)
		{
			if constexpr ( sizeof(_Ty) == 1 )
				return numOfElements;
			else
			{
				constexpr size_t maxPossible = static_cast<size_t>(-1) / sizeof(_Ty);
				size_t ret = numOfElements * sizeof(_Ty);
				if (maxPossible < numOfElements)
					ret = static_cast<size_t>(-1);
				return ret;
			}
		}

	public:
		static_assert(!std::is_const_v<_Ty>, "The C++ Standard forbids containers of const elements because allocator<const T> is ill-formed.");

		using _Not_user_specialized = void;
		using value_type = _Ty;
		using propagate_on_container_move_assignment = std::true_type;
		using is_always_equal = std::true_type;

		template<class _Other>
			struct rebind
			{	// convert this type to allocator<_Other>
			using other = selective_allocator<RawAllocT, _Other>;
			};

		constexpr selective_allocator() noexcept {}
		constexpr selective_allocator(const selective_allocator&) noexcept = default;
		template<class RawAllocT1, class _Other>
		constexpr selective_allocator(const selective_allocator<RawAllocT1, _Other>&) noexcept {}

		void deallocate(_Ty * const ptr, size_t count)
		{
			size_t sz = count * sizeof( _Ty );
			constexpr size_t alignment = alignof(_Ty) > static_cast<size_t>(__STDCPP_DEFAULT_NEW_ALIGNMENT__) ? alignof(_Ty) : static_cast<size_t>(__STDCPP_DEFAULT_NEW_ALIGNMENT__);

			NODECPP_ASSERT( nodecpp::foundation::module_id, nodecpp::assert::AssertLevel::pedantic, ((uintptr_t)ptr & (alignment-1)) == 0, "indeed: alignment = {}, ret = {:x}", alignment, (uintptr_t)ptr );
			RawAllocT::deallocate(ptr); // TODO: check that we can ignore other params
		}

		NODISCARD _Ty * allocate(const size_t _Count)
		{
			static_assert( std::alignment_of_v<_Ty> <= RawAllocT::guaranteed_alignment );
			size_t iniByteSz = getByteSizeOfNElem(_Count);
			constexpr size_t alignment = alignof(_Ty) > static_cast<size_t>(__STDCPP_DEFAULT_NEW_ALIGNMENT__) ? alignof(_Ty) : static_cast<size_t>(__STDCPP_DEFAULT_NEW_ALIGNMENT__);

			if (iniByteSz == 0)
				return static_cast<_Ty *>(nullptr);

			_Ty* ret = static_cast<_Ty *>(RawAllocT::allocate(iniByteSz));
			NODECPP_ASSERT( nodecpp::foundation::module_id, nodecpp::assert::AssertLevel::pedantic, ((uintptr_t)ret & (alignment-1)) == 0, "indeed: alignment = {}, ret = {:x}", alignment, (uintptr_t)ret );
			return ret;
		}
	};

	struct StdRawAllocator
	{
		static constexpr size_t guaranteed_alignment = NODECPP_GUARANTEED_IIBMALLOC_ALIGNMENT;
		static NODECPP_FORCEINLINE void* allocate( size_t allocSize ) { return ::malloc( allocSize ); }
		static NODECPP_FORCEINLINE void* allocate( size_t allocSize, size_t allignment ) { return ::malloc( allocSize ); } // TODO: address alignment
		static NODECPP_FORCEINLINE void deallocate( void* ptr) { ::free( ptr ); }
	};

	template<class _Ty>
	using stdallocator = selective_allocator<StdRawAllocator, _Ty>;
	template< class T1, class T2 >
	bool operator==( const stdallocator<T1>& lhs, const stdallocator<T2>& rhs ) noexcept { return true; }
	template< class T1, class T2 >
	bool operator!=( const stdallocator<T1>& lhs, const stdallocator<T2>& rhs ) noexcept { return false; }

} //namespace nodecpp

#endif // ALLOCATOR_TEMPLATE_H
