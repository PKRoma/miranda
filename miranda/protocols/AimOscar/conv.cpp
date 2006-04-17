#include "conv.h"
char* strip_html(char *src)
{
    char *ptr;
    char *ptrl;
    char *rptr;
	char* dest=strldup(src,strlen(src));
	while ((ptr = strstr(dest, "<P>")) != NULL || (ptr = strstr(dest, "<p>")) != NULL) {
        memmove(ptr + 4, ptr + 3, strlen(ptr + 3) + 1);
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
        memmove(ptr + 2, ptr + 4, strlen(ptr + 4) + 1);
    }
    while ((ptr = strstr(dest, "<HR>")) != NULL || (ptr = strstr(dest, "<hr>")) != NULL) {
        *ptr = '\r';
        *(ptr + 1) = '\n';
        memmove(ptr + 2, ptr + 4, strlen(ptr + 4) + 1);
    }
    rptr = dest;
	while ((ptr = strstr(rptr, "<A HREF=\"")) || (ptr = strstr(rptr, "<a href=\"")))
	{
		int addr=ptr-rptr;
		dest=renew(dest,strlen(dest),7);
		rptr=dest;
		ptr=rptr+addr;
		memcpy(ptr,"[link: ",7);
		ptrl=ptr+7;
		memmove(ptrl, ptrl + 1, strlen(ptrl + 1) + 1);
        if ((ptrl = strstr(ptr, "\">")))
		{
			memmove(ptrl+9,ptrl+2,strlen(ptrl+2)+1);
			memcpy(ptrl+1," title: ",8);
			if ((ptr = strstr(ptrl, "</A")) || (ptr = strstr(ptrl, "</a")))
			{
				*ptr=']';
				memmove(ptr+1, ptr + 4, strlen(ptr + 4) + 1);
			}
		}
        else
            rptr++;
    }
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
	while ((ptr = wcsstr(rptr, L"<A HREF=\"")) || (ptr = wcsstr(rptr, L"<a href=\"")))
	{
       	int addr=ptr-rptr;
		dest=renew(dest,wcslen(dest),7);
		rptr=dest;
		ptr=rptr+addr;
		memcpy(ptr,L"[link: ",14);
		ptrl=ptr+7;
		memmove(ptrl, ptrl + 1, wcslen(ptrl + 1)*2 + 2);
        if ((ptrl = wcsstr(ptr, L"\">")))
		{
			memmove(ptrl+9,ptrl+2,wcslen(ptrl+2)*2 + 2);
			memcpy(ptrl+1,L" title: ",16);
			if ((ptr = wcsstr(ptrl, L"</A")) || (ptr = wcsstr(ptrl, L"</a")))
			{
				*ptr=L']';
				memmove(ptr+1, ptr + 4, wcslen(ptr + 4)*2 + 2);
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
void strip_special_chars(char *src, HANDLE hContact)
{
	DBVARIANT dbv;
	if (!DBGetContactSetting(hContact, AIM_PROTOCOL_NAME, AIM_KEY_SN, &dbv))
	{
		char *ptr;
		while ((ptr = strstr(src, "%n")) != NULL)
		{
			int	sn_length=strlen(dbv.pszVal);
			memmove(ptr + sn_length, ptr + 2, strlen(ptr + 2) + sn_length);
			memcpy(ptr,dbv.pszVal,sn_length);
		}
		DBFreeVariant(&dbv);
	}
}
char* strip_carrots(char *src)// EAT!!!!!!!!!!!!!
{
		char *ptr;
		char* dest=strldup(src,strlen(src));
		while ((ptr = strstr(dest, "<")) != NULL)
		{
			int addr=ptr-dest;
			dest=renew(dest,strlen(dest),7);
			ptr=dest+addr;
			memmove(ptr + 4, ptr + 1, strlen(ptr + 1)+1);
			memcpy(ptr,"&lt;",4);
		}
		while ((ptr = strstr(dest, ">")) != NULL)
		{
			int addr=ptr-dest;
			dest=renew(dest,strlen(dest),7);
			ptr=dest+addr;
			memmove(ptr + 4, ptr + 1, strlen(ptr + 1) + 4);
			memcpy(ptr,"&gt;",4);
		}
		dest[strlen(dest)]='\0';
		return dest;
}
wchar_t* strip_carrots(wchar_t *src)// EAT!!!!!!!!!!!!!
{
		wchar_t *ptr;
		wchar_t* dest=wcsldup(src,wcslen(src));
		while ((ptr = wcsstr(dest, L"<")) != NULL)
		{
			int addr=ptr-dest;
			dest=renew(dest,wcslen(dest),7);
			ptr=dest+addr;
			memmove(ptr + 4, ptr + 1, wcslen(ptr + 1)*2 + 8);
			memcpy(ptr,L"&lt;",8);
		}
		while ((ptr = wcsstr(dest, L">")) != NULL)
		{
			int addr=ptr-dest;
			dest=renew(dest,wcslen(dest),7);
			ptr=dest+addr;
			memmove(ptr + 4, ptr + 1, wcslen(ptr + 1)*2 + 8);
			memcpy(ptr,L"&gt;",8);
		}
		dest[wcslen(dest)]='\0';
		return dest;
}
char* strip_linebreaks(char *src)
{
		char* dest=strldup(src,strlen(src));
		char *ptr;
		while ((ptr = strstr(dest, "\r")) != NULL)
		{
			memmove(ptr, ptr + 1, strlen(ptr + 1)+1);
		}
		while ((ptr = strstr(dest, "\n")) != NULL)
		{
			int addr=ptr-dest;
			dest=renew(dest,strlen(dest),7);
			ptr=dest+addr;
			memmove(ptr + 4, ptr + 1, strlen(ptr + 1) + 4);
			memcpy(ptr,"<br>",4);
		}
		dest[strlen(dest)]='\0';
		return dest;
}
void html_to_bbcodes(char *src)
{
    char *ptr;
    char *ptrl;
    char *rptr;
    while ((ptr = strstr(src, "<B>")) != NULL || (ptr = strstr(src, "<b>")) != NULL) {
        *ptr = '[';
		*(ptr+1) = 'b';
        *(ptr+2) = ']';
    }
	while ((ptr = strstr(src, "</B>")) != NULL || (ptr = strstr(src, "</b>")) != NULL) {
        *ptr = '[';
		*(ptr+2) = 'b';
        *(ptr+3) = ']';
    }
	while ((ptr = strstr(src, "<I>")) != NULL || (ptr = strstr(src, "<i>")) != NULL) {
        *ptr = '[';
		*(ptr+1) = 'i';
        *(ptr+2) = ']';
    }
	while ((ptr = strstr(src, "</I>")) != NULL || (ptr = strstr(src, "</i>")) != NULL) {
        *ptr = '[';
		*(ptr+2) = 'i';
        *(ptr+3) = ']';
    }
	while ((ptr = strstr(src, "<U>")) != NULL || (ptr = strstr(src, "<u>")) != NULL) {
        *ptr = '[';
		*(ptr+1) = 'u';
        *(ptr+2) = ']';
    }
	while ((ptr = strstr(src, "</U>")) != NULL || (ptr = strstr(src, "</u>")) != NULL) {
        *ptr = '[';
		*(ptr+2) = 'u';
        *(ptr+3) = ']';
    }
    rptr = src;
	while ((ptr = strstr(rptr, "<FONT COLOR=\"")) || (ptr = strstr(rptr, "<font color=\""))) {
        ptrl = ptr + 6;
		memcpy(ptrl,"[color=",7);
		memmove(ptr, ptrl, strlen(ptrl) + 1);
        if ((ptr = strstr(ptrl, ">"))) {
			memmove(ptrl+7,ptr,strlen(ptr)+1);
			*(ptrl+7)=']';
			if ((ptr = strstr(ptrl, "</FONT")) || (ptr = strstr(ptrl, "</font"))) {
				memmove(ptr+1, ptr, strlen(ptr) + 1);
				memcpy(ptr,"[/color]",8);
			}
			else{
			memcpy(&src[strlen(src)],"[/color]\0",9);
			}
		}
        else
            rptr++;
	}
	while ((ptr = strstr(rptr, "<FONT COLOR=")) || (ptr = strstr(rptr, "<font color="))) {
        ptrl = ptr + 5;
		memcpy(ptrl,"[color=",7);
		memmove(ptr, ptrl, strlen(ptrl) + 1);
        if ((ptr = strstr(ptrl, ">"))) {
			*(ptr)=']';
			if ((ptrl = strstr(ptr, "</FONT")) || (ptrl = strstr(ptr, "</font"))) {
				memmove(ptrl+1, ptrl, strlen(ptrl) + 1);
				memcpy(ptrl,"[/color]",8);
			}
			else{
			memcpy(&src[strlen(src)],"[/color]\0",9);	
			}
		}
        else
            rptr++;
    }
}
void html_to_bbcodes(wchar_t *src)
{
    wchar_t *ptr;
    wchar_t *ptrl;
    wchar_t *rptr;
    while ((ptr = wcsstr(src,L"<B>")) != NULL || (ptr = wcsstr(src,L"<b>")) != NULL) {
        *ptr = '[';
		*(ptr+1) = 'b';
        *(ptr+2) = ']';
    }
	while ((ptr = wcsstr(src,L"</B>")) != NULL || (ptr = wcsstr(src,L"</b>")) != NULL) {
        *ptr = '[';
		*(ptr+2) = 'b';
        *(ptr+3) = ']';
    }
	while ((ptr = wcsstr(src,L"<I>")) != NULL || (ptr = wcsstr(src,L"<i>")) != NULL) {
        *ptr = '[';
		*(ptr+1) = 'i';
        *(ptr+2) = ']';
    }
	while ((ptr = wcsstr(src,L"</I>")) != NULL || (ptr = wcsstr(src,L"</i>")) != NULL) {
        *ptr = '[';
		*(ptr+2) = 'i';
        *(ptr+3) = ']';
    }
	while ((ptr = wcsstr(src,L"<U>")) != NULL || (ptr = wcsstr(src,L"<u>")) != NULL) {
        *ptr = '[';
		*(ptr+1) = 'u';
        *(ptr+2) = ']';
    }
	while ((ptr = wcsstr(src,L"</U>")) != NULL || (ptr = wcsstr(src,L"</u>")) != NULL) {
        *ptr = '[';
		*(ptr+2) = 'u';
        *(ptr+3) = ']';
    }
    rptr = src;
	while ((ptr = wcsstr(rptr,L"<FONT COLOR=\"")) || (ptr = wcsstr(rptr,L"<font color=\""))) {
        ptrl = ptr + 6;
		memcpy(ptrl,L"[color=",7*sizeof(wchar_t));
		memmove(ptr, ptrl, wcslen(ptrl)*2 + 2);
        if ((ptr = wcsstr(ptrl,L">"))) {
			memmove(ptrl+7,ptr,wcslen(ptr)*2 + 2);
			*(ptrl+7)=']';
			if ((ptr = wcsstr(ptrl,L"</FONT")) || (ptr = wcsstr(ptrl,L"</font"))) {
				memmove(ptr+1, ptr, wcslen(ptr)*2 + 2);
				memcpy(ptr,L"[/color]",8*sizeof(wchar_t));
			}
			else{
			memcpy(&src[wcslen(src)],L"[/color]",8*sizeof(wchar_t));
			}
		}
        else
            rptr++;
	}
	while ((ptr = wcsstr(rptr,L"<FONT COLOR=")) || (ptr = wcsstr(rptr,L"<font color="))) {
        ptrl = ptr + 5;
		memcpy(ptrl,L"[color=",7*sizeof(wchar_t));
		memmove(ptr, ptrl, wcslen(ptrl)*2 + 2);
        if ((ptr = wcsstr(ptrl,L">"))) {
			*(ptr)=']';
			if ((ptr = wcsstr(ptrl,L"</FONT")) || (ptr = wcsstr(ptrl,L"</font"))) {
				memmove(ptr+1, ptr, wcslen(ptr)*2 + 2);
				memcpy(ptr,L"[/color]\0",9*sizeof(wchar_t));
			}
			else{
			memcpy(&src[wcslen(src)],L"[/color]\0",9*sizeof(wchar_t));
			}
		}
        else
            rptr++;
    }
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
		dest=renew(dest,strlen(dest),7);
		rptr=dest;
		ptr=rptr+addr;
		memmove(ptr+5, ptr, strlen(ptr) + 1);
		memcpy(ptr,"<font ",6);
        if ((ptr = strstr(ptr, "]")))
		{
			*(ptr)='>';
			if ((ptr = strstr(ptr, "[/color]")))
			{
				memcpy(ptr,"</font>",7);
				memmove(ptr+7,ptr+8,strlen(ptr+8)+1);
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
		dest=renew(dest,wcslen(dest),7);
		rptr=dest;
		ptr=rptr+addr;
		memmove(ptr+5, ptr, wcslen(ptr)*2 + 2);
		memcpy(ptr,L"<font ",6*sizeof(wchar_t));
        if ((ptr = wcsstr(ptr,L"]")))
		{
			*(ptr)=L'>';
			if ((ptr = wcsstr(ptr,L"[/color]")))
			{
				memcpy(ptr,L"</font>",7*sizeof(wchar_t));
				memmove(ptr+7,ptr+8,wcslen(ptr+8)*2+2);
			}
		}
        else
            rptr++;
    }
	return dest;
}
