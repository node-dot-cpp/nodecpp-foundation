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

namespace nodecpp::platform::internal_msg { 

	constexpr size_t pageSize = 0x1000;
	constexpr size_t pageSizeExp = 12;
	static_assert( (1<<pageSizeExp) == pageSize );

	///////  page_ptr_and_data

	struct generic_page_ptr_and_data_ {
	private:
		uint8_t* page_;
		size_t data_;

	public:
	#ifdef NODECPP_X64
		static constexpr size_t max_data_exp = pageSizeExp + 16;
	#else
		static constexpr size_t max_data_exp = pageSizeExp;
	#endif

		generic_page_ptr_and_data_() {init();}
		generic_page_ptr_and_data_( void* page, size_t data) { init(page, data); }
		generic_page_ptr_and_data_( const generic_page_ptr_and_data_& other ) { page_ = other.page_; data_ = other.data_; }
		generic_page_ptr_and_data_& operator =( const generic_page_ptr_and_data_& other ) { page_ = other.page_; data_ = other.data_; return *this; }
		generic_page_ptr_and_data_( generic_page_ptr_and_data_&& other ) { page_ = other.page_; data_ = other.data_; }
		generic_page_ptr_and_data_& operator =( generic_page_ptr_and_data_&& other ) { page_ = other.page_; data_ = other.data_; return *this; }
		uint8_t* page() { return page_; }
		size_t data() { return data_; }
		void init() {page_ = nullptr; data_ = 0;}
		void init( void* page, size_t data) {page_ = reinterpret_cast<uint8_t*>(page); data_ = data;}
	};

#ifdef NODECPP_X64
	struct optimized_page_ptr_and_data_64_ {
	private:
		uintptr_t ptr;

	public:
		static constexpr size_t max_data_exp = pageSizeExp + 16;
		static constexpr size_t upper_data_offset = 48;
		static constexpr uint64_t data_low_mask = (1 << pageSizeExp) - 1;
		static constexpr uint64_t data_high_mask = 0xFFFF000000000000ULL;
		static constexpr uint64_t page_ptr_mask = 0xFFFFFFFFFFFFULL & ~data_low_mask;
		static_assert( (data_low_mask | data_high_mask | page_ptr_mask) == ~((uint64_t)0) );
		static_assert( (data_low_mask & page_ptr_mask) == 0 );
		static_assert( (data_high_mask & page_ptr_mask) == 0 );

		optimized_page_ptr_and_data_64_() {init();}
		optimized_page_ptr_and_data_64_( void* page, size_t data ) {init( page, data );}
		optimized_page_ptr_and_data_64_( const optimized_page_ptr_and_data_64_& other ) { ptr = other.ptr; }
		optimized_page_ptr_and_data_64_& operator =( const optimized_page_ptr_and_data_64_& other ) { ptr = other.ptr; return *this; }
		optimized_page_ptr_and_data_64_( optimized_page_ptr_and_data_64_&& other ) { ptr = other.ptr; }
		optimized_page_ptr_and_data_64_& operator =( optimized_page_ptr_and_data_64_&& other ) { ptr = other.ptr; return *this; }
		uint8_t* page() { return reinterpret_cast<uint8_t*>( ptr & page_ptr_mask ); }
		size_t data() { return (ptr & data_low_mask) | ((ptr & data_high_mask) >> (upper_data_offset - pageSizeExp)); }
		void init() {ptr = 0;}
		void init( void* page, size_t data )
		{
			NODECPP_ASSERT( nodecpp::foundation::module_id, nodecpp::assert::AssertLevel::critical, ( ((uintptr_t)page) & ((1<<pageSizeExp)-1) ) == 0 ); 
			NODECPP_ASSERT( nodecpp::foundation::module_id, nodecpp::assert::AssertLevel::critical, data < (1<<max_data_exp) ); 
			ptr = (uintptr_t)page & page_ptr_mask; 
			ptr |= data & data_low_mask;
			ptr |= (data >> pageSizeExp) << upper_data_offset;
		}
	};

#else
	using page_ptr_and_data = generic_page_ptr_and_data_<dataminsize, nflags>; // TODO: consider writing optimized version for other platforms
#endif // NODECPP_X64

#ifdef NODECPP_DECLARE_PTR_STRUCTS_AS_OPTIMIZED
	using page_ptr_and_data = optimized_page_ptr_and_data_64_;
#elif defined NODECPP_DECLARE_PTR_STRUCTS_AS_GENERIC
	using page_ptr_and_data = ::nodecpp::platform::ptrwithdatastructsdefs::generic_page_ptr_and_data_<dataminsize, nflags>;
#else
	#error Unsupported configuration
#endif // NODECPP_DECLARE_PTR_STRUCTS_AS_OPTIMIZED



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
		page_ptr_and_data firstPage;
		size_t pageCnt = 0;
		static constexpr size_t highWatermark = 16;
	public:
		GlobalPagePool() {}
		page_ptr_and_data acquirePage() {
			// TODO: thread-sync
			NODECPP_ASSERT( nodecpp::foundation::module_id, nodecpp::assert::AssertLevel::pedantic, ( pageCnt == 0 && firstPage.page() == nullptr ) || ( pageCnt != 0 && firstPage.page() != nullptr ) );
			page_ptr_and_data page;
			if ( pageCnt != 0 )
			{
				page = firstPage;
				firstPage.init(firstPage.page(), 0);
				*reinterpret_cast<uint8_t**>(firstPage.page()) = nullptr;
				--pageCnt;
			}
			else
				page.init( VirtualMemory::allocate( pageSize ), 0 );
			return page;
		}
		void releasePage( page_ptr_and_data page ) {
			// TODO: thread-sync
			NODECPP_ASSERT( nodecpp::foundation::module_id, nodecpp::assert::AssertLevel::pedantic, ( pageCnt == 0 && firstPage.page() == nullptr ) || ( pageCnt != 0 && firstPage.page() != nullptr ) );
			if ( pageCnt < highWatermark )
			{
				*reinterpret_cast<uint8_t**>(page.page()) = firstPage.page();
				firstPage = page;
				++pageCnt;
			}
			else
				VirtualMemory::deallocate( page.page(), pageSize );
		}
	};
	extern GlobalPagePool globalPagePool;

#if 0
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
#endif // 0

	struct PagePtrWrapper
	{
	private:
		uint8_t* ptr;
	public:
		PagePtrWrapper() {init();}
		PagePtrWrapper( void* page ) {init( page );}
		PagePtrWrapper( const PagePtrWrapper& other ) { ptr = other.ptr; }
		PagePtrWrapper& operator =( const PagePtrWrapper& other ) { ptr = other.ptr; return *this; }
		PagePtrWrapper( PagePtrWrapper&& other ) { ptr = other.ptr; }
		PagePtrWrapper& operator =( PagePtrWrapper&& other ) { ptr = other.ptr; return *this; }
		uint8_t* page() { return ptr; }
		void init() {ptr = 0;}
		void init( void* page ) {ptr = reinterpret_cast<uint8_t*>(page); }
	};

	using PagePointer = PagePtrWrapper;
//	using PagePointer = page_ptr_and_data;

	class MallocPageProvider // used just for interface purposes
	{
	public:
		static PagePointer acquirePage() { return PagePointer( VirtualMemory::allocate( BasePageBlockHedaer::pageSize ) ); }
		static void releasePage( PagePointer page ) { VirtualMemory::deallocate( page.page(), BasePageBlockHedaer::pageSize ); }
	};

	class InternalMsg
	{
		PagePointer implAcquirePageWrapper()
		{
			return MallocPageProvider::acquirePage();
			//return globalPagePool.acquirePage();
		}
		void implReleasePageWrapper( PagePointer page )
		{
			MallocPageProvider::releasePage( page );
			//globalPagePool.releasePage( page );
		}
		struct IndexPageHeader
		{
			PagePointer next_;
			size_t usedCnt;
			PagePointer* pages() { return reinterpret_cast<PagePointer*>(this+1); }
			void init() { next_.init(); usedCnt = 0; pages()[0].init();}
			void init(PagePointer page) { next_.init(); usedCnt = 1; pages()[0] = page;}
			void setNext( PagePointer n ) { next_ = n; }
			IndexPageHeader* next() { return reinterpret_cast<IndexPageHeader*>( next_.page() ); }
		};
		static constexpr size_t maxAddressedByPage = ( GlobalPagePool::pageSize - sizeof( IndexPageHeader ) ) / sizeof( uint8_t*);

		static constexpr size_t localStorageSize = 4;
		struct FirstHeader : public IndexPageHeader
		{
			PagePointer firstPages[ localStorageSize ];
		};
		static_assert( sizeof( IndexPageHeader ) + sizeof( PagePointer ) * localStorageSize == sizeof(FirstHeader) );
		FirstHeader firstHeader;

		class ReadIter
		{
			friend class InternalMsg;
			IndexPageHeader* ip;
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
						ip = ip->next();
						idxInIndexPage = 0;
						page = ip->pages()[idxInIndexPage].page();
					}
					else
						page = ip->pages()[++idxInIndexPage].page();
					sizeRemainingInBlock = totalSz <= GlobalPagePool::pageSize ? totalSz : GlobalPagePool::pageSize;
				}
				else
					page += sz;
				return ret;
			}
		};

		size_t pageCnt = 0; // payload pages (es not include index pages)
		PagePointer lip;
		IndexPageHeader* lastIndexPage() { return reinterpret_cast<IndexPageHeader*>( lip.page() ); }
		PagePointer currentPage;
		size_t totalSz = 0;

		size_t offsetInCurrentPage() { return totalSz & ( GlobalPagePool::pageSize - 1 ); }
		size_t remainingSizeInCurrentPage() { return GlobalPagePool::pageSize - offsetInCurrentPage(); }
		void implReleaseAllPages()
		{
			size_t i;
			for ( i=0; i<localStorageSize && i<pageCnt; ++i )
				implReleasePageWrapper( firstHeader.firstPages[i] );
			pageCnt -= i;
			if ( pageCnt )
			{
				NODECPP_ASSERT( nodecpp::foundation::module_id, nodecpp::assert::AssertLevel::pedantic, firstHeader.next() != nullptr );
				while ( firstHeader.next() != nullptr )
				{
					NODECPP_ASSERT( nodecpp::foundation::module_id, nodecpp::assert::AssertLevel::pedantic, pageCnt > localStorageSize || firstHeader.next()->next() == nullptr );
					NODECPP_ASSERT( nodecpp::foundation::module_id, nodecpp::assert::AssertLevel::pedantic, firstHeader.next()->usedCnt <= pageCnt );
					for ( size_t i=0; i<firstHeader.next()->usedCnt; ++i )
						implReleasePageWrapper( firstHeader.next()->pages()[i] );
					pageCnt -= firstHeader.next()->usedCnt;
//					PagePointer page = *reinterpret_cast<PagePointer*>(firstHeader.next);
					PagePointer page = firstHeader.next_;
					firstHeader.next_ = firstHeader.next()->next_;
					implReleasePageWrapper( page );
				}
			}
		}

		void implAddPage() // TODO: revise
		{
			NODECPP_ASSERT( nodecpp::foundation::module_id, nodecpp::assert::AssertLevel::pedantic, currentPage.page() == nullptr );
			NODECPP_ASSERT( nodecpp::foundation::module_id, nodecpp::assert::AssertLevel::pedantic, offsetInCurrentPage() == 0 );
			if ( pageCnt < localStorageSize )
			{
				NODECPP_ASSERT( nodecpp::foundation::module_id, nodecpp::assert::AssertLevel::pedantic, firstHeader.next() == nullptr );
				NODECPP_ASSERT( nodecpp::foundation::module_id, nodecpp::assert::AssertLevel::pedantic, lip.page() == nullptr );
				firstHeader.firstPages[pageCnt] = implAcquirePageWrapper();
				currentPage = firstHeader.firstPages[pageCnt];
				++(firstHeader.usedCnt);
			}
			else if ( lastIndexPage() == nullptr )
			{
				NODECPP_ASSERT( nodecpp::foundation::module_id, nodecpp::assert::AssertLevel::pedantic, firstHeader.next() == nullptr );
				lip = implAcquirePageWrapper();
				firstHeader.setNext(lip);
				currentPage = implAcquirePageWrapper();
				lastIndexPage()->init( currentPage );
//				lip = currentPage;
			}
			else if ( lastIndexPage()->usedCnt == maxAddressedByPage )
			{
				NODECPP_ASSERT( nodecpp::foundation::module_id, nodecpp::assert::AssertLevel::pedantic, firstHeader.next() != nullptr );
				PagePointer nextip_ = implAcquirePageWrapper();
				IndexPageHeader* nextip = reinterpret_cast<IndexPageHeader*>(nextip_.page());
				currentPage = implAcquirePageWrapper();
				nextip->init( currentPage );
				lastIndexPage()->next_ = nextip_;
				lip = nextip_;
			}
			else
			{
				NODECPP_ASSERT( nodecpp::foundation::module_id, nodecpp::assert::AssertLevel::pedantic, firstHeader.next() != nullptr );
				NODECPP_ASSERT( nodecpp::foundation::module_id, nodecpp::assert::AssertLevel::pedantic, lastIndexPage()->usedCnt < maxAddressedByPage );
				currentPage = implAcquirePageWrapper();
				lastIndexPage()->pages()[lastIndexPage()->usedCnt] = currentPage;
				++(lastIndexPage()->usedCnt);
			}
			++pageCnt;
		}

	public:
		InternalMsg() { firstHeader.init(); memset( firstHeader.firstPages, 0, sizeof( firstHeader.firstPages ) ); }
		void append( void* buff_, size_t sz )
		{
			uint8_t* buff = reinterpret_cast<uint8_t*>(buff_);
			while( sz != 0 )
			{
				if ( currentPage.page() == nullptr )
				{
					implAddPage();
					NODECPP_ASSERT( nodecpp::foundation::module_id, nodecpp::assert::AssertLevel::pedantic, currentPage.page() != nullptr );
				}
				size_t remainingInPage = remainingSizeInCurrentPage();
				if ( sz <= remainingInPage )
				{
					memcpy( currentPage.page() + offsetInCurrentPage(), buff, sz );
					totalSz += sz;
					if( sz == remainingInPage )
					{
						NODECPP_ASSERT( nodecpp::foundation::module_id, nodecpp::assert::AssertLevel::pedantic, offsetInCurrentPage() == 0 );
						currentPage.init();
					}
					break;
				}
				else
				{
					memcpy( currentPage.page() + offsetInCurrentPage(), buff, remainingInPage );
					sz -= remainingInPage;
					buff += remainingInPage;
					totalSz += remainingInPage;
					NODECPP_ASSERT( nodecpp::foundation::module_id, nodecpp::assert::AssertLevel::pedantic, offsetInCurrentPage() == 0 );
					currentPage.init();
				}
			}
		}

		ReadIter getReadIter()
		{
			return ReadIter( &firstHeader, firstHeader.pages()[0].page(), totalSz );
		}

		void clear() 
		{
			implReleaseAllPages();
			NODECPP_ASSERT( nodecpp::foundation::module_id, nodecpp::assert::AssertLevel::pedantic, pageCnt == 0 );
			firstHeader.next_.init();
			lip.init();
			currentPage.init();
			totalSz = 0;
		}
		~InternalMsg() { implReleaseAllPages(); }
	};

} // nodecpp


#endif // VECTOR_OF_PAGES_H