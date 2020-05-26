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

#ifdef _MSC_VER
#include <windows.h>
#else
#include <time.h>
#endif

#include "../include/log.h"
#include <chrono>

namespace nodecpp::logging_impl {
	using namespace nodecpp::log;

	thread_local ::nodecpp::log::Log* currentLog = nullptr;
	thread_local size_t instanceId = invalidInstanceID;
	
	uint64_t getCurrentTime()
	{
	#ifdef _MSC_VER
		return GetTickCount64() * 1000; // mks
	#else
		struct timespec ts;
	//    timespec_get(&ts, TIME_UTC);
		clock_gettime(CLOCK_MONOTONIC, &ts);
		return (uint64_t)ts.tv_sec * 1000000 + ts.tv_nsec / 1000; // mks
	#endif
	}

	class LogWriter
	{
		LogBufferBaseData* logData;

		void justWrite( uint64_t start, uint64_t end )
		{
			size_t startoff = start & (logData->buffSize - 1);
			size_t endoff = end & (logData->buffSize - 1);
			if ( endoff > startoff )
				fwrite( logData->buff + startoff, 1, endoff - startoff, logData->target );
			else
			{
				fwrite( logData->buff + startoff, 1, logData->buffSize - startoff, logData->target );
				fwrite( logData->buff, 1, endoff, logData->target );
			}
		}

		void runLoop_()
		{
			for (;;)
			{
				std::unique_lock<std::mutex> lock(logData->mx);
				while ( ( logData->action == LogBufferBaseData::Action::proceed && logData->end == logData->start && logData->mustBeWrittenImmediately <= logData->start && logData->firstToRelease == nullptr && logData->firstToReleaseGuaranteed == nullptr ) ||
						( ( logData->action == LogBufferBaseData::Action::proceedToTermination || logData->action == LogBufferBaseData::Action::terminationAllowed ) && logData->end == logData->start && logData->firstToRelease == nullptr && logData->firstToReleaseGuaranteed == nullptr ) )
//					logData->waitWriter.wait(lock);
					logData->waitWriter.wait_for(lock, std::chrono::milliseconds(200));
				lock.unlock();

				// there are two things we can do here:
				// 1. write data of the amount exceeding some threshold or required to be written immediately
				// 2. release a thread waiting for free size in buffer (if any) or guaranteed write

				uint64_t start;
				uint64_t end;
				ChainedWaitingData* p = nullptr;
				ChainedWaitingForGuaranteedWrite* gww = nullptr;
				uint64_t mustBeWrittenImmediately = 0;
				{
					std::unique_lock<std::mutex> lock(logData->mx);
					mustBeWrittenImmediately = logData->mustBeWrittenImmediately;
					start = logData->start;
					end = logData->end;
					logData->writerPromisedNextStart = end;

					if ( logData->firstToReleaseGuaranteed != nullptr )
					{
						NODECPP_ASSERT( foundation::module_id, ::nodecpp::assert::AssertLevel::critical, mustBeWrittenImmediately > start );
						gww = logData->firstToReleaseGuaranteed;
						logData->firstToReleaseGuaranteed = nullptr;
						logData->nextToAddGuaranteed = nullptr;
					}

					if ( logData->firstToRelease != nullptr && logData->availableSize() >= LogBufferBaseData::maxMessageSize )
					{
						p = logData->firstToRelease;
						logData->firstToRelease = nullptr;
					}

				} // unlocking

				
				if ( gww ) // perform guaranteed writing, if necessary, and let waiting threads go
				{
					NODECPP_ASSERT( foundation::module_id, ::nodecpp::assert::AssertLevel::critical, p == nullptr );
					NODECPP_ASSERT( foundation::module_id, ::nodecpp::assert::AssertLevel::critical, mustBeWrittenImmediately > start );
					NODECPP_ASSERT( foundation::module_id, ::nodecpp::assert::AssertLevel::critical, mustBeWrittenImmediately <= end );
					justWrite( start, end );
					fflush( logData->target );
					{
						std::unique_lock<std::mutex> lock(logData->mx);
						logData->start = end;
						logData->writerPromisedNextStart = end;
						start = end;
					} // unlocking
					while ( gww )
					{
						{
							std::unique_lock<std::mutex> lock(gww->mx);
							NODECPP_ASSERT( foundation::module_id, ::nodecpp::assert::AssertLevel::critical, !gww->canRun );
							gww->canRun = true;
						}
						auto tmp = gww->next;
						gww->w.notify_one();
						gww = tmp; // yes, former gww could now be destroyed/reused in the respective thread
					}
				}
				else
				{
					justWrite( start, end );
					{
						std::unique_lock<std::mutex> lock(logData->mx);
						logData->start = end;
						start = end;
						logData->writerPromisedNextStart = end;
					} // unlocking
				}

				if ( p )
				{
					{
						std::unique_lock<std::mutex> lock(p->mx);
						NODECPP_ASSERT( foundation::module_id, ::nodecpp::assert::AssertLevel::critical, !p->canRun );
						p->canRun = true;
					}
					p->w.notify_one();
					if ( start == end )
						continue; // Note: we let it to add some data, and, therefore, we won't be in an infinite waiting loop in the beginning of this processing cycle
				}

				if ( logData->_writerOnly_TerminateIfAlone() )
					return;
			}
		}

	public:
		LogWriter( LogBufferBaseData* logData_ ) : logData( logData_ ) {}

		void runLoop()
		{
			NODECPP_ASSERT( foundation::module_id, ::nodecpp::assert::AssertLevel::critical, logData != nullptr );
			logData->addRef();
			try { runLoop_(); }
			catch ( ... ) {}
			size_t refCtr = logData->removeRef();
			if ( refCtr == 0 && logData != nullptr ) 
			{
				logData->deinit();
			}
		}
	};

	void logWriterThreadMain( void* pdata )
	{
		LogWriter writer( reinterpret_cast<::nodecpp::log::LogBufferBaseData*>( pdata ) );
		writer.runLoop();
	}

	void createLogWriterThread( ::nodecpp::log::LogBufferBaseData* data )
	{
		NODECPP_ASSERT( foundation::module_id, ::nodecpp::assert::AssertLevel::critical, data != nullptr ); 
		nodecpp::log::default_log::info("about to start LogWriterThread..." );
		std::thread t( logWriterThreadMain, (void*)(data) );
		t.detach();
	}

} // nodecpp::logging_impl

namespace nodecpp::log {
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
