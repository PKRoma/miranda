

#include <windows.h>
#include "miranda.h"
#include "svc.h"

extern char g_Name[MAX_PATH];

// find a contact given a normalised screen name, returns NULL if no contact existed
HANDLE Miranda_FindContact(char * sn, int add)
{
	char buf[1024];
	DBVARIANT dbv;
	DBCONTACTGETSETTING cgs;
	cgs.szModule=g_Name;
	cgs.szSetting=OSCAR_CONTACT_KEY;
	cgs.pValue=&dbv;
	HANDLE hContact=(HANDLE)CallService(MS_DB_CONTACT_FINDFIRST, 0, 0);
	while ( hContact != NULL ) {
		char * szProto = ( char * ) CallService(MS_PROTO_GETCONTACTBASEPROTO, (WPARAM)hContact, 0);
		if ( szProto != NULL && strcmp(szProto,g_Name)==0 ) {
			dbv.type=DBVT_ASCIIZ;
			dbv.pszVal=(char *) &buf;
			dbv.cchVal=sizeof(buf);
			if ( CallService(MS_DB_CONTACT_GETSETTINGSTATIC, (WPARAM)hContact, (LPARAM)&cgs) == 0
				&& strcmpi(buf,sn) == 0 ) {
				return hContact;
			}
		}
		hContact=(HANDLE)CallService(MS_DB_CONTACT_FINDNEXT, (WPARAM)hContact, 0);
	}
	hContact=NULL;
	if ( add ) {
		hContact=(HANDLE)CallService(MS_DB_CONTACT_ADD, 0, 0);
		CallService(MS_PROTO_ADDTOCONTACT, (WPARAM)hContact, (LPARAM) g_Name);
		DBWriteContactSettingString(hContact, g_Name, OSCAR_CONTACT_KEY, sn);
	}
	return hContact;
}

// given a hContact, find the screenname/icq#, returns 1 if found, 0 if not
int Miranda_ReverseFindContact(HANDLE hContact, char * sn, size_t cch)
{
	DBVARIANT dbv;
	DBCONTACTGETSETTING cgs;
	char * szProto;
	cgs.szModule=g_Name;
	cgs.szSetting=OSCAR_CONTACT_KEY;
	cgs.pValue=&dbv;
	szProto = ( char * ) CallService(MS_PROTO_GETCONTACTBASEPROTO, (WPARAM)hContact, 0);
	if ( szProto != NULL && strcmp(szProto,g_Name)==0 ) {
		dbv.type=DBVT_ASCIIZ;
		dbv.pszVal=(char *)sn;
		dbv.cchVal=cch;
		if ( CallService(MS_DB_CONTACT_GETSETTINGSTATIC, (WPARAM)hContact, (LPARAM)&cgs) == 0 ) {
			return 1;
		}
	}
	return 0;	
}

// set all contacts to offline
void Miranda_SetContactsOffline(void)
{
	HANDLE hContact=(HANDLE)CallService(MS_DB_CONTACT_FINDFIRST, 0, 0);
	while ( hContact != NULL ) {
		char * szProto = ( char * ) CallService(MS_PROTO_GETCONTACTBASEPROTO, (WPARAM)hContact, 0);
		if ( szProto != NULL && strcmp(szProto,g_Name)==0 ) {
			DBWriteContactSettingWord(hContact, g_Name, "Status", ID_STATUS_OFFLINE);
		}
		hContact=(HANDLE)CallService(MS_DB_CONTACT_FINDNEXT, (WPARAM)hContact, 0);
	}
}

unsigned int Miranda_TransformStatus(unsigned int status)
{
	/* libfaim returns a bitmask of statuses, of these the lower 16bits is the status and only one bit
	in a field is set, but instead we test for every bit */
	if ( status & AIM_ICQ_STATE_CHAT ) return ID_STATUS_FREECHAT;
	if ( status & AIM_ICQ_STATE_BUSY ) return ID_STATUS_OCCUPIED;
	if ( status & AIM_ICQ_STATE_OUT) return ID_STATUS_NA;
	if ( status & AIM_ICQ_STATE_DND ) return ID_STATUS_DND;
	if ( status & AIM_ICQ_STATE_AWAY ) return ID_STATUS_AWAY;		
	if ( status & AIM_ICQ_STATE_INVISIBLE ) return ID_STATUS_INVISIBLE;
	return ID_STATUS_ONLINE;
}

// given a screen name, remove whitespace and uppercasing, returns 1 if successful
int Miranda_Normalise(char * sn, char * buf, size_t cch)
{
	// remove all whitespaces, and lowercase
	while ( sn && *sn && cch > 0 ) {
		while ( *sn <= ' ' && *sn != 0 ) sn++;
		*buf++ = tolower(*sn++);
		cch--;
	}
	if ( cch == 0 ) return 0;
	*buf=0;
	return 1;
}




