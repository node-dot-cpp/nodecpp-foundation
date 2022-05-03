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
#include "nodecpp_assert.h"

namespace nodecpp::logging_impl {
	using namespace nodecpp::log;

	// TODO: gather all thread local data in a single structure
	thread_local ::nodecpp::log::Log* currentLog = nullptr;
	thread_local size_t instanceId = invalidInstanceID;
	thread_local uint64_t lastTimeReported;
	thread_local size_t ordinalSinceLastReportedTime;
	
	LoggingTimeStamp getCurrentTimeStamp()
	{
		uint64_t ret = 0;
	#ifdef _MSC_VER
		ret = GetTickCount64(); // ms
	#else
		struct timespec ts;
	//    timespec_get(&ts, TIME_UTC);
		clock_gettime(CLOCK_MONOTONIC, &ts);
		ret = (uint64_t)ts.tv_sec * 1000 + ts.tv_nsec / 1000000; // ms
	#endif
		if ( ret != lastTimeReported )
		{
			lastTimeReported = ret;
			ordinalSinceLastReportedTime = 0;
		}
		else
		{
			++ordinalSinceLastReportedTime;
		}
		LoggingTimeStamp lts;
		lts.t = ret * 1000 + ordinalSinceLastReportedTime;
		return lts;
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
				std::unique_lock<std::mutex> lock1(logData->mx);
				while ( ( logData->action == LogBufferBaseData::Action::proceed && logData->end == logData->start && logData->mustBeWrittenImmediately <= logData->start && logData->firstToRelease == nullptr && logData->firstToReleaseGuaranteed == nullptr ) ||
						( ( logData->action == LogBufferBaseData::Action::proceedToTermination || logData->action == LogBufferBaseData::Action::terminationAllowed ) && logData->end == logData->start && logData->firstToRelease == nullptr && logData->firstToReleaseGuaranteed == nullptr ) )
//					logData->waitWriter.wait(lock1);
					logData->waitWriter.wait_for(lock1, std::chrono::milliseconds(200));
				lock1.unlock();

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

	size_t SkippedMsgCounters::toStr( char* buff, size_t sz) {
		NODECPP_ASSERT( foundation::module_id, ::nodecpp::assert::AssertLevel::critical, sz >= reportMaxSize ); 
		memcpy( buff, "<skipped: ", 9 );
		size_t pos = 9;
		bool added = false;
		for ( size_t i=0; i<log_level_count; ++i )
			if ( skippedCtrs[i] && pos < sz )					
			{
				auto r = ::fmt::format_to_n( buff + pos, sz - pos, "{}:{}, ", LogLevelNames[i], skippedCtrs[i] );
				pos += r.size;
				added = true;
			}
		if ( added )
		{
			buff[pos - 2] = '>';
			buff[pos - 1] = '\n';
			return pos;
		}
		else
		{
			NODECPP_ASSERT( foundation::module_id, ::nodecpp::assert::AssertLevel::critical, fullCnt_ == 0, "indeed: {}", fullCnt_ ); 
			return 0;
		}
	}

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

	void LogTransport::insertSingleMsg( const char* msg, size_t sz ) // under lock
	{
		NODECPP_ASSERT( foundation::module_id, ::nodecpp::assert::AssertLevel::critical, sz <= logData->availableSize() );
		size_t endoff = logData->end & (logData->buffSize - 1);
		if ( logData->buffSize - endoff >= sz )
		{
			memcpy( logData->buff + endoff, msg, sz );
		}
		else
		{
			memcpy( logData->buff + endoff, msg, logData->buffSize - endoff );
			memcpy( logData->buff, msg + logData->buffSize - endoff, sz - (logData->buffSize - endoff) );
		}
		logData->end += sz;
	}

	void LogTransport::insertMessage( const char* msg, size_t sz, SkippedMsgCounters& ctrs, bool isCritical ) // under lock
	{
		if ( ctrs.fullCount() )
		{
			char b[SkippedMsgCounters::reportMaxSize];
			size_t bsz = ctrs.toStr( b, SkippedMsgCounters::reportMaxSize );
			static_assert( SkippedMsgCounters::reportMaxSize <= LogBufferBaseData::skippedCntMsgSz );
			insertSingleMsg( b, bsz );
			ctrs.clear();
		}
		insertSingleMsg( msg, sz );
		if ( isCritical || logData->action == LogBufferBaseData::Action::proceedToTermination )
		{
			logData->mustBeWrittenImmediately = logData->end;
			NODECPP_ASSERT( foundation::module_id, ::nodecpp::assert::AssertLevel::critical, !gww.canRun );
			gww.next = nullptr;
			if ( logData->nextToAddGuaranteed == nullptr ) 
			{
				NODECPP_ASSERT( foundation::module_id, ::nodecpp::assert::AssertLevel::critical, logData->firstToReleaseGuaranteed == nullptr ); 
				logData->firstToReleaseGuaranteed = &gww;
				logData->nextToAddGuaranteed = &gww;
			}
			else
			{
				NODECPP_ASSERT( foundation::module_id, ::nodecpp::assert::AssertLevel::critical, logData->firstToReleaseGuaranteed != nullptr ); 
				logData->nextToAddGuaranteed->next = &gww;
				logData->nextToAddGuaranteed = &gww;
			}
			logData->waitWriter.notify_one();
		}
		else if ( logData->end - logData->start < logData->maxMessageSize )
		{
			logData->waitWriter.notify_one();
		}
	}

	bool LogTransport::addMsg( const char* msg, size_t sz, LogLevel l )
	{
		bool isCritical = l <= logData->levelGuaranteedWrite;
		bool wait4critialWrt = false;
		bool waitAgain = false;
		ChainedWaitingData d;
		{
			std::unique_lock<std::mutex> lock(logData->mx);
			size_t fullSzRequired = logData->skippedCtrs.fullCount() == 0 ? sz : sz + logData->skippedCntMsgSz;
			if ( logData->nextToAdd != nullptr)
			{
				NODECPP_ASSERT( foundation::module_id, ::nodecpp::assert::AssertLevel::critical, logData->firstToRelease != nullptr ); 
				if (l >= logData->levelCouldBeSkipped)
				{
					logData->nextToAdd->skippedCtrs.increment(l);
					return false;
				}
				else
				{
					logData->nextToAdd->next = &d;
					logData->nextToAdd = &d;
					waitAgain = true;
				}
			}
			else if ( logData->end + fullSzRequired <= logData->start + logData->buffSize ) // can copy
			{
				if ( isCritical )
					wait4critialWrt = true;
				insertMessage( msg, sz, logData->skippedCtrs, isCritical );
			}
			else
			{
				if ( l >= logData->levelCouldBeSkipped ) // skip
				{
					logData->skippedCtrs.increment(l);
					return false;
				}
				else // add to waiting list
				{
					NODECPP_ASSERT( foundation::module_id, ::nodecpp::assert::AssertLevel::critical, logData->nextToAdd == nullptr );
					NODECPP_ASSERT( foundation::module_id, ::nodecpp::assert::AssertLevel::critical, logData->firstToRelease == nullptr ); 
					logData->firstToRelease = &d;
//						d.canRun = true;
					logData->nextToAdd = &d;
					waitAgain = true;
				}
			}
		} // unlocking

		if ( wait4critialWrt )
		{
			NODECPP_ASSERT( foundation::module_id, ::nodecpp::assert::AssertLevel::critical, !waitAgain );
			std::unique_lock<std::mutex> lock(gww.mx);
			while (!gww.canRun)
				gww.w.wait(lock);
			gww.canRun = false;
			lock.unlock();
			return true;
		}

		while ( waitAgain )
		{
			waitAgain = false;
			std::unique_lock<std::mutex> lock1(d.mx);
			while (!d.canRun)
				d.w.wait(lock1);
			d.canRun = false;
			lock1.unlock();

			{
				std::unique_lock<std::mutex> lock(logData->mx);

				size_t fullSzRequired = logData->skippedCtrs.fullCount() == 0 ? sz : sz + logData->skippedCntMsgSz;
				if ( logData->end + fullSzRequired > logData->start + logData->buffSize )
				{
					NODECPP_ASSERT( foundation::module_id, ::nodecpp::assert::AssertLevel::critical, logData->firstToRelease == nullptr );
					logData->firstToRelease = &d;
					NODECPP_ASSERT( foundation::module_id, ::nodecpp::assert::AssertLevel::critical, logData->nextToAdd != nullptr );
					waitAgain = true;
					lock.unlock();
					continue;
				}

				insertMessage( msg, sz, d.skippedCtrs, l <= logData->levelGuaranteedWrite );

				if ( logData->nextToAdd == &d )
				{
					NODECPP_ASSERT( foundation::module_id, ::nodecpp::assert::AssertLevel::critical, d.next == nullptr ); 
					logData->nextToAdd = nullptr;
				}
				else
				{
					NODECPP_ASSERT( foundation::module_id, ::nodecpp::assert::AssertLevel::critical, d.next != nullptr ); 
				}
			} // unlocking

			if ( d.next )
			{
				{
					std::unique_lock<std::mutex> lock(d.next->mx);
					NODECPP_ASSERT( foundation::module_id, ::nodecpp::assert::AssertLevel::critical, !d.next->canRun );
					d.next->canRun = true;
				}
				d.next->w.notify_one();
			}
		}
		return true;
	}

}
