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

DWORD GetModuleNameOfs(const char *szName);
DBCachedContactValueList* AddToCachedContactList(HANDLE hContact, int index);

static int mirCp = CP_ACP;

HANDLE hCacheHeap = NULL;
SortedList lContacts = {0};
HANDLE hLastCachedContact = NULL;
static DBCachedContactValueList *LastVL = NULL;

static SortedList lSettings={0}, lGlobalSettings={0}, lResidentSettings={0};
static HANDLE hSettingChangeEvent = NULL;

static DWORD GetSettingsGroupOfsByModuleNameOfs(struct DBContact *dbc,DWORD ofsModuleName)
{
	struct DBContactSettings *dbcs;
	DWORD ofsThis;

	ofsThis=dbc->ofsFirstSettings;
	while(ofsThis) {
		dbcs=(struct DBContactSettings*)DBRead(ofsThis,sizeof(struct DBContactSettings),NULL);
		if(dbcs->signature!=DBCONTACTSETTINGS_SIGNATURE) DatabaseCorruption(NULL);
		if(dbcs->ofsModuleName==ofsModuleName)
			return ofsThis;

		ofsThis=dbcs->ofsNext;
	}
	return 0;
}

static DWORD __inline GetSettingValueLength(PBYTE pSetting)
{
	if(pSetting[0]&DBVTF_VARIABLELENGTH) return 2+*(PWORD)(pSetting+1);
	return pSetting[0];
}

static char* InsertCachedSetting( const char* szName, size_t cbNameLen, int index )
{
	char* newValue = (char*)HeapAlloc( hCacheHeap, 0, cbNameLen );
	*newValue = 0;
	strcpy(newValue+1,szName+1);
	li.List_Insert(&lSettings,newValue,index);
	return newValue;
}

static char* GetCachedSetting(const char *szModuleName,const char *szSettingName, int moduleNameLen, int settingNameLen)
{
	static char *lastsetting = NULL;
	int index;
	char szFullName[512];

	strcpy(szFullName+1,szModuleName);
	szFullName[moduleNameLen+1]='/';
	strcpy(szFullName+moduleNameLen+2,szSettingName);

	if (lastsetting && strcmp(szFullName+1,lastsetting) == 0)
		return lastsetting;

	if (li.List_GetIndex(&lSettings,szFullName,&index))
		lastsetting = (char*)lSettings.items[index]+1;
	else
		lastsetting = InsertCachedSetting( szFullName, settingNameLen+moduleNameLen+3, index )+1;

	return lastsetting;
}

static void SetCachedVariant( DBVARIANT* s /* new */, DBVARIANT* d /* cached */ )
{
	char* szSave = ( d->type == DBVT_UTF8 || d->type == DBVT_ASCIIZ ) ? d->pszVal : NULL;

	memcpy( d, s, sizeof( DBVARIANT ));
	if (( s->type == DBVT_UTF8 || s->type == DBVT_ASCIIZ ) && s->pszVal != NULL ) {
		if ( szSave != NULL )
			d->pszVal = (char*)HeapReAlloc(hCacheHeap,0,szSave,strlen(s->pszVal)+1);
		else
			d->pszVal = (char*)HeapAlloc(hCacheHeap,0,strlen(s->pszVal)+1);
		strcpy(d->pszVal,s->pszVal);
	}
	else if ( szSave != NULL )
		HeapFree(hCacheHeap,0,szSave);

#ifdef DBLOGGING
	switch( d->type ) {
		case DBVT_BYTE:	log1( "set cached byte: %d", d->bVal ); break;
		case DBVT_WORD:	log1( "set cached word: %d", d->wVal ); break;
		case DBVT_DWORD:	log1( "set cached dword: %d", d->dVal ); break;
		case DBVT_UTF8:
		case DBVT_ASCIIZ: log1( "set cached string: '%s'", d->pszVal ); break;
		default:				log1( "set cached crap: %d", d->type ); break;
	}
#endif
}

void FreeCachedVariant( DBVARIANT* V )
{
	if (( V->type == DBVT_ASCIIZ || V->type == DBVT_UTF8 ) && V->pszVal != NULL )
		HeapFree(hCacheHeap,0,V->pszVal);
}

static DBVARIANT* GetCachedValuePtr( HANDLE hContact, char* szSetting, int bAllocate )
{
	int index;

	if ( hContact == 0 ) {
		DBCachedGlobalValue Vtemp, *V;
		Vtemp.name = szSetting;
		if ( li.List_GetIndex(&lGlobalSettings,&Vtemp,&index)) {
			V = (DBCachedGlobalValue*)lGlobalSettings.items[index];
			if ( bAllocate == -1 ) {
				FreeCachedVariant( &V->value );
				li.List_Remove(&lGlobalSettings,index);
				HeapFree(hCacheHeap,0,V);
				return NULL;
		}	}
		else {
			if ( bAllocate != 1 )
				return NULL;

			V = (DBCachedGlobalValue*)HeapAlloc(hCacheHeap,HEAP_ZERO_MEMORY,sizeof(DBCachedGlobalValue));
			V->name = szSetting;
			li.List_Insert(&lGlobalSettings,V,index);
		}

		return &V->value;
	}
	else {
		DBCachedContactValue *V, *V1;
		DBCachedContactValueList VLtemp,*VL;

		if (hLastCachedContact==hContact && LastVL) {
			VL = LastVL;
		}
		else {
			VLtemp.hContact=hContact;

			if ( !li.List_GetIndex(&lContacts,&VLtemp,&index))
			{
				if ( bAllocate != 1 )
					return NULL;

				VL = AddToCachedContactList(hContact,index);
			}
			else VL = (DBCachedContactValueList*)lContacts.items[index];

			LastVL = VL;
			hLastCachedContact = hContact;
		}

		for ( V = VL->first; V != NULL; V = V->next)
			if (V->name == szSetting)
				break;

		if ( V == NULL ) {
			if ( bAllocate != 1 )
				return NULL;

			V = HeapAlloc(hCacheHeap,HEAP_ZERO_MEMORY,sizeof(DBCachedContactValue));
			if (VL->last)
				VL->last->next = V;
			else
				VL->first = V;
			VL->last = V;
			V->name = szSetting;
		}
		else if ( bAllocate == -1 ) {
		   LastVL = NULL;
			FreeCachedVariant(&V->value);
			if ( VL->first == V ) {
				VL->first = V->next;
				if (VL->last == V)
					VL->last = V->next; // NULL
			}
			else
				for ( V1 = VL->first; V1 != NULL; V1 = V1->next )
					if ( V1->next == V ) {
						V1->next = V->next;
						if (VL->last == V)
							VL->last = V1;
						break;
					}
			HeapFree(hCacheHeap,0,V);
			return NULL;
		}

		return &V->value;
}	}

#define NeedBytes(n)   if(bytesRemaining<(n)) pBlob=(PBYTE)DBRead(ofsBlobPtr,(n),&bytesRemaining)
#define MoveAlong(n)   {int x=n; pBlob+=(x); ofsBlobPtr+=(x); bytesRemaining-=(x);}
#define VLT(n) ((n==DBVT_UTF8)?DBVT_ASCIIZ:n)
static __inline int GetContactSettingWorker(HANDLE hContact,DBCONTACTGETSETTING *dbcgs,int isStatic)
{
	struct DBContact *dbc;
	DWORD ofsModuleName,ofsContact,ofsSettingsGroup,ofsBlobPtr;
	int settingNameLen,moduleNameLen;
	int bytesRemaining;
	PBYTE pBlob;
	char* szCachedSettingName;

	if ((!dbcgs->szSetting) || (!dbcgs->szModule))
		return 1;
	// the db format can't tolerate more than 255 bytes of space (incl. null) for settings+module name
	settingNameLen=strlen(dbcgs->szSetting);
	moduleNameLen=strlen(dbcgs->szModule);
	if ( settingNameLen > 0xFE )
	{
		#ifdef _DEBUG
			OutputDebugString("GetContactSettingWorker() got a > 255 setting name length. \n");
		#endif
		return 1;
	}
	if ( moduleNameLen > 0xFE )
	{
		#ifdef _DEBUG
			OutputDebugString("GetContactSettingWorker() got a > 255 module name length. \n");
		#endif
		return 1;
	}

	EnterCriticalSection(&csDbAccess);

	log3("get [%08p] %s/%s",hContact,dbcgs->szModule,dbcgs->szSetting);

	szCachedSettingName = GetCachedSetting(dbcgs->szModule,dbcgs->szSetting,moduleNameLen,settingNameLen);
	{
		DBVARIANT* pCachedValue = GetCachedValuePtr( hContact, szCachedSettingName, 0 );
		if ( pCachedValue != NULL ) {
			if ( pCachedValue->type == DBVT_ASCIIZ || pCachedValue->type == DBVT_UTF8 ) {
				int   cbOrigLen = dbcgs->pValue->cchVal;
				char* cbOrigPtr = dbcgs->pValue->pszVal;
				memcpy( dbcgs->pValue, pCachedValue, sizeof( DBVARIANT ));
				if ( isStatic ) {
					int cbLen = 0;
					if ( pCachedValue->pszVal != NULL )
						cbLen = strlen( pCachedValue->pszVal );

					cbOrigLen--;
					dbcgs->pValue->pszVal = cbOrigPtr;
					if(cbLen<cbOrigLen) cbOrigLen=cbLen;
					CopyMemory(dbcgs->pValue->pszVal,pCachedValue->pszVal,cbOrigLen);
					dbcgs->pValue->pszVal[cbOrigLen]=0;
					dbcgs->pValue->cchVal=cbLen;
				}
				else {
					dbcgs->pValue->pszVal = (char*)mir_alloc(strlen(pCachedValue->pszVal)+1);
					strcpy(dbcgs->pValue->pszVal,pCachedValue->pszVal);
				}
			}
			else
				memcpy( dbcgs->pValue, pCachedValue, sizeof( DBVARIANT ));

			switch( dbcgs->pValue->type ) {
				case DBVT_BYTE:	log1( "get cached byte: %d", dbcgs->pValue->bVal ); break;
				case DBVT_WORD:	log1( "get cached word: %d", dbcgs->pValue->wVal ); break;
				case DBVT_DWORD:	log1( "get cached dword: %d", dbcgs->pValue->dVal ); break;
				case DBVT_UTF8:
				case DBVT_ASCIIZ: log1( "get cached string: '%s'", dbcgs->pValue->pszVal); break;
				default:				log1( "get cached crap: %d", dbcgs->pValue->type ); break;
			}

			LeaveCriticalSection(&csDbAccess);
			return ( pCachedValue->type == DBVT_DELETED ) ? 1 : 0;
	}	}

	ofsModuleName=GetModuleNameOfs(dbcgs->szModule);
	if(hContact==NULL) ofsContact=dbHeader.ofsUser;
	else ofsContact=(DWORD)hContact;
	dbc=(struct DBContact*)DBRead(ofsContact,sizeof(struct DBContact),NULL);
	if(dbc->signature!=DBCONTACT_SIGNATURE) {
		LeaveCriticalSection(&csDbAccess);
		return 1;
	}
	ofsSettingsGroup=GetSettingsGroupOfsByModuleNameOfs(dbc,ofsModuleName);
	if(ofsSettingsGroup) {
		ofsBlobPtr=ofsSettingsGroup+offsetof(struct DBContactSettings,blob);
		pBlob = DBRead(ofsBlobPtr,sizeof(struct DBContactSettings),&bytesRemaining);
		while(pBlob[0]) {
			NeedBytes(1+settingNameLen);
			if(pBlob[0]==settingNameLen && !memcmp(pBlob+1,dbcgs->szSetting,settingNameLen)) {
				MoveAlong(1+settingNameLen);
				NeedBytes(5);
				if(isStatic && pBlob[0]&DBVTF_VARIABLELENGTH && VLT(dbcgs->pValue->type) != VLT(pBlob[0])) {
					LeaveCriticalSection(&csDbAccess);
					return 1;
				}
				dbcgs->pValue->type=pBlob[0];
				switch(pBlob[0]) {
					case DBVT_DELETED: { /* this setting is deleted */
						dbcgs->pValue->type=DBVT_DELETED;
						LeaveCriticalSection(&csDbAccess);
						return 2;
					}
					case DBVT_BYTE: dbcgs->pValue->bVal=pBlob[1]; break;
					case DBVT_WORD: dbcgs->pValue->wVal=*(PWORD)(pBlob+1); break;
					case DBVT_DWORD: dbcgs->pValue->dVal=*(PDWORD)(pBlob+1); break;
					case DBVT_UTF8:
					case DBVT_ASCIIZ:
						NeedBytes(3+*(PWORD)(pBlob+1));
						if(isStatic) {
							dbcgs->pValue->cchVal--;
							if(*(PWORD)(pBlob+1)<dbcgs->pValue->cchVal) dbcgs->pValue->cchVal=*(PWORD)(pBlob+1);
							CopyMemory(dbcgs->pValue->pszVal,pBlob+3,dbcgs->pValue->cchVal);
							dbcgs->pValue->pszVal[dbcgs->pValue->cchVal]=0;
							dbcgs->pValue->cchVal=*(PWORD)(pBlob+1);
						}
						else {
							dbcgs->pValue->pszVal=(char*)mir_alloc(1+*(PWORD)(pBlob+1));
							CopyMemory(dbcgs->pValue->pszVal,pBlob+3,*(PWORD)(pBlob+1));
							dbcgs->pValue->pszVal[*(PWORD)(pBlob+1)]=0;
						}
						break;
					case DBVT_BLOB:
						NeedBytes(3+*(PWORD)(pBlob+1));
						if(isStatic) {
							if(*(PWORD)(pBlob+1)<dbcgs->pValue->cpbVal) dbcgs->pValue->cpbVal=*(PWORD)(pBlob+1);
							CopyMemory(dbcgs->pValue->pbVal,pBlob+3,dbcgs->pValue->cchVal);
						}
						else {
							dbcgs->pValue->pbVal=(char*)mir_alloc(*(PWORD)(pBlob+1));
							CopyMemory(dbcgs->pValue->pbVal,pBlob+3,*(PWORD)(pBlob+1));
						}
						dbcgs->pValue->cpbVal=*(PWORD)(pBlob+1);
						break;
				}

				/**** add to cache **********************/
				if ( dbcgs->pValue->type != DBVT_BLOB )
				{
					DBVARIANT* pCachedValue = GetCachedValuePtr( hContact, szCachedSettingName, 1 );
					if ( pCachedValue != NULL )
						SetCachedVariant(dbcgs->pValue,pCachedValue);
				}

				LeaveCriticalSection(&csDbAccess);
				logg();
				return 0;
			}
			NeedBytes(1);
			MoveAlong(pBlob[0]+1);
			NeedBytes(3);
			MoveAlong(1+GetSettingValueLength(pBlob));
			NeedBytes(1);
	}	}

	/**** add missing setting to cache **********************/
	if ( dbcgs->pValue->type != DBVT_BLOB )
	{
		DBVARIANT* pCachedValue = GetCachedValuePtr( hContact, szCachedSettingName, 1 );
		if ( pCachedValue != NULL )
			pCachedValue->type = DBVT_DELETED;
	}

	LeaveCriticalSection(&csDbAccess);
	logg();
	return 1;
}

static int GetContactSetting(WPARAM wParam,LPARAM lParam)
{
	DBCONTACTGETSETTING* dgs = ( DBCONTACTGETSETTING* )lParam;
	dgs->pValue->type = 0;
	if ( GetContactSettingWorker(( HANDLE )wParam, dgs, 0 ))
		return 1;

	if ( dgs->pValue->type == DBVT_UTF8 ) {
		WCHAR* tmp = NULL;
		char*  p = NEWSTR_ALLOCA(dgs->pValue->pszVal);
		if ( mir_utf8decode( p, &tmp ) != NULL ) {
			BOOL bUsed = FALSE;
			int  result = WideCharToMultiByte( mirCp, 0, tmp, -1, NULL, 0, "?", &bUsed );

			mir_free( dgs->pValue->pszVal );

			if ( bUsed || result == 0 ) {
				dgs->pValue->type = DBVT_WCHAR;
				dgs->pValue->pwszVal = tmp;
			}
			else {
				dgs->pValue->type = DBVT_ASCIIZ;
				dgs->pValue->pszVal = mir_strdup( p );
			}
		}
		else {
			dgs->pValue->type = DBVT_ASCIIZ;
			mir_free( tmp );
	}	}

	return 0;
}

static int GetContactSettingStr(WPARAM wParam,LPARAM lParam)
{
	DBCONTACTGETSETTING* dgs = (DBCONTACTGETSETTING*)lParam;
	int iSaveType = dgs->pValue->type;

	if ( GetContactSettingWorker(( HANDLE )wParam, dgs, 0 ))
		return 1;

	if ( iSaveType == 0 || iSaveType == dgs->pValue->type )
		return 0;

	if ( dgs->pValue->type != DBVT_ASCIIZ && dgs->pValue->type != DBVT_UTF8 )
		return 0;

	if ( iSaveType == DBVT_WCHAR ) {
		if ( dgs->pValue->type != DBVT_UTF8 ) {
			int len = MultiByteToWideChar( CP_ACP, 0, dgs->pValue->pszVal, -1, NULL, 0 );
			wchar_t* wszResult = ( wchar_t* )mir_alloc(( len+1 )*sizeof( wchar_t ));
			if ( wszResult == NULL )
				return 1;

			MultiByteToWideChar( CP_ACP, 0, dgs->pValue->pszVal, -1, wszResult, len );
			wszResult[ len ] = 0;
			mir_free( dgs->pValue->pszVal );
			dgs->pValue->pwszVal = wszResult;
		}
		else {
			char* savePtr = NEWSTR_ALLOCA(dgs->pValue->pszVal);
			mir_free( dgs->pValue->pszVal );
			if ( !mir_utf8decode( savePtr, &dgs->pValue->pwszVal ))
				return 1;
		}
	}
	else if ( iSaveType == DBVT_UTF8 ) {
		char* tmpBuf = mir_utf8encode( dgs->pValue->pszVal );
		if ( tmpBuf == NULL )
			return 1;

		mir_free( dgs->pValue->pszVal );
		dgs->pValue->pszVal = tmpBuf;
	}
	else if ( iSaveType == DBVT_ASCIIZ )
		mir_utf8decode( dgs->pValue->pszVal, NULL );

	dgs->pValue->type = iSaveType;
	return 0;
}

int GetContactSettingStatic(WPARAM wParam,LPARAM lParam)
{
	DBCONTACTGETSETTING* dgs = (DBCONTACTGETSETTING*)lParam;
	if ( GetContactSettingWorker(( HANDLE )wParam, dgs, 1 ))
		return 1;

	if ( dgs->pValue->type == DBVT_UTF8 ) {
		mir_utf8decode( dgs->pValue->pszVal, NULL );
		dgs->pValue->type = DBVT_ASCIIZ;
	}

	return 0;
}

static int FreeVariant(WPARAM wParam,LPARAM lParam)
{
	DBVARIANT *dbv=(DBVARIANT*)lParam;
	if ( dbv == 0 ) return 1;
	switch ( dbv->type ) {
		case DBVT_ASCIIZ:
		case DBVT_UTF8:
		case DBVT_WCHAR:
		{
			if ( dbv->pszVal ) mir_free(dbv->pszVal);
			dbv->pszVal=0;
			break;
		}
		case DBVT_BLOB:
		{
			if ( dbv->pbVal ) mir_free(dbv->pbVal);
			dbv->pbVal=0;
			break;
		}
	}
	dbv->type=0;
	return 0;
}

static int SetSettingResident(WPARAM wParam,LPARAM lParam)
{
	size_t cbSettingNameLen = strlen(( char* )lParam) + 2;
	if (cbSettingNameLen  < 512)
	{
		char*  szSetting;
		int    idx;
		char  szTemp[512];
		strcpy( szTemp+1, ( char* )lParam );

		EnterCriticalSection(&csDbAccess);
		if ( !li.List_GetIndex( &lSettings, szTemp, &idx ))
			szSetting = InsertCachedSetting( szTemp, cbSettingNameLen, idx );
		else
			szSetting = lSettings.items[ idx ];

		*szSetting = (char)wParam;

		if ( !li.List_GetIndex( &lResidentSettings, szSetting+1, &idx ))
		{
			if (wParam)
				li.List_Insert(&lResidentSettings,szSetting+1,idx);
		}
		else if (!wParam)
			li.List_Remove(&lResidentSettings,idx);

		LeaveCriticalSection(&csDbAccess);
	}
	return 0;
}

static int WriteContactSetting(WPARAM wParam,LPARAM lParam)
{
	DBCONTACTWRITESETTING *dbcws=(DBCONTACTWRITESETTING*)lParam;
	struct DBContact dbc;
	DWORD ofsModuleName;
	struct DBContactSettings dbcs;
	PBYTE pBlob;
	int settingNameLen=0;
	int moduleNameLen=0;
	int settingDataLen=0;
	int bytesRequired,bytesRemaining;
	DWORD ofsContact,ofsSettingsGroup,ofsBlobPtr;

	if (dbcws == NULL || dbcws->szSetting==NULL || dbcws->szModule==NULL )
		return 1;

	// the db format can't tolerate more than 255 bytes of space (incl. null) for settings+module name
	settingNameLen=strlen(dbcws->szSetting);
	moduleNameLen=strlen(dbcws->szModule);
	if ( settingNameLen > 0xFE )
	{
		#ifdef _DEBUG
			OutputDebugString("WriteContactSetting() got a > 255 setting name length. \n");
		#endif
		return 1;
	}
	if ( moduleNameLen > 0xFE )
	{
		#ifdef _DEBUG
			OutputDebugString("WriteContactSetting() got a > 255 module name length. \n");
		#endif
		return 1;
	}

	if (dbcws->value.type == DBVT_WCHAR) {
		if (dbcws->value.pszVal != NULL) {
			char* val = mir_utf8encodeW(dbcws->value.pwszVal);
			if ( val == NULL )
				return 1;

			dbcws->value.pszVal = ( char* )alloca( strlen( val )+1 );
			strcpy( dbcws->value.pszVal, val );
			mir_free(val);
			dbcws->value.type = DBVT_UTF8;
		}
		else return 1;
	}

	if(dbcws->value.type!=DBVT_BYTE && dbcws->value.type!=DBVT_WORD && dbcws->value.type!=DBVT_DWORD && dbcws->value.type!=DBVT_ASCIIZ && dbcws->value.type!=DBVT_UTF8 && dbcws->value.type!=DBVT_BLOB)
		return 1;
	if ((!dbcws->szModule) || (!dbcws->szSetting) || ((dbcws->value.type == DBVT_ASCIIZ || dbcws->value.type == DBVT_UTF8 )&& dbcws->value.pszVal == NULL) || (dbcws->value.type == DBVT_BLOB && dbcws->value.pbVal == NULL) )
		return 1;

	// the db can not tolerate strings/blobs longer than 0xFFFF since the format writes 2 lengths
	switch( dbcws->value.type ) {
	case DBVT_ASCIIZ:		case DBVT_BLOB:	case DBVT_UTF8:
		{	int len = ( dbcws->value.type != DBVT_BLOB ) ? strlen(dbcws->value.pszVal) : dbcws->value.cpbVal;
			if ( len >= 0xFFFF ) {
				#ifdef _DEBUG
					OutputDebugString("WriteContactSetting() writing huge string/blob, rejecting ( >= 0xFFFF ) \n");
				#endif
				return 1;
			}
		}
	}

	EnterCriticalSection(&csDbAccess);
	{
		char* szCachedSettingName = GetCachedSetting(dbcws->szModule, dbcws->szSetting, moduleNameLen, settingNameLen);
		if ( dbcws->value.type != DBVT_BLOB ) {
			DBVARIANT* pCachedValue = GetCachedValuePtr((HANDLE)wParam, szCachedSettingName, 1);
			if ( pCachedValue != NULL ) {
				BOOL bIsIdentical = FALSE;
				if ( pCachedValue->type == dbcws->value.type ) {
					switch(dbcws->value.type) {
						case DBVT_BYTE:   bIsIdentical = pCachedValue->bVal == dbcws->value.bVal;  break;
						case DBVT_WORD:   bIsIdentical = pCachedValue->wVal == dbcws->value.wVal;  break;
						case DBVT_DWORD:  bIsIdentical = pCachedValue->dVal == dbcws->value.dVal;  break;
						case DBVT_UTF8:
						case DBVT_ASCIIZ: bIsIdentical = strcmp( pCachedValue->pszVal, dbcws->value.pszVal ) == 0; break;
					}
					if ( bIsIdentical ) {
						LeaveCriticalSection(&csDbAccess);
						return 0;
					}
				}
				SetCachedVariant(&dbcws->value, pCachedValue);
			}
			if ( szCachedSettingName[-1] != 0 ) {
				LeaveCriticalSection(&csDbAccess);
				NotifyEventHooks(hSettingChangeEvent,wParam,lParam);
				return 0;
			}
		}
		else GetCachedValuePtr((HANDLE)wParam, szCachedSettingName, -1);
	}

	ofsModuleName=GetModuleNameOfs(dbcws->szModule);
 	if(wParam==0) ofsContact=dbHeader.ofsUser;
	else ofsContact=wParam;

	dbc=*(struct DBContact*)DBRead(ofsContact,sizeof(struct DBContact),NULL);
	if(dbc.signature!=DBCONTACT_SIGNATURE) {
		LeaveCriticalSection(&csDbAccess);
		return 1;
	}
	log0("write setting");
	//make sure the module group exists
	ofsSettingsGroup=GetSettingsGroupOfsByModuleNameOfs(&dbc,ofsModuleName);
	if(ofsSettingsGroup==0) {  //module group didn't exist - make it
		if(dbcws->value.type&DBVTF_VARIABLELENGTH) {
		  if(dbcws->value.type==DBVT_ASCIIZ || dbcws->value.type==DBVT_UTF8) bytesRequired=strlen(dbcws->value.pszVal)+2;
		  else if(dbcws->value.type==DBVT_BLOB) bytesRequired=dbcws->value.cpbVal+2;
		}
		else bytesRequired=dbcws->value.type;
		bytesRequired+=2+settingNameLen;
		bytesRequired+=(DB_SETTINGS_RESIZE_GRANULARITY-(bytesRequired%DB_SETTINGS_RESIZE_GRANULARITY))%DB_SETTINGS_RESIZE_GRANULARITY;
		ofsSettingsGroup=CreateNewSpace(bytesRequired+offsetof(struct DBContactSettings,blob));
		dbcs.signature=DBCONTACTSETTINGS_SIGNATURE;
		dbcs.ofsNext=dbc.ofsFirstSettings;
		dbcs.ofsModuleName=ofsModuleName;
		dbcs.cbBlob=bytesRequired;
		dbcs.blob[0]=0;
		dbc.ofsFirstSettings=ofsSettingsGroup;
		DBWrite(ofsContact,&dbc,sizeof(struct DBContact));
		DBWrite(ofsSettingsGroup,&dbcs,sizeof(struct DBContactSettings));
		ofsBlobPtr=ofsSettingsGroup+offsetof(struct DBContactSettings,blob);
		pBlob=(PBYTE)DBRead(ofsBlobPtr,1,&bytesRemaining);
	}
	else {
		dbcs=*(struct DBContactSettings*)DBRead(ofsSettingsGroup,sizeof(struct DBContactSettings),&bytesRemaining);
		//find if the setting exists
		ofsBlobPtr=ofsSettingsGroup+offsetof(struct DBContactSettings,blob);
		pBlob=(PBYTE)DBRead(ofsBlobPtr,1,&bytesRemaining);
		while(pBlob[0]) {
			NeedBytes(settingNameLen+1);
			if(pBlob[0]==settingNameLen && !memcmp(pBlob+1,dbcws->szSetting,settingNameLen))
				break;
			NeedBytes(1);
			MoveAlong(pBlob[0]+1);
			NeedBytes(3);
			MoveAlong(1+GetSettingValueLength(pBlob));
			NeedBytes(1);
		}
		if(pBlob[0]) {	 //setting already existed, and up to end of name is in cache
			MoveAlong(1+settingNameLen);
			//if different type or variable length and length is different
			NeedBytes(3);
			if(pBlob[0]!=dbcws->value.type || ((pBlob[0]==DBVT_ASCIIZ || pBlob[0]==DBVT_UTF8) && *(PWORD)(pBlob+1)!=strlen(dbcws->value.pszVal)) || (pBlob[0]==DBVT_BLOB && *(PWORD)(pBlob+1)!=dbcws->value.cpbVal)) {
				//bin it
				int nameLen,valLen;
				DWORD ofsSettingToCut;
				NeedBytes(3);
				nameLen=1+settingNameLen;
				valLen=1+GetSettingValueLength(pBlob);
				ofsSettingToCut=ofsBlobPtr-nameLen;
				MoveAlong(valLen);
				NeedBytes(1);
				while(pBlob[0]) {
					MoveAlong(pBlob[0]+1);
					NeedBytes(3);
					MoveAlong(1+GetSettingValueLength(pBlob));
					NeedBytes(1);
				}
				DBMoveChunk(ofsSettingToCut,ofsSettingToCut+nameLen+valLen,ofsBlobPtr+1-ofsSettingToCut);
				ofsBlobPtr-=nameLen+valLen;
				pBlob=(PBYTE)DBRead(ofsBlobPtr,1,&bytesRemaining);
			}
			else {
				//replace existing setting at pBlob
				MoveAlong(1);	//skip data type
				switch(dbcws->value.type) {
					case DBVT_BYTE: DBWrite(ofsBlobPtr,&dbcws->value.bVal,1); break;
					case DBVT_WORD: DBWrite(ofsBlobPtr,&dbcws->value.wVal,2); break;
					case DBVT_DWORD: DBWrite(ofsBlobPtr,&dbcws->value.dVal,4); break;
					case DBVT_UTF8:
					case DBVT_ASCIIZ: DBWrite(ofsBlobPtr+2,dbcws->value.pszVal,strlen(dbcws->value.pszVal)); break;
					case DBVT_BLOB: DBWrite(ofsBlobPtr+2,dbcws->value.pbVal,dbcws->value.cpbVal); break;
				}
				//quit
				DBFlush(1);
				LeaveCriticalSection(&csDbAccess);
				//notify
				NotifyEventHooks(hSettingChangeEvent,wParam,lParam);
				return 0;
			}
		}
	}
	//cannot do a simple replace, add setting to end of list
	//pBlob already points to end of list
	//see if it fits
	if(dbcws->value.type&DBVTF_VARIABLELENGTH) {
	  if(dbcws->value.type==DBVT_ASCIIZ || dbcws->value.type==DBVT_UTF8) bytesRequired=strlen(dbcws->value.pszVal)+2;
	  else if(dbcws->value.type==DBVT_BLOB) bytesRequired=dbcws->value.cpbVal+2;
	}
	else bytesRequired=dbcws->value.type;
	bytesRequired+=2+settingNameLen;
	bytesRequired+=ofsBlobPtr+1-(ofsSettingsGroup+offsetof(struct DBContactSettings,blob));
	if((DWORD)bytesRequired>dbcs.cbBlob) {
		//doesn't fit: move entire group
		struct DBContactSettings *dbcsPrev;
		DWORD ofsDbcsPrev,ofsNew;

		bytesRequired+=(DB_SETTINGS_RESIZE_GRANULARITY-(bytesRequired%DB_SETTINGS_RESIZE_GRANULARITY))%DB_SETTINGS_RESIZE_GRANULARITY;
		//find previous group to change its offset
		ofsDbcsPrev=dbc.ofsFirstSettings;
		if(ofsDbcsPrev==ofsSettingsGroup) ofsDbcsPrev=0;
		else {
			dbcsPrev=(struct DBContactSettings*)DBRead(ofsDbcsPrev,sizeof(struct DBContactSettings),NULL);
			while(dbcsPrev->ofsNext!=ofsSettingsGroup) {
				if(dbcsPrev->ofsNext==0) DatabaseCorruption(NULL);
				ofsDbcsPrev=dbcsPrev->ofsNext;
				dbcsPrev=(struct DBContactSettings*)DBRead(ofsDbcsPrev,sizeof(struct DBContactSettings),NULL);
			}
		}

		//create the new one
		ofsNew=ReallocSpace(ofsSettingsGroup, dbcs.cbBlob+offsetof(struct DBContactSettings,blob), bytesRequired+offsetof(struct DBContactSettings,blob));

		dbcs.cbBlob=bytesRequired;

		DBWrite(ofsNew,&dbcs,offsetof(struct DBContactSettings,blob));
		if(ofsDbcsPrev==0) {
			dbc.ofsFirstSettings=ofsNew;
			DBWrite(ofsContact,&dbc,sizeof(struct DBContact));
		}
		else {
			dbcsPrev=(struct DBContactSettings*)DBRead(ofsDbcsPrev,sizeof(struct DBContactSettings),NULL);
			dbcsPrev->ofsNext=ofsNew;
			DBWrite(ofsDbcsPrev,dbcsPrev,offsetof(struct DBContactSettings,blob));
		}
		ofsBlobPtr+=ofsNew-ofsSettingsGroup;
		ofsSettingsGroup=ofsNew;
		pBlob=(PBYTE)DBRead(ofsBlobPtr,1,&bytesRemaining);
	}
	//we now have a place to put it and enough space: make it
	DBWrite(ofsBlobPtr,&settingNameLen,1);
	DBWrite(ofsBlobPtr+1,(PVOID)dbcws->szSetting,settingNameLen);
	MoveAlong(1+settingNameLen);
	DBWrite(ofsBlobPtr,&dbcws->value.type,1);
	MoveAlong(1);
	switch(dbcws->value.type) {
		case DBVT_BYTE: DBWrite(ofsBlobPtr,&dbcws->value.bVal,1); MoveAlong(1); break;
		case DBVT_WORD: DBWrite(ofsBlobPtr,&dbcws->value.wVal,2); MoveAlong(2); break;
		case DBVT_DWORD: DBWrite(ofsBlobPtr,&dbcws->value.dVal,4); MoveAlong(4); break;
		case DBVT_UTF8:
		case DBVT_ASCIIZ:
			{	int len=strlen(dbcws->value.pszVal);
				DBWrite(ofsBlobPtr,&len,2);
				DBWrite(ofsBlobPtr+2,dbcws->value.pszVal,len);
				MoveAlong(2+len);
			}
			break;
		case DBVT_BLOB:
			DBWrite(ofsBlobPtr,&dbcws->value.cpbVal,2);
			DBWrite(ofsBlobPtr+2,dbcws->value.pbVal,dbcws->value.cpbVal);
			MoveAlong(2+dbcws->value.cpbVal);
			break;
	}
	{	BYTE zero=0;
		DBWrite(ofsBlobPtr,&zero,1);
	}
	//quit
	DBFlush(1);
	LeaveCriticalSection(&csDbAccess);
	//notify
	NotifyEventHooks(hSettingChangeEvent,wParam,lParam);
	return 0;
}

static int DeleteContactSetting(WPARAM wParam,LPARAM lParam)
{
	DBCONTACTGETSETTING *dbcgs=(DBCONTACTGETSETTING*)lParam;
	struct DBContact *dbc;
	DWORD ofsModuleName,ofsSettingsGroup,ofsBlobPtr;
	PBYTE pBlob;
	int settingNameLen,moduleNameLen,bytesRemaining;
	char* szCachedSettingName;
	WPARAM saveWparam = wParam;

	if ((!dbcgs->szModule) || (!dbcgs->szSetting))
		return 1;
	// the db format can't tolerate more than 255 bytes of space (incl. null) for settings+module name
	settingNameLen=strlen(dbcgs->szSetting);
	moduleNameLen=strlen(dbcgs->szModule);
	if ( settingNameLen > 0xFE )
	{
		#ifdef _DEBUG
			OutputDebugString("DeleteContactSetting() got a > 255 setting name length. \n");
		#endif
		return 1;
	}
	if ( moduleNameLen > 0xFE )
	{
		#ifdef _DEBUG
			OutputDebugString("DeleteContactSetting() got a > 255 module name length. \n");
		#endif
		return 1;
	}

	EnterCriticalSection(&csDbAccess);
	ofsModuleName=GetModuleNameOfs(dbcgs->szModule);
 	if(wParam==0) wParam=dbHeader.ofsUser;

	dbc=(struct DBContact*)DBRead(wParam,sizeof(struct DBContact),NULL);
	if(dbc->signature!=DBCONTACT_SIGNATURE) {
		LeaveCriticalSection(&csDbAccess);
		return 1;
	}
	//make sure the module group exists
	ofsSettingsGroup=GetSettingsGroupOfsByModuleNameOfs(dbc,ofsModuleName);
	if(ofsSettingsGroup==0) {
		LeaveCriticalSection(&csDbAccess);
		return 1;
	}
	//find if the setting exists
	ofsBlobPtr=ofsSettingsGroup+offsetof(struct DBContactSettings,blob);
	pBlob=(PBYTE)DBRead(ofsBlobPtr,1,&bytesRemaining);
	while(pBlob[0]) {
		NeedBytes(settingNameLen+1);
		if(pBlob[0]==settingNameLen && !memcmp(pBlob+1,dbcgs->szSetting,settingNameLen))
			break;
		NeedBytes(1);
		MoveAlong(pBlob[0]+1);
		NeedBytes(3);
		MoveAlong(1+GetSettingValueLength(pBlob));
		NeedBytes(1);
	}
	if(!pBlob[0]) {     //setting didn't exist
		LeaveCriticalSection(&csDbAccess);
		return 1;
	}
	{	//bin it
		int nameLen,valLen;
		DWORD ofsSettingToCut;
		MoveAlong(1+settingNameLen);
		NeedBytes(3);
		nameLen=1+settingNameLen;
		valLen=1+GetSettingValueLength(pBlob);
		ofsSettingToCut=ofsBlobPtr-nameLen;
		MoveAlong(valLen);
		NeedBytes(1);
		while(pBlob[0]) {
			MoveAlong(pBlob[0]+1);
			NeedBytes(3);
			MoveAlong(1+GetSettingValueLength(pBlob));
			NeedBytes(1);
		}
		DBMoveChunk(ofsSettingToCut,ofsSettingToCut+nameLen+valLen,ofsBlobPtr+1-ofsSettingToCut);
	}

	szCachedSettingName = GetCachedSetting(dbcgs->szModule,dbcgs->szSetting,moduleNameLen,settingNameLen);
	GetCachedValuePtr((HANDLE)saveWparam, szCachedSettingName, -1 );

	//quit
	DBFlush(1);
	LeaveCriticalSection(&csDbAccess);
	{	//notify
		DBCONTACTWRITESETTING dbcws={0};
		dbcws.szModule=dbcgs->szModule;
		dbcws.szSetting=dbcgs->szSetting;
		dbcws.value.type=DBVT_DELETED;
		NotifyEventHooks(hSettingChangeEvent,saveWparam,(LPARAM)&dbcws);
	}
	return 0;
}

static int EnumContactSettings(WPARAM wParam,LPARAM lParam)
{
	DBCONTACTENUMSETTINGS *dbces=(DBCONTACTENUMSETTINGS*)lParam;
	struct DBContact *dbc;
	DWORD ofsModuleName,ofsContact,ofsBlobPtr;
	int bytesRemaining, result;
	PBYTE pBlob;
	char szSetting[256];

	if (!dbces->szModule)
		return -1;

	EnterCriticalSection(&csDbAccess);

	ofsModuleName=GetModuleNameOfs(dbces->szModule);
	if(wParam==0) ofsContact=dbHeader.ofsUser;
	else ofsContact=wParam;
	dbc=(struct DBContact*)DBRead(ofsContact,sizeof(struct DBContact),NULL);
	if(dbc->signature!=DBCONTACT_SIGNATURE) {
		LeaveCriticalSection(&csDbAccess);
		return -1;
	}
	dbces->ofsSettings=GetSettingsGroupOfsByModuleNameOfs(dbc,ofsModuleName);
	if(!dbces->ofsSettings) {
		LeaveCriticalSection(&csDbAccess);
		return -1;
	}
	ofsBlobPtr=dbces->ofsSettings+offsetof(struct DBContactSettings,blob);
	pBlob=(PBYTE)DBRead(ofsBlobPtr,1,&bytesRemaining);
	if(pBlob[0]==0) {
		LeaveCriticalSection(&csDbAccess);
		return -1;
	}
	result = 0;
	while(pBlob[0]) {
		NeedBytes(1);
		NeedBytes(1+pBlob[0]);
		CopyMemory(szSetting,pBlob+1,pBlob[0]); szSetting[pBlob[0]]=0;
		result = (dbces->pfnEnumProc)(szSetting,dbces->lParam);
		MoveAlong(1+pBlob[0]);
		NeedBytes(3);
		MoveAlong(1+GetSettingValueLength(pBlob));
		NeedBytes(1);
	}
	LeaveCriticalSection(&csDbAccess);
	return result;
}

static int EnumResidentSettings(WPARAM wParam,LPARAM lParam)
{
	int i;
	int ret;
	for(i = 0; i < lResidentSettings.realCount; i++) {
		ret=((DBMODULEENUMPROC)lParam)(lResidentSettings.items[i],0,wParam);
		if(ret) return ret;
	}
	return 0;
}

/////////////////////////////////////////////////////////////////////////////////////////
//
//   Module initialization procedure

static int stringCompare( DBCachedSettingName* p1, DBCachedSettingName* p2 )
{
	return strcmp( p1->name, p2->name );
}

static int ptrCompare( DBCachedGlobalValue* p1, DBCachedGlobalValue* p2 )
{
	return (p1->name - p2->name);
}

static int stringCompare2( char* p1, char* p2 )
{
	return strcmp( p1, p2);
}

static int handleCompare( void* p1, void* p2 )
{
	return *( long* )p1 - *( long* )p2;
}

int InitSettings(void)
{
	CreateServiceFunction(MS_DB_CONTACT_GETSETTING,GetContactSetting);
	CreateServiceFunction(MS_DB_CONTACT_GETSETTING_STR,GetContactSettingStr);
	CreateServiceFunction(MS_DB_CONTACT_GETSETTINGSTATIC,GetContactSettingStatic);
	CreateServiceFunction(MS_DB_CONTACT_FREEVARIANT,FreeVariant);
	CreateServiceFunction(MS_DB_CONTACT_WRITESETTING,WriteContactSetting);
	CreateServiceFunction(MS_DB_CONTACT_DELETESETTING,DeleteContactSetting);
	CreateServiceFunction(MS_DB_CONTACT_ENUMSETTINGS,EnumContactSettings);
	CreateServiceFunction(MS_DB_SETSETTINGRESIDENT,SetSettingResident);
	CreateServiceFunction("DB/ResidentSettings/Enum",EnumResidentSettings);

	hSettingChangeEvent=CreateHookableEvent(ME_DB_CONTACT_SETTINGCHANGED);

	hCacheHeap=HeapCreate(0,0,0);
	lSettings.sortFunc=stringCompare;
	lSettings.increment=100;
	lContacts.sortFunc=handleCompare;
	lContacts.increment=50;
	lGlobalSettings.sortFunc=ptrCompare;
	lGlobalSettings.increment=50;
	lResidentSettings.sortFunc=stringCompare2;
	lResidentSettings.increment=50;

	mirCp = CallService( MS_LANGPACK_GETCODEPAGE, 0, 0 );
	return 0;
}

void UninitSettings(void)
{
	HeapDestroy(hCacheHeap);
	li.List_Destroy(&lContacts);
	li.List_Destroy(&lSettings);
	li.List_Destroy(&lGlobalSettings);
	li.List_Destroy(&lResidentSettings);
}
