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
#ifdef NODECPP_MSVC
#include <intrin.h>
#endif

namespace nodecpp { 

	struct BasePageBlockHedaer
	{
		// NOTE: typical size is 1^(page_size + pageCntExp) = 250K - 4M
		static constexpr size_t pageSize = 0x1000;
//		uint8_t* segmentBlock = nullptr;
		uint8_t* firstFree = nullptr;
		void init( uint8_t* segmentBlock_, size_t pageCnt )
		{
//			segmentBlock = segmentBlock_;
			NODECPP_ASSERT( nodecpp::foundation::module_id, nodecpp::assert::AssertLevel::pedantic, pageCnt );
			firstFree = segmentBlock_;
			uint8_t* page = segmentBlock_;
			for ( ; page < segmentBlock_ + (pageCnt - 1) * pageSize; page += pageSize )
				*reinterpret_cast<uint8_t**>(page) = page + pageSize;
			*reinterpret_cast<uint8_t**>(page) = nullptr;
		}
		uint8_t* acquirePage() { 
			NODECPP_ASSERT( nodecpp::foundation::module_id, nodecpp::assert::AssertLevel::pedantic, firstFree );
			uint8_t* ret = firstFree;
			firstFree = *reinterpret_cast<uint8_t**>(firstFree);
			return ret;
		}
		void releasePage( size_t pagesInBlock, uint8_t* page ) { 
//			NODECPP_ASSERT( nodecpp::foundation::module_id, nodecpp::assert::AssertLevel::pedantic, page >= segmentBlock && page < segmentBlock + pagesInBlock * pageSize );
			NODECPP_ASSERT( nodecpp::foundation::module_id, nodecpp::assert::AssertLevel::pedantic, (((uintptr_t)page) & (pageSize - 1)) == 0 ); // page-aligned address
			*reinterpret_cast<uint8_t**>(page) = firstFree;
			firstFree = page;
		}
	};
	static_assert( sizeof(BasePageBlockHedaer) == sizeof(uint8_t*) );

	struct LevelBase
	{
		// availability MASK legend: 
		// bit value of 1 means adding is possible ( UINT64_MAX is a starting value, and 0 means all occupied (no acquiring is possible))
		// particularly: availabilityMask == 0 => no further acquiring action is expected
		static constexpr size_t itemsPerLevel = 64;
		uint64_t availabilityMask; 
		static size_t firstNonzero64( uint64_t mask )
		{
			NODECPP_ASSERT( nodecpp::foundation::module_id, nodecpp::assert::AssertLevel::pedantic, mask != 0 );
#if defined NODECPP_MSVC
			unsigned long ix = 0;
			uint8_t r = _BitScanForward64(&ix, mask);
			return r;
#elif (defined NODECPP_CLANG) || (defined NODECPP_GCC)
			return __builtin_ctzll( mask );
#else
			size_t ret = 0;
			for( ; ret<64; ++ret )
				if ( (((uint64_t)1)<<ret) & mask )
					return ret;
			return ret;
#endif
		}
		void markAvailable(size_t idx)
		{ 
			static_assert( itemsPerLevel == 64 );
			NODECPP_ASSERT( nodecpp::foundation::module_id, nodecpp::assert::AssertLevel::pedantic, idx < 64 );
			availabilityMask |= ((uint64_t)1) << idx;
		}
		bool isAnyAvailable() { return availabilityMask != 0; }
		size_t getFirstAvailable()
		{ 
			static_assert( itemsPerLevel == 64 );
			NODECPP_ASSERT( nodecpp::foundation::module_id, nodecpp::assert::AssertLevel::pedantic, isAnyAvailable() );
			return firstNonzero64( availabilityMask );
		}
		void markInavailable(size_t idx)
		{ 
			static_assert( itemsPerLevel == 64 );
			NODECPP_ASSERT( nodecpp::foundation::module_id, nodecpp::assert::AssertLevel::pedantic, idx < 64 );
			availabilityMask &= ~((uint64_t)1) << idx;
		}
	};
	static_assert( sizeof(LevelBase) * 8 == LevelBase::itemsPerLevel );

	struct SegmentLevelOne : public LevelBase
	{
		// commited MASK legend: 
		// bit value of 1 means yet to be commited
		// particularly: commitedMask == 0 => none is yet commited
		uint64_t commitedMask;
//		size_t allocCnt = 0;
//		size_t pagesPerBlock;
//		uint8_t* segmentStart;
		void markCommited(size_t idx)
		{ 
			static_assert( itemsPerLevel == 64 );
			NODECPP_ASSERT( nodecpp::foundation::module_id, nodecpp::assert::AssertLevel::pedantic, idx < 64 );
			commitedMask &= ~((uint64_t)1) << idx;
		}
		bool areAllCommited() { return commitedMask != 0; }
		size_t getFirstNotCommited()
		{ 
			static_assert( itemsPerLevel == 64 );
			NODECPP_ASSERT( nodecpp::foundation::module_id, nodecpp::assert::AssertLevel::pedantic, isAnyAvailable() );
			return firstNonzero64( commitedMask );
		}
		void markDecommited(size_t idx)
		{ 
			static_assert( itemsPerLevel == 64 );
			NODECPP_ASSERT( nodecpp::foundation::module_id, nodecpp::assert::AssertLevel::pedantic, idx < 64 );
			commitedMask |= ((uint64_t)1) << idx;
		}
	};
	static_assert( sizeof(SegmentLevelOne) == 16 );

	class OSPageProvider // used just for interface purposes
	{
	public:
		uint8_t* acquirePage() { return reinterpret_cast<uint8_t*>( VirtualMemory::allocate( BasePageBlockHedaer::pageSize ) ); }
		void releasePage( uint8_t* page ) { VirtualMemory::deallocate( page, BasePageBlockHedaer::pageSize ); }
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
			uint8_t** pages() { return reinterpret_cast<uint8_t**>(this+1); }
			void init() { next = nullptr; usedCnt = 0; pages()[0] = nullptr;}
			void init(uint8_t* page) { next = nullptr; usedCnt = 1; pages()[0] = page;}
		};
		static constexpr size_t maxAddressedByPage = ( GlobalPagePool::pageSize - sizeof( IndexPageHeader ) ) / sizeof( uint8_t*);

		static constexpr size_t localStorageSize = 4;
		struct FirstHeader : public IndexPageHeader
		{
			uint8_t* firstPages[ localStorageSize ];
		};
		static_assert( sizeof( IndexPageHeader ) + sizeof( uint8_t* ) * localStorageSize == sizeof(FirstHeader) );
		FirstHeader firstHeader;

		class ReadIter
		{
			friend class VectorOfPages;
			IndexPageHeader* ip = nullptr;
			const uint8_t* page;
			size_t totalSz;
			size_t sizeRemainingInBlock;
			size_t idxInIndexPage;
			ReadIter( IndexPageHeader* ip_, const uint8_t* page_, size_t sz ) : ip( ip_ ), page( page_ ), totalSz( sz )
			{
				sizeRemainingInBlock = sz <= GlobalPagePool::pageSize ? sz : GlobalPagePool::pageSize;
				idxInIndexPage = 0;
			}
		public:
			size_t availableSize() {return sizeRemainingInBlock;}
			const uint8_t* read( size_t sz )
			{
				NODECPP_ASSERT( nodecpp::foundation::module_id, nodecpp::assert::AssertLevel::pedantic, sz <= sizeRemainingInBlock );
				NODECPP_ASSERT( nodecpp::foundation::module_id, nodecpp::assert::AssertLevel::pedantic, ip != nullptr );
				sizeRemainingInBlock -= sz;
				totalSz -= sz;
				const uint8_t* ret = page;
				if ( totalSz && sizeRemainingInBlock == 0 )
				{
					if ( idxInIndexPage + 1 >= ip->usedCnt )
					{
						ip = ip->next;
						idxInIndexPage = 0;
						page = ip->pages()[idxInIndexPage];
					}
					else
						page = ip->pages()[++idxInIndexPage];
					sizeRemainingInBlock = totalSz <= GlobalPagePool::pageSize ? totalSz : GlobalPagePool::pageSize;
				}
				else
					page += sz;
				return ret;
			}
		};

		size_t pageCnt = 0; // payload pages (es not include index pages)
		IndexPageHeader* lastIndexPage = nullptr;
		uint8_t* currentPage = nullptr;
		size_t totalSz = 0;

		size_t offsetInCurrentPage() { return totalSz & ( GlobalPagePool::pageSize - 1 ); }
		size_t remainingSizeInCurrentPage() { return GlobalPagePool::pageSize - offsetInCurrentPage(); }
		void implReleaseAllPages()
		{
			size_t i;
			for ( i=0; i<localStorageSize && i<pageCnt; ++i )
				threadLocalPagePool.releasePage( firstHeader.firstPages[i] );
			pageCnt -= i;
			if ( pageCnt )
			{
				NODECPP_ASSERT( nodecpp::foundation::module_id, nodecpp::assert::AssertLevel::pedantic, firstHeader.next != nullptr );
				while ( firstHeader.next != nullptr )
				{
					NODECPP_ASSERT( nodecpp::foundation::module_id, nodecpp::assert::AssertLevel::pedantic, pageCnt > localStorageSize || firstHeader.next->next == nullptr );
					NODECPP_ASSERT( nodecpp::foundation::module_id, nodecpp::assert::AssertLevel::pedantic, firstHeader.next->usedCnt <= pageCnt );
					for ( size_t i=0; i<firstHeader.next->usedCnt; ++i )
						threadLocalPagePool.releasePage( firstHeader.next->pages()[i] );
					pageCnt -= firstHeader.next->usedCnt;
					uint8_t* page = reinterpret_cast<uint8_t*>(firstHeader.next);
					firstHeader.next = firstHeader.next->next;
					threadLocalPagePool.releasePage( page );
				}
			}
		}

		void implAddPage() // TODO: revise
		{
			NODECPP_ASSERT( nodecpp::foundation::module_id, nodecpp::assert::AssertLevel::pedantic, currentPage == nullptr );
			NODECPP_ASSERT( nodecpp::foundation::module_id, nodecpp::assert::AssertLevel::pedantic, offsetInCurrentPage() == 0 );
			if ( pageCnt < localStorageSize )
			{
				NODECPP_ASSERT( nodecpp::foundation::module_id, nodecpp::assert::AssertLevel::pedantic, firstHeader.next == nullptr );
				NODECPP_ASSERT( nodecpp::foundation::module_id, nodecpp::assert::AssertLevel::pedantic, lastIndexPage == nullptr );
				firstHeader.firstPages[pageCnt] = threadLocalPagePool.acquirePage();
				currentPage = firstHeader.firstPages[pageCnt];
				++(firstHeader.usedCnt);
			}
			else if ( lastIndexPage == nullptr )
			{
				NODECPP_ASSERT( nodecpp::foundation::module_id, nodecpp::assert::AssertLevel::pedantic, firstHeader.next == nullptr );
				firstHeader.next = lastIndexPage = reinterpret_cast<IndexPageHeader*>(threadLocalPagePool.acquirePage());
				currentPage = threadLocalPagePool.acquirePage();
				lastIndexPage->init( currentPage );
			}
			else if ( lastIndexPage->usedCnt == maxAddressedByPage )
			{
				NODECPP_ASSERT( nodecpp::foundation::module_id, nodecpp::assert::AssertLevel::pedantic, firstHeader.next != nullptr );
				IndexPageHeader* nextip = reinterpret_cast<IndexPageHeader*>(threadLocalPagePool.acquirePage());
				currentPage = threadLocalPagePool.acquirePage();
				nextip->init( currentPage );
				lastIndexPage->next = nextip;
				lastIndexPage = nextip;
			}
			else
			{
				NODECPP_ASSERT( nodecpp::foundation::module_id, nodecpp::assert::AssertLevel::pedantic, firstHeader.next != nullptr );
				NODECPP_ASSERT( nodecpp::foundation::module_id, nodecpp::assert::AssertLevel::pedantic, lastIndexPage->usedCnt < maxAddressedByPage );
				currentPage = threadLocalPagePool.acquirePage();
				lastIndexPage->pages()[lastIndexPage->usedCnt] = currentPage;
				++(lastIndexPage->usedCnt);
			}
			++pageCnt;
		}

	public:
		VectorOfPages() { memset( firstHeader.firstPages, 0, sizeof( firstHeader.firstPages ) ); }
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
				size_t remainingInPage = remainingSizeInCurrentPage();
				if ( sz <= remainingInPage )
				{
					memcpy( currentPage + offsetInCurrentPage(), buff, sz );
					totalSz += sz;
					if( sz == remainingInPage )
					{
						NODECPP_ASSERT( nodecpp::foundation::module_id, nodecpp::assert::AssertLevel::pedantic, offsetInCurrentPage() == 0 );
						currentPage = nullptr;
					}
					break;
				}
				else
				{
					memcpy( currentPage + offsetInCurrentPage(), buff, remainingInPage );
					sz -= remainingInPage;
					buff += remainingInPage;
					totalSz += remainingInPage;
					NODECPP_ASSERT( nodecpp::foundation::module_id, nodecpp::assert::AssertLevel::pedantic, offsetInCurrentPage() == 0 );
					currentPage = nullptr;
				}
			}
		}

		ReadIter getReadIter()
		{
			return ReadIter( &firstHeader, firstHeader.pages()[0], totalSz );
		}

		void clear() 
		{
			implReleaseAllPages();
			NODECPP_ASSERT( nodecpp::foundation::module_id, nodecpp::assert::AssertLevel::pedantic, pageCnt == 0 );
			firstHeader.next = nullptr;
			lastIndexPage = nullptr;
			currentPage = nullptr;
			totalSz = 0;
		}
		~VectorOfPages() { implReleaseAllPages(); }
	};

} // nodecpp


#endif // VECTOR_OF_PAGES_H