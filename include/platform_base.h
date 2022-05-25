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

#ifndef NODECPP_PLATFORM_BASE_H
#define NODECPP_PLATFORM_BASE_H 

// PLATFORM SELECTION

//COMPILER
#if defined(__clang__)
#define NODECPP_CLANG
#elif defined(__GNUC__) || defined(__GNUG__)
#define NODECPP_GCC
#elif defined(_MSC_VER)
#define NODECPP_MSVC
#else
#error unknown/unsupported compiler
#endif

//CPU
#if defined(__X86_64__) || defined(__X86_64) || defined(__amd64__) || defined(__amd64) || defined(_M_X64)
#define NODECPP_X64
static_assert(sizeof(void*) == 8);
#elif defined(__i386__) || defined(i386) || defined(__i386) || defined(__I86__) || defined(_M_IX86)
#define NODECPP_X86
static_assert(sizeof(void*) == 4);
#elif defined(__arm64) || defined(arm64) || defined(__aarch64__)
#define NODECPP_ARM64
static_assert(sizeof(void*) == 8);
//#pragma message( "ARM architecture is only partially supported. Use with precaution" ) 
#else
#error unknown/unsupported CPU
#endif

//OS
#if defined(__ANDROID__)
//mb: while android is a kind of linux, it has its own libc (Bionic) so we need to diferenciate
#define NODECPP_ANDROID
#elif (defined __linux) || (defined linux) || (defined __linux__)
#define NODECPP_LINUX
#elif (defined __WINDOWS__) || (defined _WIN32) || (defined _WIN64)
#define NODECPP_WINDOWS
#elif (defined __OSX__) || (defined __APPLE__)
#define NODECPP_MAC
#else
#error unknown/unsupported OS
#endif

#if defined(NODECPP_MSVC)
#define NODECPP_NOINLINE      __declspec(noinline)
#define NODECPP_FORCEINLINE	__forceinline
#elif (defined NODECPP_CLANG) || (defined NODECPP_GCC)
#define NODECPP_NOINLINE      __attribute__ ((noinline))
#define NODECPP_FORCEINLINE inline __attribute__((always_inline))
#else
#define NODECPP_FORCEINLINE inline
#define NODECPP_NOINLINE
#pragma message( "Unknown compiler. Force-inlining and force-uninlining are disabled" )
#endif

#if (defined NODECPP_CLANG) || (defined NODECPP_GCC)
#define NODECPP_LIKELY(x)       __builtin_expect(!!(x),1)
#define NODECPP_UNLIKELY(x)     __builtin_expect(!!(x),0)
#else
#define NODECPP_LIKELY(x) (x)
#define NODECPP_UNLIKELY(x) (x)
#endif

//!!!NO COMPILER-SPECIFIC #defines PAST THIS POINT!!!

//PLATFORM PROPERTIES

// inevitable headers
//#include <stddef.h>
#include <cstdint>
#include <utility>
#include <cstddef>
#include <functional>

//MMU-BASED SYSTEMS IN PROTECTED MODE
#if defined(NODECPP_LINUX) || defined(NODECPP_WINDOWS) || defined(NODECPP_MAC) || defined(NODECPP_ANDROID)

#if defined(NODECPP_X86) || defined(NODECPP_X64) || defined(NODECPP_ARM64)
#define NODECPP_SECOND_NULLPTR ((void*)1)
#define NODECPP_MINIMUM_CPU_PAGE_SIZE 4096
#define NODECPP_MINIMUM_ZERO_GUARD_PAGE_SIZE 4096
struct _TMP_STRUCT_WITH_STD_FUNCTION { std::function<size_t(size_t)> dummy; };
#define NODECPP_MAX_SUPPORTED_ALIGNMENT ( std::max( sizeof(::std::max_align_t), alignof(_TMP_STRUCT_WITH_STD_FUNCTION) ) )
#define NODECPP_MAX_SUPPORTED_ALIGNMENT_FOR_NEW ( std::max( std::max( sizeof(::std::max_align_t), static_cast<size_t>(__STDCPP_DEFAULT_NEW_ALIGNMENT__)), alignof(_TMP_STRUCT_WITH_STD_FUNCTION) ) )
#define NODECPP_GUARANTEED_MALLOC_ALIGNMENT (sizeof(::std::max_align_t))
#define NODECPP_GUARANTEED_IIBMALLOC_ALIGNMENT_EXP 5 // rather a forward declaration
#define NODECPP_GUARANTEED_IIBMALLOC_ALIGNMENT (1<<NODECPP_GUARANTEED_IIBMALLOC_ALIGNMENT_EXP)

namespace nodecpp::platform { 
NODECPP_FORCEINLINE
bool is_guaranteed_on_stack( void* ptr )
{
	//on a page-based system, if a pointer to current on-stack variable (int a below) 
	//   belongs to the same CPU page as ptr being analysed, ptr points to stack regardless of stack being contiguous etc. etc.
	//it ensures that false positives are impossible, and false negatives are possible but rare
	int a;
	constexpr uintptr_t upperBitsMask = ~( NODECPP_MINIMUM_CPU_PAGE_SIZE - 1 );
//	printf( "   ---> isGuaranteedOnStack(%zd), &a = %zd (%s)\n", ((uintptr_t)(ptr)), ((uintptr_t)(&a)), ( ( ((uintptr_t)(ptr)) ^ ((uintptr_t)(&a)) ) & upperBitsMask ) == 0 ? "YES" : "NO" );
	return ( ( ((uintptr_t)(ptr)) ^ ((uintptr_t)(&a)) ) & upperBitsMask ) == 0;
}
}//nodecpp::platform

#else

//#define NODECPP_SECOND_NULLPTR ((void*)1) // TODO: define accordingly
#define NODECPP_MINIMUM_CPU_PAGE_SIZE 0 // protective value; redefine properly wherever possible
#define NODECPP_MINIMUM_ZERO_GUARD_PAGE_SIZE 0 // protective value; redefine properly wherever possible
#define NODECPP_MAX_SUPPORTED_ALIGNMENT 1 // protective value; redefine properly wherever possible
#define NODECPP_MAX_SUPPORTED_ALIGNMENT_FOR_NEW 1 // protective value; redefine properly wherever possible
#define NODECPP_GUARANTEED_MALLOC_ALIGNMENT 1 // protective value; redefine properly wherever possible
#define NODECPP_GUARANTEED_IIBMALLOC_ALIGNMENT_EXP 0 // rather a forward declaration; protective value; is iibmalloc is at all implemented?
#define NODECPP_GUARANTEED_IIBMALLOC_ALIGNMENT (1<<NODECPP_GUARANTEED_IIBMALLOC_ALIGNMENT_EXP)

#endif//defined(NODECPP_X86) || defined(NODECPP_X64) || defined(NODECPP_ARM64)

#else // other OSs

#define NODECPP_MINIMUM_CPU_PAGE_SIZE 0 // protective value; redefine properly wherever possible
#define NODECPP_MINIMUM_ZERO_GUARD_PAGE_SIZE 0 // protective value; redefine properly wherever possible
#define NODECPP_GUARANTEED_MALLOC_ALIGNMENT 1 // protective value; redefine properly wherever possible
#define NODECPP_GUARANTEED_IIBMALLOC_ALIGNMENT_EXP 0 // rather a forward declaration; protective value; is iibmalloc is at all implemented?
#define NODECPP_GUARANTEED_IIBMALLOC_ALIGNMENT (1<<NODECPP_GUARANTEED_IIBMALLOC_ALIGNMENT_EXP)

#endif//defined(NODECPP_LINUX) || defined(NODECPP_WINDOWS) || defined(NODECPP_MAC) || defined(NODECPP_ANDROID)

#ifndef NODECPP_MINIMUM_CPU_PAGE_SIZE
namespace nodecpp::platform { 
NODECPP_FORCEINLINE
constexpr bool is_guaranteed_on_stack(void*)
{
	return false;
}
}//nodecpp::platform
#endif

//CLASS LAYOUT
namespace nodecpp::platform { 
	
//USAGE FOR read_vmt_pointer()/restore_vmt_pointer():
//   auto backup = backup_vmt_pointer(p);//in general, return type of backup_vmt_pointer() is NOT guaranteed to be void*
//   do_something();
//   restore_vmt_pointer(p,backup);

#if defined(NODECPP_CLANG) || defined(NODECPP_GCC) || defined(NODECPP_MSVC)
	
//IMPORTANT: in general, return type of backup_vmt_pointer() MAY differ from void*; callers MUST treat it as an opaque value
//           which may ONLY be used to feed it to restore_vmt_pointer() as a second parameter 
NODECPP_FORCEINLINE
void* backup_vmt_pointer(void* p) { return *((void**)p); }

NODECPP_FORCEINLINE
void restore_vmt_pointer(void* p, void* backup) { *((void**)p) = backup; }

inline
constexpr std::pair<size_t, size_t> get_vmt_pointer_size_pos() { return std::make_pair( size_t(0), sizeof(void*) ); }
#else//defined(NODECPP_CLANG) || defined(NODECPP_GCC) || defined(NODECPP_MSVC)
#pragma message("Unknown compiler. Trying to guess VMT pointer layout. Don't complain if it will crash.")
	
NODECPP_FORCEINLINE
void* backup_vmt_pointer(void* p) { return *((void**)p); }

NODECPP_FORCEINLINE
void restore_vmt_pointer(void* p, void* vpt) { *((void**)p) = vpt; }

inline
constexpr std::pair<size_t, size_t> get_vmt_pointer_size_pos() { return std::make_pair( size_t(0), sizeof(void*) ); }
#endif//defined(NODECPP_CLANG) || defined(NODECPP_GCC) || defined(NODECPP_MSVC)
}//nodecpp::platform


// Compiler bugs and limitations that mignt affect a wide number of implementations

// As of mid 2019 GCC does not support coroutine staff
#if !((defined NODECPP_CLANG) || (defined NODECPP_MSVC))
#define NODECPP_NO_COROUTINES
#endif

// Earlier versions of GCC had problems with initializing non-trivial thread-local objects 
// (see GCC bug 60702, https://gcc.gnu.org/bugzilla/show_bug.cgi?id=60702)
// TODO: check __GNUC_PATCHLEVEL__
#if ((defined NODECPP_GCC) && (  (__GNUC__ < 8) || ((__GNUC__ == 8) && (__GNUC_MINOR__ < 4)) ))
#define NODECPP_THREADLOCAL_INIT_BUG_GCC_60702
#endif


#endif // NODECPP_PLATFORM_BASE_H
