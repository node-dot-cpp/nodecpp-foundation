/* -------------------------------------------------------------------------------
* Copyright (c) 2019, OLogN Technologies AG
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

#ifndef NODECPP_LOGGING_H
#define NODECPP_LOGGING_H

#include "platform_base.h"
#include "foundation.h"
#include <fmt/format.h>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <vector>
#include "page_allocator.h"
#include "nodecpp_assert.h"


namespace nodecpp {

	class ModuleID
	{
		const char* str;
	public:
		ModuleID( const char* str_) : str( str_ ) {}
		ModuleID( const ModuleID& other ) : str( other.str ) {}
		ModuleID& operator = ( const ModuleID& other ) {str = other.str; return *this;}
		ModuleID( ModuleID&& other ) = delete;
		ModuleID& operator = ( ModuleID&& other ) = delete;
		const char* id() const { return str; }
	};
	
	enum class LogLevel { fatal = 0, err = 1, warning = 2, info = 3, debug = 4 };
	static constexpr const char* LogLevelNames[] = { "fatal", "err", "warning", "info", "debug", "" };
	constexpr size_t log_level_count = sizeof( LogLevelNames ) / sizeof( const char*) - 1;

	class Log;
	class LogTransport;

	struct SkippedMsgCounters
	{
		uint32_t skippedCtrs[log_level_count];
		uint32_t fullCnt_;
		static constexpr size_t reportMaxSize = 128;
		SkippedMsgCounters() {clear();}
		void clear() { memset( skippedCtrs, 0, sizeof( skippedCtrs ) ); fullCnt_ = 0; }
		void increment( LogLevel l ) { 
			++(skippedCtrs[(size_t)l]);
			++fullCnt_;
		}
		size_t fullCount() { return fullCnt_; }
		size_t toStr( char* buff, size_t sz) {
			NODECPP_ASSERT( foundation::module_id, ::nodecpp::assert::AssertLevel::critical, sz >= reportMaxSize ); 
			memcpy( buff, "<skipped:", 9 );
			size_t pos = 9;
			for ( size_t i=0; i<log_level_count; ++i )
				if ( skippedCtrs[i] && pos < sz )					
				{
					auto r = ::fmt::format_to_n( buff + pos, sz - pos, " {}: {}", LogLevelNames[i], skippedCtrs[i] );
					pos += r.size;
				}
			return pos;
		}
	};

	struct ChainedWaitingData
	{
		std::condition_variable w;
		std::mutex mx;
		SkippedMsgCounters skippedCtrs;
		bool canRun = false;
		ChainedWaitingData* next = nullptr;
	};

	struct LogBufferBaseData
	{
		static constexpr size_t maxMessageSize = 0x1000;
		LogLevel levelCouldBeSkipped = LogLevel::info;
		size_t pageSize; // consider making a constexpr (do we consider 2Mb pages?)
		static constexpr size_t pageCount = 4; // so far it is not obvious why we really need something else
		uint8_t* buff = nullptr; // aming: a set of consequtive pages
		size_t buffSize = 0; // a multiple of page size
		uint64_t start = 0; // writable by a thread writing to a file; readable: all
		uint64_t writerPromisedNextStart = 0; // writable: logging threads; readable: all
		uint64_t end = 0; // writable: logging threads; readable: all
		static constexpr size_t skippedCntMsgSz = 128;
		SkippedMsgCounters skippedCtrs; // accessible by log-writing threads
		std::condition_variable waitWriter;
		std::mutex mx;

		ChainedWaitingData* firstToRelease = nullptr; // for writer
		ChainedWaitingData* nextToAdd = nullptr; // for loggers

		enum Action { proceed = 0, flushAndExit };
		Action action = Action::proceed;

		FILE* target = nullptr; // so far...

		void init( FILE* f );
		void init( const char* path )
		{
			FILE* f = fopen( path, "ab" );
			init( f );
		}
		void deinit()
		{
			// TODO: revise (it seems to be the most reasonable to finalize destruction in writer thread
			if ( buff )
			{
				::nodecpp::VirtualMemory::deallocate( buff, buffSize );
				buff = nullptr;
			}
			if ( target ) 
			{
				fclose( target );
				target = nullptr;
			}
		}
		size_t availableSize() { return buffSize - (end - start); }
	};

	class LogTransport
	{
		// NOTE: it is just a quick sketch
		friend class Log;

		LogBufferBaseData* logData;

		void insertSingleMsg( const char* msg, size_t sz ) // under lock
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

		void insertMessage( const char* msg, size_t sz, SkippedMsgCounters& ctrs ) // under lock
		{
			if ( ctrs.fullCount() )
			{
				char b[SkippedMsgCounters::reportMaxSize];
				size_t bsz = logData->skippedCtrs.toStr( b, SkippedMsgCounters::reportMaxSize );
				static_assert( SkippedMsgCounters::reportMaxSize <= LogBufferBaseData::skippedCntMsgSz );
				insertSingleMsg( b, bsz );
				ctrs.clear();
			}
			insertSingleMsg( msg, sz );
			if (logData->end - logData->writerPromisedNextStart >= logData->pageSize)
			{
				logData->waitWriter.notify_one();
			}
		}

		bool addMsg( const char* msg, size_t sz, LogLevel l )
		{
			bool ret = true;
			bool waitAgain = false;
			ChainedWaitingData d;
			{
				std::unique_lock<std::mutex> lock(logData->mx);
				size_t fullSzRequired = logData->skippedCtrs.fullCount() == 0 ? sz : sz + logData->skippedCntMsgSz;
				if ( logData->nextToAdd != nullptr)
				{
					if (l >= logData->levelCouldBeSkipped)
					{
						logData->nextToAdd->skippedCtrs.increment(l);
						ret = false;
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
					insertMessage( msg, sz, logData->skippedCtrs );
				}
				else
				{
					if ( l >= logData->levelCouldBeSkipped ) // skip
					{
						logData->skippedCtrs.increment(l);
						ret = false;
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

			while ( waitAgain )
			{
				waitAgain = false;
				std::unique_lock<std::mutex> lock(d.mx);
				while (!d.canRun)
					d.w.wait(lock);
				d.canRun = false;
				lock.unlock();

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

					insertMessage( msg, sz, d.skippedCtrs );

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
			return ret;
		}

		void writoToLog( ModuleID mid, const char* msg, size_t sz, LogLevel severity ) {
			char msgFormatted[LogBufferBaseData::maxMessageSize];
			auto formatRet = mid.id() != nullptr ?
				::fmt::format_to_n( msgFormatted, LogBufferBaseData::maxMessageSize - 1, "[{}][{}] {}\n", mid.id(), LogLevelNames[(size_t)severity], msg ) :
				::fmt::format_to_n( msgFormatted, LogBufferBaseData::maxMessageSize - 1, "[{}] {}\n", LogLevelNames[(size_t)severity], msg );
			if ( formatRet.size >= LogBufferBaseData::maxMessageSize )
			{
				msgFormatted[LogBufferBaseData::maxMessageSize-4] = '.';
				msgFormatted[LogBufferBaseData::maxMessageSize-3] = '.';
				msgFormatted[LogBufferBaseData::maxMessageSize-2] = '.';
				msgFormatted[LogBufferBaseData::maxMessageSize-1] = 0;
			}
			else
				msgFormatted[formatRet.size] = 0;
			addMsg( msgFormatted, formatRet.size, severity );
		}

	public:
		LogTransport( LogBufferBaseData* data ) : logData( data ) {}
		LogTransport( const LogTransport& ) = delete;
		LogTransport& operator = ( const LogTransport& ) = delete;
		LogTransport( LogTransport&& other ) {
			logData = other.logData;
			other.logData = nullptr;
		}
		LogTransport& operator = ( LogTransport&& other )
		{
			logData = other.logData;
			other.logData = nullptr;
			return *this;
		}
		~LogTransport()
		{
		}
	};

	class Log
	{
	public:
		LogLevel level = LogLevel::info;

		std::vector<LogTransport> transports;
		bool silent = false;

		//TODO::add: format
		//TODO::add: exitOnError

		size_t transportIdx = 0; // for adding purposes in case of clustering

	public:
		Log() {}
		virtual ~Log() {}

		template<class StringT, class ... Objects>
		void log( ModuleID mid, LogLevel l, StringT format_str, Objects ... obj ) {
			if ( l <= level ) {
				char msgFormatted[LogBufferBaseData::maxMessageSize];
				auto r = ::fmt::format_to_n( msgFormatted, LogBufferBaseData::maxMessageSize-1, format_str, obj ... );
				if ( r.size >= LogBufferBaseData::maxMessageSize )
					msgFormatted[LogBufferBaseData::maxMessageSize-1] = 0;
				else
					msgFormatted[r.size] = 0;
				for ( auto& transport : transports )
					transport.writoToLog( mid, msgFormatted, r.size, l );
			}				 
		}

		template<class StringT, class ... Objects>
		void log( LogLevel l, StringT format_str, Objects ... obj ) {
			log( ModuleID( NODECPP_DEFAULT_LOG_MODULE ), l, format_str, obj ... );
		}

		template<class StringT, class ... Objects>
		void fatal( StringT format_str, Objects ... obj ) { log( LogLevel::fatal, format_str, obj ... ); }
		template<class StringT, class ... Objects>
		void error( StringT format_str, Objects ... obj ) { log( LogLevel::err, format_str, obj ... ); }
		template<class StringT, class ... Objects>
		void warning( StringT format_str, Objects ... obj ) { log( LogLevel::warning, format_str, obj ... ); }
		template<class StringT, class ... Objects>
		void info( StringT format_str, Objects ... obj ) { log( LogLevel::info, format_str, obj ... ); }
		template<class StringT, class ... Objects>
		void debug( StringT format_str, Objects ... obj ) { log( LogLevel::debug, format_str, obj ... ); }

		template<class StringT, class ... Objects>
		void fatal( ModuleID mid, StringT format_str, Objects ... obj ) { log( mid, LogLevel::fatal, format_str, obj ... ); }
		template<class StringT, class ... Objects>
		void error( ModuleID mid, StringT format_str, Objects ... obj ) { log( mid, LogLevel::err, format_str, obj ... ); }
		template<class StringT, class ... Objects>
		void warning( ModuleID mid, StringT format_str, Objects ... obj ) { log( mid, LogLevel::warning, format_str, obj ... ); }
		template<class StringT, class ... Objects>
		void info( ModuleID mid, StringT format_str, Objects ... obj ) { log( mid, LogLevel::info, format_str, obj ... ); }
		template<class StringT, class ... Objects>
		void debug( ModuleID mid, StringT format_str, Objects ... obj ) { log( mid, LogLevel::debug, format_str, obj ... ); }

		void clear() { transports.clear(); }
		template<class StringT>
		bool add( StringT path ) 
		{
			LogBufferBaseData* data = reinterpret_cast<LogBufferBaseData*>( malloc( sizeof(LogBufferBaseData) ) );
			new (data) LogBufferBaseData();
			data->init( path.c_str() );
			transports.emplace_back( data ); 
//			::nodecpp::logging_impl::logDataStructures.push_back( data );
			return true; // TODO
		}

		bool add( FILE* cons ) // TODO: input param is a subject for revision
		{
			LogBufferBaseData* data = reinterpret_cast<LogBufferBaseData*>( malloc( sizeof(LogBufferBaseData) ) );
			new (data) LogBufferBaseData();
			data->init( cons );
			transports.emplace_back( data ); 
//			::nodecpp::logging_impl::logDataStructures.push_back( data );
			return true; // TODO
		}
		//TODO::add: remove()
	};

	namespace logging_impl {
		extern thread_local ::nodecpp::Log* currentLog;
	} // namespace logging_impl

	namespace default_log
	{
		template<class StringT, class ... Objects>
		void log( ModuleID mid, LogLevel l, StringT format_str, Objects ... obj ) {
			if ( ::nodecpp::logging_impl::currentLog )
				::nodecpp::logging_impl::currentLog->log( mid, l, format_str, obj... );
		}

		template<class StringT, class ... Objects>
		void log( LogLevel l, StringT format_str, Objects ... obj ) {
			log( ModuleID( NODECPP_DEFAULT_LOG_MODULE ), l, format_str, obj ... );
		}

		template<class StringT, class ... Objects>
		void fatal( StringT format_str, Objects ... obj ) { log( LogLevel::fatal, format_str, obj ... ); }
		template<class StringT, class ... Objects>
		void error( StringT format_str, Objects ... obj ) { log( LogLevel::err, format_str, obj ... ); }
		template<class StringT, class ... Objects>
		void warning( StringT format_str, Objects ... obj ) { log( LogLevel::warning, format_str, obj ... ); }
		template<class StringT, class ... Objects>
		void info( StringT format_str, Objects ... obj ) { log( LogLevel::info, format_str, obj ... ); }
		template<class StringT, class ... Objects>
		void debug( StringT format_str, Objects ... obj ) { log( LogLevel::debug, format_str, obj ... ); }

		template<class StringT, class ... Objects>
		void fatal( ModuleID mid, StringT format_str, Objects ... obj ) { log( mid, LogLevel::fatal, format_str, obj ... ); }
		template<class StringT, class ... Objects>
		void error( ModuleID mid, StringT format_str, Objects ... obj ) { log( mid, LogLevel::err, format_str, obj ... ); }
		template<class StringT, class ... Objects>
		void warning( ModuleID mid, StringT format_str, Objects ... obj ) { log( mid, LogLevel::warning, format_str, obj ... ); }
		template<class StringT, class ... Objects>
		void info( ModuleID mid, StringT format_str, Objects ... obj ) { log( mid, LogLevel::info, format_str, obj ... ); }
		template<class StringT, class ... Objects>
		void debug( ModuleID mid, StringT format_str, Objects ... obj ) { log( mid, LogLevel::debug, format_str, obj ... ); }
	} // namespace default_log
} //namespace nodecpp

#endif // NODECPP_LOGGING_H
