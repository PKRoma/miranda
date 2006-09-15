/*

Miranda IM: the free IM client for Microsoft* Windows*

Copyright 2000-2006 Miranda ICQ/IM project,
all portions of this codebase are copyrighted to the people
listed in contributors.txt.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
*/
#ifndef M_SYSTEM_H__
#define M_SYSTEM_H__ 1

#ifndef MIRANDANAME
# define MIRANDANAME		"Miranda IM"
#endif
#ifndef MIRANDACLASS
# define MIRANDACLASS	"Miranda"
#endif

// set the default compatibility lever for Miranda 0.4.x
#ifndef MIRANDA_VER
# define MIRANDA_VER    0x0400
#endif

//miranda/system/modulesloaded
//called after all modules have been successfully initialised
//wParam=lParam=0
//used to resolve double-dependencies in the module load order
#define ME_SYSTEM_MODULESLOADED  "Miranda/System/ModulesLoaded"

//miranda/system/shutdown event
//called just before the application terminates
//the database is still guaranteed to be running during this hook.
//wParam=lParam=0
#define ME_SYSTEM_SHUTDOWN   "Miranda/System/Shutdown"

//miranda/system/oktoexit event
//called before the app goes into shutdown routine to make sure everyone is
//happy to exit
//wParam=lParam=0
//return nonzero to stop the exit cycle
#define ME_SYSTEM_OKTOEXIT   "Miranda/System/OkToExitEvent"

//miranda/system/oktoexit service
//Check if everyone is happy to exit
//wParam=lParam=0
//if everyone acknowleges OK to exit then returns true, otherwise false
#define MS_SYSTEM_OKTOEXIT   "Miranda/System/OkToExit"

//gets the version number of Miranda encoded as a DWORD     v0.1.0.1+
//wParam=lParam=0
//returns the version number, encoded as one version per byte, therefore
//version 1.2.3.10 is 0x0102030a
#define MS_SYSTEM_GETVERSION "Miranda/System/GetVersion"

//gets the version of Miranda encoded as text   v0.1.0.1+
//wParam=cch
//lParam=(LPARAM)(char*)pszVersion
//cch is the size of the buffer pointed to by pszVersion, in bytes
//may return a build qualifier, such as "0.1.0.1 alpha"
//returns 0 on success, nonzero on failure
#define MS_SYSTEM_GETVERSIONTEXT "Miranda/System/GetVersionText"

//Adds a HANDLE to the list to be checked in the main message loop  v0.1.2.0+
//wParam=(WPARAM)(HANDLE)hObject
//lParam=(LPARAM)(const char*)pszService
//returns 0 on success or nonzero on failure
//Causes pszService to be CallService()d (wParam=hObject,lParam=0) from the
//main thread whenever hObject is signalled.
//The Miranda message loop has a MsgWaitForMultipleObjects() call in it to
//implement this feature. See the documentation for that function for
//information on what objects are supported.
//There is a limit of MAXIMUM_WAIT_OBJECTS minus one (MWO is defined in winnt.h
//to be 64) on the number of handles MSFMO() can process. This service will
//return nonzero if that many handles are already being waited on.

//As of writing, the following parts of Miranda are thread-safe, so can be
//called from any thread:
//All of modules.h except NotifyEventHooks()
//Read-only parts of m_database.h (since the write parts will call hooks)
//All of m_langpack.h
//for all other routines your mileage may vary, but I would strongly recommend
//that you call them from the main thread, or ask about it on plugin-dev if you
//think it really ought to work.

//Update during 0.1.2.0 development, 16/10/01:
//NotifyEventHooks() now translates all calls into the context of the main
//thread, which means that all of m_database.h is now completely safe.

//Update during 0.1.2.2 development, 17/4/02:
//The main thread's message loop now also deals with asynchronous procedure
//calls. Loop up QueueUserAPC() for a neater way to accomplish a lot of the
//things that used to require ms_system_waitonhandle.

//Miranda is compiled with the multithreaded runtime - don't forget to do the
//same with your plugin.
#define MS_SYSTEM_WAITONHANDLE   "Miranda/System/WaitOnHandle"

//Removes a HANDLE from the wait list   v0.1.2.0+
//wParam=(WPARAM)(HANDLE)hObject
//lParam=0
//returns 0 on success or nonzero on failure.
#define MS_SYSTEM_REMOVEWAIT     "Miranda/System/RemoveWait"

/* Returns Miranda's RTL/CRT function poiners to malloc() free() realloc() -- 0.1.2.2+
This is useful for preallocation of memory for use with Miranda's services
that Miranda can free -- or reallocation of a block of memory passed with a service.
Do not use with memory unless it is explicitly expected the memory *can*
or *shall* be used in this way. the passed structure is expected to have it's .cbSize initialised

wParam=0, lParam = (LPARAM) &MM_INTERFACE
*/

struct MM_INTERFACE {
	int cbSize;
	void* (*mmi_malloc) (size_t);
	void* (*mmi_realloc) (void*, size_t);
	void (*mmi_free) (void*);
};

#define MS_SYSTEM_GET_MMI  "Miranda/System/GetMMI"

/* Returns the pointer to the simple lists manager.
If the sortFunc member of the list gets assigned, the list becomes sorted

wParam=0, lParam = 0
*/

#define LIST_INTERFACE_V1_SIZE  (sizeof(int)+7*sizeof(void*))
#define LIST_INTERFACE_V2_SIZE  (sizeof(int)+9*sizeof(void*))

typedef int ( *FSortFunc )( void*, void* );

typedef struct
{
	void**		items;
	int			realCount;
	int			limit;
	int			increment;

	FSortFunc	sortFunc;
}
	SortedList;

struct LIST_INTERFACE {
	int    cbSize;

   SortedList* ( *List_Create )( int, int );
	void        ( *List_Destroy )( SortedList* );

	void*	( *List_Find )( SortedList*, void* );
	int	( *List_GetIndex )( SortedList*, void*, int* );
	int   ( *List_Insert )( SortedList*, void*, int );
	int   ( *List_Remove )( SortedList*, int );
	int   ( *List_IndexOf )( SortedList*, void* );

	#if MIRANDA_VER >= 0x0600
	int   ( *List_InsertPtr)( SortedList* list, void* p );
	int   ( *List_RemovePtr)( SortedList* list, void* p );
	#endif
};

#define MS_SYSTEM_GET_LI  "Miranda/System/GetLI"

/*

	-- Thread Safety --

	Proper thread safe shutdown was implemented in 0.3.0.0 (2003/04/18)
	and not	before, therefore it is improper that any MT plugins be used
	with earlier versions of Miranda (as hav0c will result)

	Note: This does not apply to MT plugins which included their own
	thread-safe shutdown routines.

	Shutdown thread safety works thusly:

	All new threads must call MS_SYSTEM_THREAD_PUSH and MS_SYSTEM_THREAD_POP
	when they return.

	Due to the nature of thread creation, it is illegal to assume
	just a call pair of MS_SYSTEM_THREAD_PUSH inside the thread will
	be enough -- the source thread may only return when the new child
	thread has actually executed MS_SYSTEM_THREAD_PUSH

	This is because a thread maybe in an undefined state at the point
	when the thread creation routine returns, thus Miranda may exit
	thinking it is safe to do so, because MS_SYSTEM_THREAD_PUSH was not
	called in time.

	See miranda.c for how this can be done using an event object
	which is signalled just after the MS_SYSTEM_THREAD_PUSH call is executed
	and so the source thread knows that the created thread is known to Miranda.

	-- What happens when Miranda exits --

	Miranda will firstly set an event object to signalled, this will
	make MS_SYSTEM_TERMINATED return TRUE, it will then fire ME_SYSTEM_PRESHUTDOWN
	at this point, no plugins or modules are unloaded.

	Miranda will then enumerate all active threads and queue an APC call
	to each thread, so any thread in an alertable state will become active,
	this functionailty may not be required by your threads: but if you use
	the Winsock2 event object system or Sleep() please use the alertable
	wait functions, so that the thread will 'wake up' when Miranda queues
	a message to it, SleepEx() will return WAIT_IO_COMPLETION if this happens.

	After all threads have been signalled, Miranda will spin on the unwind thread stack waiting
	for it to become empty, in this time, it will carry on processing thread
	switches, clearing it's own APC calls (used by NotifyEventHooks(), CallServiceSync())

	So a thread should be written in this kind of form:

	void mythread(void *arg)
	{
		// assume all thread pushing/popping is done by forkthread()
		int run=1;
		for (;run;)
		{
			Beep(4391,500);
			SleepEx(1500,TRUE);
			if (Miranda_Terminated()) {
				Beep(5000,150); run=0;
			} //if
		} //for
	}

	The above will make a nice Beep every 1.5 seconds and when the UI
	quits, it will make a lower beep and then return.

	As many copies of this thread maybe running, the creator does not need
	to worry about what to do with previous threads, as long as they're on the
	unwind stack.If there are any global resources (and they're mutex) you can free() them
	at Unload(), which will only be called, after all threads have returned.

	-- Summary --

	MS_SYSTEM_TERMINATED (will start returning TRUE)
	ME_SYSTEM_PRESHUTDOWN will be fired (The CList won't be visible at this point)

	All PROTOTYPE_PROTOCOL registered plugins will be sent ID_STATUS_OFFLINE
	automatically.

	All the threads will be notified via QueueUserAPC() and then Miranda
	will poll on the unwind thread queue until it is empty.

	ME_SYSTEM_SHUTDOWN will be fired, the database will be unloaded, the core
	will be unloaded -- Miranda will return.

*/

/*
wParam=0
lParam=0

Add a thread to the unwind wait stack that Miranda will poll on
when it is tearing down modules.

This must be called in the context of the thread that is to be pushed
i.e. there are no args, it works out what thread is being called
and gets itself a handle to the calling thread.

*/
#define MS_SYSTEM_THREAD_PUSH		"Miranda/Thread/Push"

/*
wParam=0
lParam=0

Remove a thread from the unwind wait stack -- it is expected
that the call be made in the context of the thread to be removed.

Miranda will begin to tear down modules and plugins if/when the
last thread from the unwind stack is removed.
*/
#define MS_SYSTEM_THREAD_POP		"Miranda/Thread/Pop"

/*
wParam=0
lParam=0

This hook is fired just before the thread unwind stack is used,
it allows MT plugins to shutdown threads if they have any special
processing to do, etc.

*/
#define ME_SYSTEM_PRESHUTDOWN		"Miranda/System/PShutdown"

/*
wParam=0
lParam=0

Returns TRUE when Miranda has got WM_QUIT and is in the process
of shutting down
*/
#define MS_SYSTEM_TERMINATED		"Miranda/SysTerm"

/*
	wParam : 0
	lParam : (address) void (__cdecl *callback) (void)
	Affect : Setup a function pointer to be called after main loop iterations, it allows for
		     idle processing, See notes
	Returns: 1 on success, 0 on failure

	Notes  : This service will only allow one function to be registered, if one is registered, 0 will be returned
		     Remember that this uses __cdecl.
	Version: Added during 0.3.4+

*/
#define MS_SYSTEM_SETIDLECALLBACK	"Miranda/SetIdleCallback"

/*
	wParam : 0
	lParam : &tick
	Affect : return the last window tick where a monitored event was seen, currently WM_CHAR/WM_MOUSEMOVE
	Returns: Always returns 0
	Version: Added during 0.3.4+ (2004/09/12)
*/
#define MS_SYSTEM_GETIDLE "Miranda/GetIdle"

/*
	wParam: cchMax (max length of buffer)
	lParam: pointer to buffer to fill
	Affect: Returns the build timestamp of the core, as a string of YYYYMMDDhhmmss, this service might
		not exist and therefore the build is before 2004-09-30
	Returns: zero on success, non zero on failure
	Version: 0.3.4a+ (2004/09/30)
	DEFUNCT: This service was removed on 0.3.4.3+ (2004/11/19) use APILEVEL
*/
#define MS_SYSTEM_GETBUILDSTRING "Miranda/GetBuildString"

__inline static int Miranda_Terminated(void)
{
	return CallService(MS_SYSTEM_TERMINATED,0,0);
}

__inline static void miranda_sys_free(void *ptr)
{
	if (ptr) {
		struct MM_INTERFACE mm;
		mm.cbSize=sizeof(struct MM_INTERFACE);
		CallService(MS_SYSTEM_GET_MMI,0,(LPARAM)&mm);
		mm.mmi_free(ptr);
	}
}

/* Missing service catcher
Is being called when one calls the non-existent service.
All parameters are stored in the special structure

The event handler takes 0 as wParam and TMissingServiceParams* as lParam.

0.4.3+ addition (2006/03/27)
*/

typedef struct
{
	const char* name;
	WPARAM      wParam;
	LPARAM      lParam;
}
	MISSING_SERVICE_PARAMS;

#define ME_SYSTEM_MISSINGSERVICE "System/MissingService"

#endif // M_SYSTEM_H
