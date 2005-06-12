/*

Miranda IM: the free IM client for Microsoft* Windows*

Copyright 2000-2003 Miranda ICQ/IM project, 
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

#include "commonheaders.h"
#include "database.h"
#include "dblists.h"


static int GetContactCount(WPARAM wParam,LPARAM lParam);
static int FindFirstContact(WPARAM wParam,LPARAM lParam);
static int FindNextContact(WPARAM wParam,LPARAM lParam);
static int DeleteContact(WPARAM wParam,LPARAM lParam);
static int AddContact(WPARAM wParam,LPARAM lParam);
static int IsDbContact(WPARAM wParam,LPARAM lParam);
static int AddContactVolatile(WPARAM wParam,LPARAM lParam);
static int DeleteContactVolatile(WPARAM wParam, LPARAM lParam);

extern CRITICAL_SECTION csDbAccess;
extern struct DBHeader dbHeader;
static HANDLE hContactDeletedEvent,hContactAddedEvent;

extern HANDLE hCacheHeap;
extern SortedList lContacts;

typedef struct {
	HANDLE hContact;
	DWORD hashOfModule;
	DWORD hashOfSetting;
	DBVARIANT dbv;
} DBCONTACT_VOLATILE_ROW;
static unsigned g_VolatileContactSpace;
static SortedList lVolatileContacts;

static int SortVolatileContacts(DBCONTACT_VOLATILE_ROW * a, DBCONTACT_VOLATILE_ROW * b)
{
	if ( a && b ) {
		// same hContact?
		if ( a->hContact < b->hContact ) return -1;
		if ( a->hContact > b->hContact ) return 1;
		// same module?
		if ( a->hashOfModule < b->hashOfModule ) return -1;
		if ( a->hashOfModule > b->hashOfModule ) return 1;
		// same setting?
		if ( a->hashOfSetting < b->hashOfSetting ) return -1;
		if ( a->hashOfSetting > b->hashOfSetting ) return 1;
		// same everything
		return 0;
	}
	return 1;
}

static int SortVolatileContactsWithFlags(DBCONTACT_VOLATILE_ROW * a, DBCONTACT_VOLATILE_ROW * b, unsigned flags)
{
	if ( flags == 0 ) return SortVolatileContacts(a, b);
	if ( a && b ) {
		if ( a->hContact < b->hContact ) return -1;
		if ( a->hContact > b->hContact ) return 1;
		return 0;
	}
	return 1;
}

int InitContacts(void)
{
	CreateServiceFunction(MS_DB_CONTACT_GETCOUNT,GetContactCount);
	CreateServiceFunction(MS_DB_CONTACT_FINDFIRST,FindFirstContact);
	CreateServiceFunction(MS_DB_CONTACT_FINDNEXT,FindNextContact);
	CreateServiceFunction(MS_DB_CONTACT_DELETE,DeleteContact);
	CreateServiceFunction(MS_DB_CONTACT_ADD,AddContact);
	CreateServiceFunction(MS_DB_CONTACT_IS,IsDbContact);
	CreateServiceFunction(MS_DB_ADD_VOLATILE,AddContactVolatile);
	CreateServiceFunction(MS_DB_DELETE_VOLATILE,DeleteContactVolatile);
	hContactDeletedEvent=CreateHookableEvent(ME_DB_CONTACT_DELETED);
	hContactAddedEvent=CreateHookableEvent(ME_DB_CONTACT_ADDED);
	g_VolatileContactSpace=0xEFFFFFFF;
	lVolatileContacts.increment=5;
	lVolatileContacts.sortFunc=SortVolatileContacts;
	return 0;
}

void UninitContacts(void)
{
	List_Destroy(&lVolatileContacts);
}

static int GetContactCount(WPARAM wParam,LPARAM lParam)
{
	int ret;
	
	EnterCriticalSection(&csDbAccess);
	ret=dbHeader.contactCount;
	LeaveCriticalSection(&csDbAccess);
	return ret;
}

static int FindFirstContact(WPARAM wParam,LPARAM lParam)
{
	int ret = 0;
	EnterCriticalSection(&csDbAccess);
	ret = (int)(HANDLE)dbHeader.ofsFirstContact;
	LeaveCriticalSection(&csDbAccess);
	return ret;
}

static int FindNextContact(WPARAM wParam,LPARAM lParam)
{
	int ret, index;
	struct DBContact *dbc;
	DBCachedContactValueList *VL = NULL;
	
	EnterCriticalSection(&csDbAccess);
	{
		DBCachedContactValueList VLtemp;
		VLtemp.hContact = (HANDLE)wParam;
		if ( List_GetIndex(&lContacts,&VLtemp,&index)) {
			VL = ( DBCachedContactValueList* )lContacts.items[index];
			if ( VL->hNext != NULL ) {
				LeaveCriticalSection(&csDbAccess);
				return (int)VL->hNext;
	}	}	}

	dbc=(struct DBContact*)DBRead(wParam,sizeof(struct DBContact),NULL);
	if(dbc->signature!=DBCONTACT_SIGNATURE)
		ret=(int)(HANDLE)NULL;
	else {
		if ( VL == NULL ) {
			VL = (DBCachedContactValueList*)HeapAlloc(hCacheHeap,HEAP_NO_SERIALIZE+HEAP_ZERO_MEMORY,sizeof(DBCachedContactValueList));
			VL->hContact = (HANDLE)wParam;
			List_Insert(&lContacts,VL,index);
		}
		VL->hNext = (HANDLE)dbc->ofsNext;
		ret=(int)(HANDLE)dbc->ofsNext;
	}
	LeaveCriticalSection(&csDbAccess);
	return ret;
}

static int DeleteContact(WPARAM wParam,LPARAM lParam)
{
	struct DBContact *dbc,*dbcPrev;
	DWORD ofsThis,ofsNext,ofsFirstEvent;
	struct DBContactSettings *dbcs;
	struct DBEvent *dbe;
	int index;
	
	if((HANDLE)wParam==NULL) return 1;
	EnterCriticalSection(&csDbAccess);
	dbc=(struct DBContact*)DBRead(wParam,sizeof(struct DBContact),NULL);
	if(dbc->signature!=DBCONTACT_SIGNATURE) {
		LeaveCriticalSection(&csDbAccess);
		return 1;
	}
	if ( (HANDLE)wParam == (HANDLE)dbHeader.ofsUser ) {
		LeaveCriticalSection(&csDbAccess);
		log0("FATAL: del of user chain attempted.");
		return 1;
	}
	log0("del contact");
	LeaveCriticalSection(&csDbAccess);
	//call notifier while outside mutex
	NotifyEventHooks(hContactDeletedEvent,wParam,0);
	//get back in
	EnterCriticalSection(&csDbAccess);

	{	DBCachedContactValueList VLtemp;
		VLtemp.hContact = (HANDLE)wParam;
		if ( List_GetIndex(&lContacts,&VLtemp,&index))
		{
			DBCachedContactValueList *VL = ( DBCachedContactValueList* )lContacts.items[index];
			DBCachedContactValue* V = VL->first;
			while ( V != NULL ) {
				DBCachedContactValue* V1 = V->next;
				if ( V->value.type == DBVT_ASCIIZ )
					HeapFree( hCacheHeap, HEAP_NO_SERIALIZE, V->value.pszVal );
				HeapFree( hCacheHeap, HEAP_NO_SERIALIZE, V );
				V = V1;
			}
			HeapFree( hCacheHeap, HEAP_NO_SERIALIZE, VL );

			List_Remove(&lContacts,index);
	}	}

	dbc=(struct DBContact*)DBRead(wParam,sizeof(struct DBContact),NULL);
	//delete settings chain
	ofsThis=dbc->ofsFirstSettings;
	ofsFirstEvent=dbc->ofsFirstEvent;
	while(ofsThis) {
		dbcs=(struct DBContactSettings*)DBRead(ofsThis,sizeof(struct DBContactSettings),NULL);
		ofsNext=dbcs->ofsNext;
		DeleteSpace(ofsThis,offsetof(struct DBContactSettings,blob)+dbcs->cbBlob);
		ofsThis=ofsNext;
	}
	//delete event chain
	ofsThis=ofsFirstEvent;
	while(ofsThis) {
		dbe=(struct DBEvent*)DBRead(ofsThis,sizeof(struct DBEvent),NULL);
		ofsNext=dbe->ofsNext;
		DeleteSpace(ofsThis,offsetof(struct DBEvent,blob)+dbe->cbBlob);
		ofsThis=ofsNext;
	}
	//find previous contact in chain and change ofsNext
	dbc=(struct DBContact*)DBRead(wParam,sizeof(struct DBContact),NULL);
	if(dbHeader.ofsFirstContact==wParam) {
		dbHeader.ofsFirstContact=dbc->ofsNext;
		DBWrite(0,&dbHeader,sizeof(dbHeader));
	}
	else {
		ofsNext=dbc->ofsNext;
		ofsThis=dbHeader.ofsFirstContact;
		dbcPrev=(struct DBContact*)DBRead(ofsThis,sizeof(struct DBContact),NULL);
		while(dbcPrev->ofsNext!=wParam) {
			if(dbcPrev->ofsNext==0) DatabaseCorruption();
			ofsThis=dbcPrev->ofsNext;
			dbcPrev=(struct DBContact*)DBRead(ofsThis,sizeof(struct DBContact),NULL);
		}
		dbcPrev->ofsNext=ofsNext;
		DBWrite(ofsThis,dbcPrev,sizeof(struct DBContact));
		{
			DBCachedContactValueList VLtemp;
			VLtemp.hContact = (HANDLE)ofsThis;
			if(List_GetIndex(&lContacts,&VLtemp,&index))
			{
				DBCachedContactValueList *VL = ( DBCachedContactValueList* )lContacts.items[index];
				VL->hNext = ( HANDLE )ofsNext;
		}	}
	}
	//delete contact
	DeleteSpace(wParam,sizeof(struct DBContact));
	//decrement contact count
	dbHeader.contactCount--;
	DBWrite(0,&dbHeader,sizeof(dbHeader));
	DBFlush(0);
	//quit
	LeaveCriticalSection(&csDbAccess);
	return 0;
}

static int AddContact(WPARAM wParam,LPARAM lParam)
{
	struct DBContact dbc;
	DWORD ofsNew;

	log0("add contact");
	EnterCriticalSection(&csDbAccess);
	ofsNew=CreateNewSpace(sizeof(struct DBContact));
	dbc.signature=DBCONTACT_SIGNATURE;
	dbc.eventCount=0;
	dbc.ofsFirstEvent=dbc.ofsLastEvent=0;
	dbc.ofsFirstSettings=0;
	dbc.ofsNext=dbHeader.ofsFirstContact;
	dbc.ofsFirstUnreadEvent=0;
	dbc.timestampFirstUnread=0;
	dbHeader.ofsFirstContact=ofsNew;
	dbHeader.contactCount++;
	DBWrite(ofsNew,&dbc,sizeof(struct DBContact));
	DBWrite(0,&dbHeader,sizeof(dbHeader));
	DBFlush(0);

	{	int index;

		DBCachedContactValueList *VL = (DBCachedContactValueList*)HeapAlloc(hCacheHeap,HEAP_NO_SERIALIZE+HEAP_ZERO_MEMORY,sizeof(DBCachedContactValueList));
		VL->hContact = (HANDLE)ofsNew;

		List_GetIndex(&lContacts,VL,&index);
		List_Insert(&lContacts,VL,index);
	}

	LeaveCriticalSection(&csDbAccess);
	NotifyEventHooks(hContactAddedEvent,(WPARAM)ofsNew,0);
	return (int)ofsNew;
} 

static int IsDbContact(WPARAM wParam,LPARAM lParam)
{
	struct DBContact dbc;
	DWORD ofsContact=(DWORD)wParam;
	int ret;

	EnterCriticalSection(&csDbAccess);
	{
		int index;
		DBCachedContactValueList VLtemp,*VL;
		VLtemp.hContact = (HANDLE)wParam;
		if(List_GetIndex(&lContacts,&VLtemp,&index))
			ret = TRUE;
		else {
			dbc=*(struct DBContact*)DBRead(ofsContact,sizeof(struct DBContact),NULL);
			ret=dbc.signature==DBCONTACT_SIGNATURE;

			if (ret) {
				VL = (DBCachedContactValueList*)HeapAlloc(hCacheHeap,HEAP_NO_SERIALIZE+HEAP_ZERO_MEMORY,sizeof(DBCachedContactValueList));
				VL->hContact = (HANDLE)wParam;
				List_Insert(&lContacts,VL,index);
	}	}	}

	LeaveCriticalSection(&csDbAccess);
	return ret;
}

static int AddContactVolatile(WPARAM wParam, LPARAM lParam)
{
	DBCONTACTVOLATILE * ai = (DBCONTACTVOLATILE *) lParam;
	if ( !( ai && ai->cbSize==sizeof(DBCONTACTVOLATILE) && ai->szProto ) ) return 1;
	EnterCriticalSection(&csDbAccess);
	ai->hContact=(HANDLE)g_VolatileContactSpace--;
	if ( (unsigned) ai->hContact < dbHeader.ofsFileEnd ) {
		/* the hContact "hit" allocated space, the likelyhood of this happening is low even with a 
		10mb profile it handled even 12million allocations and could take more */
		LeaveCriticalSection(&csDbAccess);
		return 1;
	}	
	// allocate a 'blank' entry into lVolatileContacts
	{
		DBCONTACT_VOLATILE_ROW * p = NULL;
		int index=0;
		p=HeapAlloc(hCacheHeap, HEAP_ZERO_MEMORY, sizeof(DBCONTACT_VOLATILE_ROW));
		if ( p == NULL ) {
			LeaveCriticalSection(&csDbAccess);
			return 1;
		}
		p->hContact=ai->hContact;
		List_GetIndex(&lVolatileContacts,p,&index);
		List_Insert(&lVolatileContacts,p,index);
	}
	LeaveCriticalSection(&csDbAccess);
	return 0;
}

// called without owning csDbAccess
int GetVolatileSetting(HANDLE hContact, const char * szModule, const char * szSetting, DBVARIANT * dbv, int is_static)
{
	/* This func is called by DBGetSetting so will be queried for every get attempt, that is O(log2 N) for the real query 
		if this fails. Lookup here will cost O(log2 N) where N = number of volatile contact settings, however List_GetIndex()
		is shortcut'd to look at the first item, because fake hContacts start high and decrease, it will find that
		real_hContact < fake_hcontact, given this, the total cost ends up being O(1)+ O(log2 N) which isnt bad at all.
	*/
	DBCONTACT_VOLATILE_ROW r={0};
	int index;
	r.hContact=hContact;
	r.hashOfModule=NameHashFunction(szModule);
	r.hashOfSetting=NameHashFunction(szSetting);
	EnterCriticalSection(&csDbAccess);
	if ( List_GetIndex(&lVolatileContacts,&r,&index) ) {
		DBCONTACT_VOLATILE_ROW * p = lVolatileContacts.items[index];
		switch ( p->dbv.type ) {
			case DBVT_BYTE: 
			case DBVT_WORD:
			case DBVT_DWORD: {
				memcpy(dbv, &p->dbv, sizeof(DBVARIANT));
				break;
			}
			case DBVT_ASCIIZ: {
				if ( !is_static ) dbv->pszVal=calloc(p->dbv.cchVal,1);
				strncpy(dbv->pszVal, p->dbv.pszVal, dbv->cchVal < p->dbv.cchVal ? dbv->cchVal : p->dbv.cchVal );
				dbv->type=DBVT_ASCIIZ;
				break;
			}
			case DBVT_BLOB: {
				if ( !is_static ) dbv->pbVal=calloc(p->dbv.cpbVal,1);
				memcpy(dbv->pbVal, p->dbv.pbVal, dbv->cpbVal < p->dbv.cpbVal ? dbv->cpbVal : p->dbv.cpbVal);
				dbv->type=DBVT_BLOB;
				break;
			}
		}
		LeaveCriticalSection(&csDbAccess);
		return 0;
	}
	LeaveCriticalSection(&csDbAccess);
	return 1;
}

// called without owning csDbAccess
int SetVolatileSetting(HANDLE hContact, const char * szModule, const char * szSetting, DBVARIANT * dbv)
{
	// have to decide if this hContact is really volatile or not
	DBCONTACT_VOLATILE_ROW r={0}, * p = NULL;
	int index;
	r.hContact=hContact;
	EnterCriticalSection(&csDbAccess);
	// is this hContact dynamic?
	if ( List_GetIndex(&lVolatileContacts,&r,&index) ) {
		// Yep, set up struct without mutex
		LeaveCriticalSection(&csDbAccess);
		// set up the entire structure
		r.hashOfModule=NameHashFunction(szModule);
		r.hashOfSetting=NameHashFunction(szSetting);
		// get back in
		EnterCriticalSection(&csDbAccess);
		// does this setting exist already?
		if ( List_GetIndex(&lVolatileContacts,&r,&index) ) {
			// setting already exists?
			p = lVolatileContacts.items[index];
			// free any existing dynamic DBV
			switch ( p->dbv.type ) {
				case DBVT_ASCIIZ: {
					HeapFree(hCacheHeap, 0, p->dbv.pszVal);
					break;
				}
				case DBVT_BLOB: {
					HeapFree(hCacheHeap, 0, p->dbv.pbVal);
					break;
				}
			}	
			p->dbv.type=0;
			// copy over the new stuff
		} else { 
			// setting is new, add it.
			p=HeapAlloc(hCacheHeap, HEAP_ZERO_MEMORY, sizeof(DBCONTACT_VOLATILE_ROW)), 
			memcpy(p, &r, sizeof(DBCONTACT_VOLATILE_ROW));
		}
		memcpy(&p->dbv, dbv, sizeof(DBVARIANT));
		// if dynamic data, clone it.
		switch ( p->dbv.type ) {
			case DBVT_ASCIIZ: {
				p->dbv.cchVal=lstrlen(dbv->pszVal)+1;
				p->dbv.pszVal=HeapAlloc(hCacheHeap, HEAP_ZERO_MEMORY, p->dbv.cchVal);
				strcpy(p->dbv.pszVal, dbv->pszVal);				
				break;
			}
			case DBVT_BLOB: {				
				p->dbv.pbVal=HeapAlloc(hCacheHeap, HEAP_ZERO_MEMORY, dbv->cpbVal);
				memcpy(p->dbv.pbVal, dbv->pbVal, dbv->cpbVal);
				break;
			}
		}		
		// successful write/update
		List_Insert(&lVolatileContacts,p,index);
		LeaveCriticalSection(&csDbAccess);
		return 0;
	}
	LeaveCriticalSection(&csDbAccess);
	return 1;
}

static void FreeCachedVariant(DBVARIANT * dbv)
{
	switch ( dbv->type ) {
		case DBVT_ASCIIZ: {
			HeapFree(hCacheHeap, 0, dbv->pszVal);
			break;
		}
		case DBVT_BLOB: {
			HeapFree(hCacheHeap, 0, dbv->pbVal);
			break;
		}
	}
	dbv->type=0;
}

// csDbAccess isnt owned, SortVolatileContactsWithFlags
static int DeleteContactVolatile(WPARAM wParam, LPARAM lParam)
{	
	HANDLE hContact=(HANDLE)wParam;
	int rc = 0;	
	unsigned c=0;
	DBCONTACT_VOLATILE_ROW r={0};
	r.hContact=hContact;
	// find all hContact's in the table and free them
	do {
		int index;
		c++;
		EnterCriticalSection(&csDbAccess);
		if ( rc=List_GetIndexEx(&lVolatileContacts,&r,&index,SortVolatileContactsWithFlags,1) ) {
			DBCONTACT_VOLATILE_ROW * p = lVolatileContacts.items[index];
			List_Remove(&lVolatileContacts,index);
			FreeCachedVariant(&p->dbv);
			HeapFree(hCacheHeap, 0, p);
		} 
		LeaveCriticalSection(&csDbAccess);
	} while ( rc != 0 );	
	return c > 0 ? 0 : 1;
}

