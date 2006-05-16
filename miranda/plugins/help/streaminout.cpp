/*
Miranda IM Help Plugin
Copyright (C) 2002 Richard Hughes

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
#if !defined(VK_OEM_PLUS)
#define _WIN32_WINNT 0x0500
#include <win2k.h>
#endif
#include <newpluginapi.h>
#include "help.h"

#include <richedit.h>

struct EditStreamData {
	PBYTE pbBuff;
	int cbBuff;
	int iCurrent;
};

#ifndef EDITOR
struct HyperlinkData {
	CHARRANGE range;
	char *szLink;
} static *hyperlink=NULL;
static int hyperlinkCount=0;
#endif

static DWORD CALLBACK EditStreamInRtf(DWORD dwCookie,LPBYTE pbBuff,LONG cb,LONG *pcb)
{
	struct EditStreamData *esd=(struct EditStreamData*)dwCookie;
	*pcb=min(esd->cbBuff-esd->iCurrent,cb);
	CopyMemory(pbBuff,esd->pbBuff,*pcb);
	esd->iCurrent+=*pcb;
	return 0;
}

#ifdef EDITOR
static DWORD CALLBACK EditStreamOutRtf(DWORD dwCookie,LPBYTE pbBuff,LONG cb,LONG *pcb)
{
	struct EditStreamData *esd=(struct EditStreamData*)dwCookie;
	esd->cbBuff+=cb;
	esd->pbBuff=(PBYTE)realloc(esd->pbBuff,esd->cbBuff+1);
	CopyMemory(esd->pbBuff+esd->iCurrent,pbBuff,cb);
	esd->iCurrent+=cb;
	esd->pbBuff[esd->iCurrent]='\0';
	*pcb=cb;
	return 0;
}
#endif

struct {
	const char *szSym;
	char ch;
} static const htmlSymbolChars[]={
	{"lt",'<'},
	{"gt",'>'},
	{"amp",'&'},
	{"quot",'\"'},
	{"nbsp",' '},
};

//a quick test to see who's read their comp.lang.c FAQ recently:
#define stringize2(n)  #n
#define stringize(n) stringize2(n)
struct {
	const char *szHtml;
	const char *szRtf;
} static const simpleHtmlRtfConversions[]={
	{"i","i"},
	{"/i","i0"},
	{"b","b"},
	{"/b","b0"},
	{"u","ul"},
	{"/u","ul0"},
	{"p","par\\par"},
	{"br","par"},
	{"big","fs" stringize(TEXTSIZE_BIG)},
	{"/big","fs" stringize(TEXTSIZE_NORMAL)},
	{"small","fs" stringize(TEXTSIZE_SMALL)},
	{"/small","fs" stringize(TEXTSIZE_NORMAL)},
	{"/font","cf0"}
};

//free() the return value
char *GetHtmlTagAttribute(const char *pszTag,const char *pszAttr)
{
	int iAttrName,iAttrNameEnd,iAttrEquals,iAttrValue,iAttrValueEnd,iAttrEnd;
	int attrLen=lstrlenA(pszAttr);

	for(iAttrName=0;!isspace(pszTag[iAttrName]) && pszTag[iAttrName]!='>';iAttrName++);
	for(;;) {
		for(;isspace(pszTag[iAttrName]);iAttrName++);
		if(pszTag[iAttrName]=='>' || pszTag[iAttrName]=='\0') break;
		for(iAttrNameEnd=iAttrName;isalnum(pszTag[iAttrNameEnd]);iAttrNameEnd++);
		for(iAttrEquals=iAttrNameEnd;isspace(pszTag[iAttrEquals]);iAttrEquals++);
		if(pszTag[iAttrEquals]!='=') {iAttrName=iAttrEquals; continue;}
		for(iAttrValue=iAttrEquals+1;isspace(pszTag[iAttrValue]);iAttrValue++);
		if(pszTag[iAttrValue]=='>' || pszTag[iAttrValue]=='\0') break;
		if(pszTag[iAttrValue]=='"' || pszTag[iAttrValue]=='\'') {
			for(iAttrValueEnd=iAttrValue+1;pszTag[iAttrValueEnd] && pszTag[iAttrValueEnd]!=pszTag[iAttrValue];iAttrValueEnd++);
			iAttrValue++;
			iAttrEnd=iAttrValueEnd+1;
		}
		else {
			for(iAttrValueEnd=iAttrValue;pszTag[iAttrValueEnd] && pszTag[iAttrValueEnd]!='>' && !isspace(pszTag[iAttrValueEnd]);iAttrValueEnd++);
			iAttrEnd=iAttrValueEnd;
		}
		if(pszTag[iAttrValueEnd]=='\0') break;
		if(attrLen==iAttrNameEnd-iAttrName && !_strnicmp(pszAttr,pszTag+iAttrName,attrLen)) {
			char *szValue;
			szValue=(char*)malloc(iAttrValueEnd-iAttrValue+1);
			CopyMemory(szValue,pszTag+iAttrValue,iAttrValueEnd-iAttrValue);
			szValue[iAttrValueEnd-iAttrValue]='\0';
			return szValue;
		}
		iAttrName=iAttrEnd;
	}
	return NULL;
}

void StreamInHtml(HWND hwndEdit,const char *szHtml)
{
	EDITSTREAM stream={0};
	struct EditStreamData esd={0};
	struct ResizableCharBuffer header={0},body={0};
	COLORREF *colourTbl=NULL;
	int colourTblCount=0;
	const char *pszHtml;
	char *szThisTagHref=NULL;
	int charCount=0;

#ifndef EDITOR
	FreeHyperlinkData();
#endif

	AppendToCharBuffer(&header,"{\\rtf1\\ansi\\deff0{\\fonttbl{\\f0 Tahoma;}}");
	for(pszHtml=szHtml;*pszHtml;) {
		if(*pszHtml=='<') {
			const char *pszTagEnd;
			int iNameEnd,i;
			char szTagName[16];
			
			pszTagEnd=strchr(pszHtml+1,'>');
			if(pszTagEnd==NULL) break;
			for(iNameEnd=1;pszHtml[iNameEnd] && pszHtml[iNameEnd]!='>' && !isspace(pszHtml[iNameEnd]);iNameEnd++);
			lstrcpynA(szTagName,pszHtml+1,min(sizeof(szTagName),iNameEnd));
			if(!lstrcmpiA(szTagName,"br")) charCount++;
			else if(!lstrcmpiA(szTagName,"p")) charCount+=2;
			for(i=0;i<sizeof(simpleHtmlRtfConversions)/sizeof(simpleHtmlRtfConversions[0]);i++) {
				if(!lstrcmpiA(szTagName,simpleHtmlRtfConversions[i].szHtml)) {
					AppendToCharBuffer(&body,"\\%s ",simpleHtmlRtfConversions[i].szRtf);
					break;
				}
			}
			if(i==sizeof(simpleHtmlRtfConversions)/sizeof(simpleHtmlRtfConversions[0])) {
				if(!lstrcmpiA(szTagName,"a")) {
					if(szThisTagHref) free(szThisTagHref);
					szThisTagHref=GetHtmlTagAttribute(pszHtml,"href");
#ifdef EDITOR
					AppendToCharBuffer(&body,"\\strike ");
#else
					hyperlinkCount++;
					hyperlink=(struct HyperlinkData*)realloc(hyperlink,sizeof(struct HyperlinkData)*hyperlinkCount);
					hyperlink[hyperlinkCount-1].range.cpMin=charCount;
					hyperlink[hyperlinkCount-1].range.cpMax=-1;
					hyperlink[hyperlinkCount-1].szLink=NULL;
#endif
				}
				else if(!lstrcmpiA(szTagName,"/a")) {
					if(szThisTagHref) {
#ifdef EDITOR
						AppendToCharBuffer(&body,":%s\\strike0 ",szThisTagHref);
						free(szThisTagHref);
#else
						hyperlink[hyperlinkCount-1].range.cpMax=charCount;
						hyperlink[hyperlinkCount-1].szLink=szThisTagHref;
#endif
						szThisTagHref=NULL;
					}
				}
				else if(!lstrcmpiA(szTagName,"font")) {
					char *szColour=GetHtmlTagAttribute(pszHtml,"color");
					if(szColour && szColour[0]=='#' && lstrlenA(szColour)==7) {	 //can't cope with colour names
						COLORREF colour;
						int i;
						char szRed[3],szGreen[3],szBlue[3];
						szRed[0]=szColour[1]; szRed[1]=szColour[2]; szRed[2]='\0';
						szGreen[0]=szColour[3]; szGreen[1]=szColour[4]; szGreen[2]='\0';
						szBlue[0]=szColour[5]; szBlue[1]=szColour[6]; szBlue[2]='\0';
						colour=RGB(strtol(szRed,NULL,16),strtol(szGreen,NULL,16),strtol(szBlue,NULL,16));
						free(szColour);
						for(i=0;i<colourTblCount;i++)
							if(colourTbl[i]==colour) break;
						if(i==colourTblCount) {
							colourTbl=(COLORREF*)realloc(colourTbl,++colourTblCount*sizeof(COLORREF));
							colourTbl[i]=colour;
						}
						AppendToCharBuffer(&body,"\\cf%d ",i+2);
					}
				}
			}
			pszHtml=pszTagEnd+1;
		}
		else if(*pszHtml=='&') {
			const char *pszTagEnd;
			char szTag[16];
			int i;

			pszTagEnd=strchr(pszHtml+1,';');
			if(pszTagEnd==NULL) break;
			lstrcpynA(szTag,pszHtml+1,min(sizeof(szTag),pszTagEnd-pszHtml));
			if(szTag[0]=='#') {
				int ch;
				if(szTag[1]=='x' || szTag[1]=='X') ch=strtol(szTag+2,NULL,0x10);
				else ch=strtol(szTag+1,NULL,10);
				if(ch>=0x100) AppendToCharBuffer(&body,"\\u%d ",ch);
				else AppendToCharBuffer(&body,"\\'%02x",ch);
			}
			else {
				for(i=0;i<sizeof(htmlSymbolChars)/sizeof(htmlSymbolChars[0]);i++) {
					if(!lstrcmpiA(szTag,htmlSymbolChars[i].szSym)) {
						AppendCharToCharBuffer(&body,htmlSymbolChars[i].ch);
						charCount++;
						break;
					}
				}
			}
			pszHtml=pszTagEnd+1;
		}
		else {
			if((BYTE)*pszHtml>=' ') charCount++;
			if(*pszHtml=='\\' || *pszHtml=='{' || *pszHtml=='}')
				AppendCharToCharBuffer(&body,'\\');
			AppendCharToCharBuffer(&body,*pszHtml++);
		}
	}
	if(szThisTagHref) free(szThisTagHref);

	AppendToCharBuffer(&header,"{\\colortbl ;\\red0\\green0\\blue255;");
	{	int i;
		for(i=0;i<colourTblCount;i++)
			AppendToCharBuffer(&header,"\\red%d\\green%d\\blue%d;",GetRValue(colourTbl[i]),GetGValue(colourTbl[i]),GetBValue(colourTbl[i]));
	}
	AppendToCharBuffer(&header,"}\\pard\\fs16\\uc0 ");
	if(colourTbl) free(colourTbl);

	AppendToCharBuffer(&header,"%s}",body.sz);
	esd.pbBuff=(PBYTE)header.sz;
	esd.cbBuff=header.iEnd;
	stream.dwCookie=(DWORD)&esd;
	stream.pfnCallback=EditStreamInRtf;
	SendMessage(hwndEdit,EM_STREAMIN,SF_RTF,(LPARAM)&stream);
	free(header.sz);
	free(body.sz);

#ifndef EDITOR
	{	int i;
		CHARFORMAT cf={0};
		cf.cbSize=sizeof(cf);
		cf.dwMask=CFM_LINK|CFM_UNDERLINE|CFM_COLOR;
		cf.dwEffects=CFE_LINK|CFE_UNDERLINE;
		cf.crTextColor=GetSysColor(COLOR_HOTLIGHT);
		for(i=0;i<hyperlinkCount;i++) {
			SendMessage(hwndEdit,EM_EXSETSEL,0,(LPARAM)&hyperlink[i].range);
			SendMessage(hwndEdit,EM_SETCHARFORMAT,SCF_SELECTION,(LPARAM)&cf);
		}
		SendMessage(hwndEdit,EM_SETSEL,0,0);
	}
#endif
}

#ifndef EDITOR
void FreeHyperlinkData(void)
{
	int i;
	for(i=0;i<hyperlinkCount;i++)
		if(hyperlink[i].szLink) free(hyperlink[i].szLink);
	if(hyperlink) free(hyperlink);
	hyperlink=NULL;
	hyperlinkCount=0;
}

int IsHyperlink(int cpMin,int cpMax,char **ppszLink)
{
	int i;
	
	if(ppszLink) *ppszLink=NULL;
	for(i=0;i<hyperlinkCount;i++)
		if(cpMin>=hyperlink[i].range.cpMin && cpMax<=hyperlink[i].range.cpMax) {
			if(ppszLink) *ppszLink=hyperlink[i].szLink;
			return 1;
		}
	return 0;
}
#endif  //!defined EDITOR

#ifdef EDITOR
struct RtfGroupStackData {
	BYTE bold,italic,underline,strikeout;
	BYTE isDestination,isColourTbl,isFontTbl;
	int colour;
	int fontSize;
	int unicodeSkip;
	int charset;
};

char *StreamOutHtml(HWND hwndEdit)
{
	EDITSTREAM stream={0};
	struct EditStreamData esd={0};
	struct ResizableCharBuffer htmlOut={0},hyperlink={0},*output;
	COLORREF *colourTbl=NULL;
	int colourTblCount=0;
	struct RtfGroupStackData *groupStack;
	int groupLevel;
	int inFontTag=0,inAnchorTag=0,inBigTag=0,inSmallTag=0,lineBreakBefore=0;
	char *pszRtf;
	int *fontTblCharsets=NULL;
	int fontTblCount=0;
	int normalTextSize=0;

	stream.dwCookie=(DWORD)&esd;
	stream.pfnCallback=EditStreamOutRtf;
	SendMessage(hwndEdit,EM_STREAMOUT,SF_RTF|SFF_PLAINRTF,(LPARAM)&stream);

	output=&htmlOut;
	groupStack=(struct RtfGroupStackData*)malloc(sizeof(struct RtfGroupStackData));
	groupLevel=0;
	ZeroMemory(&groupStack[0],sizeof(struct RtfGroupStackData));
	groupStack[0].unicodeSkip=1;
	for(pszRtf=(char*)esd.pbBuff;*pszRtf;) {
		if(*pszRtf=='{') {
			groupLevel++;
			groupStack=(struct RtfGroupStackData*)realloc(groupStack,sizeof(struct RtfGroupStackData)*(groupLevel+1));
			groupStack[groupLevel]=groupStack[groupLevel-1];
			pszRtf++;
		}
		else if(*pszRtf=='}') {
			groupLevel--;
			if(groupStack[groupLevel].bold!=groupStack[groupLevel+1].bold)
				AppendToCharBuffer(output,groupStack[groupLevel].bold?"<b>":"</b>");
			if(groupStack[groupLevel].italic!=groupStack[groupLevel+1].italic)
				AppendToCharBuffer(output,groupStack[groupLevel].bold?"<i>":"</i>");
			if(groupStack[groupLevel].underline!=groupStack[groupLevel+1].underline)
				AppendToCharBuffer(output,groupStack[groupLevel].bold?"<u>":"</u>");
			if(groupStack[groupLevel].strikeout!=groupStack[groupLevel+1].strikeout && groupStack[groupLevel+1].strikeout)
				if(inAnchorTag) {AppendToCharBuffer(output,"</a>"); inAnchorTag=0;}
			if(groupStack[groupLevel].colour!=groupStack[groupLevel+1].colour)
				if(inFontTag) {AppendToCharBuffer(output,"</font>"); inFontTag=0;}
			if(groupStack[groupLevel].fontSize!=groupStack[groupLevel+1].fontSize) {
				if(inBigTag) {AppendToCharBuffer(output,"</big>"); inBigTag=0;}
				if(inSmallTag) {AppendToCharBuffer(output,"</small>"); inSmallTag=0;}
				if(groupStack[groupLevel].fontSize<normalTextSize)
					{AppendToCharBuffer(output,"<small>"); inSmallTag=1;}
				else if(groupStack[groupLevel].fontSize>normalTextSize)
					{AppendToCharBuffer(output,"<big>"); inBigTag=1;}
			}
			if(groupLevel==0) break;
			pszRtf++;
		}
		else if(*pszRtf=='\\' && pszRtf[1]=='*') {
			groupStack[groupLevel].isDestination=1;
			pszRtf+=2;
		}
		else if(*pszRtf=='\\' && pszRtf[1]=='\'') {
			char szHex[3]="\0\0";
			char szChar[2];
			szHex[0]=pszRtf[2];
			if(pszRtf[2]) szHex[1]=pszRtf[3];
			else pszRtf--;
			szChar[0]=(char)strtol(szHex,NULL,0x10); szChar[1]='\0';
			if(groupStack[groupLevel].charset) {
				WCHAR szwChar[2];
				CHARSETINFO csi;
				TranslateCharsetInfo((PDWORD)groupStack[groupLevel].charset,&csi,TCI_SRCCHARSET);
				MultiByteToWideChar(csi.ciACP,0,szChar,1,szwChar,2);
				AppendToCharBuffer(output,"&#%u;",(WORD)szwChar[0]);
			}
			else AppendToCharBuffer(output,"&#%u;",(BYTE)szChar[0]);
			pszRtf+=4;
		}
		else if(*pszRtf=='\\' && isalpha(pszRtf[1])) {
			char szControlWord[32];
			int iWordEnd;
			int hasParam=0;
			int param;

			for(iWordEnd=1;isalpha(pszRtf[iWordEnd]);iWordEnd++);
			lstrcpyn(szControlWord,pszRtf+1,min(sizeof(szControlWord),iWordEnd));
			if(isdigit(pszRtf[iWordEnd]) || pszRtf[iWordEnd]=='-') {
				hasParam=1;
				param=strtol(pszRtf+iWordEnd,&pszRtf,10);
			}
			else pszRtf=pszRtf+iWordEnd;
			if(*pszRtf==' ') pszRtf++;
			if(!lstrcmpi(szControlWord,"colortbl")) {
				groupStack[groupLevel].isColourTbl=1;
				colourTblCount=1;
				colourTbl=(COLORREF*)realloc(colourTbl,sizeof(COLORREF));
				colourTbl[0]=0;
				groupStack[groupLevel].isDestination=1;
			}
			else if(!lstrcmpi(szControlWord,"fonttbl")) {
				groupStack[groupLevel].isFontTbl=1;
				groupStack[groupLevel].isDestination=1;
			}
			else if(!lstrcmpi(szControlWord,"stylesheet")) {
				groupStack[groupLevel].isDestination=1;
			}
			else if(!lstrcmpi(szControlWord,"red")) {
				if(!hasParam) break;
				colourTbl[colourTblCount-1]&=~RGB(255,0,0);
				colourTbl[colourTblCount-1]|=RGB(param,0,0);
			}
			else if(!lstrcmpi(szControlWord,"green")) {
				if(!hasParam) break;
				colourTbl[colourTblCount-1]&=~RGB(0,255,0);
				colourTbl[colourTblCount-1]|=RGB(0,param,0);
			}
			else if(!lstrcmpi(szControlWord,"blue")) {
				if(!hasParam) break;
				colourTbl[colourTblCount-1]&=~RGB(0,0,255);
				colourTbl[colourTblCount-1]|=RGB(0,0,param);
			}
			else if(!lstrcmpi(szControlWord,"f")) {
				if(groupStack[groupLevel].isFontTbl) {
					fontTblCount++;
					fontTblCharsets=(int*)realloc(fontTblCharsets,sizeof(int)*fontTblCount);
					fontTblCharsets[fontTblCount-1]=0;
				}
				else {
					if(hasParam && param>=0 && param<fontTblCount)
						groupStack[groupLevel].charset=fontTblCharsets[param];
				}
			}
			else if(!lstrcmpi(szControlWord,"fcharset")) {
				if(groupStack[groupLevel].isFontTbl && fontTblCount && hasParam)
					fontTblCharsets[fontTblCount-1]=param;
			}
			else if(!lstrcmpi(szControlWord,"cf")) {
				if(inFontTag) AppendToCharBuffer(output,"</font>");
				if(hasParam && param) {
					AppendToCharBuffer(output,"<font color=\"#%02x%02x%02x\">",GetRValue(colourTbl[param]),GetGValue(colourTbl[param]),GetBValue(colourTbl[param]));
					inFontTag=1;
					groupStack[groupLevel].colour=param;
				}
				else groupStack[groupLevel].colour=0;
			}
			else if(!lstrcmpi(szControlWord,"fs")) {
				if(normalTextSize==0 && hasParam) {
					normalTextSize=param;
					groupStack[0].fontSize=normalTextSize;
				}
				if(inBigTag) {AppendToCharBuffer(output,"</big>"); inBigTag=0;}
				if(inSmallTag) {AppendToCharBuffer(output,"</small>"); inSmallTag=0;}
				if(hasParam) {
					groupStack[groupLevel].fontSize=param;
					if(groupStack[groupLevel].fontSize<normalTextSize)
						{AppendToCharBuffer(output,"<small>"); inSmallTag=1;}
					else if(groupStack[groupLevel].fontSize>normalTextSize)
						{AppendToCharBuffer(output,"<big>"); inBigTag=1;}
				}
			}
			else if(!lstrcmpi(szControlWord,"uc")) {
				if(hasParam) groupStack[groupLevel].unicodeSkip=param;
			}
			else if(!lstrcmpi(szControlWord,"u")) {
				if(hasParam) {
					AppendToCharBuffer(output,"&#%u;",param);
					pszRtf+=groupStack[groupLevel].unicodeSkip;
				}
			}
			else if(!lstrcmpi(szControlWord,"b")) {
				if(!hasParam || param) {
					groupStack[groupLevel].bold=1;
					AppendToCharBuffer(output,"<b>");
				}
				else {
					groupStack[groupLevel].bold=0;
					AppendToCharBuffer(output,"</b>");
				}
			}
			else if(!lstrcmpi(szControlWord,"i")) {
				if(!hasParam || param) {
					groupStack[groupLevel].italic=1;
					AppendToCharBuffer(output,"<i>");
				}
				else {
					groupStack[groupLevel].italic=0;
					AppendToCharBuffer(output,"</i>");
				}
			}
			else if(!lstrcmpi(szControlWord,"ul")) {
				if(!hasParam || param) {
					groupStack[groupLevel].underline=1;
					AppendToCharBuffer(output,"<u>");
				}
				else {
					groupStack[groupLevel].underline=0;
					AppendToCharBuffer(output,"</u>");
				}
			}
			else if(!lstrcmpi(szControlWord,"ulnone")) {
				groupStack[groupLevel].underline=0;
				AppendToCharBuffer(output,"</u>");
			}
			else if(!lstrcmpi(szControlWord,"strike")) {
				if(!hasParam || param) {
					groupStack[groupLevel].strikeout=1;
					if(hyperlink.sz) free(hyperlink.sz);
					hyperlink.iEnd=hyperlink.cbAlloced=0;
					hyperlink.sz=NULL;
					output=&hyperlink;
				}
				else {
					groupStack[groupLevel].strikeout=0;
					if(hyperlink.iEnd) {
						char *pszColon;
						output=&htmlOut;
						pszColon=strchr(hyperlink.sz,':');
						if(pszColon==NULL) pszColon="";
						else *pszColon++='\0';
						AppendToCharBuffer(output,"<a href=\"%s\">%s</a>",pszColon,hyperlink.sz);
						free(hyperlink.sz);
						hyperlink.iEnd=hyperlink.cbAlloced=0;
						hyperlink.sz=NULL;
					}
				}
			}
			else if(!lstrcmpi(szControlWord,"par")) {
				if(lineBreakBefore) AppendToCharBuffer(output,"<br>");
				lineBreakBefore=1;   //RTF puts a \par right at the end. Annoying.
			}
		}
		else {
			int i;
			if(*pszRtf=='\\') pszRtf++;
			if(!groupStack[groupLevel].isDestination) {
				if(lineBreakBefore && (BYTE)*pszRtf>=' ') {AppendToCharBuffer(output,"<br>"); lineBreakBefore=0;}
				if(*pszRtf==' ')
					AppendCharToCharBuffer(output,*pszRtf);
				else {
					for(i=0;i<sizeof(htmlSymbolChars)/sizeof(htmlSymbolChars[0]);i++) {
						if(*pszRtf==htmlSymbolChars[i].ch) {
							AppendToCharBuffer(output,"&%s;",htmlSymbolChars[i].szSym);
							break;
						}
					}
					if(i==sizeof(htmlSymbolChars)/sizeof(htmlSymbolChars[0]))
						AppendCharToCharBuffer(output,*pszRtf);
				}
			}
			else if(groupStack[groupLevel].isColourTbl && *pszRtf==';') {
				colourTblCount++;
				colourTbl=(COLORREF*)realloc(colourTbl,sizeof(COLORREF)*(colourTblCount+1));
				colourTbl[colourTblCount-1]=0;
			}
			pszRtf++;
		}
	}
	free(groupStack);
	if(colourTbl) free(colourTbl);
	if(fontTblCharsets) free(fontTblCharsets);
	if(hyperlink.sz) free(hyperlink.sz);

	free(esd.pbBuff);
	return htmlOut.sz;
}
#endif  //defined EDITOR