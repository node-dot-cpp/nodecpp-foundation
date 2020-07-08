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
	#ifdef _DEBUG
		static constexpr size_t reserverdSize4Vecor = 2 * sizeof(void *) + alignment4BigAlloc - 1;
	#else
		static constexpr size_t reserverdSize4Vecor = sizeof(void *) + alignment4BigAlloc - 1;
	#endif // _DEBUG

	#ifdef NODECPP_X64
		static constexpr size_t bigAllocGuardSignature = 0xECECECECECECECECULL;
	#else
		static constexpr size_t bigAllocGuardSignature = 0xECECECECUL;
	#endif // NODECPP_X64

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

		void* allocAlignedVector(const size_t sz)
		{
			size_t allocSize = reserverdSize4Vecor + sz;
			if (allocSize <= sz)
				allocSize = static_cast<size_t>(-1);

			uintptr_t container_;
			container_ = reinterpret_cast<uintptr_t>(RawAllocT::allocate(allocSize));
			const uintptr_t container = container_;
			NODECPP_ASSERT(nodecpp::foundation::module_id, nodecpp::assert::AssertLevel::critical, container != 0 );
			void * const ret = reinterpret_cast<void *>((container + reserverdSize4Vecor)	& ~(alignment4BigAlloc - 1));
			static_cast<uintptr_t *>(ret)[-1] = container;

	#ifdef _DEBUG
			static_cast<uintptr_t *>(ret)[-2] = bigAllocGuardSignature;
	#endif // _DEBUG
			return ret;
		}

		void vectorValuesToAlignedValues(void *& ptr, size_t& sz)
		{
			sz += reserverdSize4Vecor;

			const uintptr_t * const iniPtr = reinterpret_cast<uintptr_t *>(ptr);
			const uintptr_t container = iniPtr[-1];

			NODECPP_ASSERT(nodecpp::foundation::module_id, nodecpp::assert::AssertLevel::critical, iniPtr[-2] == bigAllocGuardSignature );

	#ifdef _DEBUG
			constexpr uintptr_t minBackShift = 2 * sizeof(void *);
	#else
			constexpr uintptr_t minBackShift = sizeof(void *);
	#endif // _DEBUG
			const uintptr_t backShift = reinterpret_cast<uintptr_t>(ptr) - container;
			NODECPP_ASSERT(nodecpp::foundation::module_id, nodecpp::assert::AssertLevel::critical, backShift >= minBackShift, "{} vs. {}", backShift, minBackShift );
			NODECPP_ASSERT(nodecpp::foundation::module_id, nodecpp::assert::AssertLevel::critical, backShift <= reserverdSize4Vecor, "{} vs. {}", backShift, reserverdSize4Vecor );
			ptr = reinterpret_cast<void *>(container);
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

		void deallocate(_Ty * const ptr, size_t sz)
		{
			constexpr size_t alignment = alignof(_Ty) > static_cast<size_t>(__STDCPP_DEFAULT_NEW_ALIGNMENT__) ? alignof(_Ty) : static_cast<size_t>(__STDCPP_DEFAULT_NEW_ALIGNMENT__);

	#ifdef NODECPP_MSVC
			if constexpr ( alignment > __STDCPP_DEFAULT_NEW_ALIGNMENT__ )
			{
				size_t iniAlignment = alignment;
	#if (defined NODECPP_X64) || (defined NODECPP_X86)
				if (sz >= std::_Big_allocation_threshold)
					iniAlignment = _Max_value(alignment, alignment4BigAlloc);
	#endif // (defined NODECPP_X64) || (defined NODECPP_X86)
	//			::operator delete(ptr, sz, std::align_val_t{iniAlignment}, StdAllocEnforcer::enforce);
				RawAllocT::deallocate(ptr); // TODO: check that we can ignore other params
			}
			else
	#endif
			{
				void* ptr_ = ptr;
	#if (defined NODECPP_X64) || (defined NODECPP_X86)
	#ifdef NODECPP_MSVC
				if (sz >= std::_Big_allocation_threshold)
					vectorValuesToAlignedValues(ptr_, sz);
	#endif
	#endif // (defined NODECPP_X64) || (defined NODECPP_X86)
	//			::operator delete(ptr, sz, StdAllocEnforcer::enforce);
				RawAllocT::deallocate(ptr); // TODO: check that we can ignore other params
			}
		}

		NODISCARD _Ty * allocate(const size_t _Count)
		{
			size_t iniByteSz = getByteSizeOfNElem(_Count);
			constexpr size_t alignment = alignof(_Ty) > static_cast<size_t>(__STDCPP_DEFAULT_NEW_ALIGNMENT__) ? alignof(_Ty) : static_cast<size_t>(__STDCPP_DEFAULT_NEW_ALIGNMENT__);

			if (iniByteSz == 0)
				return static_cast<_Ty *>(nullptr);

	#ifdef NODECPP_MSVC
			if constexpr ( alignment > __STDCPP_DEFAULT_NEW_ALIGNMENT__ )
			{
				size_t iniAlignment = alignment;
	#if (defined NODECPP_X64) || (defined NODECPP_X86)
				if (iniByteSz >= std::_Big_allocation_threshold)
					iniAlignment = _Max_value(alignment, alignment4BigAlloc);
	#endif // (defined NODECPP_X64) || (defined NODECPP_X86)
	//			return static_cast<_Ty *>(::operator new(iniByteSz, iniAlignment, StdAllocEnforcer::enforce));
				return static_cast<_Ty *>(RawAllocT::allocate(iniByteSz, iniAlignment));
			}
			else
	#endif
			{
	#if (defined NODECPP_X64) || (defined NODECPP_X86)
	#ifdef NODECPP_MSVC
				if (iniByteSz >= std::_Big_allocation_threshold)
					return static_cast<_Ty *>(allocAlignedVector(iniByteSz));
	#endif
	#endif // (defined NODECPP_X64) || (defined NODECPP_X86)
				return static_cast<_Ty *>(RawAllocT::allocate(iniByteSz));
			}
		}
	};

	struct StdRawAllocator
	{
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
