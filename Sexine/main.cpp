#include <string>
#include <string.h>
#include <Windows.h>
#include <TlHelp32.h>
#include <iostream>
#include <intrin.h>

extern "C" {
#include "Lua/lua.h"
#include "Lua/lauxlib.h"
#include "Lua/ldo.h"
#include "Lua/lfunc.h"
#include "Lua/lmem.h"
#include "Lua/lualib.h"
#include "Lua/lobject.h"
#include "Lua/lopcodes.h"
#include "Lua/lstring.h"
#include "Lua/lundump.h"
#include "Lua/lvm.h"
#include "Lua/ldo.h"
}

typedef enum { // types of messages...... not illegal........................................
	MESSAGE_OUTPUT,
	MESSAGE_INFO,
	MESSAGE_WARNING,
	MESSAGE_ERROR
} MessageType;

DWORD RL = 0;
// self-explanatory
typedef DWORD(__cdecl* _rluaF_newlclosure)(DWORD, int, DWORD);
_rluaF_newlclosure rluaF_newlclosure = (_rluaF_newlclosure)0x9CEF20;

typedef DWORD(__cdecl* _rluaF_newproto)(DWORD);
_rluaF_newproto rluaF_newproto = (_rluaF_newproto)0x9CF140;

typedef const char*(__cdecl* _rlua_tolstring)(DWORD, int, int);
_rlua_tolstring rlua_tolstring = (_rlua_tolstring)0x8A3D90;

typedef DWORD*(__cdecl* _rlua_newlstr)(DWORD, const char*, size_t);
_rlua_newlstr rlua_newlstr = (_rlua_newlstr)0x9CED80;

typedef DWORD(__cdecl* _rluaM_malloc)(DWORD, void*, size_t, size_t);
_rluaM_malloc rluaM_malloc = (_rluaM_malloc)0x9CF9B0;

typedef DWORD(__cdecl* _rluaF_newupval)(DWORD);
_rluaF_newupval rluaF_newupval = (_rluaF_newupval)0x9CEF80;

typedef void(__cdecl* _rlua_settop)(DWORD, int);
_rlua_settop rlua_settop = (_rlua_settop)0x8A39A0;

typedef DWORD(__cdecl* _rlua_newthread)(DWORD);
_rlua_newthread rlua_newthread = (_rlua_newthread)0x8A4F40;

typedef int(__cdecl* _rlua_resume)(DWORD, int);
_rlua_resume rlua_resume = (_rlua_resume)0x8D7F70;

void pause_roblox() { // these help stability when doing something as reckless as hooking a frequently-used function. i think so at least
	HANDLE h = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);

	THREADENTRY32 th;
	th.dwSize = sizeof(th);

	Thread32First(h, &th);

	do {
		if (th.th32OwnerProcessID == GetCurrentProcessId() && th.th32ThreadID != GetCurrentThreadId()) {
			HANDLE thread = OpenThread(THREAD_SUSPEND_RESUME, false, th.th32ThreadID);
			if (thread) {
				SuspendThread(thread);
				CloseHandle(thread);
			}
		}
	} while (Thread32Next(h, &th));
}

void resume_roblox() {
	HANDLE h = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);

	THREADENTRY32 th;
	th.dwSize = sizeof(th);

	Thread32First(h, &th);

	do {
		if (th.th32OwnerProcessID == GetCurrentProcessId() && th.th32ThreadID != GetCurrentThreadId()) {
			HANDLE thread = OpenThread(THREAD_SUSPEND_RESUME, false, th.th32ThreadID);
			if (thread) {
				ResumeThread(thread);
				CloseHandle(thread);
			}
		}
	} while (Thread32Next(h, &th));
}

union r_Value {
	void* gc;
	void* p;
	double n;
	int b;
};

struct r_TValue {
	r_Value value;
	int tt;
};

struct r_Proto {
	CommonHeader;
	r_TValue* k;
	Instruction* code;
	DWORD** p;
	int* lineinfo;
	LocVar* locvars;
	DWORD** upvalues;
	DWORD* source;
	int sizeupvalues;
	int sizek;
	int sizecode;
	int sizelineinfo;
	int sizep;
	int sizelocvars;
	int linedefined;
	int lastlinedefined;
	DWORD* gclist;
	BYTE nups;
	BYTE numparams;
	BYTE is_vararg;
	BYTE maxstacksize;
};

long __stdcall VectoredExceptionHandler(_EXCEPTION_POINTERS* ep) {
	if (ep->ExceptionRecord->ExceptionCode == STATUS_BREAKPOINT) {
		if (ep->ContextRecord->Eip == 0x631CCC) { // trustcheck
			ep->ContextRecord->Eax = 1;
			ep->ContextRecord->Eip++;
			return EXCEPTION_CONTINUE_EXECUTION;
		}
		if (ep->ContextRecord->Eip == 0x8A41E0) { // lua state
			pause_roblox(); // i decided to pause roblox when hooking because not doing so fucked up the script it was hooking sometimes

			RL = *(DWORD*)(*(DWORD*)(*(DWORD*)(ep->ContextRecord->Esp + 4) + 16) + 112); // L->g_T->mainthread seems to work

			DWORD ex;
			VirtualProtect((void*)0x8A41E0, 1, PAGE_EXECUTE_READWRITE, &ex);
			memcpy((void*)0x8A41E0, "\x8B", 1);
			VirtualProtect((void*)0x8A41E0, 1, ex, &ex);
			
			resume_roblox();

			return EXCEPTION_CONTINUE_EXECUTION;
		}
		if (ep->ContextRecord->Eip == 0x678B61) { // roblox output
			const char* str;
			if (*(DWORD*)(ep->ContextRecord->Esi + 24) < 0x10)
				str = (const char*)((DWORD*)ep->ContextRecord->Esi + 1);
			else
				str = (const char*)*(DWORD*)(ep->ContextRecord->Esi + 4);

			if (str) {
				switch (ep->ContextRecord->Eax) {
				case MessageType::MESSAGE_OUTPUT:
					std::cout << "[OUTPUT] ";
					break;
				case MessageType::MESSAGE_INFO:
					std::cout << "[INFO] ";
					break;
				case MessageType::MESSAGE_WARNING:
					std::cout << "[WARNING] ";
					break;
				case MessageType::MESSAGE_ERROR:
					std::cout << "[ERROR] ";
					break;
				default: // hopefully not..
					std::cout << "[OTHER] ";
				}
				std::cout << str << std::endl;
			}

			ep->ContextRecord->EFlags |= (((ep->ContextRecord->Eax == 3) << 6) | (ep->ContextRecord->Eax < 3)); // cmp eax, 3; zf flag is 0x40 (1 << 6) while cf flag is 0x1
			ep->ContextRecord->Eip += 3;

			return EXCEPTION_CONTINUE_EXECUTION;
		}
	}
	return EXCEPTION_CONTINUE_SEARCH;
}

r_Proto* convert_proto(Proto* p, int& nups) {
	r_Proto* rp = (r_Proto*)rluaF_newproto(RL);

	rp->sizek = p->sizek; // first and foremost, set the ez members. these are int/byte members of the proto and can be transferred without much difficulty
	rp->sizecode = p->sizecode;
	rp->sizep = p->sizep;
	rp->linedefined = p->linedefined;
	rp->lastlinedefined = p->lastlinedefined;
	rp->numparams = p->numparams;
	rp->is_vararg = p->is_vararg;
	rp->maxstacksize = p->maxstacksize;
	rp->nups = p->nups;
	rp->sizelineinfo = p->sizelineinfo;

	rp->code = (Instruction*)rluaM_malloc(RL, 0, 0, p->sizecode * sizeof(Instruction)); // allocate space for all of the big boy Proto members. this one holds all of the instructions
	rp->k = (r_TValue*)rluaM_malloc(RL, 0, 0, p->sizek * sizeof(r_TValue)); // this one is the constant table
	rp->p = (DWORD**)rluaM_malloc(RL, 0, 0, p->sizep * sizeof(Proto*)); // this one is other protos!!!! (just nested functions within the code im pretty sure)
	rp->lineinfo = (int*)rluaM_malloc(RL, 0, 0, p->sizelineinfo * sizeof(int));

	for (int i = 0; i < p->sizek; i++) { // go through the constant table of our vanilla Proto..
		switch (p->k[i].tt) { // ..and set new constants accordingly
		case LUA_TNIL:
			rp->k[i].value.n = 0;
			rp->k[i].tt = LUA_TNIL;
			break;
		case LUA_TBOOLEAN:
			rp->k[i].value.b = p->k[i].value.b;
			rp->k[i].tt = LUA_TBOOLEAN;
			break;
		case LUA_TSTRING:
			rp->k[i].value.gc = rlua_newlstr(RL, getstr(&p->k[i].value.gc->ts), p->k[i].value.gc->ts.tsv.len);
			rp->k[i].tt = LUA_TSTRING;
			break;
		case LUA_TNUMBER:
			rp->k[i].value.n = p->k[i].value.n;
			rp->k[i].tt = LUA_TNUMBER;
			break;
		default: // wtf
			rp->k[i].value.gc = p->k[i].value.gc;
			rp->k[i].tt = p->k[i].tt;
			break;
		}
	}

	for (int i = 0; i < p->sizecode; i++) { // responsible for transferring instructions. this is 2013 so there is no crazy instruction encryption yet - standard Lua instructions should be read fine
		rp->code[i] = p->code[i];
	}

	for (int i = 0; i < p->sizep; i++) { // convert all of the nested functions too.
		rp->p[i] = (DWORD*)convert_proto(p->p[i], nups);
	}

	for (int i = 0; i < p->sizelineinfo; i++) {
		rp->lineinfo[i] = p->lineinfo[i];
	}

	rp->source = (DWORD*)p->source; // source member is actually important to execution. contains chunk name which is used for printing errors. took me way too long to figure out...

	nups += rp->nups; // add to the total upvalue count (passed by reference). note that this function is called once for the top-level function and once for every proto within it, so *all* upvalues will count

	return rp;
}

void execute_script(lua_State* L, std::string& source) {
	//source.insert(0, "coroutine.resume(coroutine.create(function() "); // run code in a new thread
	//source.append(" end))");
	/*
	you may be asking me... scripter john-kun, why create a new thread for every script?
	it is because............
	i wanna run scripts without possibly yielding the current thread that i'm on, which is the global state's main thread (ouch).
	but then you might also inquire.....................
	why not just create one new thread and use that instead?
	that is, too, because.....
		1. i felt like it
		2. most script execution exploits that i've developed in the past did this anyway, though it was (shittily) done through appending code (see above). this is just a more efficient way of carrying it out.
		3. what if the user runs scripts super duper fast and they have a potato? if we put all of that action in one thread, stability might be compromised.
		^ proven by my old shittop
		







	also i don't go by scripter john anymore. stop
	*/
	DWORD RL2 = rlua_newthread(RL);

	if (luaL_loadbuffer(L, source.c_str(), source.length(), "=Sexine") != 0) { // first, load the user's code in vanilla Lua
		std::cout << "[Sexine] COMPILATION ERROR: " << lua_tolstring(L, -1, 0) << std::endl;
		lua_settop(L, 0);
		return;
	}
	Proto* p = (L->top - 1)->value.gc->cl.l.p; // obtain the compiled code's Proto

	int nups = 0;
	r_Proto* rp = convert_proto(p, nups); // convert it to a Proto that Roblox can use (without security for the most part, it's only a matter of setting the right members). a Proto is the "proto"type of a lua function and contains all of the relevant details and instructions necessary to its execution.

	DWORD cl = rluaF_newlclosure(RL2, 0, *(DWORD*)(RL + 72)); // create new L closure (think of it like a Lua function) and set its env to our thread's current env ( + 72). to execute a Proto, you create a new LClosure and set its Proto member (yeah it has one) to the Proto, and then run it using call/pcall/whatever you want
	*(DWORD*)(cl + 16) = (DWORD)rp; // set the Proto member to our proto

	for (int i = 0; i < nups; i++) { // "nups" is the total amount of upvalues that is going to be used by our Roblox proto, nested functions included. create the upvalues before execution, if any, to allow them to be used
		*((DWORD*)(cl + 20) + i) = rluaF_newupval(RL2); // ( + 20) is the lclosure's array of upvalues. set the upvalues accordingly
	}

	DWORD top = *(DWORD*)(RL2 + 8);
	*(DWORD*)top = cl;
	*(DWORD*)(top + 8) = LUA_TFUNCTION;
	*(DWORD*)(RL2 + 8) += 16; // this is just pushing our lclosure to the stack


	/* why i chose resume to execute the function:
	if i chose pcall, it wouldn't be able to yield (wait()).
	if i chose call, it would crash every time it errored.
	if i chose calling Spawn, it would work but errors would crash if i had yielded at least once
	lua_resume seemed like the best option because:
	1. we are (technically) using a coroutine
	2. fixes the yielding error with Spawn, which was caused by our thread's CallInfo being off; since lua_resume calls luaD_precall before it runs the script, all necessary members of our CallInfo were filled out.
	3. can yield (duh)
	4. bruh
	*/

	if (rlua_resume(RL2, 0) == 2) {
		std::cout << "[Sexine] RUNTIME ERROR: " << rlua_tolstring(RL2, -1, 0) << std::endl; // if you're getting an empty string, security context is inadequate (should not happen)
	}
	
	rlua_settop(RL, -2);
	rlua_settop(RL2, 0); // clear the stacks
	lua_settop(L, 0);
	// we are done!
	// pretty sure lua's gc will handle the thread
	// i hope
	// idk too much about gc so
}

int main() {
	AllocConsole();
	SetConsoleTitleA("Sexine");
	freopen("CONIN$", "r", stdin);
	freopen("CONOUT$", "w", stdout);

	lua_State* L = luaL_newstate();
	luaL_openlibs(L);

	std::cout << "Hooking ..." << std::endl;
	//Sleep(5000);

	AddVectoredExceptionHandler(1, VectoredExceptionHandler); // so that our epic int 3 hook shitstains can run
	/* if you're unfamiliar with the line above, let me explain (long subpar x86 explanation warning):
	
	a "Vectored Exception Handler" is a function that can be used to watch or handle exceptions, regardless of the frame it's actually in.
	as you'll see below, i write "\xCC" (the interrupt-3 instruction, used to set breakpoints) to certain places of interest to trigger exceptions.
	
	when roblox eventually comes across this instruction, an exception (called "STATUS_BREAKPOINT" when handled) is generated and our Vectored Exception Handler is a w o k e n
	for every exception that occurs, our handler checks the code against STATUS_BREAKPOINT, checks the exception address against the places of interest, and if they both match, the actual hook(s) begin.
	for the first hook in lua_getfield, i grab the lua state (the first argument, [esp + 4]), get to its associated global state ( + 16), get to the global state's main thread (+ 112), and restore the patched byte. this main thread is what i create threads from for execution
	
	the function that is hooked in the second one is the same function Roblox uses for printing to the output. this includes errors, information, warnings, and actual output.
	at this point in the function, the type of the message is stored in eax and the message is stored in some offset of esi. i print both of these out
	
	to make up for the three pitiful bytes i patched over (cmp eax, 3) and return to the function, i did a little trickery.
	the cmp instruction modifies two "flags": the zero flag to one, if the two operands are equal, and the carry flag to one, if the first operand was less.
	
	since it would be too much work to redirect the WHOOOOLE function to one of my own to execute a cmp instruction, i had to work with what i had.
	the ->EFlags member of the ContextRecord struct is a single DWORD that contains the information of all of the flags. you can modify the flags by ORing it with certain values
	if you OR EFlags by 0x40, it sets the zero flag to 1. if you OR it by 1, it sets the carry flag to 1.

	i ended up oring it by ((eax == 3) << 6) | (eax < 3). 
	what this means is that if eax is equal to 3, the value i or with starts out with (1 << 6) (0 if vice versa). 
	*1 << 6 is 0x40*. think of it like eax == 3 ? 0x40 : 0x00 but much cooler

	now we take the potential 0x40 and or it by whether or not eax is less than 3 (boolean, 0 or 1 value) to set the carry flag, and then or EFlags by the result.

	finally, we add 3 to the eip to continue with the rest of the function.

	trustcheck hook is self-explanatory i dont feel like explaining it
	*/
	DWORD ex;
	VirtualProtect((void*)0x8A41E0, 1, PAGE_EXECUTE_READWRITE, &ex);
	memcpy((void*)0x8A41E0, "\xCC", 1); // plant lua state hook in getfield
	VirtualProtect((void*)0x8A41E0, 1, ex, &ex);

	VirtualProtect((void*)0x678B20, 1, PAGE_EXECUTE_READWRITE, &ex);
	memcpy((void*)0x678B61, "\xCC\x90\x90", 3); // this is the roblox console hook
	VirtualProtect((void*)0x678B20, 1, ex, &ex);

	VirtualProtect((void*)0x631A30, 1, PAGE_EXECUTE_READWRITE, &ex);
	memcpy((void*)0x631CCC, "\xCC\x90", 2); // trustcheck
	VirtualProtect((void*)0x631A30, 1, ex, &ex);

	while (RL == 0) { // wait..
		Sleep(1);
	}

	*(int*)(((int(*)())0x53D490)()) = 7; // i tried setting the identity of just the fucking thread instead and it would never work
	// ^^ very bad!! if you work on a client thats new enough you could probably remove this

	std::cout << "Welcome to Sexine!" << std::endl; // funny
	std::cout << "lua state: " << RL << std::endl;

	std::string s;

	while (true) {
		std::getline(std::cin, s);
		execute_script(L, s);
	}
	return 0;
}

bool __stdcall DllMain(HMODULE hModule, DWORD  ul_reason_for_call, LPVOID lpReserved) {
	if (ul_reason_for_call == DLL_PROCESS_ATTACH)
		CreateThread(0, 0, (PTHREAD_START_ROUTINE)main, 0, 0, 0);
	return TRUE;
}

/* note to self: c3 exception */
