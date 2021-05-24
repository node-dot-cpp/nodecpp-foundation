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

#ifndef INTERNAL_MSG_H
#define INTERNAL_MSG_H

#include "foundation.h"
#include "page_allocator.h"
#ifdef NODECPP_WINDOWS
#include <intrin.h>
#endif

namespace nodecpp::platform::internal_msg { 

	constexpr size_t pageSize = 0x1000;
	constexpr size_t pageSizeExp = 12;
	static_assert( (1<<pageSizeExp) == pageSize );

	struct PagePtrWrapper
	{
	private:
		uint8_t* ptr;
	public:
		PagePtrWrapper() {init();}
		PagePtrWrapper( void* page ) {init( page );}
		PagePtrWrapper( const PagePtrWrapper& other ) = default;
		PagePtrWrapper& operator =( const PagePtrWrapper& other ) = default;
		PagePtrWrapper( PagePtrWrapper&& other ) = default;
		PagePtrWrapper& operator =( PagePtrWrapper&& other ) = default;
		~PagePtrWrapper() {}
		uint8_t* page() { return ptr; }
		void init() {ptr = 0;}
		void init( void* page ) {ptr = reinterpret_cast<uint8_t*>(page); }
	};

	using PagePointer = PagePtrWrapper;
//	using PagePointer = page_ptr_and_data;

	class MallocPageProvider // used just for interface purposes
	{
	public:
		static PagePointer acquirePage() { return PagePointer( ::malloc( pageSize ) ); }
		static void releasePage( PagePointer page ) { ::free( page.page() ); }
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
			IndexPageHeader() { init(); }
			IndexPageHeader( const IndexPageHeader& ) = delete;
			IndexPageHeader& operator = ( const IndexPageHeader& ) = delete;
			IndexPageHeader( IndexPageHeader&& other )
			{
				usedCnt = other.usedCnt;
				other.usedCnt = 0;
				next_ = other.next_;
				other.next_.init();
			}
			IndexPageHeader& operator = ( IndexPageHeader&& other )
			{
				usedCnt = other.usedCnt;
				other.usedCnt = 0;
				next_ = other.next_;
				other.next_.init();
				return *this;
			}
			void init() { next_.init(); usedCnt = 0; pages()[0].init();}
			void init(PagePointer page) { next_.init(); usedCnt = 1; pages()[0] = page;}
			void setNext( PagePointer n ) { next_ = n; }
			IndexPageHeader* next() { return reinterpret_cast<IndexPageHeader*>( next_.page() ); }
		};
		static constexpr size_t maxAddressedByPage = ( pageSize - sizeof( IndexPageHeader ) ) / sizeof( uint8_t*);

		static constexpr size_t localStorageSize = 4;
		struct FirstHeader : public IndexPageHeader
		{
			PagePointer firstPages[ localStorageSize ];
			FirstHeader() : IndexPageHeader() {}
			FirstHeader( const FirstHeader& ) = delete;
			FirstHeader& operator = ( const FirstHeader& ) = delete;
			FirstHeader( FirstHeader&& other ) : IndexPageHeader( std::move( other ) )
			{
				for ( size_t i=0; i<localStorageSize;++i )
				{
					firstPages[i] = other.firstPages[i];
					other.firstPages[i].init();
				}
			}
			FirstHeader& operator = ( FirstHeader&& other )
			{
				IndexPageHeader::operator = ( std::move( other ) );
				for ( size_t i=0; i<localStorageSize;++i )
				{
					firstPages[i] = other.firstPages[i];
					other.firstPages[i].init();
				}
				return *this;
			}
		};
		static_assert( sizeof( IndexPageHeader ) + sizeof( PagePointer ) * localStorageSize == sizeof(FirstHeader) );
		FirstHeader firstHeader;

		size_t pageCnt = 0; // payload pages (that is, not include index pages)
		PagePointer lip;
		IndexPageHeader* lastIndexPage() { return reinterpret_cast<IndexPageHeader*>( lip.page() ); }
		PagePointer currentPage;
		size_t totalSz = 0;

		size_t offsetInCurrentPage() { return totalSz & ( pageSize - 1 ); }
		size_t remainingSizeInCurrentPage() { return pageSize - offsetInCurrentPage(); }
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
		static constexpr size_t app_reserved = 8 * sizeof( void* );
		static constexpr size_t total_reserved = app_reserved + sizeof(FirstHeader) + 2 * sizeof( PagePointer ) + 2 * sizeof( size_t );

	public:
		class ReadIter
		{
			friend class InternalMsg;
			IndexPageHeader* ip;
			const uint8_t* page;
			size_t totalSz;
			size_t sizeRemainingInBlock;
			size_t idxInIndexPage;
			size_t currentOffset = 0;

		public:
			using BufferT = InternalMsg;

			ReadIter( IndexPageHeader* ip_, const uint8_t* page_, size_t sz ) : ip( ip_ ), page( page_ ), totalSz( sz )
			{
//				NODECPP_ASSERT( nodecpp::foundation::module_id, nodecpp::assert::AssertLevel::pedantic, sz >= total_reserved );
				sizeRemainingInBlock = sz <= pageSize - total_reserved ? sz : pageSize - total_reserved;
//				sizeRemainingInBlock = sz <= pageSize ? sz : pageSize;
				idxInIndexPage = 0;
			}
			void impl_skip( size_t sz )
			{
				NODECPP_ASSERT( nodecpp::foundation::module_id, nodecpp::assert::AssertLevel::pedantic, sz <= sizeRemainingInBlock );
				NODECPP_ASSERT( nodecpp::foundation::module_id, nodecpp::assert::AssertLevel::pedantic, ip != nullptr );
				sizeRemainingInBlock -= sz;
				totalSz -= sz;
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
					sizeRemainingInBlock = totalSz <= pageSize ? totalSz : pageSize;
				}
				else
					page += sz;
			}
		public:
			size_t directlyAvailableSize() {return sizeRemainingInBlock;}
			size_t totalAvailableSize() {return totalSz;}
			bool isData() { return totalSz != 0; }
			const uint8_t* directRead( size_t sz )
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
					sizeRemainingInBlock = totalSz <= pageSize ? totalSz : pageSize;
				}
				else
					page += sz;
				currentOffset += sz;
				return ret;
			}
			uint8_t operator * ()
			{
				NODECPP_ASSERT( nodecpp::foundation::module_id, nodecpp::assert::AssertLevel::pedantic, totalSz );
				return *page;
			}
			void operator ++ () 
			{
				impl_skip( 1 );
				++currentOffset;
			}
			size_t read( void* buff, size_t size )
			{
				uint8_t* buffer = reinterpret_cast<uint8_t*>(buff);
				NODECPP_ASSERT( nodecpp::foundation::module_id, nodecpp::assert::AssertLevel::pedantic, buff != nullptr );
				size_t asz = directlyAvailableSize();
				while ( asz )
				{
					if ( size <= asz )
					{
						memcpy( buffer, directRead( size ), size );
						buffer += size;
						break;
					}
					else
					{
						memcpy( buffer, directRead( asz ), asz );
						buffer += asz;
						size -= asz;
					}
					asz = directlyAvailableSize();
				}				
				return buffer - reinterpret_cast<uint8_t*>(buff); 
			}
			size_t skip( size_t size )
			{
				if ( size > totalSz )
					size = totalSz;
				size_t ret = size;
				size_t asz = directlyAvailableSize();
				while ( asz )
				{
					if ( size <= asz )
					{
						impl_skip( size );
						break;
					}
					else
					{
						impl_skip( asz );
						size -= asz;
					}
					asz = directlyAvailableSize();
				}
				currentOffset += size;
				return ret;
			}
			size_t offset() { return currentOffset; }
		};

		using ReadIteratorT = ReadIter;

		ReadIter getReadIter()
		{
//			return ReadIter( &firstHeader, firstHeader.pages()[0].page(), totalSz );
			NODECPP_ASSERT( nodecpp::foundation::module_id, nodecpp::assert::AssertLevel::pedantic, totalSz == 0 || totalSz >= total_reserved, "{} vs. {}", totalSz, total_reserved );
			if ( totalSz )
				return ReadIter( &firstHeader, firstHeader.pages()[0].page() + total_reserved, totalSz - total_reserved );
			else
				return ReadIter( &firstHeader, nullptr, 0 );
		}

		void reserveSpaceForConvertionToTag()
		{
			NODECPP_ASSERT( nodecpp::foundation::module_id, nodecpp::assert::AssertLevel::pedantic, currentPage.page() == nullptr );
			implAddPage();
			NODECPP_ASSERT( nodecpp::foundation::module_id, nodecpp::assert::AssertLevel::pedantic, currentPage.page() != nullptr );
			size_t remainingInPage = remainingSizeInCurrentPage();
			NODECPP_ASSERT( nodecpp::foundation::module_id, nodecpp::assert::AssertLevel::pedantic, total_reserved < remainingInPage, "{} vs. {}", total_reserved, remainingInPage );
			totalSz += total_reserved;
		}

		void impl_clear() 
		{
			implReleaseAllPages();
			NODECPP_ASSERT( nodecpp::foundation::module_id, nodecpp::assert::AssertLevel::pedantic, pageCnt == 0 );
			firstHeader.next_.init();
			lip.init();
			currentPage.init();
			totalSz = 0;
		}

	public:
		InternalMsg() { 
			firstHeader.init(); 
			memset( firstHeader.firstPages, 0, sizeof( firstHeader.firstPages ) );
		}
		InternalMsg( const InternalMsg& ) = delete;
		InternalMsg& operator = ( const InternalMsg& ) = delete;
		InternalMsg( InternalMsg&& other )
		{
			firstHeader = std::move( other.firstHeader );
			pageCnt = other.pageCnt;
			other.pageCnt = 0;
			lip = other.lip;
			other.lip.init();
			currentPage = other.currentPage;
			other.currentPage.init();
			totalSz = other.totalSz;
			other.totalSz = 0;
		}
		InternalMsg& operator = ( InternalMsg&& other )
		{
			if ( this == &other ) return *this;
			firstHeader = std::move( other.firstHeader );
			pageCnt = other.pageCnt;
			other.pageCnt = 0;
			lip = other.lip;
			other.lip.init();
			currentPage = other.currentPage;
			other.currentPage.init();
			totalSz = other.totalSz;
			other.totalSz = 0;
			return *this;
		}
		void appWriteData( void* data, size_t offset, size_t sz )
		{
			NODECPP_ASSERT( nodecpp::foundation::module_id, nodecpp::assert::AssertLevel::pedantic, totalSz == 0 || totalSz >= total_reserved, "{} vs. {}", totalSz, total_reserved );
			if ( totalSz );
			else
			{
				NODECPP_ASSERT( nodecpp::foundation::module_id, nodecpp::assert::AssertLevel::pedantic, currentPage.page() == nullptr );
				implAddPage();
			}
			NODECPP_ASSERT( nodecpp::foundation::module_id, nodecpp::assert::AssertLevel::pedantic, offset + sz <= app_reserved, "{} + {} vs. {}", offset, sz, app_reserved );
			uint8_t* appPrefix = firstHeader.pages()[0].page() + total_reserved - app_reserved;
			memcpy( appPrefix + offset, data, sz );
		}
		void appReadData( void* data, size_t offset, size_t sz )
		{
			NODECPP_ASSERT( nodecpp::foundation::module_id, nodecpp::assert::AssertLevel::pedantic, totalSz == 0 || totalSz >= total_reserved, "{} vs. {}", totalSz, total_reserved );
			NODECPP_ASSERT( nodecpp::foundation::module_id, nodecpp::assert::AssertLevel::pedantic, offset + sz <= app_reserved, "{} + {} vs. {}", offset, sz, app_reserved );
			NODECPP_ASSERT( nodecpp::foundation::module_id, nodecpp::assert::AssertLevel::pedantic, firstHeader.pages()[0].page() != nullptr );
			uint8_t* appPrefix = firstHeader.pages()[0].page() + total_reserved - app_reserved;
			memcpy( data, appPrefix + offset, sz );
		}
		InternalMsg* convertToPointer()
		{
			if ( totalSz )
			{
				NODECPP_ASSERT( nodecpp::foundation::module_id, nodecpp::assert::AssertLevel::pedantic, firstHeader.pages()[0].page() != nullptr );
				uint8_t* prefix = firstHeader.pages()[0].page();
				memcpy( prefix, this, sizeof( InternalMsg ) );
				memset( this, 0, sizeof( InternalMsg ) );
				return (InternalMsg*)prefix;
			}
			else
			{
				NODECPP_ASSERT( nodecpp::foundation::module_id, nodecpp::assert::AssertLevel::pedantic, firstHeader.pages()[0].page() == nullptr );
				return (InternalMsg*)nullptr;
			}
		}
		void restoreFromPointer(InternalMsg* ptr)
		{
			impl_clear();
			if ( ptr != nullptr )
			{
				reserveSpaceForConvertionToTag();
				uint8_t* prefix = firstHeader.pages()[0].page();
				memcpy( this, ptr, sizeof( InternalMsg ) );
			}
		}
		void append( const void* buff_, size_t sz )
		{
			const uint8_t* buff = reinterpret_cast<const uint8_t*>(buff_);
			while( sz != 0 )
			{
				if ( currentPage.page() == nullptr )
				{
					if ( totalSz )
						implAddPage();
					else
					{
						NODECPP_ASSERT( nodecpp::foundation::module_id, nodecpp::assert::AssertLevel::pedantic, currentPage.page() == nullptr );
						reserveSpaceForConvertionToTag();
					}
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
		void appendUint8( uint8_t what )
		{
			if ( currentPage.page() == nullptr )
			{
				if ( totalSz )
					implAddPage();
				else
				{
					NODECPP_ASSERT( nodecpp::foundation::module_id, nodecpp::assert::AssertLevel::pedantic, currentPage.page() == nullptr );
					reserveSpaceForConvertionToTag();
				}
				NODECPP_ASSERT( nodecpp::foundation::module_id, nodecpp::assert::AssertLevel::pedantic, currentPage.page() != nullptr );
			}
			size_t remainingInPage = remainingSizeInCurrentPage();
			NODECPP_ASSERT( nodecpp::foundation::module_id, nodecpp::assert::AssertLevel::pedantic, remainingInPage != 0 );
			*(currentPage.page() + offsetInCurrentPage()) = what;
			++totalSz;
			if( 1 == remainingInPage )
			{
				NODECPP_ASSERT( nodecpp::foundation::module_id, nodecpp::assert::AssertLevel::pedantic, offsetInCurrentPage() == 0 );
				currentPage.init();
			}
		}

		void clear() 
		{
			impl_clear();
		}

		size_t size() { 
			NODECPP_ASSERT( nodecpp::foundation::module_id, nodecpp::assert::AssertLevel::pedantic, totalSz == 0 || totalSz >= total_reserved, "{} vs. {}", totalSz, total_reserved );
			return totalSz != 0 ? totalSz - total_reserved : 0;
		}
		~InternalMsg() { implReleaseAllPages(); }
	};
	static_assert( sizeof( InternalMsg ) + InternalMsg::app_reserved <= InternalMsg::total_reserved );

} // nodecpp


#endif // INTERNAL_MSG_H