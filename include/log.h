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
#include "foundation_module.h"
#include <fmt/format.h>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <vector>
#include "page_allocator.h"


namespace nodecpp::logging_impl {
	constexpr size_t invalidInstanceID = ((size_t)0 - 2);
	extern thread_local size_t instanceId;
	struct LoggingTimeStamp
	{
		uint64_t t;
	};
	LoggingTimeStamp getCurrentTimeStamp();
} // namespace logging_impl

template<>
struct ::fmt::formatter<nodecpp::logging_impl::LoggingTimeStamp>
{
	template<typename ParseContext> constexpr auto parse(ParseContext& ctx) {return ctx.begin();}
	template<typename FormatContext> auto format(nodecpp::logging_impl::LoggingTimeStamp const& lts, FormatContext& ctx) {return fmt::format_to(ctx.out(), "{:.06f}", lts.t / 1000000.0 );}
};	

namespace nodecpp::log {

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
		size_t toStr( char* buff, size_t sz);
	};

	struct ChainedWaitingData
	{
		std::condition_variable w;
		std::mutex mx;
		SkippedMsgCounters skippedCtrs;
		bool canRun = false;
		ChainedWaitingData* next = nullptr;
	};

	struct ChainedWaitingForGuaranteedWrite
	{
		std::condition_variable w;
		std::mutex mx;
		uint64_t end;
		bool canRun = false;
		ChainedWaitingForGuaranteedWrite* next = nullptr;
	};

	struct LogBufferBaseData
	{
		static constexpr size_t maxMessageSize = 0x1000;
		LogLevel levelCouldBeSkipped = LogLevel::info;
		LogLevel levelGuaranteedWrite = LogLevel::fatal;
		size_t pageSize; // consider making a constexpr (do we consider 2Mb pages?)
		static constexpr size_t pageCount = 4; // so far it is not obvious why we really need something else
		uint8_t* buff = nullptr; // aming: a set of consequtive pages
		size_t buffSize = 0; // a multiple of page size
		uint64_t start = 0; // mx-protected; writable by a thread writing to a file; readable: all
		uint64_t writerPromisedNextStart = 0; // mx-protected; writable: writing thread; readable: all
		uint64_t end = 0; // mx-protected; writable: logging threads, writing thread(in case of periodic flushing); readable: all
		uint64_t mustBeWrittenImmediately = 0; // mx-protected; writable: logging threads, writing thread(in case of periodic flushing); readable: all
		static constexpr size_t skippedCntMsgSz = 128; // an upper estimation for quick calculations
		SkippedMsgCounters skippedCtrs; // mx-protected; accessible by log-writing threads
		std::condition_variable waitWriter;
		size_t refCounter = 0; // mx-protected
		std::mutex mx;

		ChainedWaitingData* firstToRelease = nullptr; // for writer
		ChainedWaitingData* nextToAdd = nullptr; // for loggers
		ChainedWaitingForGuaranteedWrite* firstToReleaseGuaranteed = nullptr; // for writer
		ChainedWaitingForGuaranteedWrite* nextToAddGuaranteed = nullptr; // for loggers

		enum Action { proceed = 0, proceedToTermination, terminationAllowed };
		Action action = Action::proceed;

		FILE* target = nullptr; // so far...

		void init( FILE* f );
		void init( const char* path )
		{
			FILE* f = fopen( path, "ab" );
			setbuf( f, nullptr ); // no bufferig
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
		size_t addRef() {
			std::unique_lock<std::mutex> lock(mx);
			return ++refCounter;
		}
		size_t removeRef() {
			std::unique_lock<std::mutex> lock(mx);
			return --refCounter;
		}
		void setEnterTerminatingPhase() {
			std::unique_lock<std::mutex> lock(mx);
			action = Action::proceedToTermination;
			waitWriter.notify_one();
		}
		void setTerminationAllowed() {
			std::unique_lock<std::mutex> lock(mx);
			action = Action::terminationAllowed;
			waitWriter.notify_one();
		}
		bool _writerOnly_TerminateIfAlone() {
			std::unique_lock<std::mutex> lock(mx);
			if ( action == Action::terminationAllowed && refCounter == 1 && start == end )
			{
				refCounter = 0;
				deinit();
				return true;
			}
			return false;
		}
	};

	class LogTransport
	{
		// NOTE: it is just a quick sketch
		friend class Log;

		LogBufferBaseData* logData;
		ChainedWaitingForGuaranteedWrite gww;

		void insertSingleMsg( const char* msg, size_t sz );
		void insertMessage( const char* msg, size_t sz, SkippedMsgCounters& ctrs, bool isCritical );
		bool addMsg( const char* msg, size_t sz, LogLevel l );

		void setEnterTerminatingPhase() { if ( logData ) logData->setEnterTerminatingPhase(); }
		void setTerminationAllowed() { if ( logData ) logData->setTerminationAllowed(); }

		void writoToLog( ModuleID mid, const char* msg, size_t sz, LogLevel severity, bool addTimeStamp ) {
			char msgFormatted[LogBufferBaseData::maxMessageSize];
			size_t wrtPos = 0;
			if ( addTimeStamp )
			{
				auto formatRet = ::fmt::format_to_n( msgFormatted, LogBufferBaseData::maxMessageSize - 1, "[{}]", logging_impl::getCurrentTimeStamp() );
				wrtPos = formatRet.size;
			}
			auto formatRet = mid.id() != nullptr ?
				( logging_impl::instanceId != logging_impl::invalidInstanceID ?
					::fmt::format_to_n( msgFormatted + wrtPos, LogBufferBaseData::maxMessageSize - 1 - wrtPos, "[{}:{}][{}] {}\n", mid.id(), logging_impl::instanceId, LogLevelNames[(size_t)severity], msg ) :
					::fmt::format_to_n( msgFormatted + wrtPos, LogBufferBaseData::maxMessageSize - 1 - wrtPos, "[{}][{}] {}\n", mid.id(), LogLevelNames[(size_t)severity], msg ) ) :
				( logging_impl::instanceId != logging_impl::invalidInstanceID ?
					::fmt::format_to_n( msgFormatted + wrtPos, LogBufferBaseData::maxMessageSize - 1 - wrtPos, "[:{}][{}] {}\n", logging_impl::instanceId, LogLevelNames[(size_t)severity], msg ) :
					::fmt::format_to_n( msgFormatted + wrtPos, LogBufferBaseData::maxMessageSize - 1 - wrtPos, "[:][{}] {}\n", LogLevelNames[(size_t)severity], msg ) );
			if ( formatRet.size + wrtPos >= LogBufferBaseData::maxMessageSize )
			{
				msgFormatted[LogBufferBaseData::maxMessageSize-4] = '.';
				msgFormatted[LogBufferBaseData::maxMessageSize-3] = '.';
				msgFormatted[LogBufferBaseData::maxMessageSize-2] = '.';
				msgFormatted[LogBufferBaseData::maxMessageSize-1] = 0;
			}
			else
				msgFormatted[formatRet.size + wrtPos] = 0;
			addMsg( msgFormatted, formatRet.size + wrtPos, severity );
		}

	public:
		LogTransport( LogBufferBaseData* data ) : logData( data ) { data->addRef(); }
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
			if ( logData != nullptr )
			{
				size_t refCtr = logData->removeRef();
				if ( refCtr == 0 && logData != nullptr ) 
				{
					logData->deinit();
				}
			}
		}
	};

	class Log
	{
		LogLevel levelCouldBeSkipped = LogLevel::info;

	public:
		LogLevel level = LogLevel::info;

		std::vector<LogTransport> transports;
		bool silent = false;
		bool addTimeStamp = true;

		//TODO::add: format
		//TODO::add: exitOnError

	public:
		Log() {}
		virtual ~Log()
		{
			setEnterTerminatingPhase();
		}

		void setGuaranteedLevel( LogLevel l )
		{
			levelCouldBeSkipped = LogLevel((size_t)l + 1);
			for ( auto& t : transports )
				t.logData->levelCouldBeSkipped = levelCouldBeSkipped;
		}
		void setCriticalLevel( LogLevel l )
		{
			for ( auto& t : transports )
				t.logData->levelGuaranteedWrite = l;
		}
		void resetGuaranteedLevel() { setGuaranteedLevel( LogLevel::fatal ); }
		void resetCriticalLevel() { setCriticalLevel( LogLevel::fatal ); }

		void setEnterTerminatingPhase()
		{
			for ( auto& t : transports )
				t.logData->setEnterTerminatingPhase();
		}
		void setTerminationAllowed()
		{
			for ( auto& t : transports )
				t.logData->setTerminationAllowed();
		}

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
					transport.writoToLog( mid, msgFormatted, r.size, l, addTimeStamp );
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
			data->levelCouldBeSkipped = levelCouldBeSkipped;
			transports.emplace_back( data ); 
//			::nodecpp::logging_impl::logDataStructures.push_back( data );
			return true; // TODO
		}

		bool add( FILE* cons ) // TODO: input param is a subject for revision
		{
			LogBufferBaseData* data = reinterpret_cast<LogBufferBaseData*>( malloc( sizeof(LogBufferBaseData) ) );
			new (data) LogBufferBaseData();
			data->init( cons );
			data->levelCouldBeSkipped = levelCouldBeSkipped;
			transports.emplace_back( data ); 
//			::nodecpp::logging_impl::logDataStructures.push_back( data );
			return true; // TODO
		}
		//TODO::add: remove()
	};

} // namespace nodecpp::log

namespace nodecpp::logging_impl {
	extern thread_local ::nodecpp::log::Log* currentLog;
} // namespace logging_impl

namespace nodecpp::log {
	namespace default_log
	{
		template<class StringT, class ... Objects>
		void log( ModuleID mid, LogLevel l, StringT format_str, Objects ... obj ) {
			if ( ::nodecpp::logging_impl::currentLog )
				::nodecpp::logging_impl::currentLog->log<StringT, Objects ...>( mid, l, format_str, obj... );
		}

		template<class StringT, class ... Objects>
		void log( LogLevel l, StringT format_str, Objects ... obj ) {
			log<StringT, Objects ...>( ModuleID( NODECPP_DEFAULT_LOG_MODULE ), l, format_str, obj ... );
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
} //namespace nodecpp::log

#endif // NODECPP_LOGGING_H
