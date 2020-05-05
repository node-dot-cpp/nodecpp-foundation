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

#include "../include/foundation.h"
#include "../include/cpu_exceptions_translator.h"


#if defined __clang__

// TODO clang doesn't handle cpu exceptions well yet
// https://github.com/node-dot-cpp/nodecpp-foundation/issues/1

#elif defined __GNUC__ && __linux__

#include <signal.h>
#include <unistd.h>
#include <sys/syscall.h>
#include <execinfo.h>

#define STRINGIFY_VALUE_OF_HELPER(x) #x
#define STRINGIFY_VALUE_OF(x) STRINGIFY_VALUE_OF_HELPER(x)

struct kernel_sigaction // see http://man7.org/linux/man-pages/man2/sigaction.2.html
{
	void (*k_sa_sigaction)(int,siginfo_t *,void *);
	unsigned long k_sa_flags;
	void (*k_sa_restorer) (void);
	sigset_t k_sa_mask;
};

#ifdef __x86_64__

__asm__
  (
   ".align 16\n"
   "sigRestorerAsm: movq $" STRINGIFY_VALUE_OF(__NR_rt_sigreturn) ", %rax\n"
   "syscall\n"
   );
   
void sigRestorer () __asm__ ("sigRestorerAsm");

#else // __x86_64__

#error

#endif // __x86_64__

static void unblockSignal(int signum)
{
    sigset_t sigs;
    sigemptyset(&sigs);
    sigaddset(&sigs, signum);
    sigprocmask(SIG_UNBLOCK, &sigs, NULL);
}

static void sigAction(int, siginfo_t* siginfo, void*)
{
    unblockSignal(SIGSEGV);    
    if( (uintptr_t)(siginfo->si_addr) >= NODECPP_MINIMUM_ZERO_GUARD_PAGE_SIZE )
		throw MemoryAccessViolationException( siginfo->si_addr );
	else 
		throw NullPointerException();
}

void initTranslator()
{
    struct kernel_sigaction ks;
    ks.k_sa_sigaction = sigAction;
    sigemptyset (&ks.k_sa_mask);
    ks.k_sa_flags = SA_SIGINFO|0x4000000;
    ks.k_sa_restorer = sigRestorer;
    syscall (SYS_rt_sigaction, SIGSEGV, &ks, NULL, _NSIG / 8);
}

#elif defined _MSC_VER

#include <stdio.h>
#include <windows.h>
#include <eh.h>

void trans_func(unsigned int u, EXCEPTION_POINTERS* ep)
{
    if (ep->ExceptionRecord->ExceptionCode == EXCEPTION_ACCESS_VIOLATION && ep->ExceptionRecord->NumberParameters >= 2 )
	{
		void * at = (void*)(ep->ExceptionRecord->ExceptionInformation[1]);
		if ( (uintptr_t)at >= NODECPP_MINIMUM_ZERO_GUARD_PAGE_SIZE )
			throw MemoryAccessViolationException(at);
		else 
			throw NullPointerException();
	}
	else
		throw std::exception();
}

void initTranslator()
{
    _set_se_translator(trans_func);
}


#endif
