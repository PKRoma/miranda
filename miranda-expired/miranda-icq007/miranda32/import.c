/*
Miranda ICQ: the free icq client for MS Windows 
Copyright (C) 2000  Roland Rabien

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

For more information, e-mail figbug@users.sourceforge.net
*/

//ICQ DB IMPORTER - By Tristan Van de Vreede
//only been tested with icq 2000b .dat files, still has some problems
//#define WINVER 0x0500
//#define _WIN32_WINNT 0x0500

#include <windows.h>
#include <stdio.h>

#include "import.h"
#include "miranda.h"
#include "global.h"
#include "internal.h"
//from miranda.c
//////////////////

void ImportContacts(char* src)
{
	FILE *f;
	char c[1];
	unsigned long UIN;
	char un[6];
	int i;
	BOOL match;
	unsigned long *UINS;
	int uincnt=0;

	f=fopen(src,"rb");
	if (f)
	{
		for(;;)
		{
			if(feof(f)){break;}
			fgets(c,3,f);
			
			if (c[0]==0x4E &&  c[1]==0)
			{
				fgets(c,2,f);
				//cnt++;
				if (c[0]==0x069)
				{
					fgets(un,6,f);
					
					match=FALSE;

					UIN=(unsigned long)((un[3] & 0xFF)<<24) | ((un[2] & 0xFF)<<16) | ((un[1] & 0xFF)<<8) | (un[0] & 0xFF);
					
					
					for (i=0;i<uincnt;i++)
					{
						if (UIN==UINS[i])
						{ //match
							match=TRUE;
							break;
						}
					}

					if (match==FALSE)
					{
						
						if (uincnt==0)
						{
							UINS=(unsigned long*)malloc(sizeof(unsigned long));
						}
						else
						{
							UINS=(unsigned long*)realloc(UINS,sizeof(unsigned long)*(uincnt+1));
						}

						UINS[uincnt]=UIN;
						AddToContactList(UIN);
						uincnt++;
					}

					//}


				}
			}


		}
		free(UINS);
		fclose(f);
	}
	

	if (uincnt==0)
	{
		MessageBox(ghwnd,"No contacts were found.",MIRANDANAME,MB_OK);
	}
	else
	{
		MessageBox(ghwnd,"Import complete. You must be online to get the user's details.",MIRANDANAME,MB_OK);
	}
}

void ImportPrompt()
{
	char buf[MAX_PATH] = {0};
	OPENFILENAME ofn;

	MessageBox(ghwnd,"Note: The ICQ DB Importer has known problems. Don't expect too much from it.",MIRANDANAME,MB_OK);

	memset(&ofn, 0, sizeof(ofn));
	ofn.lStructSize = sizeof(ofn);
	ofn.hwndOwner = ghwnd;
	ofn.hInstance = NULL;
	ofn.lpstrFilter = "ICQ .dat file\0*.DAT\0";
	ofn.lpstrFile = buf;
	ofn.Flags = OFN_FILEMUSTEXIST;
	ofn.nMaxFile = sizeof(buf);
	ofn.nMaxFileTitle = MAX_PATH;
	ofn.lpstrDefExt = "dat";
	
	if (GetOpenFileName(&ofn))
		ImportContacts(buf);
}