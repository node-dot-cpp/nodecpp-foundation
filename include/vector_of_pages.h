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

#ifndef VECTOR_OF_PAGES_H
#define VECTOR_OF_PAGES_H

#include "foundation.h"
#include "page_allocator.h"

namespace nodecpp { 

	class OSPageProvider // used just for interface purposes
	{
	public:
		static constexpr size_t pageSize = 0x1000;
		uint8_t* acquirePage() { return reinterpret_cast<uint8_t*>( VirtualMemory::allocate( pageSize ) ); }
		void releasePage( uint8_t* page ) { VirtualMemory::deallocate( page, pageSize ); }
	};

	class GlobalPagePool
	{
		static constexpr size_t pageSize = 0x1000;
		uint8_t* firstPage = nullptr;
		size_t pageCnt = 0;
		static constexpr size_t highWatermark = 16;
	public:
		GlobalPagePool() {}
		uint8_t* acquirePage() {
			// TODO: thread-sync
			NODECPP_ASSERT( nodecpp::foundation::module_id, nodecpp::assert::AssertLevel::pedantic, ( pageCnt == 0 && firstPage == nullptr ) || ( pageCnt != 0 && firstPage != nullptr ) );
			uint8_t* page;
			if ( pageCnt != 0 )
			{
				page = firstPage;
				firstPage = *reinterpret_cast<uint8_t**>(firstPage);
				*reinterpret_cast<uint8_t**>(firstPage) = nullptr;
				--pageCnt;
			}
			else
				page = reinterpret_cast<uint8_t*>( VirtualMemory::allocate( pageSize ) );
			return page;
		}
		void releasePage( uint8_t* page ) {
			// TODO: thread-sync
			NODECPP_ASSERT( nodecpp::foundation::module_id, nodecpp::assert::AssertLevel::pedantic, ( pageCnt == 0 && firstPage == nullptr ) || ( pageCnt != 0 && firstPage != nullptr ) );
			if ( pageCnt < highWatermark )
			{
				*reinterpret_cast<uint8_t**>(page) = firstPage;
				firstPage = page;
				++pageCnt;
			}
			else
				VirtualMemory::deallocate( page, pageSize );
		}
	};
	extern GlobalPagePool globalPagePool;

	class ThreadLocalPagePool
	{
		uint8_t* firstPage = nullptr;
		size_t pageCnt = 0;
		static constexpr size_t highWatermark = 16;
	public:
		ThreadLocalPagePool() {}
		uint8_t* acquirePage() {
			NODECPP_ASSERT( nodecpp::foundation::module_id, nodecpp::assert::AssertLevel::pedantic, ( pageCnt == 0 && firstPage == nullptr ) || ( pageCnt != 0 && firstPage != nullptr ) );
			uint8_t* page;
			if ( pageCnt != 0 )
			{
				page = firstPage;
				firstPage = *reinterpret_cast<uint8_t**>(firstPage);
				*reinterpret_cast<uint8_t**>(firstPage) = nullptr;
				--pageCnt;
			}
			else
				page = globalPagePool.acquirePage();
			return page;
		}
		void releasePage( uint8_t* page ) {
			NODECPP_ASSERT( nodecpp::foundation::module_id, nodecpp::assert::AssertLevel::pedantic, ( pageCnt == 0 && firstPage == nullptr ) || ( pageCnt != 0 && firstPage != nullptr ) );
			if ( pageCnt < highWatermark )
			{
				*reinterpret_cast<uint8_t**>(page) = firstPage;
				firstPage = page;
				++pageCnt;
			}
			else
				globalPagePool.releasePage( page );
		}
	};
	extern thread_local ThreadLocalPagePool threadLocalPagePool;

	class VectorOfPages
	{
		static constexpr size_t localStorageSize = 4;
		uint8_t* firstPages[ localStorageSize ];
	public:
		VectorOfPages() { memset( firstPages, 0, sizeof( firstPages ) ); }
		~VectorOfPages() { /*TODO: release all pages*/ }
	};

} // nodecpp


#endif // VECTOR_OF_PAGES_H