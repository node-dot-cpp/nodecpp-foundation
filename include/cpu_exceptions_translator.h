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

#ifndef CPU_EXCEPTIONS_TRANSLATOR_H
#define CPU_EXCEPTIONS_TRANSLATOR_H

#include <fmt/format.h>
#include <stdexcept>

void initTranslator();

class MemoryAccessViolationException : public std::exception {
	static constexpr size_t buffsz = 0x100;
	char whatText[buffsz];
public:
	MemoryAccessViolationException( void* p ) { auto res = fmt::format_to_n( whatText, buffsz - 1, "Memory Access Violation at 0x{:x}", (size_t)p ); if ( res.size >= buffsz ) whatText[buffsz-1] = 0; else whatText[res.size] = 0; }
	MemoryAccessViolationException( const MemoryAccessViolationException& other ) = default;
	const char* what() const noexcept override {  return whatText; };

};

class NullPointerException : public std::exception {
public:
	const char* what() const noexcept override { return "Null Ptr Access"; };
};

# endif // CPU_EXCEPTIONS_TRANSLATOR_H
