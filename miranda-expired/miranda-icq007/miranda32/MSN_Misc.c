//MSN Messenger Support for Miranda ICQ
//Copyright 2000, Tristan Van de Vreede

//MISC FUNC

	#include <windows.h>
	#include <winsock.h>
	#include <stdio.h>
	#include "msn_global.h"
	#include "miranda.h"
	#include "global.h"
	#include "pluginapi.h"

	void MSN_DebugLog(char*msg)
	{
		char *tmp;
		tmp=(char*)malloc(strlen(msg)+10);
		sprintf(tmp,"[MSN]%s\n",msg);
		
		//Send to Plugins
		Plugin_NotifyPlugins(PM_ICQDEBUGMSG,(WPARAM)strlen(tmp),(LPARAM)tmp);
		//send to VC debug wnd
		OutputDebugString(tmp);


		free(tmp);		
	}
	
	void MSN_DebugLogEx(char *msg,char*msg2,char*msg3)
	{
		char *m;
		long len;
		len=strlen(msg);
		if (msg2) len=len+strlen(msg2);
		if (msg3) len=len+strlen(msg3);

		m=(char*)malloc(len+1);

		strcpy(m,msg);
		if (msg2)
			strcat(m,msg2);
		if (msg3)
			strcat(m,msg3);

		MSN_DebugLog(m);
		free(m);
	}
	void MSN_GetWd(char*src,int wordno,char*out)
	{
		char *fpos;
		char *epos;
		int i=0;
		long ln;
		
		fpos=src;
		
		for (;;)
		{
			fpos=strchr(fpos,' ')+1;
			if (fpos)
			{
				i++;
			}
			else
			{
				out[0]=0;
				return;
			}

			if ((i+1)==wordno)
				break;
			
							
		}

		//found the start of the wd
		epos=strchr(fpos,' ');
		if (!epos)
		{
			//end of word is end of the str
			strcpy(out,fpos);
			
			if (strchr(out,13))
			{//there is a CR in there, assume at the end,strip it
				out[strlen(out)]=0;
			}

			return ;
		}

		ln=epos-src+1;
		ln=ln-(fpos-src+1);
		strncpy(out,fpos,ln);
		out[ln]=0;

	}


	int MSN_GetIntStatus(char *state)
	{//convert msn status (NLN,AWY) to a int, to store in the contact struct
		if (stricmp(state,MSN_STATUS_OFFLINE)==0)
			return MSN_INTSTATUS_OFFLINE;
		
		if (stricmp(state,MSN_STATUS_ONLINE)==0)
			return MSN_INTSTATUS_ONLINE;

		if (stricmp(state,MSN_STATUS_BUSY)==0)
			return MSN_INTSTATUS_BUSY;

		if (stricmp(state,MSN_STATUS_AWAY)==0)
			return MSN_INTSTATUS_AWAY;

		if (stricmp(state,MSN_STATUS_BRB)==0)
			return MSN_INTSTATUS_BRB;

		if (stricmp(state,MSN_STATUS_PHONE)==0)
			return MSN_INTSTATUS_PHONE;
		
		if (stricmp(state,MSN_STATUS_LUNCH)==0)
			return MSN_INTSTATUS_LUNCH;
	}

	void MSN_RemoveContact(char* uhandle)
	{
		if (opts.MSN.sNS)
			MSN_SendPacket(opts.MSN.sNS,"REM",uhandle);

	}


	
/* returned value MUST be freed by the caller */
char *str_to_UTF8(unsigned char *in)
{
    int n, i = 0;
    char *result = NULL;

    /* for now, allocate more than enough space... */
    result = (char *) malloc(strlen(in) * 2 + 1);

    //convert a string to UTF-8 Format
    for (n = 0; n < strlen(in); n++)
    {
        //if(c<0x80)
        long c = (long) in[n];

        if (c < 128)
        {
            result[i++] = (char) c;
        }
        else
        {
            /*
              ' Character range from 0x80 to 0xFF is treated as an *eleven* bit
              ' character for Unicode compatibility, even though we only have
              ' 8 bits.  The first byte indicates how many total bytes in each
              ' character by the number of high-bits set.  Each following byte
              ' ALWAYS begins with the bit sequence 10.
              
              ' First byte is a combination of 0xC0 (11000000) and the first
              ' 5 bits of the 11-bit sequence (the 2 highest bits of our char),
              ' which we get by shifting our char 6 bits right, and or'ing
              ' it with 0xC0.  The initial "110" bits indicate a 2-byte encoding.
            */
            //Result += Chr((Ord(value[n]) shr 6) or $C0);
            //result[i++]=(char)((c>>6)|0xC0);
            result[i++] = (char) ((c >> 6) | 192);

            /*
              ' Second byte is a combination of x80 (10000000) and the last
              ' 6 bits of the 11-bit sequence, which we get by masking off
              ' our char with 00111111 (0x3F), and or'ing itr with 0x80.  The
              ' initial "10" marks this as the middle of a multi-byte sequence.
            */
            //Result := Result + Chr((Ord(value[n]) and $3F) or $80);
            //result[i++]=(char)((c&0x3F)|0x80);
            result[i++] = (char) ((c & 63) | 128);
        }
    }
    result[i] = '\0';
    return result;
}
