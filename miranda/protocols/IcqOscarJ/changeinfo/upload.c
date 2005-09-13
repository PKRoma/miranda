// ---------------------------------------------------------------------------80
//                ICQ plugin for Miranda Instant Messenger
//                ________________________________________
// 
// Copyright � 2001,2002,2003,2004 Richard Hughes, Martin �berg
// Copyright � 2004,2005 Joe Kucera, Bio
// 
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
//
// -----------------------------------------------------------------------------
//
// File name      : $Source$
// Revision       : $Revision$
// Last change on : $Date$
// Last change by : $Author$
//
// DESCRIPTION:
//
//  ChangeInfo Plugin stuff
//
// -----------------------------------------------------------------------------

#include "icqoscar.h"



static DWORD DBStringToDWord(const char *szSetting)
{
  char szTmp[256];
  DWORD dw = 0;

  if (!ICQGetContactStaticString(NULL, szSetting, szTmp, sizeof(szTmp)))
  {
    if (IsStringUIN(szTmp))
      dw = atoi(szTmp);
  }

  return dw;
}


int StringToListItemId(const char *szSetting,int def)
{
  int i,listCount;
  char szTmp[256];
  ListTypeDataItem *list;

  for(i=0;i<settingCount;i++)
    if(!strcmpnull(szSetting,setting[i].szDbSetting))
      break;

  if(i==settingCount) return def;

  list=(ListTypeDataItem*)setting[i].pList;
  listCount=setting[i].listCount;

  if(ICQGetContactStaticString(NULL, szSetting, szTmp, sizeof(szTmp)))
    return def;

  for(i=0;i<listCount;i++)
    if(!strcmpnull(list[i].szValue, szTmp)) break;

  if(i==listCount) return def;

  return list[i].id;
}


int UploadSettings(HWND hwndParent)
{
  PBYTE buf = NULL;
  int buflen = 0;
  BYTE b;
  WORD w;

  if (!icqOnline) 
  {
    MessageBox(hwndParent, Translate("You are not currently connected to the ICQ network. You must be online in order to update your information on the server."), Translate("Change ICQ Details"), MB_OK);
    return 0;
  }

   ppackLEWord(&buf, &buflen, 0);   //this will be the data length in the end

  // userinfo
  ppackTLVLNTSfromDB(&buf, &buflen, "Nick", TLV_NICKNAME);
  ppackTLVLNTSfromDB(&buf, &buflen, "FirstName", TLV_FIRSTNAME);
  ppackTLVLNTSfromDB(&buf, &buflen, "LastName", TLV_LASTNAME);

  b = !ICQGetContactSettingByte(NULL, "PublishPrimaryEmail", 0);
  ppackTLVLNTSBytefromDB(&buf, &buflen, "e-mail", b, TLV_EMAIL);
  ppackTLVLNTSBytefromDB(&buf, &buflen, "e-mail0", 0, TLV_EMAIL);
  ppackTLVLNTSBytefromDB(&buf, &buflen, "e-mail1", 0, TLV_EMAIL);

  ppackTLVLNTSfromDB(&buf, &buflen, "City", TLV_CITY);
  ppackTLVLNTSfromDB(&buf, &buflen, "State", TLV_STATE);
  ppackTLVLNTSfromDB(&buf, &buflen, "Phone", TLV_PHONE);
  ppackTLVLNTSfromDB(&buf, &buflen, "Fax", TLV_FAX);
  ppackTLVLNTSfromDB(&buf, &buflen, "Cellular", TLV_MOBILE);
  ppackTLVLNTSfromDB(&buf, &buflen, "Street", TLV_STREET);

  ppackTLVDWord(&buf, &buflen, DBStringToDWord("ZIP"), TLV_ZIPCODE, 0);

  ppackTLVWord(&buf, &buflen, ICQGetContactSettingWord(NULL, "Country", 0), TLV_COUNTRY, 1);
  ppackTLVByte(&buf, &buflen, ICQGetContactSettingByte(NULL, "Timezone", 0), TLV_TIMEZONE, 1);
  ppackTLVWord(&buf, &buflen, ICQGetContactSettingWord(NULL, "Age", 0), TLV_AGE, 1);

  b = ICQGetContactSettingByte(NULL, "Gender", 0);
  ppackTLVByte(&buf, &buflen, (BYTE)(b ? (b == 'M' ? 2 : 1) : 0), TLV_GENDER, 1);

  ppackTLVLNTSfromDB(&buf, &buflen, "Homepage", TLV_URL);

  ppackLEWord(&buf, &buflen, TLV_BIRTH);
  ppackLEWord(&buf, &buflen, 0x06);
  ppackLEWord(&buf, &buflen, ICQGetContactSettingWord(NULL, "BirthYear", 0));
  ppackLEWord(&buf, &buflen, (WORD)ICQGetContactSettingByte(NULL, "BirthMonth", 0));
  ppackLEWord(&buf, &buflen, (WORD)ICQGetContactSettingByte(NULL, "BirthDay", 0));

  ppackTLVByte(&buf, &buflen, ICQGetContactSettingByte(NULL, "MaritalStatus", 0), TLV_MARITAL, 1);

  ppackTLVWord(&buf, &buflen, (WORD)StringToListItemId("Language1", 0), TLV_LANGUAGE, 1);
  ppackTLVWord(&buf, &buflen, (WORD)StringToListItemId("Language2", 0), TLV_LANGUAGE, 1);
  ppackTLVWord(&buf, &buflen, (WORD)StringToListItemId("Language3", 0), TLV_LANGUAGE, 1);

  ppackTLVLNTSfromDB(&buf, &buflen, "OriginCity", TLV_ORGCITY);
  ppackTLVLNTSfromDB(&buf, &buflen, "OriginState", TLV_ORGSTATE);
  ppackTLVWord(&buf, &buflen, ICQGetContactSettingWord(NULL, "OriginCountry", 0), TLV_ORGCOUNTRY, 1);

  ppackTLVLNTSfromDB(&buf, &buflen, "About", TLV_ABOUT);

  ppackTLVLNTSfromDB(&buf, &buflen, "CompanyCity", TLV_WORKCITY);
  ppackTLVLNTSfromDB(&buf, &buflen, "CompanyState", TLV_WORKSTATE);
  ppackTLVLNTSfromDB(&buf, &buflen, "CompanyPhone", TLV_WORKPHONE);
  ppackTLVLNTSfromDB(&buf, &buflen, "CompanyFax", TLV_WORKFAX);
  ppackTLVLNTSfromDB(&buf, &buflen, "CompanyStreet", TLV_WORKSTREET);

  ppackTLVDWord(&buf, &buflen, DBStringToDWord("CompanyZIP"), TLV_WORKZIPCODE, 0);

  ppackTLVWord(&buf, &buflen, ICQGetContactSettingWord(NULL, "CompanyCountry", 0), TLV_WORKCOUNTRY, 1);
  ppackTLVLNTSfromDB(&buf, &buflen, "Company", TLV_COMPANY);
  ppackTLVLNTSfromDB(&buf, &buflen, "CompanyDepartment", TLV_DEPARTMENT);
  ppackTLVLNTSfromDB(&buf, &buflen, "CompanyPosition", TLV_POSITION);
  ppackTLVWord(&buf, &buflen, ICQGetContactSettingWord(NULL, "CompanyOccupation", 0), TLV_OCUPATION, 1);
  ppackTLVLNTSfromDB(&buf, &buflen, "CompanyHomepage", TLV_WORKURL);

  w = StringToListItemId("Interest0Cat", 0);
  ppackTLVWordLNTSfromDB(&buf, &buflen, w, "Interest0Text", TLV_INTERESTS);
  w = StringToListItemId("Interest1Cat", 0);
  ppackTLVWordLNTSfromDB(&buf, &buflen, w, "Interest1Text", TLV_INTERESTS);
  w = StringToListItemId("Interest2Cat", 0);
  ppackTLVWordLNTSfromDB(&buf, &buflen, w, "Interest2Text", TLV_INTERESTS);
  w = StringToListItemId("Interest3Cat", 0);
  ppackTLVWordLNTSfromDB(&buf, &buflen, w, "Interest3Text", TLV_INTERESTS);

  w = StringToListItemId("Past0", 0);
  ppackTLVWordLNTSfromDB(&buf, &buflen, w, "Past0Text", TLV_PASTINFO);
  w = StringToListItemId("Past1", 0);
  ppackTLVWordLNTSfromDB(&buf, &buflen, w, "Past1Text", TLV_PASTINFO);
  w = StringToListItemId("Past2", 0);
  ppackTLVWordLNTSfromDB(&buf, &buflen, w, "Past2Text", TLV_PASTINFO);

  w = StringToListItemId("Affiliation0", 0);
  ppackTLVWordLNTSfromDB(&buf, &buflen, w, "Affiliation0Text", TLV_AFFILATIONS);
  w = StringToListItemId("Affiliation1", 0);
  ppackTLVWordLNTSfromDB(&buf, &buflen, w, "Affiliation1Text", TLV_AFFILATIONS);
  w = StringToListItemId("Affiliation2", 0);
  ppackTLVWordLNTSfromDB(&buf, &buflen, w, "Affiliation2Text", TLV_AFFILATIONS);

  *(PWORD)buf = buflen - 2;
  hUpload[0] = (HANDLE)IcqChangeInfo(META_SET_FULLINFO_REQ, (LPARAM)buf);

  //password
  {
    char* tmp;
    buflen = 2;

    tmp = GetUserPassword(TRUE);
    if(tmp)
    {
      if (strlennull(Password) > 0 && strcmpnull(Password, tmp))
      {
        ppackLELNTS(&buf, &buflen, tmp);

        *(PWORD)buf = buflen - 2;
        hUpload[1] = (HANDLE)IcqChangeInfo(ICQCHANGEINFO_PASSWORD, (LPARAM)buf);

        {
          DBVARIANT dbv;

          if (!ICQGetContactSetting(NULL, "Password", &dbv) && strlennull(dbv.pszVal))
          { // password is stored in DB, update
            char ptmp[16];

            strcpy(ptmp, tmp);

            CallService(MS_DB_CRYPT_ENCODESTRING, sizeof(ptmp), (LPARAM)ptmp);

            ICQWriteContactSettingString(NULL, "Password", ptmp);
          }
          DBFreeVariant(&dbv);
        }
      }
    }
  }

  SAFE_FREE(&buf);

  return 1;
}
