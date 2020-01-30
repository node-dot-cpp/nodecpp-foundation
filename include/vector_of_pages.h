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
	public:
		static constexpr size_t pageSize = 0x1000;
	private:
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
		struct IndexPageHeader
		{
			IndexPageHeader* next;
			size_t usedCnt;
			uint8_t* pages[1];
			void init() { next = nullptr; usedCnt = 0; pages[0] = nullptr;}
			void init(uint8_t* page) { next = nullptr; usedCnt = 1; pages[0] = page;}
			static constexpr size_t maxAddressed = ( GlobalPagePool::pageSize - sizeof( next ) - sizeof( usedCnt ) ) / sizeof( uint8_t*);
		};
		static_assert( sizeof( IndexPageHeader ) == sizeof( IndexPageHeader::next ) + sizeof( IndexPageHeader::usedCnt ) + sizeof( IndexPageHeader::pages ) );

		static constexpr size_t localStorageSize = 4;
		uint8_t* firstPages[ localStorageSize ];
		size_t pageCnt = 0; // payload pages (es not include index pages)
		IndexPageHeader* firstIndexPage = nullptr;
		IndexPageHeader* lastIndexPage = nullptr;
		uint8_t* currentPage = nullptr;
		size_t size = 0;

		size_t offsetInCurrentPage() { return size & ( GlobalPagePool::pageSize - 1 ); }
		size_t remainingSizeInCurrentPage() { return GlobalPagePool::pageSize - offsetInCurrentPage(); }
		void implReleaseAllPages()
		{
			size_t i;
			for ( i=0; i<localStorageSize && i<pageCnt; ++i )
				threadLocalPagePool.releasePage( firstPages[i] );
			pageCnt -= i;
			if ( pageCnt )
			{
				NODECPP_ASSERT( nodecpp::foundation::module_id, nodecpp::assert::AssertLevel::pedantic, firstIndexPage != nullptr );
				while ( firstIndexPage != nullptr )
				{
					NODECPP_ASSERT( nodecpp::foundation::module_id, nodecpp::assert::AssertLevel::pedantic, pageCnt > localStorageSize || firstIndexPage->next == nullptr );
					NODECPP_ASSERT( nodecpp::foundation::module_id, nodecpp::assert::AssertLevel::pedantic, firstIndexPage->usedCnt <= pageCnt );
					for ( size_t i=0; i<firstIndexPage->usedCnt; ++i )
						threadLocalPagePool.releasePage( firstIndexPage->pages[i] );
					pageCnt -= firstIndexPage->usedCnt;
					uint8_t* page = reinterpret_cast<uint8_t*>(firstIndexPage);
					firstIndexPage = firstIndexPage->next;
					threadLocalPagePool.releasePage( page );
				}
			}
		}

		void implAddPage()
		{
			NODECPP_ASSERT( nodecpp::foundation::module_id, nodecpp::assert::AssertLevel::pedantic, currentPage == nullptr );
			NODECPP_ASSERT( nodecpp::foundation::module_id, nodecpp::assert::AssertLevel::pedantic, offsetInCurrentPage() == 0 );
			if ( pageCnt < localStorageSize )
			{
				NODECPP_ASSERT( nodecpp::foundation::module_id, nodecpp::assert::AssertLevel::pedantic, firstIndexPage == nullptr );
				NODECPP_ASSERT( nodecpp::foundation::module_id, nodecpp::assert::AssertLevel::pedantic, lastIndexPage == nullptr );
				firstPages[pageCnt] = threadLocalPagePool.acquirePage();
				currentPage = firstPages[pageCnt];
			}
			else if ( lastIndexPage == nullptr )
			{
				NODECPP_ASSERT( nodecpp::foundation::module_id, nodecpp::assert::AssertLevel::pedantic, firstIndexPage == nullptr );
				firstIndexPage = lastIndexPage = reinterpret_cast<IndexPageHeader*>(threadLocalPagePool.acquirePage());
				currentPage = threadLocalPagePool.acquirePage();
				lastIndexPage->init( currentPage );
			}
			else if ( lastIndexPage->usedCnt == IndexPageHeader::maxAddressed )
			{
				NODECPP_ASSERT( nodecpp::foundation::module_id, nodecpp::assert::AssertLevel::pedantic, firstIndexPage != nullptr );
				IndexPageHeader* nextip = reinterpret_cast<IndexPageHeader*>(threadLocalPagePool.acquirePage());
				currentPage = threadLocalPagePool.acquirePage();
				nextip->init( currentPage );
				lastIndexPage->next = nextip;
				lastIndexPage = nextip;
			}
			else
			{
				NODECPP_ASSERT( nodecpp::foundation::module_id, nodecpp::assert::AssertLevel::pedantic, firstIndexPage != nullptr );
				NODECPP_ASSERT( nodecpp::foundation::module_id, nodecpp::assert::AssertLevel::pedantic, lastIndexPage->usedCnt < IndexPageHeader::maxAddressed );
				currentPage = threadLocalPagePool.acquirePage();
				lastIndexPage->pages[lastIndexPage->usedCnt] = currentPage;
				++(lastIndexPage->usedCnt);
			}
			++pageCnt;
		}

	public:
		VectorOfPages() { memset( firstPages, 0, sizeof( firstPages ) ); }
		void append( void* buff_, size_t sz )
		{
			uint8_t* buff = reinterpret_cast<uint8_t*>(buff_);
			while( sz != 0 )
			{
				if ( currentPage == nullptr )
				{
					implAddPage();
					NODECPP_ASSERT( nodecpp::foundation::module_id, nodecpp::assert::AssertLevel::pedantic, currentPage != nullptr );
				}
				size_t sz2copy = sz <= remainingSizeInCurrentPage() ? sz : remainingSizeInCurrentPage();
				memcpy( currentPage, buff, sz2copy );
				sz -= sz2copy;
				buff += sz2copy;
			}
		}
		void clear() 
		{
			implReleaseAllPages();
			NODECPP_ASSERT( nodecpp::foundation::module_id, nodecpp::assert::AssertLevel::pedantic, pageCnt == 0 );
			firstIndexPage = nullptr;
			lastIndexPage = nullptr;
			currentPage = nullptr;
			size = 0;
		}
		~VectorOfPages() { implReleaseAllPages(); }
	};

} // nodecpp


#endif // VECTOR_OF_PAGES_H