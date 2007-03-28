#include "conv.h"
#if _MSC_VER
#pragma warning( disable: 4706 )
#endif
char* strip_html(char *src)
{
    char *ptr;
    char *ptrl;
    char *rptr;
	char* dest=strldup(src,lstrlen(src));
	while ((ptr = strstr(dest, "<P>")) != NULL || (ptr = strstr(dest, "<p>")) != NULL) {
        memmove(ptr + 4, ptr + 3, lstrlen(ptr + 3) + 1);
        *ptr = '\r';
        *(ptr + 1) = '\n';
        *(ptr + 2) = '\r';
        *(ptr + 3) = '\n';
    }
    while ((ptr = strstr(dest, "</P>")) != NULL || (ptr = strstr(dest, "</p>")) != NULL) {
        *ptr = '\r';
        *(ptr + 1) = '\n';
        *(ptr + 2) = '\r';
        *(ptr + 3) = '\n';
    }
    while ((ptr = strstr(dest, "<BR>")) != NULL || (ptr = strstr(dest, "<br>")) != NULL) {
        *ptr = '\r';
        *(ptr + 1) = '\n';
        memmove(ptr + 2, ptr + 4, lstrlen(ptr + 4) + 1);
    }
    while ((ptr = strstr(dest, "<HR>")) != NULL || (ptr = strstr(dest, "<hr>")) != NULL) {
        *ptr = '\r';
        *(ptr + 1) = '\n';
        memmove(ptr + 2, ptr + 4, lstrlen(ptr + 4) + 1);
    }
    /*rptr = dest;
	while (ptr = strstr(rptr, "<A HREF=\""))
	{
		int addr=ptr-rptr;
		dest=renew(dest,lstrlen(dest)+1,7);
		rptr=dest;
		ptr=rptr+addr;
		memcpy(ptr,"[link: ",7);
		ptrl=ptr+7;
		memmove(ptrl, ptrl + 2, lstrlen(ptrl + 2) + 1);
        if ((ptrl = strstr(ptr, "\">")))
		{
			memmove(ptrl+7,ptrl+1,lstrlen(ptrl+1)+1);
			memcpy(ptrl," title: ",8);
			
			char* s1 = strstr(ptrl,"</A");
			char* s2 = strstr(rptr,"<A HREF");
			if (s1&&s1<s2||s1&&!s2)
			{
				ptr=s1;
				*ptr=' ';
				ptr[1]=']';
				memmove(ptr+2, ptr + 4, lstrlen(ptr + 4) + 1);
			}
			else if(s2&&s2<s1||s2&&!s1)
			{
				ptr=s2;
				memmove(ptr+2, ptr, lstrlen(ptr) + 1);
				*ptr=' ';
				ptr[1]=']';
			}
			else
			{
				ptr=dest;
				memcpy(&ptr[lstrlen(ptr)]," ]",3);
			}
		}
        else
            rptr++;
    }*/
	rptr = dest;
	while (ptr = strstr(rptr, "<a href=\""))
	{
		int addr=ptr-rptr;
		dest=renew(dest,lstrlen(dest)+1,7);
		rptr=dest;
		ptr=rptr+addr;
		memcpy(ptr,"[link: ",7);
		ptrl=ptr+7;
		memmove(ptrl, ptrl + 2, lstrlen(ptrl + 2) + 1);
        if ((ptrl = strstr(ptr, "\">")))
		{
			memmove(ptrl+7,ptrl+1,lstrlen(ptrl+1)+1);
			memcpy(ptrl," title: ",8);
			
			char* s1 = strstr(ptrl,"</a href");
			char* s2 = strstr(rptr,"<a href");
			if (s1&&s1<s2||s1&&!s2)
			{
				ptr=s1;
				*ptr=' ';
				ptr[1]=']';
				memmove(ptr+2, ptr + 4, lstrlen(ptr + 4) + 1);
			}
			else if(s2&&s2<s1||s2&&!s1)
			{
				ptr=s2;
				memmove(ptr+2, ptr, lstrlen(ptr) + 1);
				*ptr=' ';
				ptr[1]=']';
			}
			else
			{
				ptr=dest;
				memcpy(&ptr[lstrlen(ptr)]," ]",3);
			}
		}
        else
            rptr++;
    }
    while ((ptr = strstr(rptr, "<"))) {
        ptrl = ptr + 1;
        if ((ptrl = strstr(ptrl, ">"))) {
            memmove(ptr, ptrl + 1, lstrlen(ptrl + 1) + 1);
        }
        else
            rptr++;
    }
    ptrl = NULL;
    while ((ptr = strstr(dest, "&quot;")) != NULL && (ptrl == NULL || ptr > ptrl)) {
        *ptr = '"';
        memmove(ptr + 1, ptr + 6, lstrlen(ptr + 6) + 1);
        ptrl = ptr;
    }
    ptrl = NULL;
    while ((ptr = strstr(dest, "&lt;")) != NULL && (ptrl == NULL || ptr > ptrl)) {
        *ptr = '<';
        memmove(ptr + 1, ptr + 4, lstrlen(ptr + 4) + 1);
        ptrl = ptr;
    }
    ptrl = NULL;
    while ((ptr = strstr(dest, "&gt;")) != NULL && (ptrl == NULL || ptr > ptrl)) {
        *ptr = '>';
        memmove(ptr + 1, ptr + 4, lstrlen(ptr + 4) + 1);
        ptrl = ptr;
    }
    ptrl = NULL;
    while ((ptr = strstr(dest, "&amp;")) != NULL && (ptrl == NULL || ptr > ptrl)) {
        *ptr = '&';
        memmove(ptr + 1, ptr + 5, lstrlen(ptr + 5) + 1);
        ptrl = ptr;
    }
	return dest;
}
wchar_t* strip_html(wchar_t *src)
{
    wchar_t *ptr;
    wchar_t *ptrl;
    wchar_t *rptr;
	wchar_t* dest=wcsldup(src,wcslen(src));
    while ((ptr = wcsstr(dest,L"<P>")) != NULL || (ptr = wcsstr(dest, L"<p>")) != NULL) {
        memmove(ptr + 4, ptr + 3, wcslen(ptr + 3)*2 + 2);
        *ptr = '\r';
        *(ptr + 1) = '\n';
        *(ptr + 2) = '\r';
        *(ptr + 3) = '\n';
    }
    while ((ptr = wcsstr(dest,L"</P>")) != NULL || (ptr = wcsstr(dest, L"</p>")) != NULL) {
        *ptr = L'\r';
        *(ptr + 1) = L'\n';
        *(ptr + 2) = L'\r';
        *(ptr + 3) = L'\n';
    }
    while ((ptr = wcsstr(dest, L"<BR>")) != NULL || (ptr = wcsstr(dest, L"<br>")) != NULL) {
        *ptr = L'\r';
        *(ptr + 1) = L'\n';
        memmove(ptr + 2, ptr + 4, wcslen(ptr + 4)*2 + 2);
    }
    while ((ptr = wcsstr(dest, L"<HR>")) != NULL || (ptr = wcsstr(dest, L"<hr>")) != NULL) {
        *ptr = L'\r';
        *(ptr + 1) = L'\n';
        memmove(ptr + 2, ptr + 4, wcslen(ptr + 4)*2 + 2);
    }
    rptr = dest;
	while (ptr = wcsstr(rptr, L"<A HREF=\""))
	{
		int addr=ptr-rptr;
		dest=renew(dest,wcslen(dest)*2+2,7);
		rptr=dest;
		ptr=rptr+addr;
		memcpy(ptr,L"[link: ",7*2);
		ptrl=ptr+7;
		memmove(ptrl, ptrl + 2, wcslen(ptrl + 2)*2 + 2);
        if ((ptrl = wcsstr(ptr, L"\">")))
		{
			memmove(ptrl+7,ptrl+1,wcslen(ptrl+1)*2+2);
			memcpy(ptrl,L" title: ",8*2);
			
			wchar_t* s1 = wcsstr(ptrl,L"</A");
			wchar_t* s2 = wcsstr(rptr,L"<A HREF");
			if (s1&&s1<s2||s1&&!s2)
			{
				ptr=s1;
				*ptr=L' ';
				ptr[1]=L']';
				memmove(ptr+2, ptr + 4, wcslen(ptr + 4)*2 + 2);
			}
			else if(s2&&s2<s1||s2&&!s1)
			{
				ptr=s2;
				memmove(ptr+2, ptr, wcslen(ptr)*2 + 2);
				*ptr=L' ';
				ptr[1]=L']';
			}
			else
			{
				ptr=dest;
				memcpy(&ptr[wcslen(ptr)],L" ]",3*2);
			}
		}
        else
            rptr++;
    }
	rptr = dest;
	while (ptr = wcsstr(rptr, L"<a href=\""))
	{
		int addr=ptr-rptr;
		dest=renew(dest,wcslen(dest)*2+2,7*2);
		rptr=dest;
		ptr=rptr+addr;
		memcpy(ptr,L"[link: ",7*2);
		ptrl=ptr+7;
		memmove(ptrl, ptrl + 2, wcslen(ptrl + 2)*2 + 2);
        if ((ptrl = wcsstr(ptr, L"\">")))
		{
			memmove(ptrl+7,ptrl+1,wcslen(ptrl+1)*2+2);
			memcpy(ptrl,L" title: ",8*2);
			
			wchar_t* s1 = wcsstr(ptrl,L"</a href");
			wchar_t* s2 = wcsstr(rptr,L"<a href");
			if (s1&&s1<s2||s1&&!s2)
			{
				ptr=s1;
				*ptr=L' ';
				ptr[1]=L']';
				memmove(ptr+2, ptr + 4, wcslen(ptr + 4)*2 + 2);
			}
			else if(s2&&s2<s1||s2&&!s1)
			{
				ptr=s2;
				memmove(ptr+2, ptr, wcslen(ptr)*2 + 2);
				*ptr=L' ';
				ptr[1]=L']';
			}
			else
			{
				ptr=dest;
				memcpy(&ptr[wcslen(ptr)],L" ]",3*2);
			}
		}
        else
            rptr++;
    }
    while ((ptr = wcsstr(rptr, L"<"))) {
        ptrl = ptr + 1;
        if ((ptrl = wcsstr(ptrl, L">"))) {
            memmove(ptr, ptrl + 1, wcslen(ptrl + 1)*2 + 2);
        }
        else
            rptr++;
    }
    ptrl = NULL;
    while ((ptr = wcsstr(dest, L"&quot;")) != NULL && (ptrl == NULL || ptr > ptrl)) {
        *ptr = L'"';
        memmove(ptr + 1, ptr + 6, wcslen(ptr + 6)*2 + 2);
        ptrl = ptr;
    }
    ptrl = NULL;
    while ((ptr = wcsstr(dest, L"&lt;")) != NULL && (ptrl == NULL || ptr > ptrl)) {
        *ptr = L'<';
        memmove(ptr + 1, ptr + 4, wcslen(ptr + 4)*2 + 2);
        ptrl = ptr;
    }
    ptrl = NULL;
    while ((ptr = wcsstr(dest, L"&gt;")) != NULL && (ptrl == NULL || ptr > ptrl)) {
        *ptr = L'>';
        memmove(ptr + 1, ptr + 4, wcslen(ptr + 4)*2 + 2);
        ptrl = ptr;
    }
    ptrl = NULL;
    while ((ptr = wcsstr(dest, L"&amp;")) != NULL && (ptrl == NULL || ptr > ptrl)) {
        *ptr = L'&';
        memmove(ptr + 1, ptr + 5, wcslen(ptr + 5)*2 + 2);
        ptrl = ptr;
    }
	return dest;
}
char* strip_special_chars(char *src, HANDLE hContact)
{
	DBVARIANT dbv;
	if (!DBGetContactSetting(hContact, AIM_PROTOCOL_NAME, AIM_KEY_SN, &dbv))
	{
		char *ptr;
		char* dest=strldup(src,lstrlen(src));
		while ((ptr = strstr(dest, "%n")) != NULL)
		{
			int addr=ptr-dest;
			int	sn_length=lstrlen(dbv.pszVal);
			dest=renew(dest,lstrlen(dest)+1,sn_length*2);
			ptr=dest+addr;
			memmove(ptr + sn_length, ptr + 2, lstrlen(ptr + 2) + sn_length);
			memcpy(ptr,dbv.pszVal,sn_length);
		}
		DBFreeVariant(&dbv);
		return dest;
	}
	return 0;
}
wchar_t* plain_to_html(wchar_t *src)
{   // first mangle all html special chars
    wchar_t *ptr;
    wchar_t* dest=wcsldup(src,wcslen(src));
    ptr = dest; // need to go over
    while ((ptr = wcsstr(ptr, L"&")) != NULL)
    {
        int addr=ptr-dest;
        dest=renew(dest,wcslen(dest)+2,8);
        ptr=dest+addr;
        memmove(ptr + 5, ptr + 1, wcslen(ptr + 1)*2 + 10);
        memcpy(ptr,L"&amp;",10);
        ptr++; // break the infinite loop
    }
    while ((ptr = wcsstr(dest, L"<")) != NULL)
    {
        int addr=ptr-dest;
        dest=renew(dest,wcslen(dest)+2,7);
        ptr=dest+addr;
        memmove(ptr + 4, ptr + 1, wcslen(ptr + 1)*2 + 8);
        memcpy(ptr,L"&lt;",8);
    }
    while ((ptr = wcsstr(dest, L">")) != NULL)
    {
        int addr=ptr-dest;
        dest=renew(dest,wcslen(dest)+2,7);
        ptr=dest+addr;
        memmove(ptr + 4, ptr + 1, wcslen(ptr + 1)*2 + 8);
        memcpy(ptr,L"&gt;",8);
    }
    while ((ptr = wcsstr(dest, L"\"")) != NULL)
    {
        int addr=ptr-dest;
        dest=renew(dest,wcslen(dest)+2,9);
        ptr=dest+addr;
        memmove(ptr + 5, ptr + 1, wcslen(ptr + 1)*2 + 12);
        memcpy(ptr,L"&quot;",12);
    }
    dest[wcslen(dest)]='\0';
    // add basic html tags
    ptr = new wchar_t[wcslen(dest)+30];
    wcscpy(ptr, L"<HTML><BODY>");
    wcscat(ptr, dest);
    wcscat(ptr, L"</HTML></BODY>");
    delete[] dest;
	return ptr;
}
char* strip_carrots(char *src)// EAT!!!!!!!!!!!!!
{
		char *ptr;
		char* dest=strldup(src,lstrlen(src));
		while ((ptr = strstr(dest, "<")) != NULL)
		{
			int addr=ptr-dest;
			dest=renew(dest,lstrlen(dest)+1,7);
			ptr=dest+addr;
			memmove(ptr + 4, ptr + 1, lstrlen(ptr + 1)+1);
			memcpy(ptr,"&lt;",4);
		}
		while ((ptr = strstr(dest, ">")) != NULL)
		{
			int addr=ptr-dest;
			dest=renew(dest,lstrlen(dest)+1,7);
			ptr=dest+addr;
			memmove(ptr + 4, ptr + 1, lstrlen(ptr + 1) + 4);
			memcpy(ptr,"&gt;",4);
		}
		dest[lstrlen(dest)]='\0';
		return dest;
}
wchar_t* strip_carrots(wchar_t *src)// EAT!!!!!!!!!!!!!
{
		wchar_t *ptr;
		wchar_t* dest=wcsldup(src,wcslen(src));
		while ((ptr = wcsstr(dest, L"<")) != NULL)
		{
			int addr=ptr-dest;
			dest=renew(dest,wcslen(dest)+2,7);
			ptr=dest+addr;
			memmove(ptr + 4, ptr + 1, wcslen(ptr + 1)*2 + 8);
			memcpy(ptr,L"&lt;",8);
		}
		while ((ptr = wcsstr(dest, L">")) != NULL)
		{
			int addr=ptr-dest;
			dest=renew(dest,wcslen(dest)+2,7);
			ptr=dest+addr;
			memmove(ptr + 4, ptr + 1, wcslen(ptr + 1)*2 + 8);
			memcpy(ptr,L"&gt;",8);
		}
		dest[wcslen(dest)]='\0';
		return dest;
}
char* strip_linebreaks(char *src)
{
		char* dest=strldup(src,lstrlen(src));
		char *ptr;
		while ((ptr = strstr(dest, "\r")) != NULL)
		{
			memmove(ptr, ptr + 1, lstrlen(ptr + 1)+1);
		}
		while ((ptr = strstr(dest, "\n")) != NULL)
		{
			int addr=ptr-dest;
			dest=renew(dest,lstrlen(dest)+1,7);
			ptr=dest+addr;
			memmove(ptr + 4, ptr + 1, lstrlen(ptr + 1) + 4);
			memcpy(ptr,"<br>",4);
		}
		dest[lstrlen(dest)]='\0';
		return dest;
}
char* html_to_bbcodes(char *src)
{
    char *ptr;
    char *ptrl;
    char *rptr;
	char* dest=strldup(src,lstrlen(src));
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
			dest=renew(dest,lstrlen(dest)+1,5);
			memcpy(&dest[lstrlen(dest)],"[/b]",5);
		}
    }
	while ((ptr = strstr(dest, "<I>")) != NULL || (ptr = strstr(dest, "<i>")) != NULL)
	{
        *ptr = '[';
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
			dest=renew(dest,lstrlen(dest)+1,5);
			memcpy(&dest[lstrlen(dest)],"[/i]",5);
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
			dest=renew(dest,lstrlen(dest)+1,5);
			memcpy(&dest[lstrlen(dest)],"[/u]",5);
		}
    }
	rptr = dest;
	while (ptr = strstr(rptr,"<A HREF"))
	{
		char* begin=ptr;
        ptrl = ptr + 4;
		memcpy(ptrl,"[url=",5);
		memmove(ptr, ptrl, lstrlen(ptrl) + 1);
        if ((ptr = strstr(ptrl,">")))
		{	
			ptr-=1;
			memmove(ptr, ptr+1, lstrlen(ptr+1) + 1);
			*(ptr)=']';
			ptrl-=1;
			char* s1 = strstr(ptrl,"</A");
			char* s2 = strstr(rptr,"<A HREF");
			if (s1&&s1<s2||s1&&!s2)
			{
				ptr=s1;
				ptr=strip_tag_within(begin,ptr);
				memmove(ptr+2, ptr, lstrlen(ptr) + 1);
				memcpy(ptr,"[/url]",6);
			}
			else if(s2&&s2<s1||s2&&!s1)
			{
				ptr=s2;
				ptr=strip_tag_within(begin,ptr);
				int addr=ptr-rptr;
				dest=renew(dest,lstrlen(dest)+1,7);
				rptr=dest;
				ptr=rptr+addr;
				memmove(ptr+6, ptr, lstrlen(ptr) + 1);
				memcpy(ptr,"[/url]",6);
			}
			else
			{
				strip_tag_within(begin,&dest[lstrlen(dest)]);
				//int addr=ptr-rptr;
				dest=renew(dest,lstrlen(dest)+1,7);
				rptr=dest;
				ptr=dest;
				memcpy(&ptr[lstrlen(ptr)],"[/url]",7);
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
		memmove(ptr, ptrl, lstrlen(ptrl) + 1);
        if ((ptr = strstr(ptrl,">")))
		{
			ptr-=1;
			memmove(ptr, ptr+1, lstrlen(ptr+1) + 1);
			*(ptr)=']';
			ptrl-=1;
			char* s1 = strstr(ptrl,"</a");
			char* s2 = strstr(ptrl,"<a href");
			if (s1&&s1<s2||s1&&!s2)
			{
				ptr=s1;
				ptr=strip_tag_within(begin,ptr);
				memmove(ptr+2, ptr, lstrlen(ptr) + 1);
				memcpy(ptr,"[/url]",6);
			}
			else if(s2&&s2<s1||s2&&!s1)
			{
				ptr=s2;
				ptr=strip_tag_within(begin,ptr);
				int addr=ptr-rptr;
				dest=renew(dest,lstrlen(dest)+1,7);
				rptr=dest;
				ptr=rptr+addr;
				memmove(ptr+6, ptr, lstrlen(ptr) + 1);
				memcpy(ptr,"[/url]",6);
			}
			else
			{
				strip_tag_within(begin,&dest[lstrlen(dest)]);
				//int addr=ptr-rptr;
				dest=renew(dest,lstrlen(dest)+1,7);
				rptr=dest;
				ptr=dest;
				memcpy(&ptr[lstrlen(ptr)],"[/url]",7);
			}
		}
        else
            rptr++;
    }
    rptr = dest;
	while (ptr = strstr(rptr, "<FONT COLOR=\""))
	{
		int addr=ptr-rptr;
		dest=renew(dest,lstrlen(dest)+1,7);
		rptr=dest;
		ptr=rptr+addr;
        ptrl = ptr + 6;
		memcpy(ptrl,"[color=",7);
		memmove(ptr, ptrl, lstrlen(ptrl) + 1);
        if ((ptr = strstr(ptrl, ">")))
		{
			memmove(ptrl+7,ptr,lstrlen(ptr)+1);
			*(ptrl+7)=']';
			ptr=ptrl+7;
			char* s1 = strstr(ptr,"</FONT");
			char* s2 = strstr(ptr,"<FONT COLOR=\"");	
			if (s1&&s1<s2||s1&&!s2)
			{
				ptr=s1;
				memmove(ptr+1, ptr, lstrlen(ptr) + 1);
				memcpy(ptr,"[/color]",8);
			}
			else if(s2&&s2<s1||s2&&!s1)
			{
				ptr=s2;
				memmove(ptr+8, ptr, lstrlen(ptr) + 1);
				memcpy(ptr,"[/color]",8);
			}
			else
			{
				ptr=dest;
				memcpy(&ptr[lstrlen(ptr)],"[/color]",9);
			}
		}
        else
            rptr++;
	}
    rptr = dest;
	while (ptr = strstr(rptr, "<font color=\""))
	{
		int addr=ptr-rptr;
		dest=renew(dest,lstrlen(dest)+1,7);
		rptr=dest;
		ptr=rptr+addr;
        ptrl = ptr + 6;
		memcpy(ptrl,"[color=",7);
		memmove(ptr, ptrl, lstrlen(ptrl) + 1);
        if ((ptr = strstr(ptrl, ">")))
		{
			memmove(ptrl+7,ptr,lstrlen(ptr)+1);
			*(ptrl+7)=']';
			ptr=ptrl+7;
			char* s1 = strstr(ptr,"</font");
			char* s2 = strstr(ptr,"<font color=\"");	
			if (s1&&s1<s2||s1&&!s2)
			{
				ptr=s1;
				memmove(ptr+1, ptr, lstrlen(ptr) + 1);
				memcpy(ptr,"[/color]",8);
			}
			else if(s2&&s2<s1||s2&&!s1)
			{
				ptr=s2;
				memmove(ptr+8, ptr, lstrlen(ptr) + 1);
				memcpy(ptr,"[/color]",8);
			}
			else
			{
				ptr=dest;
				memcpy(&ptr[lstrlen(ptr)],"[/color]",9);
			}
		}
        else
            rptr++;
	}
	rptr = dest;
	while ((ptr = strstr(rptr, "<FONT COLOR=")) || (ptr = strstr(rptr, "<font color=")))
	{
		int addr=ptr-rptr;
		dest=renew(dest,lstrlen(dest)+1,7);
		rptr=dest;
		ptr=rptr+addr;
        ptrl = ptr + 5;
		memcpy(ptrl,"[color=",7);
		memmove(ptr, ptrl, lstrlen(ptrl) + 1);
        if ((ptr = strstr(ptrl, ">")))
		{
			*(ptr)=']';
			if ((ptrl = strstr(ptr, "</FONT")) || (ptrl = strstr(ptr, "</font")))
			{
				memmove(ptrl+1, ptrl, lstrlen(ptrl) + 1);
				memcpy(ptrl,"[/color]",8);
			}
			else
			{
				memcpy(&dest[lstrlen(dest)],"[/color]",9);
			}
		}
        else
            rptr++;
	}
	return dest;
}
wchar_t* html_to_bbcodes(wchar_t *src)
{
    wchar_t *ptr;
    wchar_t *ptrl;
    wchar_t *rptr;
	wchar_t* dest=wcsldup(src,wcslen(src));
    while ((ptr = wcsstr(dest, L"<B>")) != NULL || (ptr = wcsstr(dest, L"<b>")) != NULL)
	{
        *ptr = L'[';
		*(ptr+1) = L'b';
        *(ptr+2) = L']';
		if((ptr = wcsstr(dest, L"</B>")) != NULL || (ptr = wcsstr(dest, L"</b>")) != NULL)
		{
			*ptr = L'[';
			*(ptr+2) = L'b';
			*(ptr+3) = L']';
		}
		else
		{
			dest=renew(dest,wcslen(dest)*2+2,5*2);
			memcpy(&dest[wcslen(dest)],L"[/b]",5*2);
		}
    }
	while ((ptr = wcsstr(dest, L"<I>")) != NULL || (ptr = wcsstr(dest, L"<i>")) != NULL)
	{
        *ptr = L'[';
		*(ptr+1) = L'i';
        *(ptr+2) = L']';
		if((ptr = wcsstr(dest, L"</I>")) != NULL || (ptr = wcsstr(dest, L"</i>")) != NULL)
		{
			*ptr = L'[';
			*(ptr+2) = L'i';
			*(ptr+3) = L']';
		}
		else
		{
			dest=renew(dest,wcslen(dest)*2+2,5*2);
			memcpy(&dest[wcslen(dest)],L"[/i]",5*2);
		}
	}
	while ((ptr = wcsstr(dest, L"<U>")) != NULL || (ptr = wcsstr(dest, L"<u>")) != NULL)
	{
        *ptr = L'[';
		*(ptr+1) = L'u';
        *(ptr+2) = L']';
		if((ptr = wcsstr(dest, L"</U>")) != NULL || (ptr = wcsstr(dest, L"</u>")) != NULL)
		{
			*ptr = L'[';
			*(ptr+2) = L'u';
			*(ptr+3) = L']';
		}
		else
		{
			dest=renew(dest,wcslen(dest)*2+2,5*2);
			memcpy(&dest[wcslen(dest)],L"[/u]",5*2);
		}
    }
	rptr = dest;
	while (ptr = wcsstr(rptr,L"<A HREF"))
	{
		wchar_t* begin=ptr;
        ptrl = ptr + 4;
		memcpy(ptrl,L"[url=",5*2);
		memmove(ptr, ptrl, wcslen(ptrl)*2 + 2);
        if ((ptr = wcsstr(ptrl,L">")))
		{	
			ptr-=1;
			memmove(ptr, ptr+1, wcslen(ptr+1)*2 + 2);
			*(ptr)=L']';
			ptrl-=1;
			wchar_t* s1 = wcsstr(ptrl,L"</A");
			wchar_t* s2 = wcsstr(rptr,L"<A HREF");
			if (s1&&s1<s2||s1&&!s2)
			{
				ptr=s1;
				ptr=strip_tag_within(begin,ptr);
				memmove(ptr+2, ptr, wcslen(ptr)*2 + 2);
				memcpy(ptr,L"[/url]",6*2);
			}
			else if(s2&&s2<s1||s2&&!s1)
			{
				ptr=s2;
				ptr=strip_tag_within(begin,ptr);
				int addr=ptr-rptr;
				dest=renew(dest,wcslen(dest)*2+2,7*2);
				rptr=dest;
				ptr=rptr+addr;
				memmove(ptr+6, ptr, wcslen(ptr)*2 + 2);
				memcpy(ptr,L"[/url]",6*2);
			}
			else
			{
				strip_tag_within(begin,&dest[wcslen(dest)]);
				//int addr=ptr-rptr;
				dest=renew(dest,wcslen(dest)*2+2,7*2);
				rptr=dest;
				ptr=dest;
				memcpy(&ptr[wcslen(ptr)],L"[/url]",7*2);
			}
		}
        else
            rptr++;
    }
	rptr = dest;
	while (ptr = wcsstr(rptr,L"<a href"))
	{
		wchar_t* begin=ptr;
        ptrl = ptr + 4;
		memcpy(ptrl,L"[url=",5*2);
		memmove(ptr, ptrl, wcslen(ptrl)*2 + 2);
        if ((ptr = wcsstr(ptrl,L">")))
		{
			ptr-=1;
			memmove(ptr, ptr+1, wcslen(ptr+1)*2 + 2);
			*(ptr)=L']';
			ptrl-=1;
			wchar_t* s1 = wcsstr(ptrl,L"</a");
			wchar_t* s2 = wcsstr(ptrl,L"<a href");
			if (s1&&s1<s2||s1&&!s2)
			{
				ptr=s1;
				ptr=strip_tag_within(begin,ptr);
				memmove(ptr+2, ptr, wcslen(ptr)*2 + 2);
				memcpy(ptr,L"[/url]",6*2);
			}
			else if(s2&&s2<s1||s2&&!s1)
			{
				ptr=s2;
				ptr=strip_tag_within(begin,ptr);
				int addr=ptr-rptr;
				dest=renew(dest,wcslen(dest)*2+2,7*2);
				rptr=dest;
				ptr=rptr+addr;
				memmove(ptr+6, ptr, wcslen(ptr)*2 + 2);
				memcpy(ptr,L"[/url]",6*2);
			}
			else
			{
				strip_tag_within(begin,&dest[wcslen(dest)]);
				//int addr=ptr-rptr;
				dest=renew(dest,wcslen(dest)*2+2,7*2);
				rptr=dest;
				ptr=dest;
				memcpy(&ptr[wcslen(ptr)],L"[/url]",7*2);
			}
		}
        else
            rptr++;
    }
    rptr = dest;
	while (ptr = wcsstr(rptr, L"<FONT COLOR=\""))
	{
		int addr=ptr-rptr;
		dest=renew(dest,wcslen(dest)*2+2,7*2);
		rptr=dest;
		ptr=rptr+addr;
        ptrl = ptr + 6;
		memcpy(ptrl,L"[color=",7);
		memmove(ptr, ptrl, wcslen(ptrl)*2 + 2);
        if ((ptr = wcsstr(ptrl, L">")))
		{
			memmove(ptrl+7,ptr,wcslen(ptr)*2+2);
			*(ptrl+7)=L']';
			ptr=ptrl+7;
			wchar_t* s1 = wcsstr(ptr,L"</FONT");
			wchar_t* s2 = wcsstr(ptr,L"<FONT COLOR=\"");	
			if (s1&&s1<s2||s1&&!s2)
			{
				ptr=s1;
				memmove(ptr+1, ptr, wcslen(ptr)*2 + 2);
				memcpy(ptr,L"[/color]",8*2);
			}
			else if(s2&&s2<s1||s2&&!s1)
			{
				ptr=s2;
				memmove(ptr+8, ptr, wcslen(ptr)*2 + 2);
				memcpy(ptr,L"[/color]",8*2);
			}
			else
			{
				ptr=dest;
				memcpy(&ptr[wcslen(ptr)],L"[/color]",9*2);
			}
		}
        else
            rptr++;
	}
    rptr = dest;
	while (ptr = wcsstr(rptr, L"<font color=\""))
	{
		int addr=ptr-rptr;
		dest=renew(dest,wcslen(dest)*2+2,7*2);
		rptr=dest;
		ptr=rptr+addr;
        ptrl = ptr + 6;
		memcpy(ptrl,L"[color=",7*2);
		memmove(ptr, ptrl, wcslen(ptrl)*2 + 2);
        if ((ptr = wcsstr(ptrl, L">")))
		{
			memmove(ptrl+7,ptr,wcslen(ptr)*2+2);
			*(ptrl+7)=L']';
			ptr=ptrl+7;
			wchar_t* s1 = wcsstr(ptr,L"</font");
			wchar_t* s2 = wcsstr(ptr,L"<font color=\"");	
			if (s1&&s1<s2||s1&&!s2)
			{
				ptr=s1;
				memmove(ptr+1, ptr, wcslen(ptr)*2 + 2);
				memcpy(ptr,L"[/color]",8*2);
			}
			else if(s2&&s2<s1||s2&&!s1)
			{
				ptr=s2;
				memmove(ptr+8, ptr, wcslen(ptr)*2 + 2);
				memcpy(ptr,L"[/color]",8*2);
			}
			else
			{
				ptr=dest;
				memcpy(&ptr[wcslen(ptr)],L"[/color]",9*2);
			}
		}
        else
            rptr++;
	}
	rptr = dest;
	while ((ptr = wcsstr(rptr, L"<FONT COLOR=")) || (ptr = wcsstr(rptr, L"<font color=")))
	{
		int addr=ptr-rptr;
		dest=renew(dest,wcslen(dest)*2+2,7*2);
		rptr=dest;
		ptr=rptr+addr;
        ptrl = ptr + 5;
		memcpy(ptrl,L"[color=",7*2);
		memmove(ptr, ptrl, wcslen(ptrl)*2 + 2);
        if ((ptr = wcsstr(ptrl, L">")))
		{
			*(ptr)=L']';
			if ((ptrl = wcsstr(ptr, L"</FONT")) || (ptrl = wcsstr(ptr, L"</font")))
			{
				memmove(ptrl+1, ptrl, wcslen(ptrl)*2 + 2);
				memcpy(ptrl,L"[/color]",8*2);
			}
			else
			{
				memcpy(&dest[wcslen(dest)],L"[/color]",9*2);
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
	char* dest=strldup(src,lstrlen(src));
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
		dest=renew(dest,lstrlen(dest)+1,7);
		rptr=dest;
		ptr=rptr+addr;
		memmove(ptr+5, ptr, lstrlen(ptr) + 1);
		memcpy(ptr,"<font ",6);
        if ((ptr = strstr(ptr, "]")))
		{
			*(ptr)='>';
			if ((ptr = strstr(ptr, "[/color]")))
			{
				memcpy(ptr,"</font>",7);
				memmove(ptr+7,ptr+8,lstrlen(ptr+8)+1);
			}
		}
        else
            rptr++;
    }
	rptr = dest;
	while ((ptr = strstr(rptr, "[url=")))
	{
		int addr=ptr-rptr;
		dest=renew(dest,lstrlen(dest)+1,7);
		rptr=dest;
		ptr=rptr+addr;
		memmove(ptr+3, ptr, lstrlen(ptr) + 1);
		memcpy(ptr,"<a href",7);
        if ((ptr = strstr(ptr, "]")))
		{
			*(ptr)='>';
			if ((ptr = strstr(ptr, "[/url]")))
			{
				memcpy(ptr,"</a>",4);
				memmove(ptr+4,ptr+6,lstrlen(ptr+6)+1);
			}
		}
        else
            rptr++;
    }
	return dest;
}
wchar_t* bbcodes_to_html(const wchar_t *src)
{
    wchar_t *ptr;
    wchar_t *rptr;
	wchar_t* dest=wcsldup(src,wcslen(src));
    while ((ptr = wcsstr(dest, L"[b]")) != NULL) {
        *ptr = L'<';
		*(ptr+1) = L'b';
        *(ptr+2) = L'>';
    }
	while ((ptr = wcsstr(dest, L"[/b]")) != NULL) {
        *ptr = L'<';
		*(ptr+2) = L'b';
        *(ptr+3) = L'>';
    }
	while ((ptr = wcsstr(dest, L"[i]")) != NULL) {
        *ptr = L'<';
		*(ptr+1) = L'i';
        *(ptr+2) = L'>';
    }
	while ((ptr = wcsstr(dest, L"[/i]")) != NULL) {
        *ptr = L'<';
		*(ptr+2) = L'i';
		*(ptr+3) = L'>';
    }
	while ((ptr = wcsstr(dest, L"[u]")) != NULL) {
        *ptr = L'<';
		*(ptr+1) = L'u';
        *(ptr+2) = L'>';
    }
	while ((ptr = wcsstr(dest, L"[/u]")) != NULL) {
        *ptr = L'<';
		*(ptr+2) = L'u';
        *(ptr+3) = L'>';
    }
    rptr = dest;
	while ((ptr = wcsstr(rptr, L"[color=")))
	{
		int addr=ptr-rptr;
		dest=renew(dest,wcslen(dest)+2,7);
		rptr=dest;
		ptr=rptr+addr;
		memmove(ptr+5, ptr, wcslen(ptr)*sizeof(wchar_t) + 2);
		memcpy(ptr,L"<font ",6*sizeof(wchar_t));
        if ((ptr = wcsstr(ptr,L"]")))
		{
			*(ptr)=L'>';
			if ((ptr = wcsstr(ptr,L"[/color]")))
			{
				memcpy(ptr,L"</font>",7*sizeof(wchar_t));
				memmove(ptr+7,ptr+8,wcslen(ptr+8)*sizeof(wchar_t)+2);
			}
		}
        else
            rptr++;
    }
	while ((ptr = wcsstr(rptr, L"[url=")))
	{
		int addr=ptr-rptr;
		dest=renew(dest,wcslen(dest)+2,7);
		rptr=dest;
		ptr=rptr+addr;
		memmove(ptr+3, ptr, wcslen(ptr)+2);
		memcpy(ptr,L"<a href",7);
        if ((ptr = wcsstr(ptr, L"]")))
		{
			*(ptr)=L'>';
			if ((ptr = wcsstr(ptr, L"[/url]")))
			{
				memcpy(ptr,L"</a>",4*sizeof(wchar_t));
				memmove(ptr+4,ptr+6,wcslen(ptr+6)*sizeof(wchar_t)+2);
			}
		}
        else
            rptr++;
    }
	return dest;
}
void strip_tag(char* begin, char* end)
{
	memmove(begin,end+1,lstrlen(end+1)+1);
}
void strip_tag(wchar_t* begin, wchar_t* end)
{
	memmove(begin,end+1,wcslen(end+1)+2);
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
//strip a tag within a string
wchar_t* strip_tag_within(wchar_t* begin, wchar_t* end)
{
	while(wchar_t* sub_begin=wcsstr(begin,L"<"))
	{	
		if(sub_begin<end)//less than the original ending
		{
			wchar_t* sub_end=wcsstr(begin,L">");
			strip_tag(sub_begin,sub_end);
			end=end-(sub_end-sub_begin)-1;
		}
		else
			break;
	}
	return end;
}
#if _MSC_VER
#pragma warning( default: 4706 )
#endif
