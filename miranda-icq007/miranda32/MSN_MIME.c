//MSN Messenger Support for Miranda ICQ
//Copyright 2000, Tristan Van de Vreede

//PACKET HANDLING FUNCS

	#include <windows.h>
	#include <winsock.h>
	#include <stdio.h>
	#include "msn_global.h"
	

	#include "miranda.h"
	#include "global.h"
	#include "resource.h"

	void MSN_MIME_GetContentType(char*src,char*ct)
	{
		char *pos;
		char *tmp;
		pos=strstr(src,"Content-Type:")+14;
		if (pos)
		{
			tmp=(char*)malloc(strlen(pos));
			strcpy(tmp,pos);
			*strchr(tmp,'\r')=0;
			strcpy(ct,tmp);
		}
		else
		{
			ct[0]=0;
			return;
		}
		

	}
	