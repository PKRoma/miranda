{***************************************************************
 * Project     : Miranda Plugin API for Delphi
 * Unit Name   : m_database
 * Description : Converted Headerfile from m_database.h
 *
 * Author      : Christian Kästner
 * Date        : 28.04.2001
 *
 * Copyright © 2001 by Christian Kästner (christian.k@stner.de)
 ****************************************************************}

unit m_database;

interface

uses windows;
{****************** DATABASE MODULE **************************}
//Richard

{	Notes (as I think of them):
- The module is 100% thread-safe
- The database is the main routing point for the vast majority of Miranda.
  Events are sent from the protocol module to here, and the send/recv message
  module (for example) hooks the db/event/added event. Events like 'contact
  online status changed' do not come through here - icqlib will send that one.
- contacts work much the same. the find/add users module calls db/contact/add
  and db/contact/writesetting and the contact list will get db/contact/added
  and db/contact/settingchanged events
- The user is just a special contact. A hcontact of NULL in most functions
  means the user. Functions in which it cannot be used will be stated
- events attached to the user are things like system messages
- also in this module are crypt/decrypt functions for stuff that should be
  obfuscated on the disk, and some time functions for dealing with timestamps
  in events.
- the contactsettings system is designed for being read by many different
  modules. eg lots of people will be interested in "ICQ"/"UIN", but the module
  name passed to contact/writesetting should always be your own. The Mirabilis
  ICQ database importer clearly has to be an exception to this rule, along with
  a few other bits.
- the current database format means that geteventcontact is exceptionally slow.
  It should be avoidable in most cases so I'm not too concerned, but if people
  really need to use it a lot, I'll sort it out.
- handles do not need to be closed unless stated
- the database is loaded as a memory mapped file. This has various
  disadvantages but a massive advantage in speed for random access.
- The database is optimised for reading. Write performance is fairly bad,
  except for adding events which is the most common activity and pretty good.
- I'll work on caching to improve this later
- Deleted items are left as empty space and never reused. All new items are
  put at the end. A count is kept of this slack space and at some point a
  separate programme will need to be written to repack the database when the
  slack gets too high. It's going to be a good few months of usage before this
  can happen to anyone though, so no rush.
}

{******************* GENERALLY USEFUL STUFF**********************}

//DBVARIANT: used by db/contact/getsetting and db/contact/writesetting
const
  DBVT_BYTE   =1;	   //bVal and cVal are valid
  DBVT_WORD   =2;	   //wVal and sVal are valid
  DBVT_DWORD  =4;	   //dVal and lVal are valid
  DBVT_ASCIIZ =255;  //pszVal is valid
  DBVT_BLOB   =254;  //cpbVal and pbVal are valid
  DBVTF_VARIABLELENGTH  = $80;

type
  //This structure is not converted correctly to pascal. It would be possible
  //with a case statement but I've done it without it to keep clear what really
  //happens.
  //as a documentation how this must work here is a short statement from the
  //plugindev forum:
(*DBVARIANT is 12 bytes long because of padding (to make memory access more
  efficient). Without padding it would be 7 bytes long. 1 byte for the type,
  the other 6 bytes for the largest element in the union (yes, all the elements
  of the union are stuck on top of each other in memory). The largest element of
  the union is the struct (which forces its contents not to be on top of each
  other). This is a WORD for the length (2 bytes) and a pointer to a byte for
  the data (4 bytes).

  Here's a map of the memory:
  0000 BYTE type
  0004 bVal, cVal, wVal, sVal, dVal, lVal, pszVal, cpbVal
  0008 pbVal *)
  //in all Delphi plugins you need to convert this value yourself or just use
  //the functions in clisttools
  PDBVARIANT=^TDBVARIANT;
  TDBVARIANT=record
    ttype:Byte;
    val:Pointer;
    pblob:Pointer;
  end;

{****************************************************************}
{************************ SERVICES ******************************}
{****************************************************************}

{* DB/Contact/GetProfileName service
Gets the name of the profile currently being used by the database module. This
is the same as the filename of the database, minus extension
  wParam=(WPARAM)(UINT)cbName
  lParam=(LPARAM)(char*)pszName
pszName is a pointer to the buffer that receives the name of the profile
cbName is the size in bytes of the pszName buffer
Returns 0 on success or nonzero otherwise
*}
const
  MS_DB_GETPROFILENAME='DB/GetProfileName';

{************************ Contact *******************************}

{ DB/Contact/GetSetting service
Look up the value of a named setting for a specific contact in the database
  wParam=(WPARAM)(HANDLE)hContact
  lParam=(LPARAM)(DBCONTACTGETSETTING*)&dbcgs
hContact should have been returned by find*contact or addcontact
Caller is responsible for free()ing dbcgs.pValue->pszVal and pbVal if they are
returned
Returns 0 on success or nonzero if the setting name was not found or hContact
was invalid
Because this is such a common function there are some short helper function at
the bottom of this header that use it.
}
type
  TDBCONTACTGETSETTING=record
    szModule:PChar;	// pointer to name of the module that wrote the
	                        // setting to get
    szSetting:PChar;	// pointer to name of the setting to get
    pValue:PDBVariant;		// pointer to variant to receive the value
  end;
const
  MS_DB_CONTACT_GETSETTING='DB/Contact/GetSetting';
  

{ DB/Contact/FreeVariant service
Free the memory in a DBVARIANT that is allocated by a call to
db/contact/getsetting
  wParam=0
  lParam=(LPARAM)(DBVARIANT*)&dbv
Returns 0 on success, nonzero otherwise
This service is actually just a wrapper around a call to free() and a test to
check that it is a string or a blob in the variant. It exists because DLLs have
their own heap and cannot free the memory allocated in db/contact/getsetting.
Thus it need not be called if you know the variant contains some form of int,
and you will often see free() used instead in code written before I noticed
this problem.
Good style, of course, dictates that it should be present to match all calls to
db/contact/getsetting, but that's not going to happen of course.
There's a helper function for this at the bottom of this header too.
}
const
  MS_DB_CONTACT_FREEVARIANT='DB/Contact/FreeVariant';
  

{ DB/Contact/WriteSetting service
Change the value of, or create a new value with, a named setting for a specific
contact in the database to the given value
  wParam=(WPARAM)(HANDLE)hContact
  lParam=(LPARAM)(DBCONTACTWRITESETTING*)&dbcws
hContact should have been returned by find*contact or addcontact
Returns 0 on success or nonzero if hContact was invalid
Because this is such a common function there are some short helper function at
the bottom of this header that use it.
Triggers a db/contact/changed event just before it returns.
}
type
  PDBCONTACTWRITESETTING=^TDBCONTACTWRITESETTING;
  TDBCONTACTWRITESETTING=record
    szModule:PChar;	// pointer to name of the module that wrote the
	                        // setting to get
    szSetting:PChar;	// pointer to name of the setting to set
    Value:TDBVariant;		// variant containing the value to set
  end;
const
  MS_DB_CONTACT_WRITESETTING='DB/Contact/WriteSetting';
  

{ DB/Contact/DeleteSetting service
Removes a named setting for a specific contact from the database
  wParam=(WPARAM)(HANDLE)hContact
  lParam=(LPARAM)(DBCONTACTGETSETTING*)&dbcgs
hContact should have been returned by find*contact or addcontact
pValue from dbcgs is not used.
Returns 0 on success or nonzero if the setting was not present or hContact was
invalid
Does not trigger any events
}
const
  MS_DB_CONTACT_DELETESETTING='DB/Contact/DeleteSetting';
  

{ DB/Contact/GetCount service
Gets the number of contacts in the database, which does not count the user
  wParam=lParam=0
Returns the number of contacts. They can be retrieved using contact/findfirst
and contact/findnext
}
const
  MS_DB_CONTACT_GETCOUNT='DB/Contact/GetCount';
  

{ DB/Contact/FindFirst service
Gets the handle of the first contact in the database. This handle can be used
with loads of functions. It does not need to be closed.
  wParam=lParam=0
Returns a handle to the first contact in the db on success, or NULL if there
are no contacts in the db.
}
const
  MS_DB_CONTACT_FINDFIRST='DB/Contact/FindFirst';
  

{ DB/Contact/FindNext service
Gets the handle of the next contact after hContact in the database. This handle
can be used with loads of functions. It does not need to be closed.
  wParam=(WPARAM)(HANDLE)hContact
  lParam=0
Returns a handle to the contact after hContact in the db on success or NULL if
hContact was the last contact in the db or hContact was invalid.
}
const
  MS_DB_CONTACT_FINDNEXT='DB/Contact/FindNext';
  

{ DB/Contact/Delete
Deletes the contact hContact from the database and all events and settings
associated with it.
  wParam=(WPARAM)(HANDLE)hContact
  lParam=0
Returns 0 on success or nonzero if hContact was invalid
Please don't try to delete the user contact (hContact=NULL)
Triggers a db/contact/deleted event just *before* it removes anything
Because all events are deleted, lots of people may end up with invalid event
handles from this operation, which they should be prepared for.
}
const
  MS_DB_CONTACT_DELETE='DB/Contact/Delete';
  

{ DB/Contact/Add
Adds a new contact to the database. New contacts initially have no settings
whatsoever, they must all be added with db/contacts/writesetting.
  wParam=lParam=0
Returns a handle to the newly created contact on success, or NULL otherwise.
Triggers a db/contact/added event just before it returns.
}
const
  MS_DB_CONTACT_ADD='DB/Contact/Add';
  

{************************* Event ********************************}

{ DB/Event/GetCount service
Gets the number of events in the chain belonging to a contact in the database.
  wParam=(WPARAM)(HANDLE)hContact
  lParam=0
Returns the number of events in the chain owned by hContact or -1 if hContact
is invalid. They can be retrieved using the event/find* services.
}
const
  MS_DB_EVENT_GETCOUNT='DB/Event/GetCount';
  

{ DB/Event/Add
Adds a new event to a contact's event list
  wParam=(WPARAM)(HANDLE)hContact
  lParam=(LPARAM)(DBEVENTINFO*)&dbe
Returns a handle to the newly added event, or NULL on failure
Triggers a db/event/added event just before it returns.
Events are sorted chronologically as they are entered, so you cannot guarantee
that the new hEvent is the last event in the chain, however if a new event is
added that has a timestamp less than 90 seconds *before* the event that should
be after it, it will be added afterwards, to allow for protocols that only
store times to the nearest minute, and slight delays in transports.
There are a few predefined eventTypes below for easier compatibility, but
modules are free to define their own, beginning at 1000
DBEVENTINFO.timestamp is in GMT, as returned by time(). There are services
db/time{ below with useful stuff for dealing with it.
}
const
  DBEF_FIRST =   1;    //this is the first event in the chain;
                           //internal only: *do not* use this flag
  DBEF_SENT  =   2;    //this event was sent by the user. If not set this
                           //event was received.
  DBEF_READ  =   4;    //event has been read by the user. It does not need
                           //to be processed any more except for history.
type
  TDBEVENTINFO=record
    cbSize:Integer;       //size of the structure in bytes
    szModule:PChar;	  //pointer to name of the module that 'owns' this
                          //event, ie the one that is in control of the data format
    timestamp:DWord;  //seconds since 00:00, 01/01/1970. Gives us times until
                              //2106 unless you use the standard C library which is
                                              //signed and can only do until 2038. In GMT.
    flags:DWord;	  //the omnipresent flags
    eventType:Word;	  //module-defined event type field
    cbBlob:DWord;	  //size of pBlob in bytes
    pBlob:PByte;	  //pointer to buffer containing module-defined event data
  end;
const
  EVENTTYPE_MESSAGE  = 0;
  EVENTTYPE_URL      = 1;
const
  MS_DB_EVENT_ADD='DB/Event/Add';
  

{ DB/Event/Delete
Removes a single event from the database
  wParam=(WPARAM)(HANDLE)hContact
  lParam=(LPARAM)(HANDLE)hDbEvent
hDbEvent should have been returned by db/event/add or db/event/find*event
Returns 0 on success, or nonzero if hDbEvent was invalid
Triggers a db/event/deleted event just *before* the event is deleted
}
const
  MS_DB_EVENT_DELETE='DB/Event/Delete';
  

{ DB/Event/GetBlobSize
Retrieves the space in bytes required to store the blob in hDbEvent
  wParam=(WPARAM)(HANDLE)hDbEvent
  lParam=0
hDbEvent should have been returned by db/event/add or db/event/find*event
Returns the space required in bytes, or -1 if hDbEvent is invalid
}
const
  MS_DB_EVENT_GETBLOBSIZE='DB/Event/GetBlobSize';
  

{ DB/Event/Get
Retrieves all the information stored in hDbEvent
  wParam=(WPARAM)(HANDLE)hDbEvent
  lParam=(LPARAM)(DBEVENTINFO*)&dbe
hDbEvent should have been returned by db/event/add or db/event/find*event
Returns 0 on success or nonzero if hDbEvent is invalid
Don't forget to set dbe.cbSize, dbe.pBlob and dbe.cbBlob before calling this
service
The correct value dbe.cbBlob can be got using db/event/getblobsize
If successful, all the fields of dbe are filled. dbe.cbBlob is set to the
actual number of bytes retrieved and put in dbe.pBlob
If dbe.cbBlob is too small, dbe.pBlob is filled up to the size of dbe.cbBlob
and then dbe.cbBlob is set to the required size of data to go in dbe.pBlob
On return, dbe.szModule is a pointer to the database module's own internal list
of modules. Look but don't touch.
}
const
  MS_DB_EVENT_GET='DB/Event/Get';
  

{ DB/Event/MarkRead
Changes the flags for an event to mark it as read.
  wParam=(WPARAM)(HANDLE)hContact
  lParam=(LPARAM)(HANDLE)hDbEvent
hDbEvent should have been returned by db/event/add or db/event/find*event
Returns the entire flag DWORD for the event after the change, or -1 if hDbEvent
is invalid.
This is the one database write operation that does not trigger an event.
Modules should not save flags states for any length of time.
}
const
  MS_DB_EVENT_MARKREAD='DB/Event/MarkRead';
  

{ DB/Event/GetContact
Retrieves a handle to the contact that owns hDbEvent.
  wParam=(WPARAM)(HANDLE)hDbEvent
  lParam=0
hDbEvent should have been returned by db/event/add or db/event/find*event
NULL is a valid return value, meaning, as usual, the user.
Returns (HANDLE)(-1) if hDbEvent is invalid, or the handle to the contact on
success
This service is exceptionally slow. Use only when you have no other choice at
all.
}
const
  MS_DB_EVENT_GETCONTACT='DB/Event/GetContact';
  

{ DB/Event/FindFirst
Retrieves a handle to the first event in the chain for hContact
  wParam=(WPARAM)(HANDLE)hContact
  lParam=0
Returns the handle, or NULL if hContact is invalid or has no events
Events in a chain are sorted chronologically automatically
}
const
  MS_DB_EVENT_FINDFIRST='DB/Event/FindFirst';
  

{ DB/Event/FindFirstUnread
Retrieves a handle to the first unread event in the chain for hContact
  wParam=(WPARAM)(HANDLE)hContact
  lParam=0
Returns the handle, or NULL if hContact is invalid or all its events have been
read
Events in a chain are sorted chronologically automatically, but this does not
necessarily mean that all events after the first unread are unread too. They
should be checked individually with event/findnext and event/get
This service is designed for startup, reloading all the events that remained
unread from last time
}
const
  MS_DB_EVENT_FINDFIRSTUNREAD='DB/Event/FindFirstUnread';
  

{ DB/Event/FindLast
Retrieves a handle to the last event in the chain for hContact
  wParam=(WPARAM)(HANDLE)hContact
  lParam=0
Returns the handle, or NULL if hContact is invalid or has no events
Events in a chain are sorted chronologically automatically
}
const
  MS_DB_EVENT_FINDLAST='DB/Event/FindLast';
  

{ DB/Event/FindNext
Retrieves a handle to the next event in a chain after hDbEvent
  wParam=(WPARAM)(HANDLE)hDbEvent
  lParam=0
Returns the handle, or NULL if hDbEvent is invalid or is the last event
Events in a chain are sorted chronologically automatically
}
const
  MS_DB_EVENT_FINDNEXT='DB/Event/FindNext';
  

{ DB/Event/FindPrev
Retrieves a handle to the previous event in a chain before hDbEvent
  wParam=(WPARAM)(HANDLE)hDbEvent
  lParam=0
Returns the handle, or NULL if hDbEvent is invalid or is the first event
Events in a chain are sorted chronologically automatically
}
const
  MS_DB_EVENT_FINDPREV='DB/Event/FindPrev';
  

{************************* Encryption ***************************}

{ DB/Crypt/EncodeString
Scrambles pszString in-place using a strange encryption algorithm
  wParam=(WPARAM)(int)cbString
  lParam=(LPARAM)(char*)pszString
cbString is the size of the buffer pointed to by pszString, *not* the length
of pszString. This service may be changed at a later date such that it
increases the length of pszString
Returns 0 always
}
const
  MS_DB_CRYPT_ENCODESTRING='DB/Crypt/EncodeString';
  

{ DB/Crypt/DecodeString
Descrambles pszString in-place using the strange encryption algorithm
  wParam=(WPARAM)(int)cbString
  lParam=(LPARAM)(char*)pszString
Reverses the operation done by crypt/encodestring
cbString is the size of the buffer pointed to by pszString, *not* the length
of pszString.
Returns 0 always
}
const
  MS_DB_CRYPT_DECODESTRING='DB/Crypt/DecodeString';
  

{*************************** Time *******************************}

{ DB/Time/TimestampToLocal
Converts a GMT timestamp into local time
  wParam=(WPARAM)(DWORD)timestamp
  lParam=0
Returns the converted value
Timestamps have zero at midnight 1/1/1970 GMT, this service converts such a
value to be based at midnight 1/1/1970 local time.
This service does not use a simple conversion based on the current offset
between GMT and local. Rather, it figures out whether daylight savings time
would have been in place at the time of the stamp and gives the local time as
it would have been at the time and date the stamp contains.
This service isn't nearly as useful as db/time/TimestampToString below and I
recommend avoiding its use when possible so that you don't get your timezones
mixed up (like I did. Living at GMT makes things easier for me, but has certain
disadvantages :-) ).
}
const
  MS_DB_TIME_TIMESTAMPTOLOCAL ='DB/Time/TimestampToLocal';
  

{ DB/Time/TimestampToString
Converts a GMT timestamp into a customisable local time string
  wParam=(WPARAM)(DWORD)timestamp
  lParam=(LPARAM)(DBTIMETOSTRING*)&tts
Returns 0 always
Uses db/time/timestamptolocal for the conversion so read that description to
see what's going on.
The string is formatted according to the current user's locale, language and
preferences.
szFormat can have the following special characters:
  t  Time without seconds, eg hh:mm
  s  Time with seconds, eg hh:mm:ss
  m  Time without minutes, eg hh
  d  Short date, eg dd/mm/yyyy
  D  Long date, eg d mmmm yyyy
All other characters are copied across to szDest as-is
}
type
  TDBTIMETOSTRING=record
    szFormat:PChar;		//format string, as above
    szDest:PChar;		//place to put the output string
    cbDest:Integer;		//maximum number of bytes to put in szDest
  end;
const
  MS_DB_TIME_TIMESTAMPTOSTRING='DB/Time/TimestampToString';
  

{****************************************************************}
{************************* EVENTS *******************************}
{****************************************************************}

{ DB/Event/Added event
Called when a new event has been added to the event chain for a contact
  wParam=(WPARAM)(HANDLE)hContact
  lParam=(LPARAM)(HANDLE)hDbEvent
hDbEvent is a valid handle to the event. hContact is a valid handle to the
contact to which hDbEvent refers.
Since events are sorted chronologically, you cannot guarantee that hDbEvent is
at any particular position in the chain.
}
const
  ME_DB_EVENT_ADDED='DB/Event/Added';
  

{ DB/Event/Deleted event
Called when an event is about to be deleted from the event chain for a contact
  wParam=(WPARAM)(HANDLE)hContact
  lParam=(LPARAM)(HANDLE)hDbEvent
hDbEvent is a valid handle to the event which is about to be deleted, but it
won't be once your hook has returned.
hContact is a valid handle to the contact to which hDbEvent refers, and will
remain valid.
Returning nonzero from your hook will not stop the deletion, but it will, as
usual, stop other hooks from being called.
}
const
  ME_DB_EVENT_DELETED='DB/Event/Deleted';
  

{ DB/Contact/Added event
Called when a new contact has been added to the database
  wParam=(WPARAM)(HANDLE)hContact
  lParam=0
hContact is a valid handle to the new contact.
Contacts are initially created without any settings, so if you hook this event
you will almost certainly also want to hook db/contact/settingchanged as well.
}
const
  ME_DB_CONTACT_ADDED='DB/Contact/Added';
  

{ DB/Contact/Deleted event
Called when an contact is about to be deleted
  wParam=(WPARAM)(HANDLE)hContact
  lParam=0
hContact is a valid handle to the contact which is about to be deleted, but it
won't be once your hook has returned.
Returning nonzero from your hook will not stop the deletion, but it will, as
usual, stop other hooks from being called.
Deleting a contact invalidates all events in its chain.
}
const
  ME_DB_CONTACT_DELETED='DB/Contact/Deleted';
  

{ DB/Contact/SettingChanged event
Called when a contact has had one of its settings changed
  wParam=(WPARAM)(HANDLE)hContact
  lParam=(LPARAM)(DBCONTACTWRITESETTING*)&dbcws
hContact is a valid handle to the contact that has changed.
This event will be triggered many times rapidly when a whole bunch of values
are set.
Modules which hook this should be aware of this fact and quickly return if they
are not interested in the value that has been changed.
Careful not to get into infinite loops with this event.
The structure dbcws is the same one as is passed to the original service, so
don't change any of the members.
}
const
  ME_DB_CONTACT_SETTINGCHANGED='DB/Contact/SettingChanged';


          (*    not tranlated yet
{****************************************************************}
{******************** SETTINGS HELPER FUNCTIONS *****************}
{****************************************************************}

static int __inline DBGetContactSettingByte(HANDLE hContact,const char *szModule,const char *szSetting,int errorValue)
{
	DBVARIANT dbv;
	DBCONTACTGETSETTING cgs;

	cgs.szModule=szModule;
	cgs.szSetting=szSetting;
	cgs.pValue=&dbv;
	if(CallService(MS_DB_CONTACT_GETSETTING,(WPARAM)hContact,(LPARAM)&cgs))
		return errorValue;
#ifdef _DEBUG
	if(dbv.type!=DBVT_BYTE) MessageBox(NULL,'DB: Attempt to get wrong type of value, byte','',MB_OK);
#endif
	return dbv.bVal;
}

static int __inline DBGetContactSettingWord(HANDLE hContact,const char *szModule,const char *szSetting,int errorValue)
{
	DBVARIANT dbv;
	DBCONTACTGETSETTING cgs;

	cgs.szModule=szModule;
	cgs.szSetting=szSetting;
	cgs.pValue=&dbv;
	if(CallService(MS_DB_CONTACT_GETSETTING,(WPARAM)hContact,(LPARAM)&cgs))
		return errorValue;
#ifdef _DEBUG
	if(dbv.type!=DBVT_WORD) MessageBox(NULL,'DB: Attempt to get wrong type of value, word','',MB_OK);
#endif
	return dbv.wVal;
}

static DWORD __inline DBGetContactSettingDword(HANDLE hContact,const char *szModule,const char *szSetting,DWORD errorValue)
{
	DBVARIANT dbv;
	DBCONTACTGETSETTING cgs;

	cgs.szModule=szModule;
	cgs.szSetting=szSetting;
	cgs.pValue=&dbv;
	if(CallService(MS_DB_CONTACT_GETSETTING,(WPARAM)hContact,(LPARAM)&cgs))
		return errorValue;
#ifdef _DEBUG
	if(dbv.type!=DBVT_DWORD) MessageBox(NULL,'DB: Attempt to get wrong type of value, dword','',MB_OK);
#endif
	return dbv.dVal;
}

static int __inline DBGetContactSetting(HANDLE hContact,const char *szModule,const char *szSetting,DBVARIANT *dbv)
{
	DBCONTACTGETSETTING cgs;
	cgs.szModule=szModule;
	cgs.szSetting=szSetting;
	cgs.pValue=dbv;
	return CallService(MS_DB_CONTACT_GETSETTING,(WPARAM)hContact,(LPARAM)&cgs);
}

static int __inline DBFreeVariant(DBVARIANT *dbv)
{
	return CallService(MS_DB_CONTACT_FREEVARIANT,0,(LPARAM)dbv);
}

static int __inline DBDeleteContactSetting(HANDLE hContact,const char *szModule,const char *szSetting)
{
	DBCONTACTGETSETTING cgs;
	cgs.szModule=szModule;
	cgs.szSetting=szSetting;
	return CallService(MS_DB_CONTACT_DELETESETTING,(WPARAM)hContact,(LPARAM)&cgs);
}

static int __inline DBWriteContactSettingByte(HANDLE hContact,const char *szModule,const char *szSetting,BYTE val)
{
	DBCONTACTWRITESETTING cws;

	cws.szModule=szModule;
	cws.szSetting=szSetting;
	cws.value.type=DBVT_BYTE;
	cws.value.bVal=val;
	return CallService(MS_DB_CONTACT_WRITESETTING,(WPARAM)hContact,(LPARAM)&cws);
}

static int __inline DBWriteContactSettingWord(HANDLE hContact,const char *szModule,const char *szSetting,WORD val)
{
	DBCONTACTWRITESETTING cws;

	cws.szModule=szModule;
	cws.szSetting=szSetting;
	cws.value.type=DBVT_WORD;
	cws.value.wVal=val;
	return CallService(MS_DB_CONTACT_WRITESETTING,(WPARAM)hContact,(LPARAM)&cws);
}

static int __inline DBWriteContactSettingDword(HANDLE hContact,const char *szModule,const char *szSetting,DWORD val)
{
	DBCONTACTWRITESETTING cws;

	cws.szModule=szModule;
	cws.szSetting=szSetting;
	cws.value.type=DBVT_DWORD;
	cws.value.dVal=val;
	return CallService(MS_DB_CONTACT_WRITESETTING,(WPARAM)hContact,(LPARAM)&cws);
}

static int __inline DBWriteContactSettingString(HANDLE hContact,const char *szModule,const char *szSetting,const char *val)
{
	DBCONTACTWRITESETTING cws;

	cws.szModule=szModule;
	cws.szSetting=szSetting;
	cws.value.type=DBVT_ASCIIZ;
	cws.value.pszVal=(char* )val;
	return CallService(MS_DB_CONTACT_WRITESETTING,(WPARAM)hContact,(LPARAM)&cws);
}
                            *)


implementation



end.
