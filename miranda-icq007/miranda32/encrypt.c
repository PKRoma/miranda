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
//#define WINVER 0x0500
//#define _WIN32_WINNT 0x0500

#include "encryption.h"
//VERY VERY VERY BASIC ENCRYPTION FUNCTION

void Encrypt(char*msg,BOOL up)
{
	int i;
	int jump;
	if (up==TRUE)
	{
		jump=5;
	}
	else
	{
		jump=-5;
	}

	for (i=0;i<(int)strlen(msg);i++)
	{
			msg[i]=msg[i]+jump;
	}

}