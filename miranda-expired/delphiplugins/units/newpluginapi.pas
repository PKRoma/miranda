{***************************************************************
 * Project     : Miranda Plugin API for Delphi
 * Unit Name   : m_system
 * Description : Converted Headerfile from newpluginapi
 *               Contains PluginLink and PluginInfo structure
 * Author      : Christian Kästner
 * Date        : 28.04.2001
 *
 * Copyright © 2001 by Christian Kästner (christian.k@stner.de)
 ****************************************************************}

unit newpluginapi;

interface

uses windows, m_plugins;


type
  PPLUGININFO=^TPLUGININFO;
  TPLUGININFO=record
    cbSize:integer;
    shortName:pchar;
    version:dword;
    description:pchar;
    author:pchar;
    authorEmail:pchar;
    copyright:pchar;
    homepage:pchar;
    isTransient:byte;	   //leave this as 0 for now
    replacesDefaultModule:integer;//one of the DEFMOD_ constants in m_plugins.h or zero
                                  //if non-zero, this will supress the loading of the specified built-in module
                                  //with the implication that this plugin provides back-end-compatible features
  end;

type
  PMIRANDAHOOK=^TMIRANDAHOOK;
  TMIRANDAHOOK=function (wParam,lParam:DWord):integer;cdecl;
  PMIRANDASERVICE=^TMIRANDASERVICE;
  TMIRANDASERVICE=function (wParam,lParam:DWord):integer;cdecl;
const
  CALLSERVICE_NOTFOUND=$80000000;

  MAXMODULELABELLENGTH=64;
type
  PPLUGINLINK=^TPLUGINLINK;
  TPLUGINLINK=record
    {*************************hook functions***************************}
    { CreateHookableEvent
    Adds an named event to the list and returns a handle referring to it, or NULL
    on failure. Will be automatically destroyed on exit, or can be removed from the
    list earlier using DestroyHookableEvent()
    Will fail if the given name has already been used
    }
    CreateHookableEvent:function (Name:PChar):THandle;cdecl;

    { DestroyHookableEvent
    Removes the event hEvent from the list of events. All modules hooked to it are
    automatically unhooked. NotifyEventHooks() will fail if called with this hEvent
    again. hEvent must have been returned by CreateHookableEvent()
    Returns 0 on success, or nonzero if hEvent is invalid
    }
    DestroyHookableEvent:function (hEvent:THandle):Integer;cdecl;

    { NotifyEventHooks
    Calls every module in turn that has hooked hEvent, using the parameters wParam
    and lParam. hEvent must have been returned by CreateHookableEvent()
    Returns 0 on success
    -1 if hEvent is invalid
    If one of the hooks returned nonzero to indicate abort, returns that abort
      value immediately, without calling the rest of the hooks in the chain
    }
    NotifyEventHooks:function (hEvent:THandle;wParam,lParam:DWord):Integer;cdecl;

    { HookEvent
    Adds a new hook to the chain 'name', to be called when the hook owner calls
    NotifyEventHooks(). Returns NULL if name is not a valid event or a handle
    referring to the hook otherwise. Note that debug builds will warn with a
    MessageBox if a hook is attempted on an unknown event. All hooks will be
    automatically destroyed when their parent event is destroyed or the programme
    ends, but can be unhooked earlier using UnhookEvent(). hookProc() is defined as
    int HookProc(WPARAM wParam,LPARAM lParam)
    where you can substitute your own name for HookProc. wParam and lParam are
    defined by the creator of the event when NotifyEventHooks() is called.
    The return value is 0 to continue processing the other hooks, or nonzero
    to stop immediately. This abort value is returned to the caller of
    NotifyEventHooks() and should not be -1 since that is a special return code
    for NotifyEventHooks() (see above)
    }
    HookEvent:function (Name:PChar;hookProc:TMirandaHook):THandle;cdecl;

    { HookEventMessage
    Works as for HookEvent(), except that when the notifier is called a message is
    sent to a window, rather than a function being called.
    Note that SendMessage() is a decidedly slow function so please limit use of
    this function to events that are not called frequently, or to hooks that are
    only installed briefly
    The window procedure is called with the message 'message' and the wParam and
    lParam given to NotifyEventHooks(). No return value is sent.
    }
    HookEventMessage:function (Name:PChar;hhwnd:hwnd;mmessage:Cardinal):THandle;cdecl;

    { UnhookEvent
    Removes a hook from its event chain. It will no longer receive any events.
    hHook must have been returned by HookEvent() or HookEventMessage().
    Returns 0 on success or nonzero if hHook is invalid.
    }
    UnhookEvent:function (hHook:THandle):Integer;cdecl;


    {************************service functions*************************}
    { CreateServiceFunction
    Adds a new service function called 'name' to the global list and returns a
    handle referring to it. Service function handles are destroyed automatically
    on exit, but can be removed from the list earlier using
    DestroyServiceFunction()
    Returns NULL if name has already been used. serviceProc is defined by the
    caller as
    int ServiceProc(WPARAM wParam,LPARAM lParam)
    where the creator publishes the meanings of wParam, lParam and the return value
    Service functions must not return CALLSERVICE_NOTFOUND since that would confuse
    callers of CallService().
    }
    CreateServiceFunction:function (Name:PChar;serviceProc:TMIRANDASERVICE):THandle;cdecl;

    CreateTransientServiceFunction:Pointer;//Will be implemented later  ->HANDLE (*CreateTransientServiceFunction)(const char *,MIRANDASERVICE);

    { DestroyServiceFunction
    Removes the function associated with hService from the global service function
    list. Modules calling CallService() will fail if they try to call this
    service's name. hService must have been returned by CreateServiceFunction().
    Returns 0 on success or non-zero if hService is invalid.
    }
    DestroyServiceFunction:function (hService:THANDLE):Integer;cdecl;

    { CallService
    Finds and calls the service function 'name' using the parameters wParam and
    lParam.
    Returns CALLSERVICE_NOTFOUND if no service function called 'name' has been
    created, or the value the service function returned otherwise.
    }
    CallService:function (Name:PChar;wParam,lParam:DWord):Integer;cdecl;

    { ServiceExists
    Finds if a service with the given name exists
    Returns nonzero if the service was found, and zero if it was not
    }
    ServiceExists:function (Name:PChar):Integer;cdecl;		  //v0.1.0.1+
  end;


//handle versions
type
  TVersionInfo=record
    a,
    b,
    c,
    d:integer
  end;
function PLUGIN_MAKE_VERSION(a,b,c,d:integer):DWord;
function PLUGIN_GET_VERSION(ver:DWORD):TVersionInfo;
function CompareVersion(vera,verb:dword):Integer;


implementation


function PLUGIN_MAKE_VERSION(a,b,c,d:integer):DWord;
begin
  Result:=(((a and $FF) shl 24) or ((b and $FF) shl 16) or ((c and $FF) shl 8) or (d and $FF));
end;

function PLUGIN_GET_VERSION(ver:DWORD):TVersionInfo;
begin
  Result.d:=(ver and $000000FF);
  Result.c:=(ver and $0000FF00) shr 8;
  Result.b:=(ver and $00FF0000) shr 16;
  Result.a:=(ver and $FF000000) shr 24;
end;

function CompareVersion(vera,verb:dword):Integer;
{compares 2 versions. if versiona is newer than version b then the result is
greater than 0, otherwise it's smaller}
begin
  Result:=0;
  if PLUGIN_GET_VERSION(vera).a>PLUGIN_GET_VERSION(verb).a then
    Inc(Result,100);
  if PLUGIN_GET_VERSION(vera).a<PLUGIN_GET_VERSION(verb).a then
    Dec(Result,100);

  if PLUGIN_GET_VERSION(vera).b>PLUGIN_GET_VERSION(verb).b then
    Inc(Result,50);
  if PLUGIN_GET_VERSION(vera).b<PLUGIN_GET_VERSION(verb).b then
    Dec(Result,50);

  if PLUGIN_GET_VERSION(vera).c>PLUGIN_GET_VERSION(verb).c then
    Inc(Result,10);
  if PLUGIN_GET_VERSION(vera).c<PLUGIN_GET_VERSION(verb).c then
    Dec(Result,10);

  if PLUGIN_GET_VERSION(vera).d>PLUGIN_GET_VERSION(verb).d then
    Inc(Result,1);
  if PLUGIN_GET_VERSION(vera).d<PLUGIN_GET_VERSION(verb).d then
    Dec(Result,1);
end;

end.