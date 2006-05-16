/*
Change ICQ Details plugin for Miranda IM

Copyright © 2001-2005 Richard Hughes, Martin Öberg

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



#include <windows.h>
#include <newpluginapi.h>
#include <m_database.h>
#include <m_langpack.h>
#include <m_clist.h>
#include <m_protosvc.h>
#include <m_icq.h>
#include "changeinfo.h"

extern HANDLE hUpload[];

void packByte(PBYTE *buf,int *buflen,BYTE b)
{
	*buf=(PBYTE)realloc(*buf,1+*buflen);
	*(*buf+*buflen)=b;
	++*buflen;
}

void packWord(PBYTE *buf,int *buflen,WORD w)
{
	*buf=(PBYTE)realloc(*buf,2+*buflen);
	*(PWORD)(*buf+*buflen)=w;
	*buflen+=2;
}

void packLNTS(PBYTE *buf,int *buflen,const char *szSetting)
{
	DBVARIANT dbv;

	if(DBGetContactSetting(NULL,"ICQ",szSetting,&dbv))
		packWord(buf,buflen,0);
	else {
		int len=strlen(dbv.pszVal);
		packWord(buf,buflen,len);
		*buf=(PBYTE)realloc(*buf,*buflen+len);
		memcpy(*buf+*buflen,dbv.pszVal,len);
		*buflen+=len;
		DBFreeVariant(&dbv);
	}
}

int StringToListItemId(const char *szSetting,int def)
{
	int i,listCount;
	DBVARIANT dbv;
	ListTypeDataItem *list;

	for(i=0;i<settingCount;i++)
		if(!lstrcmp(szSetting,setting[i].szDbSetting)) break;
	if(i==settingCount) return def;
	list=(ListTypeDataItem*)setting[i].pList;
	listCount=setting[i].listCount;
	if(DBGetContactSetting(NULL,"ICQ",szSetting,&dbv)) return def;
	for(i=0;i<listCount;i++)
		if(!lstrcmp(list[i].szValue,dbv.pszVal)) break;
	DBFreeVariant(&dbv);
	if(i==listCount) return def;
	return list[i].id;
}

int UploadSettings(HWND hwndParent)
{
	PBYTE buf = NULL;
	int buflen = 0;
	BYTE gender ;

	if (CallService("ICQ"PS_GETSTATUS, 0, 0) < ID_STATUS_ONLINE) {
		MessageBox(hwndParent, Translate("You are not currently connected to the ICQ network. You must be online in order to update your information on the server."), Translate("Change ICQ Info"), MB_OK);
		return 0;
	}
	packWord(&buf, &buflen, 0);   //this will be the data length in the end
	packLNTS(&buf, &buflen, "Nick");
	packLNTS(&buf, &buflen, "FirstName");
	packLNTS(&buf, &buflen, "LastName");
	packLNTS(&buf, &buflen, "e-mail");
	packLNTS(&buf, &buflen, "City");
	packLNTS(&buf, &buflen, "State");
	packLNTS(&buf, &buflen, "Phone");
	packLNTS(&buf, &buflen, "Fax");
	packLNTS(&buf, &buflen, "Street");
	packLNTS(&buf, &buflen, "Cellular");
	packLNTS(&buf, &buflen, "ZIP");
	packWord(&buf, &buflen, DBGetContactSettingWord(NULL, "ICQ", "Country", 0));
	packByte(&buf, &buflen, DBGetContactSettingByte(NULL, "ICQ", "Timezone", 0));
	packByte(&buf, &buflen, !DBGetContactSettingByte(NULL, "ICQ", "PublishPrimaryEmail", 0));
	*(PWORD)buf = buflen - 2;
	hUpload[0] = (HANDLE)CallService("ICQ"PS_CHANGEINFO, ICQCHANGEINFO_MAIN, (LPARAM)buf);

	buflen = 2;
	packByte(&buf, &buflen, DBGetContactSettingByte(NULL, "ICQ", "Age", 0));
	packByte(&buf, &buflen, 0);
	gender = DBGetContactSettingByte(NULL, "ICQ", "Gender", 0);
	packByte(&buf, &buflen, gender ? (gender == 'M' ? 2 : 1) : 0);
	packLNTS(&buf, &buflen, "Homepage");
	packWord(&buf, &buflen, DBGetContactSettingWord(NULL, "ICQ", "BirthYear", 0));
	packByte(&buf, &buflen, DBGetContactSettingByte(NULL, "ICQ", "BirthMonth", 0));
	packByte(&buf, &buflen, DBGetContactSettingByte(NULL, "ICQ", "BirthDay", 0));
	packByte(&buf, &buflen, StringToListItemId("Language1", 0));
	packByte(&buf, &buflen, StringToListItemId("Language2", 0));
	packByte(&buf, &buflen, StringToListItemId("Language3", 0));
	*(PWORD)buf = buflen - 2;
	hUpload[1] = (HANDLE)CallService("ICQ"PS_CHANGEINFO, ICQCHANGEINFO_MORE, (LPARAM)buf);

	buflen = 2;
	packLNTS(&buf, &buflen, "About");
	*(PWORD)buf = buflen - 2;
	hUpload[2] = (HANDLE)CallService("ICQ"PS_CHANGEINFO, ICQCHANGEINFO_ABOUT, (LPARAM)buf);

	buflen = 2;
	packLNTS(&buf, &buflen, "CompanyCity");
	packLNTS(&buf, &buflen, "CompanyState");
	packLNTS(&buf, &buflen, "CompanyPhone");
	packLNTS(&buf, &buflen, "CompanyFax");
	packLNTS(&buf, &buflen, "CompanyStreet");
	packLNTS(&buf, &buflen, "CompanyZIP");
	packWord(&buf, &buflen, DBGetContactSettingWord(NULL, "ICQ", "CompanyCountry", 0));
	packLNTS(&buf, &buflen, "Company");
	packLNTS(&buf, &buflen, "CompanyDepartment");
	packLNTS(&buf, &buflen, "CompanyPosition");
	packWord(&buf, &buflen, DBGetContactSettingWord(NULL, "ICQ", "CompanyOccupation", 0));
	packLNTS(&buf, &buflen, "CompanyHomepage");
	*(PWORD)buf = buflen - 2;
	hUpload[3] = (HANDLE)CallService("ICQ"PS_CHANGEINFO, ICQCHANGEINFO_WORK, (LPARAM)buf);

	buflen = 2;
	{	DBVARIANT dbv;
		if(DBGetContactSetting(NULL, "ICQ", "Password", &dbv))
			packWord(&buf, &buflen, 0);
		else {
			int len = strlen(dbv.pszVal);
			CallService(MS_DB_CRYPT_DECODESTRING, len+1, (LPARAM)dbv.pszVal);
			packWord(&buf, &buflen, len);
			buf = (PBYTE)realloc(buf, buflen + len);
			memcpy(buf + buflen, dbv.pszVal, len);
			buflen += len;
			DBFreeVariant(&dbv);
		}
	}
	*(PWORD)buf = buflen - 2;
	hUpload[4] = (HANDLE)CallService("ICQ"PS_CHANGEINFO, ICQCHANGEINFO_PASSWORD, (LPARAM)buf);

	free(buf);
	return 1;
	
}