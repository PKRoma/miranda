{***************************************************************
 * Project     : Miranda Plugin API for Delphi
 * Unit Name   : m_system
 * Description : Converted Headerfile from m_system.h
 *
 * Author      : Christian Kästner
 * Date        : 28.04.2001
 * Last Change : 22.12.2001
 *
 * Copyright © 2001 by Christian Kästner (christian.k@stner.de)
 ****************************************************************}

unit m_system;

interface

const
  MIRANDANAME='Miranda';

//miranda/system/modulesloaded
//called after all modules have been successfully initialised
//wParam=lParam=0
//used to resolve double-dependencies in the module load order
const
  ME_SYSTEM_MODULESLOADED ='Miranda/System/ModulesLoaded';

//miranda/system/shutdown event
//called just before the application terminates
//the database is still guaranteed to be running during this hook.
//wParam=lParam=0
const
  ME_SYSTEM_SHUTDOWN  ='Miranda/System/Shutdown';

//miranda/system/oktoexit event
//called before the app goes into shutdown routine to make sure everyone is
//happy to exit
//wParam=lParam=0
//return nonzero to stop the exit cycle
const
  ME_SYSTEM_OKTOEXIT  ='Miranda/System/OkToExitEvent';

//miranda/system/oktoexit service
//Check if everyone is happy to exit
//wParam=lParam=0
//if everyone acknowleges OK to exit then returns true, otherwise false
const
  MS_SYSTEM_OKTOEXIT  ='Miranda/System/OkToExit'; 

//gets the version number of Miranda encoded as a DWORD     v0.1.0.1+
//wParam=lParam=0
//returns the version number, encoded as one version per byte, therefore
//version 1.2.3.10 is 0x0102030a
const
  MS_SYSTEM_GETVERSION='Miranda/System/GetVersion';

//gets the version of Miranda encoded as text   v0.1.0.1+
//wParam=cch
//lParam=(LPARAM)(char*)pszVersion
//cch is the size of the buffer pointed to by pszVersion, in bytes
//may return a build qualifier, such as='0.1.0.1 alpha';
//returns 0 on success, nonzero on failure
const
  MS_SYSTEM_GETVERSIONTEXT='Miranda/System/GetVersionText';

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

//Miranda is compiled with the multithreaded runtime - don't forget to do the
//same with your plugin.
const
  MS_SYSTEM_WAITONHANDLE  ='Miranda/System/WaitOnHandle';

//Removes a HANDLE from the wait list   v0.1.2.0+
//wParam=(WPARAM)(HANDLE)hObject
//lParam=0
//returns 0 on success or nonzero on failure.
const
  MS_SYSTEM_REMOVEWAIT    ='Miranda/System/RemoveWait';


implementation

end.
