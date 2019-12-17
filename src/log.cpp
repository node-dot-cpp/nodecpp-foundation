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

#include "../include/log.h"

namespace nodecpp::logging_impl {

	class LogWriter
	{
		LogBufferBaseData* logData;

	public:
		LogWriter( LogBufferBaseData* logData_ ) : logData( logData_ ) {}

		void runLoop()
		{
			for (;;)
			{
				std::unique_lock<std::mutex> lock(logData->mx);
				while ( logData->action == LogBufferBaseData::Action::proceed && (logData->end & ~( logData->pageSize - 1)) == logData->start )
					logData->waitWriter.wait(lock);
				lock.unlock();

				if ( logData->action != LogBufferBaseData::Action::proceed )
				{
					// TODO: ...
					return;
				}

				NODECPP_ASSERT( foundation::module_id, ::nodecpp::assert::AssertLevel::critical, ( logData->start & ( logData->pageSize - 1) ) == 0 ); 

				// there are two things we can do here:
				// 1. flush one or more ready pages
				// 2. release a thread waiting for free size in buffer (if any)

				size_t pageRoundedEnd;
				ChainedWaitingData* p = nullptr;
				{
					std::unique_lock<std::mutex> lock(logData->mx);
					pageRoundedEnd = logData->end & ~( logData->pageSize - 1);
					logData->writerPromisedNextStart = pageRoundedEnd;

					if ( logData->start == pageRoundedEnd && logData->firstToRelease != nullptr )
					{
						p = logData->firstToRelease;
						logData->firstToRelease = nullptr;
					}
				} // unlocking
				if ( p )
				{
					{
						std::unique_lock<std::mutex> lock(p->mx);
						NODECPP_ASSERT( foundation::module_id, ::nodecpp::assert::AssertLevel::critical, !p->canRun );
						p->canRun = true;
					}
					p->w.notify_one();
					if ( logData->start == pageRoundedEnd )
						return;
				}

				while ( logData->start != pageRoundedEnd )
				{
					size_t startoff = logData->start & (logData->buffSize - 1);
					size_t endoff = pageRoundedEnd & (logData->buffSize - 1);
					if ( endoff > startoff )
						fwrite( logData->buff + startoff, 1, endoff - startoff, logData->target );
					else
					{
						fwrite( logData->buff + startoff, 1, logData->buffSize - startoff, logData->target );
						fwrite( logData->buff, 1, endoff, logData->target );
					}

					ChainedWaitingData* p = nullptr;
					{
						std::unique_lock<std::mutex> lock(logData->mx);
						logData->start = pageRoundedEnd;
						if ( logData->firstToRelease )
						{
							p = logData->firstToRelease;
							logData->firstToRelease = nullptr;
						}
						pageRoundedEnd = logData->end & ~( logData->pageSize - 1);
						logData->writerPromisedNextStart = pageRoundedEnd;
					} // unlocking
					if ( p )
					{
						{
							std::unique_lock<std::mutex> lock(p->mx);
							NODECPP_ASSERT( foundation::module_id, ::nodecpp::assert::AssertLevel::critical, !p->canRun );
							p->canRun = true;
						}
						p->w.notify_one();
					}
				}
			}
		}
	};

	void logWriterThreadMain( void* pdata )
	{
		LogWriter writer( reinterpret_cast<::nodecpp::LogBufferBaseData*>( pdata ) );
		writer.runLoop();
	}

	thread_local ::nodecpp::Log* currentLog = nullptr;
	
	void createLogWriterThread( ::nodecpp::LogBufferBaseData* data )
	{
		NODECPP_ASSERT( foundation::module_id, ::nodecpp::assert::AssertLevel::critical, data != nullptr ); 
		nodecpp::default_log::info("about to start LogWriterThread..." );
		std::thread t( logWriterThreadMain, (void*)(data) );
		t.detach();
	}

} // nodecpp::logging_impl

namespace nodecpp {
	void LogBufferBaseData::init( FILE* f )
	{
		NODECPP_ASSERT( foundation::module_id, ::nodecpp::assert::AssertLevel::critical, buff == nullptr ); 
		NODECPP_ASSERT( foundation::module_id, ::nodecpp::assert::AssertLevel::critical, target == nullptr ); 
		size_t memPageSz = VirtualMemory::getPageSize();
		if ( memPageSz <= 0x4000 )
		{
			pageSize = memPageSz;
			buffSize = pageSize * pageCount;
		}
		else // TODO: revise for Large pages!!!
		{
			pageSize = memPageSz / pageCount;
			buffSize = memPageSz;
		}
		buff = reinterpret_cast<uint8_t*>( VirtualMemory::allocate( buffSize ) );
		if ( buff == nullptr )
			throw;

		NODECPP_ASSERT( foundation::module_id, ::nodecpp::assert::AssertLevel::critical, ((pageSize-1)|pageSize)+1 == (pageSize<<1) ); 
		NODECPP_ASSERT( foundation::module_id, ::nodecpp::assert::AssertLevel::critical, ((pageCount-1)|pageCount)+1 == (pageCount<<1) ); 
		NODECPP_ASSERT( foundation::module_id, ::nodecpp::assert::AssertLevel::critical, ((pageSize-1)|pageSize)+1 == (pageSize<<1) ); 

		target = f;

		nodecpp::logging_impl::createLogWriterThread( this );
	}
}
