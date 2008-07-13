#include "aim.h"
#include "conv.h"

#if _MSC_VER
	#pragma warning( disable: 4706 )
#endif

char* strip_html(char *src)
{
	char *ptr;
	char *ptrl;
	char *rptr;
	char* dest = strldup(src, strlen(src));
	while ((ptr = strstr(dest,"<P>")) != NULL || (ptr = strstr(dest, "<p>")) != NULL) {
		memmove(ptr + 4, ptr + 3, strlen(ptr + 3) + 1);
		memcpy(ptr, "\r\n\r\n", 4);
	}
	while ((ptr = strstr(dest,"</P>")) != NULL || (ptr = strstr(dest, "</p>")) != NULL) {
		memcpy(ptr, "\r\n\r\n", 4);
	}
	while ((ptr = strstr(dest, "<BR>")) != NULL || (ptr = strstr(dest, "<br>")) != NULL) {
		memcpy(ptr, "\r\n", 2);
		memmove(ptr + 2, ptr + 4, strlen(ptr + 4) + 1);
	}
	while ((ptr = strstr(dest, "<HR>")) != NULL || (ptr = strstr(dest, "<hr>")) != NULL) {
		memcpy(ptr, "\r\n", 2);
		memmove(ptr + 2, ptr + 4, strlen(ptr + 4) + 1);
	}
	rptr = dest;
	while ((ptr = strstr(rptr, "<"))) {
		ptrl = ptr + 1;
		if ((ptrl = strstr(ptrl, ">"))) {
			memmove(ptr, ptrl + 1, strlen(ptrl + 1) + 1);
		}
		else
			rptr++;
	}
	ptrl = NULL;
	while ((ptr = strstr(dest, "&quot;")) != NULL && (ptrl == NULL || ptr > ptrl)) {
		*ptr = '"';
		memmove(ptr + 1, ptr + 6, strlen(ptr + 6) + 1);
		ptrl = ptr;
	}
	ptrl = NULL;
	while ((ptr = strstr(dest, "&lt;")) != NULL && (ptrl == NULL || ptr > ptrl)) {
		*ptr = '<';
		memmove(ptr + 1, ptr + 4, strlen(ptr + 4) + 1);
		ptrl = ptr;
	}
	ptrl = NULL;
	while ((ptr = strstr(dest, "&gt;")) != NULL && (ptrl == NULL || ptr > ptrl)) {
		*ptr = '>';
		memmove(ptr + 1, ptr + 4, strlen(ptr + 4) + 1);
		ptrl = ptr;
	}
	ptrl = NULL;
	while ((ptr = strstr(dest, "&amp;")) != NULL && (ptrl == NULL || ptr > ptrl)) {
		*ptr = '&';
		memmove(ptr + 1, ptr + 5, strlen(ptr + 5) + 1);
		ptrl = ptr;
	}
	return dest;
}

char* CAimProto::strip_special_chars(char *src, HANDLE hContact)
{
	DBVARIANT dbv;
	if (!getString(hContact, AIM_KEY_SN, &dbv))
	{
		char *ptr;
		char* dest=strldup(src,lstrlenA(src));
		while ((ptr = strstr(dest, "%n")) != NULL)
		{
			int addr=ptr-dest;
			int	sn_length=lstrlenA(dbv.pszVal);
			dest=renew(dest,lstrlenA(dest)+1,sn_length*2);
			ptr=dest+addr;
			memmove(ptr + sn_length, ptr + 2, lstrlenA(ptr + 2) + sn_length);
			memcpy(ptr,dbv.pszVal,sn_length);
		}
		DBFreeVariant(&dbv);
		return dest;
	}
	return 0;
}

char* strip_carrots(char *src)// EAT!!!!!!!!!!!!!
{
	char *ptr;
	char* dest=strldup(src, strlen(src));
	while ((ptr = strstr(dest, "<")) != NULL)
	{
		int addr=ptr-dest;
		dest=renew(dest,strlen(dest),7);
		ptr=dest+addr;
		memmove(ptr + 4, ptr + 1, strlen(ptr + 1) + 4);
		memcpy(ptr,"&lt;",4);
	}
	while ((ptr = strstr(dest, ">")) != NULL)
	{
		int addr=ptr-dest;
		dest=renew(dest,strlen(dest)+2,7);
		ptr=dest+addr;
		memmove(ptr + 4, ptr + 1, strlen(ptr + 1) + 4);
		memcpy(ptr,"&gt;",4);
	}
	dest[strlen(dest)]='\0';
	return dest;
}

char* strip_linebreaks(char *src)
{
	char* dest=strldup(src,lstrlenA(src));
	char *ptr;
	while ((ptr = strstr(dest, "\r")) != NULL)
	{
		memmove(ptr, ptr + 1, lstrlenA(ptr + 1)+1);
	}
	while ((ptr = strstr(dest, "\n")) != NULL)
	{
		int addr=ptr-dest;
		dest=renew(dest,lstrlenA(dest)+1,7);
		ptr=dest+addr;
		memmove(ptr + 4, ptr + 1, lstrlenA(ptr + 1) + 4);
		memcpy(ptr,"<br>",4);
	}
	dest[lstrlenA(dest)]='\0';
	return dest;
}

char* html_to_bbcodes(char *src)
{
	char *ptr;
	char *ptrl;
	char *rptr;
	char* dest = strldup(src, strlen(src));
	while ((ptr = strstr(dest, "<B>")) != NULL || (ptr = strstr(dest, "<b>")) != NULL)
	{
		*ptr = '[';
		*(ptr+1) = 'b';
		*(ptr+2) = ']';
		if((ptr = strstr(dest, "</B>")) != NULL || (ptr = strstr(dest, "</b>")) != NULL)
		{
			*ptr = '[';
			*(ptr+2) = 'b';
			*(ptr+3) = ']';
		}
		else
		{
			dest=renew(dest,strlen(dest)+1,5);
			memcpy(&dest[strlen(dest)],"[/b]",5);
		}
	}
	while ((ptr = strstr(dest, "<I>")) != NULL || (ptr = strstr(dest, "<i>")) != NULL)
	{
		*ptr =  '[';
		*(ptr+1) = 'i';
		*(ptr+2) = ']';
		if((ptr = strstr(dest, "</I>")) != NULL || (ptr = strstr(dest, "</i>")) != NULL)
		{
			*ptr = '[';
			*(ptr+2) = 'i';
			*(ptr+3) = ']';
		}
		else
		{
			dest=renew(dest,strlen(dest)+1,5);
			memcpy(&dest[strlen(dest)],"[/i]",5);
		}
	}
	while ((ptr = strstr(dest, "<U>")) != NULL || (ptr = strstr(dest, "<u>")) != NULL)
	{
		*ptr = '[';
		*(ptr+1) = 'u';
		*(ptr+2) = ']';
		if((ptr = strstr(dest, "</U>")) != NULL || (ptr = strstr(dest, "</u>")) != NULL)
		{
			*ptr = '[';
			*(ptr+2) = 'u';
			*(ptr+3) = ']';
		}
		else
		{
			dest=renew(dest,strlen(dest)+1,5);
			memcpy(&dest[strlen(dest)],"[/u]",5);
		}
	}
	rptr = dest;
	while (ptr = strstr(rptr,"<A HREF"))
	{
		char* begin=ptr;
		ptrl = ptr + 4;
		memcpy(ptrl,"[url=",5);
		memmove(ptr, ptrl, strlen(ptrl) + 1);
		if ((ptr = strstr(ptrl,">")))
		{	
			ptr-=1;
			memmove(ptr, ptr+1, strlen(ptr+1) + 1);
			*(ptr)=']';
			ptrl-=1;
			char* s1 = strstr(ptrl,"</A");
			char* s2 = strstr(rptr,"<A HREF");
			if (s1&&s1<s2||s1&&!s2)
			{
				ptr=s1;
				ptr=strip_tag_within(begin,ptr);
				memmove(ptr+2, ptr, strlen(ptr) + 1);
				memcpy(ptr,"[/url]",6);
			}
			else if(s2&&s2<s1||s2&&!s1)
			{
				ptr=s2;
				ptr=strip_tag_within(begin,ptr);
				int addr=ptr-rptr;
				dest=renew(dest,strlen(dest)+1,7);
				rptr=dest;
				ptr=rptr+addr;
				memmove(ptr+6, ptr, strlen(ptr) + 1);
				memcpy(ptr,"[/url]",6);
			}
			else
			{
				strip_tag_within(begin,&dest[strlen(dest)]);
				//int addr=ptr-rptr;
				dest=renew(dest,strlen(dest)+1,7);
				rptr=dest;
				ptr=dest;
				memcpy(&ptr[strlen(ptr)],"[/url]",7);
			}
		}
		else
			rptr++;
	}
	rptr = dest;
	while (ptr = strstr(rptr,"<a href"))
	{
		char* begin=ptr;
		ptrl = ptr + 4;
		memcpy(ptrl,"[url=",5);
		memmove(ptr, ptrl, strlen(ptrl) + 1);
		if ((ptr = strstr(ptrl,">")))
		{
			ptr-=1;
			memmove(ptr, ptr+1, strlen(ptr+1) + 1);
			*(ptr)=']';
			ptrl-=1;
			char* s1 = strstr(ptrl,"</a");
			char* s2 = strstr(ptrl,"<a href");
			if (s1&&s1<s2||s1&&!s2)
			{
				ptr=s1;
				ptr=strip_tag_within(begin,ptr);
				memmove(ptr+2, ptr, strlen(ptr) + 1);
				memcpy(ptr,"[/url]",6);
			}
			else if(s2&&s2<s1||s2&&!s1)
			{
				ptr=s2;
				ptr=strip_tag_within(begin,ptr);
				int addr=ptr-rptr;
				dest=renew(dest,strlen(dest)+1,7);
				rptr=dest;
				ptr=rptr+addr;
				memmove(ptr+6, ptr, strlen(ptr) + 1);
				memcpy(ptr,"[/url]",6);
			}
			else
			{
				strip_tag_within(begin,&dest[strlen(dest)]);
				//int addr=ptr-rptr;
				dest=renew(dest,strlen(dest)+1,7);
				rptr=dest;
				ptr=dest;
				memcpy(&ptr[strlen(ptr)],"[/url]",7);
			}
		}
		else
			rptr++;
	}
	rptr = dest;
	while (ptr = strstr(rptr, "<FONT COLOR=\""))
	{
		int addr=ptr-rptr;
		dest=renew(dest,strlen(dest)+1,7);
		rptr=dest;
		ptr=rptr+addr;
		ptrl = ptr + 6;
		memcpy(ptrl,"[color=",7);
		memmove(ptr, ptrl, strlen(ptrl) + 1);
		if ((ptr = strstr(ptrl, ">")))
		{
			memmove(ptrl+7,ptr,strlen(ptr)+1);
			*(ptrl+7)=']';
			ptr=ptrl+7;
			char* s1 = strstr(ptr,"</FONT");
			char* s2 = strstr(ptr,"<FONT COLOR=\"");	
			if (s1&&s1<s2||s1&&!s2)
			{
				ptr=s1;
				memmove(ptr+1, ptr, strlen(ptr) + 1);
				memcpy(ptr,"[/color]",8);
			}
			else if(s2&&s2<s1||s2&&!s1)
			{
				ptr=s2;
				memmove(ptr+8, ptr, strlen(ptr) + 1);
				memcpy(ptr,"[/color]",8);
			}
			else
			{
				ptr=dest;
				memcpy(&ptr[strlen(ptr)],"[/color]",9);
			}
		}
		else
			rptr++;
	}
	rptr = dest;
	while (ptr = strstr(rptr, "<font color=\""))
	{
		int addr=ptr-rptr;
		dest=renew(dest,strlen(dest)+1,7);
		rptr=dest;
		ptr=rptr+addr;
		ptrl = ptr + 6;
		memcpy(ptrl,"[color=",7);
		memmove(ptr, ptrl, strlen(ptrl) + 1);
		if ((ptr = strstr(ptrl, ">")))
		{
			memmove(ptrl+7,ptr,strlen(ptr)+1);
			*(ptrl+7)=']';
			ptr=ptrl+7;
			char* s1 = strstr(ptr,"</font");
			char* s2 = strstr(ptr,"<font color=\"");	
			if (s1&&s1<s2||s1&&!s2)
			{
				ptr=s1;
				memmove(ptr+1, ptr, strlen(ptr) + 1);
				memcpy(ptr,"[/color]",8);
			}
			else if(s2&&s2<s1||s2&&!s1)
			{
				ptr=s2;
				memmove(ptr+8, ptr, strlen(ptr) + 1);
				memcpy(ptr,"[/color]",8);
			}
			else
			{
				ptr=dest;
				memcpy(&ptr[strlen(ptr)],"[/color]",9);
			}
		}
		else
			rptr++;
	}
	rptr = dest;
	while ((ptr = strstr(rptr, "<FONT COLOR=")) || (ptr = strstr(rptr, "<font color=")))
	{
		int addr=ptr-rptr;
		dest=renew(dest,strlen(dest)+1,7);
		rptr=dest;
		ptr=rptr+addr;
		ptrl = ptr + 5;
		memcpy(ptrl,"[color=",7);
		memmove(ptr, ptrl, strlen(ptrl) + 1);
		if ((ptr = strstr(ptrl, ">")))
		{
			*(ptr)=']';
			if ((ptrl = strstr(ptr, "</FONT")) || (ptrl = strstr(ptr, "</font")))
			{
				memmove(ptrl+1, ptrl, strlen(ptrl) + 1);
				memcpy(ptrl,"[/color]",8);
			}
			else
			{
				memcpy(&dest[strlen(dest)],"[/color]",9);
			}
		}
		else
			rptr++;
	}
	return dest;
}

char* bbcodes_to_html(const char *src)
{
	char *ptr;
	char *rptr;
	char* dest=strldup(src,strlen(src));
	while ((ptr = strstr(dest, "[b]")) != NULL) {
		*ptr = '<';
		*(ptr+1) = 'b';
		*(ptr+2) = '>';
	}
	while ((ptr = strstr(dest, "[/b]")) != NULL) {
		*ptr = '<';
		*(ptr+2) = 'b';
		*(ptr+3) = '>';
	}
	while ((ptr = strstr(dest, "[i]")) != NULL) {
		*ptr = '<';
		*(ptr+1) = 'i';
		*(ptr+2) = '>';
	}
	while ((ptr = strstr(dest, "[/i]")) != NULL) {
		*ptr = '<';
		*(ptr+2) = 'i';
		*(ptr+3) = '>';
	}
	while ((ptr = strstr(dest, "[u]")) != NULL) {
		*ptr = '<';
		*(ptr+1) = 'u';
		*(ptr+2) = '>';
	}
	while ((ptr = strstr(dest, "[/u]")) != NULL) {
		*ptr = '<';
		*(ptr+2) = 'u';
		*(ptr+3) = '>';
	}
	rptr = dest;
	while ((ptr = strstr(rptr, "[color=")))
	{
		int addr=ptr-rptr;
		dest=renew(dest,strlen(dest)+1,7);
		rptr=dest;
		ptr=rptr+addr;
		memmove(ptr+5, ptr, strlen(ptr) + 1);
		memcpy(ptr,"<font ",6);
		if ((ptr = strstr(ptr,"]")))
		{
			*(ptr)='>';
			if ((ptr = strstr(ptr,"[/color]")))
			{
				memcpy(ptr,"</font>",7);
				memmove(ptr+7,ptr+8,strlen(ptr+8)+1);
			}
		}
		else
			rptr++;
	}
	while ((ptr = strstr(rptr, "[url=")))
	{
		int addr=ptr-rptr;
		dest=renew(dest,strlen(dest)+1,7);
		rptr=dest;
		ptr=rptr+addr;
		memmove(ptr+3, ptr, strlen(ptr)+1);
		memcpy(ptr,"<a href",7);
		if ((ptr = strstr(ptr, "]")))
		{
			*(ptr)='>';
			if ((ptr = strstr(ptr, "[/url]")))
			{
				memcpy(ptr,"</a>",4);
				memmove(ptr+4,ptr+6,strlen(ptr+6)+1);
			}
		}
		else
			rptr++;
	}
	return dest;
}

void strip_tag(char* begin, char* end)
{
	memmove(begin,end+1,lstrlenA(end+1)+1);
}

//strip a tag within a string
char* strip_tag_within(char* begin, char* end)
{
	while(char* sub_begin=strstr(begin,"<"))
	{	
		if(sub_begin<end)//less than the original ending
		{
			char* sub_end=strstr(begin,">");
			strip_tag(sub_begin,sub_end);
			end=end-(sub_end-sub_begin)-1;
		}
		else
			break;
	}
	return end;
}

char* rtf_to_html(HWND hwndDlg,int DlgItem)
{
	char* buf=new char[4024];
	int pos=0;
	int start=0;
	int end=1;
	BOOL Bold=false;
	BOOL Italic=false;
	BOOL Underline=false;
	char Face[32]="\0";
	COLORREF Color;
	COLORREF BackColor;
	int Size=0;
	GETTEXTLENGTHEX tl;
	tl.flags=GTL_DEFAULT;
	tl.codepage=CP_ACP;
	int length=SendDlgItemMessage(hwndDlg, DlgItem, EM_GETTEXTLENGTHEX,(WPARAM)&tl,0);
	while(start<length)
	{
		SendDlgItemMessage(hwndDlg, DlgItem, EM_SETSEL, start, end);
		CHARFORMAT2A cfOld;
		cfOld.cbSize = sizeof(CHARFORMAT2);
		cfOld.dwMask = CFM_BOLD|CFM_ITALIC|CFM_UNDERLINE|CFM_SIZE|CFM_COLOR|CFM_BACKCOLOR|CFM_FACE;
		SendDlgItemMessage(hwndDlg, DlgItem, EM_GETCHARFORMAT, SCF_SELECTION, (LPARAM)&cfOld);
		BOOL isBold = (cfOld.dwEffects & CFE_BOLD) && (cfOld.dwMask & CFM_BOLD);
		BOOL isItalic = (cfOld.dwEffects & CFE_ITALIC) && (cfOld.dwMask & CFM_ITALIC);
		BOOL isUnderline = (cfOld.dwEffects & CFE_UNDERLINE) && (cfOld.dwMask & CFM_UNDERLINE);
		COLORREF isColor=cfOld.crTextColor;
		COLORREF isBackColor=cfOld.crBackColor;
		int isSize;
		if(cfOld.yHeight==38*20)
			isSize=7;
		else if(cfOld.yHeight==24*20)
			isSize=6;
		else if(cfOld.yHeight==18*20)
			isSize=5;
		else if(cfOld.yHeight==14*20)
			isSize=4;
		else if(cfOld.yHeight==12*20)
			isSize=3;
		else if(cfOld.yHeight==10*20)
			isSize=2;
		else if(cfOld.yHeight==8*20)
			isSize=1;
		else
			isSize=3;
		char text[2];
		SendDlgItemMessage(hwndDlg, DlgItem, EM_GETSELTEXT, 0, (LPARAM)&text);
		if(Bold!=isBold)
		{
			Bold=isBold;
			if(isBold)
			{
				strlcpy(&buf[pos],"<b>",4);
				pos+=3;
			}
			else
			{
				if(start!=0)
				{
					strlcpy(&buf[pos],"</b>",5);
					pos+=4;	
				}
			}
		}
		if(Italic!=isItalic)
		{
			Italic=isItalic;
			if(isItalic)
			{
				strlcpy(&buf[pos],"<i>",4);
				pos+=3;
			}
			else
			{
				if(start!=0)
				{
					strlcpy(&buf[pos],"</i>",5);
					pos+=4;	
				}
			}
		}
		if(Underline!=isUnderline)
		{
			Underline=isUnderline;
			if(isUnderline)
			{
				strlcpy(&buf[pos],"<u>",4);
				pos+=3;
			}
			else
			{
				if(start!=0)
				{
					strlcpy(&buf[pos],"</u>",5);
					pos+=4;	
				}
			}
		}
		if ( Size != isSize || Color != isColor || BackColor != isBackColor || lstrcmpA( Face, cfOld.szFaceName ))
		{
			Size=isSize;
			Color=isColor;
			BackColor=isBackColor;
			strlcpy(Face,cfOld.szFaceName,strlen(cfOld.szFaceName)+1);
			if(start!=0)
			{
				strlcpy(&buf[pos],"</font>",8);
				pos+=7;
			}
			strlcpy(&buf[pos],"<font",6);
			pos+=5;
			strlcpy(&buf[pos],"	face=\"",8);
			pos+=7;
			strlcpy(&buf[pos],Face,strlen(Face)+1);
			pos+=strlen(Face);
			strlcpy(&buf[pos],"\"",2);
			pos++;
			if(!(cfOld.dwEffects & CFE_AUTOBACKCOLOR))
			{
				strlcpy(&buf[pos]," back=#",7);
				pos+=6;
				char chBackColor[7];
				_itoa((_htonl(BackColor)>>8),chBackColor,16);
				int len=strlen(chBackColor);
				if(len<6)
				{
					memmove(chBackColor+(6-len),chBackColor,len+1);
					for(int i=0;i<6;i++)
						chBackColor[i]='0';
				}
				strlcpy(&buf[pos],chBackColor,7);
				pos+=6;
			}
			if(!(cfOld.dwEffects & CFE_AUTOCOLOR))
			{
				strlcpy(&buf[pos]," color=#",9);
				pos+=8;
				char chColor[7];
				_itoa((_htonl(Color)>>8),chColor,16);
				int len=strlen(chColor);
				if(len<6)
				{
					memmove(chColor+(6-len),chColor,len+1);
					for(int i=0;i<6;i++)
						chColor[i]='0';
				}
				strlcpy(&buf[pos],chColor,7);
				pos+=6;
			}
			strlcpy(&buf[pos]," size=",7);
			pos+=6;
			char chSize[2];
			_itoa(Size,chSize,10);
			strlcpy(&buf[pos],chSize,2);
			pos++;

			strlcpy(&buf[pos],">",2);
			pos++;
		}
		if(text[0]=='\r')
		{
			strlcpy(&buf[pos],"<br>",5);
			pos+=4;
		}
		else
		{
			strlcpy(&buf[pos],text,2);
			pos++;
		}
		start++;
		end++;
	}
	if(Bold)
	{
		strlcpy(&buf[pos],"</b>",5);
		pos+=4;	
	}
	if(Italic)
	{
		strlcpy(&buf[pos],"</i>",5);
		pos+=4;	
	}
	if(Underline)
	{
		strlcpy(&buf[pos],"</u>",5);
		pos+=4;	
	}
	strlcpy(&buf[pos],"</font>",8);
	pos+=7;
	return buf;
}
#if _MSC_VER
	#pragma warning( default: 4706 )
#endif
