#include "commonheaders.h"
#include "modern_statusbar.h"
#include "./m_api/m_skin_eng.h"
#include "commonprototypes.h"

HANDLE  hStatusBarShowToolTipEvent;
HANDLE  hStatusBarHideToolTipEvent;
BOOL tooltipshoing;
POINT lastpnt;

#define TM_STATUSBAR 23435234
#define TM_STATUSBARHIDE 23435235

HWND hModernStatusBar=NULL;
HANDLE hFramehModernStatusBar=NULL;

int callProxied_FindFrameID(HWND FrameHwnd);

#define DBFONTF_BOLD       1
#define DBFONTF_ITALIC     2
#define DBFONTF_UNDERLINE  4

typedef struct
{
    HICON icon;
    HICON extraIcon;
    int iconIndex;
    char * ProtoName;
    int ProtoStatus;
    char *ProtoHumanName;
    char * ProtoStatusText;
    TCHAR * ProtoXStatus;
    int ProtoPos;
    int fullWidth;
    RECT protoRect;
    BOOL DoubleIcons;
}
ProtoItemData;

ProtoItemData *ProtosData=NULL;
int allocedItemData=0;
STATUSBARDATA g_StatusBarData={0};


char * ApendSubSetting(char * buf, int size, char *first, char *second)
{
    _snprintf(buf,size,"%sFont%s",first,second);
    return buf;
}
COLORREF sttGetColor(char * module, char * color, COLORREF defColor);

int LoadStatusBarData()
{
    g_StatusBarData.showStatusName=DBGetContactSettingByte(NULL,"CLUI","SBarShow",SETTING_SBARSHOW_DEFAULT)&4;
    g_StatusBarData.showProtoName=DBGetContactSettingByte(NULL,"CLUI","SBarShow",SETTING_SBARSHOW_DEFAULT)&2;
    g_StatusBarData.rectBorders.left=DBGetContactSettingDword(NULL,"CLUI","LeftOffset",SETTING_LEFTOFFSET_DEFAULT);
    g_StatusBarData.rectBorders.right=DBGetContactSettingDword(NULL,"CLUI","RightOffset",SETTING_RIGHTOFFSET_DEFAULT);
    g_StatusBarData.extraspace=(BYTE)DBGetContactSettingDword(NULL,"CLUI","SpaceBetween",SETTING_SPACEBETWEEN_DEFAULT);
    g_StatusBarData.xStatusMode=(BYTE)(DBGetContactSettingByte(NULL,"CLUI","ShowXStatus",SETTING_SHOWXSTATUS_DEFAULT));
    g_StatusBarData.Align=DBGetContactSettingByte(NULL,"CLUI","Align",SETTING_ALIGN_DEFAULT);
    g_StatusBarData.sameWidth=DBGetContactSettingByte(NULL,"CLUI","EqualSections",SETTING_EQUALSECTIONS_DEFAULT);
    g_StatusBarData.connectingIcon=DBGetContactSettingByte(NULL,"CLUI","UseConnectingIcon",SETTING_USECONNECTINGICON_DEFAULT);
    g_StatusBarData.nProtosPerLine=DBGetContactSettingByte(NULL,"Protocols","ProtosPerLine",SETTING_PROTOSPERLINE_DEFAULT);
    g_StatusBarData.showProtoEmails=DBGetContactSettingByte(NULL,"CLUI","ShowUnreadEmails",SETTING_SHOWUNREADEMAILS_DEFAULT);
    if (g_StatusBarData.BarFont) DeleteObject(g_StatusBarData.BarFont);
    g_StatusBarData.BarFont=NULL;//LoadFontFromDB("ModernData","StatusBar",&g_StatusBarData.fontColor);
    {
        int vis=DBGetContactSettingByte(NULL,"CLUI","ShowSBar",SETTING_SHOWSBAR_DEFAULT);
        int frameopt;
        int frameID=callProxied_FindFrameID(hModernStatusBar);
        frameopt=CallService(MS_CLIST_FRAMES_GETFRAMEOPTIONS,MAKEWPARAM(FO_FLAGS,frameID),0);
        frameopt=frameopt & (~F_VISIBLE);
        if(vis)
        {
            ShowWindow(hModernStatusBar,SW_SHOW);
            frameopt|=F_VISIBLE;
        }
        else
        {
            ShowWindow(hModernStatusBar,SW_HIDE);
        };
        CallService(MS_CLIST_FRAMES_SETFRAMEOPTIONS,MAKEWPARAM(FO_FLAGS,frameID),frameopt);
    }
    g_StatusBarData.TextEffectID=DBGetContactSettingByte(NULL,"StatusBar","TextEffectID",SETTING_TEXTEFFECTID_DEFAULT);
    g_StatusBarData.TextEffectColor1=DBGetContactSettingDword(NULL,"StatusBar","TextEffectColor1",SETTING_TEXTEFFECTCOLOR1_DEFAULT);
    g_StatusBarData.TextEffectColor2=DBGetContactSettingDword(NULL,"StatusBar","TextEffectColor2",SETTING_TEXTEFFECTCOLOR2_DEFAULT);

    if(g_StatusBarData.hBmpBackground) {DeleteObject(g_StatusBarData.hBmpBackground); g_StatusBarData.hBmpBackground=NULL;}

    if (g_CluiData.fDisableSkinEngine)
    {
        DBVARIANT dbv;
        g_StatusBarData.bkColour=sttGetColor("StatusBar","BkColour",CLCDEFAULT_BKCOLOUR);
        if(DBGetContactSettingByte(NULL,"StatusBar","UseBitmap",CLCDEFAULT_USEBITMAP)) {
            if(!DBGetContactSettingString(NULL,"StatusBar","BkBitmap",&dbv)) {
                g_StatusBarData.hBmpBackground=(HBITMAP)CallService(MS_UTILS_LOADBITMAP,0,(LPARAM)dbv.pszVal);
                DBFreeVariant(&dbv);
            }
        }
        g_StatusBarData.bkUseWinColors=DBGetContactSettingByte(NULL,"StatusBar", "UseWinColours", CLCDEFAULT_USEWINDOWSCOLOURS);
        g_StatusBarData.backgroundBmpUse=DBGetContactSettingWord(NULL,"StatusBar","BkBmpUse",CLCDEFAULT_BKBMPUSE);
    }
    SendMessage(pcli->hwndContactList,WM_SIZE,0,0);
    return 1;
}

int BgStatusBarChange(WPARAM wParam,LPARAM lParam)
{
    if (MirandaExiting()) return 0;
    LoadStatusBarData();
    return 0;
}

//ProtocolData;
int NewStatusPaintCallbackProc(HWND hWnd, HDC hDC, RECT * rcPaint, HRGN rgn, DWORD dFlags, void * CallBackData)
{
    return ModernDrawStatusBar(hWnd,hDC);
}

int ModernDrawStatusBar(HWND hwnd, HDC hDC)
{
    if (hwnd==(HWND)-1) return 0;
    if (GetParent(hwnd)==pcli->hwndContactList)
        return ModernDrawStatusBarWorker(hwnd,hDC);
    else
        CLUI__cliInvalidateRect(hwnd,NULL,FALSE);
    return 0;
}
int ModernDrawStatusBarWorker(HWND hWnd, HDC hDC)
{
    int line;
    int iconHeight=GetSystemMetrics(SM_CYSMICON)+2;
    int protosperline=0;
    int visProtoCount=0;
    int protoCount;
    int SumWidth=0;
    int rectwidth=0;
    int aligndx=0;
    int * ProtoWidth=NULL;
    int i,j,po=0;
    int protcnt=0;

    char servName[40];
    char protoNameExt[40];
    // Count visible protos
    PROTOCOLDESCRIPTOR **proto;
    RECT rc;
    HFONT hOldFont;
    int iconY, textY;
    int spaceWidth;
    int maxwidth=0;
    int xstatus=0;
    SIZE textSize={0};
    GetClientRect(hWnd,&rc);
    if (g_CluiData.fDisableSkinEngine)
    {
        if (g_StatusBarData.bkUseWinColors && xpt_IsThemed(g_StatusBarData.hTheme))
        {
            xpt_DrawTheme(g_StatusBarData.hTheme, hWnd, hDC, 0, 0, &rc, &rc);           
        }
        else
            DrawBackGround(hWnd, hDC,  g_StatusBarData.hBmpBackground, g_StatusBarData.bkColour, g_StatusBarData.backgroundBmpUse );
    }
    else
    {
        SkinDrawGlyph(hDC,&rc,&rc,"Main,ID=StatusBar"); //TBD
    }
    g_StatusBarData.nProtosPerLine=DBGetContactSettingByte(NULL,"Protocols","ProtosPerLine",SETTING_PROTOSPERLINE_DEFAULT);
    hOldFont=CLCPaint_ChangeToFont(hDC,NULL,FONTID_STATUSBAR_PROTONAME,NULL);
    //  hOldFont=SelectObject(hDC,g_StatusBarData.BarFont);
    //  SetTextColor(hDC,g_StatusBarData.fontColor);
    {

        GetTextExtentPoint32A(hDC," ",1,&textSize);
        spaceWidth=textSize.cx;
        textY=rc.top+((rc.bottom-rc.top-textSize.cy)>>1);
    }
    iconY=rc.top+((rc.bottom-rc.top-GetSystemMetrics(SM_CXSMICON))>>1);

    {
        CallService(MS_PROTO_ENUMPROTOCOLS,(WPARAM)&protoCount,(LPARAM)&proto);
        if (allocedItemData && ProtosData)
        {
            int k;

            for (k=0; k<allocedItemData; k++)
            {
                if(ProtosData[k].ProtoXStatus) mir_free_and_nill (ProtosData[k].ProtoXStatus);
                if(ProtosData[k].ProtoName) mir_free_and_nill (ProtosData[k].ProtoName);
                if(ProtosData[k].ProtoHumanName) mir_free_and_nill (ProtosData[k].ProtoHumanName);
                if(ProtosData[k].ProtoStatusText) mir_free_and_nill (ProtosData[k].ProtoStatusText);      
            }
            mir_free_and_nill(ProtosData);
            ProtosData=NULL;
            allocedItemData=0;
        }
        ProtosData=mir_alloc(sizeof(ProtoItemData)*protoCount);
        memset(ProtosData,0,sizeof(ProtoItemData)*protoCount);
        protcnt=(int)DBGetContactSettingDword(0,"Protocols","ProtoCount",-1);
        for (j=0; j<protcnt; j++)
        {
            char buf[40];
            int vis;
            i=pcli->pfnGetProtoIndexByPos(proto,protoCount,j);
            if (i==-1) 
                vis=FALSE;
            else
                vis=pcli->pfnGetProtocolVisibility(proto[i]->szName);
            if (!vis) continue;

            // create service name
            mir_snprintf(servName, SIZEOF(servName), "%s/GetUnreadEmailCount", proto[i]->szName);   
            if(!CallProtoService(proto[i]->szName,PS_GETNAME,(WPARAM)SIZEOF(buf),(LPARAM)buf))
            {
                if((g_StatusBarData.showProtoEmails == 1) && ServiceExists(servName))
                {
                    mir_snprintf(protoNameExt, SIZEOF(protoNameExt),"%s [%d]",proto[i]->szName, (int) CallService(servName, 0, 0));
                    ProtosData[visProtoCount].ProtoHumanName=mir_strdup(protoNameExt);
                }
                else
                    ProtosData[visProtoCount].ProtoHumanName=mir_strdup(buf);
            }
            else
            {
                if((g_StatusBarData.showProtoEmails == 1) && ServiceExists(servName))
                {
                    mir_snprintf(protoNameExt, SIZEOF(protoNameExt),"%s [%d]",proto[i]->szName, (int) CallService(servName, 0, 0));
                    ProtosData[visProtoCount].ProtoHumanName=mir_strdup(protoNameExt);
                }
                else
                    ProtosData[visProtoCount].ProtoHumanName=mir_strdup(proto[i]->szName);
            }
            ProtosData[visProtoCount].ProtoName=mir_strdup(proto[i]->szName);
            ProtosData[visProtoCount].ProtoStatus=CallProtoService(proto[i]->szName,PS_GETSTATUS,0,0);
            ProtosData[visProtoCount].ProtoStatusText=mir_strdup((char*)CallService(MS_CLIST_GETSTATUSMODEDESCRIPTION,(WPARAM)ProtosData[visProtoCount].ProtoStatus,0));
            ProtosData[visProtoCount].ProtoPos=visProtoCount;
            visProtoCount++;
            allocedItemData++;
        }

        // Calculate rects

        //START MULTILINE HERE 
        {
            int orig_protoCount=protoCount;
            int orig_visProtoCount=visProtoCount;
            ProtoItemData * orig_ProtosData=ProtosData;
            int linecount;
            if (g_StatusBarData.nProtosPerLine)
                protosperline=g_StatusBarData.nProtosPerLine;
            else if (orig_visProtoCount)
                protosperline=orig_visProtoCount;
            else if (protoCount)
            {
                protosperline=protoCount;
                orig_visProtoCount=protoCount;
            }
            else 
            {
                protosperline=1;
                orig_visProtoCount=1;
            }
            protosperline=min(protosperline,orig_visProtoCount);
            linecount=(orig_visProtoCount+(protosperline-1))/protosperline; //divide with rounding to up
            for (line=0; line<linecount; line++)
            {    
                int height;
                int rowsdy;
                int rowheight=max(textSize.cy+2,iconHeight);
                protoCount=min(protosperline,(orig_protoCount-line*protosperline));
                visProtoCount=min(protosperline,(orig_visProtoCount-line*protosperline));
                ProtosData+=line*protosperline; 
                GetClientRect(hWnd,&rc);
                aligndx=0;
                maxwidth=0;
                xstatus=0;
                SumWidth=0;
                height=(rowheight*linecount);

                rowsdy=((rc.bottom-rc.top)-height)/2;
                if (rowheight*(line)+rowsdy<rc.top-rowheight) continue;
                if (rowheight*(line+1)+rowsdy>rc.bottom+rowheight) break;

                rc.top=rowsdy+rowheight*line+1;
                rc.bottom=rowsdy+rowheight*(line+1);

                textY=rc.top+(((rc.bottom-rc.top)-textSize.cy)/2);
                iconY=rc.top+(((rc.bottom-rc.top)-iconHeight)/2);

                { //Code for each line
                    {
                        DWORD sw;
                        rectwidth=rc.right-rc.left-g_StatusBarData.rectBorders.left-g_StatusBarData.rectBorders.right;
                        if (visProtoCount>1) sw=(rectwidth-(g_StatusBarData.extraspace*(visProtoCount-1)))/visProtoCount;
                        else sw=rectwidth;
                        if (ProtoWidth) mir_free_and_nill(ProtoWidth);
                        ProtoWidth=mir_alloc(sizeof(DWORD)*visProtoCount);
                        for (i=0; i<visProtoCount; i++)
                        {
                            SIZE textSize;
                            DWORD w=GetSystemMetrics(SM_CXSMICON)+1;
                            ProtosData[i].extraIcon=NULL;
                            if ((g_StatusBarData.xStatusMode&8) && ProtosData[i].ProtoStatus>ID_STATUS_OFFLINE) 
                            {
                                char str[MAXMODULELABELLENGTH];
                                strcpy(str,ProtosData[i].ProtoName);
                                strcat(str,"/GetXStatus");
                                if (ServiceExists(str))
                                {
                                    char * dbTitle="XStatusName";
                                    char * dbTitle2=NULL;
                                    xstatus=CallProtoService(ProtosData[i].ProtoName,"/GetXStatus",(WPARAM)&dbTitle,(LPARAM)&dbTitle2);
                                    if (dbTitle && xstatus)
                                    {
                                        DBVARIANT dbv={0};
                                        if (!DBGetContactSettingTString(NULL,ProtosData[i].ProtoName,dbTitle,&dbv))
                                        {
                                            ProtosData[i].ProtoXStatus=mir_tstrdup(dbv.ptszVal);
                                            //mir_free_and_nill(dbv.ptszVal);
                                            DBFreeVariant(&dbv);
                                        }
                                    }
                                }
                            }
                            if ((g_StatusBarData.xStatusMode&3))
                            {
                                if (ProtosData[i].ProtoStatus>ID_STATUS_OFFLINE)
                                {
                                    char str[MAXMODULELABELLENGTH];
                                    strcpy(str,ProtosData[i].ProtoName);
                                    strcat(str,"/GetXStatusIcon");
                                    if (ServiceExists(str))
                                        ProtosData[i].extraIcon=(HICON)CallService(str,0,0);
                                    if (ProtosData[i].extraIcon && (g_StatusBarData.xStatusMode&3)==3)
                                        w+=GetSystemMetrics(SM_CXSMICON)+1;

                                }
                            }
                            if (g_StatusBarData.showProtoName)
                            {
                                GetTextExtentPoint32A(hDC,ProtosData[i].ProtoHumanName,lstrlenA(ProtosData[i].ProtoHumanName),&textSize);
                                w+=textSize.cx+3+spaceWidth;
                            }
                            if (g_StatusBarData.showStatusName)
                            {
                                GetTextExtentPoint32A(hDC,ProtosData[i].ProtoStatusText,lstrlenA(ProtosData[i].ProtoStatusText),&textSize);
                                w+=textSize.cx+spaceWidth+3;

                            }
                            if ((g_StatusBarData.xStatusMode&8) && ProtosData[i].ProtoXStatus)
                            {
                                GetTextExtentPoint32(hDC,ProtosData[i].ProtoXStatus,lstrlen(ProtosData[i].ProtoXStatus),&textSize);
                                w+=textSize.cx+spaceWidth+3;
                            }
                            ProtosData[i].fullWidth=w;
                            if (g_StatusBarData.sameWidth)
                            {
                                ProtoWidth[i]=sw;
                                SumWidth+=w;
                            }
                            else
                            {
                                ProtoWidth[i]=w;
                                SumWidth+=w;
                            }
                        }

                        // Reposition rects
                        for(i=0; i<visProtoCount; i++)
                            if (ProtoWidth[i]>maxwidth) maxwidth=ProtoWidth[i];

                        if (g_StatusBarData.sameWidth)
                        {
                            for (i=0; i<visProtoCount; i++)
                                ProtoWidth[i]=maxwidth;
                            SumWidth=maxwidth*visProtoCount;
                        }
                        SumWidth+=(visProtoCount-1)*(g_StatusBarData.extraspace+1);

                        if (SumWidth>rectwidth)
                        {
                            float f=(float)rectwidth/SumWidth;
                            //dx=(int)(0.5+(float)dx/visProtoCount);
                            //SumWidth-=dx*visProtoCount;
                            SumWidth=0;
                            for (i=0; i<visProtoCount; i++)
                            {
                                ProtoWidth[i]=(int)((float)ProtoWidth[i]*f);
                                SumWidth+=ProtoWidth[i];
                            }
                            SumWidth+=(visProtoCount-1)*(g_StatusBarData.extraspace+1);
                        }
                    }
                    if (g_StatusBarData.Align==1) //center
                        aligndx=(rectwidth-SumWidth)>>1;
                    else if (g_StatusBarData.Align==2) //right
                        aligndx=(rectwidth-SumWidth);
                    // Draw in rects
                    //ske_SelectTextEffect(hDC,g_StatusBarData.TextEffectID,g_StatusBarData.TextEffectColor1,g_StatusBarData.TextEffectColor2);
                    {
                        RECT r=rc;
                        r.top+=g_StatusBarData.rectBorders.top;
                        r.bottom-=g_StatusBarData.rectBorders.bottom;
                        r.left+=g_StatusBarData.rectBorders.left+aligndx;
                        for (i=0; i< visProtoCount; i++)
                        {

                            HRGN rgn;
                            int x=r.left;
                            HICON hIcon=NULL;
                            HICON hxIcon=NULL;
                            BOOL NeedDestroy=FALSE;
                            if (ProtosData[i].ProtoStatus>ID_STATUS_OFFLINE && ((g_StatusBarData.xStatusMode)&3)>0)
                            {
                                char str[MAXMODULELABELLENGTH];
                                strcpy(str,ProtosData[i].ProtoName);
                                strcat(str,"/GetXStatusIcon");
                                if (ServiceExists(str))
                                {
                                    hxIcon=ProtosData[i].extraIcon;
                                    if (hxIcon)
                                    {
                                        if (((g_StatusBarData.xStatusMode)&3)==2)
                                        {
                                            hIcon=GetMainStatusOverlay(ProtosData[i].ProtoStatus);
                                            NeedDestroy=TRUE;
                                        }
                                        else if (((g_StatusBarData.xStatusMode)&3)==1)
                                        {
                                            hIcon=hxIcon;
                                            NeedDestroy=TRUE;
                                            hxIcon=NULL;
                                        }           

                                    }
                                }
                            }
                            if (hIcon==NULL && (hxIcon==NULL || (((g_StatusBarData.xStatusMode)&3)==3)))
                            {
                                if (hIcon==NULL && (g_StatusBarData.connectingIcon==1) && ProtosData[i].ProtoStatus>=ID_STATUS_CONNECTING&&ProtosData[i].ProtoStatus<=ID_STATUS_CONNECTING+MAX_CONNECT_RETRIES)
                                {
                                    hIcon=(HICON)CLUI_GetConnectingIconService((WPARAM)ProtosData[i].ProtoName,0);
                                    if (hIcon) NeedDestroy=TRUE;
                                    else LoadSkinnedProtoIcon(ProtosData[i].ProtoName,ProtosData[i].ProtoStatus);
                                }
                                else hIcon=LoadSkinnedProtoIcon(ProtosData[i].ProtoName,ProtosData[i].ProtoStatus);
                            }

                            r.right=r.left+ProtoWidth[i];
                            rgn=CreateRectRgn(r.left,r.top,r.right,r.bottom);
                            //
                            {
                                if (g_StatusBarData.sameWidth)
                                {
                                    int fw=ProtosData[i].fullWidth;
                                    int rw=r.right-r.left;
                                    if (g_StatusBarData.Align==1)
                                    {
                                        x=r.left+((rw-fw)/2);
                                    }
                                    else if (g_StatusBarData.Align==2)
                                    {
                                        x=r.left+((rw-fw));
                                    }
                                    else x=r.left;
                                }
                            }
                            //
                            ProtosData[i].protoRect=r;
                            SelectClipRgn(hDC,rgn);
                            ProtosData[i].DoubleIcons=FALSE;
                            if ((g_StatusBarData.xStatusMode&3)==3)
                            {
                                if (hIcon) mod_DrawIconEx_helper(hDC,x,iconY,hIcon,GetSystemMetrics(SM_CXSMICON),GetSystemMetrics(SM_CYSMICON),0,NULL,DI_NORMAL);
                                if (hxIcon) 
                                {
                                    mod_DrawIconEx_helper(hDC,x+GetSystemMetrics(SM_CXSMICON)+1,iconY,hxIcon,GetSystemMetrics(SM_CXSMICON),GetSystemMetrics(SM_CYSMICON),0,NULL,DI_NORMAL);
                                    x+=GetSystemMetrics(SM_CXSMICON)+1;
                                }
                                ProtosData[i].DoubleIcons=hIcon&&hxIcon;
                            }
                            else
                            {
                                if (hxIcon) mod_DrawIconEx_helper(hDC,x,iconY,hxIcon,GetSystemMetrics(SM_CXSMICON),GetSystemMetrics(SM_CYSMICON),0,NULL,DI_NORMAL);
                                if (hIcon) mod_DrawIconEx_helper(hDC,x,iconY,hIcon,GetSystemMetrics(SM_CXSMICON),GetSystemMetrics(SM_CYSMICON),0,NULL,DI_NORMAL|((hxIcon&&(g_StatusBarData.xStatusMode&4))?(192<<24):0));
                            }
                            if (hxIcon) DestroyIcon_protect(hxIcon);
                            if (NeedDestroy) DestroyIcon_protect(hIcon);
                            x+=GetSystemMetrics(SM_CXSMICON)+1;
                            if (g_StatusBarData.showProtoName)
                            {
                                SIZE textSize;
                                RECT rt=r;
                                rt.left=x+(spaceWidth>>1);
                                rt.top=textY;
                                ske_DrawTextA(hDC,ProtosData[i].ProtoHumanName,lstrlenA(ProtosData[i].ProtoHumanName),&rt,0);
                                //TextOutS(hDC,x,textY,ProtosData[i].ProtoName,lstrlenA(ProtosData[i].ProtoName));
                                if (g_StatusBarData.showStatusName || ((g_StatusBarData.xStatusMode&8) && ProtosData[i].ProtoXStatus))
                                {
                                    GetTextExtentPoint32A(hDC,ProtosData[i].ProtoHumanName,lstrlenA(ProtosData[i].ProtoHumanName),&textSize);
                                    x+=textSize.cx+3;
                                }
                            }
                            if (g_StatusBarData.showStatusName)
                            {
                                SIZE textSize;
                                RECT rt=r;
                                rt.left=x+(spaceWidth>>1);
                                rt.top=textY;
                                ske_DrawTextA(hDC,ProtosData[i].ProtoStatusText,lstrlenA(ProtosData[i].ProtoStatusText),&rt,0);
                                if ((g_StatusBarData.xStatusMode&8) && ProtosData[i].ProtoXStatus)
                                {
                                    GetTextExtentPoint32A(hDC,ProtosData[i].ProtoStatusText,lstrlenA(ProtosData[i].ProtoStatusText),&textSize);
                                    x+=textSize.cx+3;
                                }
                                //TextOutS(hDC,x,textY,ProtosData[i].ProtoStatusText,lstrlenA(ProtosData[i].ProtoStatusText));
                            }
                            if ((g_StatusBarData.xStatusMode&8) && ProtosData[i].ProtoXStatus)
                            {
                                RECT rt=r;
                                rt.left=x+(spaceWidth>>1);
                                rt.top=textY;
                                ske_DrawText(hDC,ProtosData[i].ProtoXStatus,lstrlen(ProtosData[i].ProtoXStatus),&rt,0);
                                //TextOutS(hDC,x,textY,ProtosData[i].ProtoStatusText,lstrlenA(ProtosData[i].ProtoStatusText));
                            }

                            r.left=r.right+g_StatusBarData.extraspace;
                            //SelectClipRgn(hDC,NULL);
                            DeleteObject(rgn);

                        }
                    }

                    if (ProtoWidth) mir_free_and_nill(ProtoWidth);
                } //code for each line
                ProtosData=orig_ProtosData;
            }
        }
    }

    SelectObject(hDC,hOldFont);
    return 0;
}

#define TOOLTIP_TOLERANCE 5
LRESULT CALLBACK ModernStatusProc(HWND hwnd,UINT msg,WPARAM wParam,LPARAM lParam)
{
    static POINT ptToolTipShow={0};
    switch (msg) 
    {
    case WM_CREATE:
        g_StatusBarData.hTheme=xpt_AddThemeHandle(hwnd,L"STATUS");
        break;
    case WM_DESTROY:
        xpt_FreeThemeForWindow(hwnd);
        if (allocedItemData && ProtosData)
        {
            int k;

            for (k=0; k<allocedItemData; k++)
            {
                if(ProtosData[k].ProtoName) mir_free_and_nill (ProtosData[k].ProtoName);
                if(ProtosData[k].ProtoHumanName) mir_free_and_nill (ProtosData[k].ProtoHumanName);
                if(ProtosData[k].ProtoStatusText) mir_free_and_nill (ProtosData[k].ProtoStatusText);
                if(ProtosData[k].ProtoXStatus) mir_free_and_nill (ProtosData[k].ProtoXStatus);
            }
            mir_free_and_nill(ProtosData);
            ProtosData=NULL;
            allocedItemData=0;
        }
        break;
    case WM_SIZE:
        if (!g_CluiData.fLayered)
            InvalidateRect(hwnd,NULL,FALSE);
        return DefWindowProc(hwnd, msg, wParam, lParam);
    case WM_ERASEBKGND:
        return 1;
    case WM_PAINT:
        if (GetParent(hwnd)==pcli->hwndContactList && g_CluiData.fLayered)
            CallService(MS_SKINENG_INVALIDATEFRAMEIMAGE,(WPARAM)hwnd,0);
        else if (GetParent(hwnd)==pcli->hwndContactList && !g_CluiData.fLayered)
        {
            HDC hdc, hdc2;
            HBITMAP hbmp,hbmpo;
            RECT rc={0};
            GetClientRect(hwnd,&rc);
            rc.right++;
            rc.bottom++;
            hdc = GetDC(hwnd);
            hdc2=CreateCompatibleDC(hdc);
            hbmp=ske_CreateDIB32(rc.right,rc.bottom);
            hbmpo=SelectObject(hdc2,hbmp);  
            SetBkMode(hdc2,TRANSPARENT);
            ske_BltBackImage(hwnd,hdc2,&rc);
            ModernDrawStatusBarWorker(hwnd,hdc2);
            BitBlt(hdc,rc.left,rc.top,rc.right-rc.left,rc.bottom-rc.top,
                hdc2,rc.left,rc.top,SRCCOPY);
            SelectObject(hdc2,hbmpo);
            DeleteObject(hbmp);
            mod_DeleteDC(hdc2);
            {
                HFONT hf=GetStockObject(DEFAULT_GUI_FONT);
                SelectObject(hdc,hf);
            }
            ReleaseDC(hwnd,hdc);
            ValidateRect(hwnd,NULL);
        }
        else
        {
            HDC hdc, hdc2;
            HBITMAP hbmp, hbmpo;
            RECT rc;
            PAINTSTRUCT ps;
            HBRUSH br=GetSysColorBrush(COLOR_3DFACE);
            GetClientRect(hwnd,&rc);
            hdc=BeginPaint(hwnd,&ps);
            hdc2=CreateCompatibleDC(hdc);
            hbmp=ske_CreateDIB32(rc.right,rc.bottom);
            hbmpo=SelectObject(hdc2,hbmp);
            FillRect(hdc2,&ps.rcPaint,br);
            ModernDrawStatusBarWorker(hwnd,hdc2);
            //BitBlt(hdc,ps.rcPaint.left,ps.rcPaint.top,ps.rcPaint.right-ps.rcPaint.left,ps.rcPaint.bottom-ps.rcPaint.top,
            //  hdc2,ps.rcPaint.left,ps.rcPaint.top,SRCCOPY);
            BitBlt(hdc,ps.rcPaint.left,ps.rcPaint.top,ps.rcPaint.right-ps.rcPaint.left,ps.rcPaint.bottom-ps.rcPaint.top,
                hdc2,ps.rcPaint.left,ps.rcPaint.top,SRCCOPY);
            SelectObject(hdc2,hbmpo);
            DeleteObject(hbmp);
            mod_DeleteDC(hdc2);
            ps.fErase=FALSE;
            EndPaint(hwnd,&ps);
        }
        return DefWindowProc(hwnd, msg, wParam, lParam);

    case WM_GETMINMAXINFO:{
        RECT rct;
        GetWindowRect(hwnd,&rct);
        memset((LPMINMAXINFO)lParam,0,sizeof(MINMAXINFO));
        ((LPMINMAXINFO)lParam)->ptMinTrackSize.x=16;
        ((LPMINMAXINFO)lParam)->ptMinTrackSize.y=rct.bottom-rct.top;
        ((LPMINMAXINFO)lParam)->ptMaxTrackSize.x=1600;
        ((LPMINMAXINFO)lParam)->ptMaxTrackSize.y=rct.bottom-rct.top;
        return(0);
                          }

    case WM_SHOWWINDOW:
        {
            int res;
            int ID;
            if (tooltipshoing){
                NotifyEventHooks(hStatusBarHideToolTipEvent,0,0);
                tooltipshoing=FALSE;
            };
            ID=callProxied_FindFrameID(hwnd);
            if (ID)
            {
                res=CallService(MS_CLIST_FRAMES_GETFRAMEOPTIONS, MAKEWPARAM(FO_FLAGS,ID),0);
                if (res>=0) DBWriteContactSettingByte(0,"CLUI","ShowSBar",(BYTE)(wParam/*(res&F_VISIBLE)*/?1:0));
            }
        }
        break;
    case WM_TIMER:
        {
            if (wParam==TM_STATUSBARHIDE)
            {
                KillTimer(hwnd,TM_STATUSBARHIDE);
                if (tooltipshoing)
                {
                    NotifyEventHooks(hStatusBarHideToolTipEvent,0,0);
                    tooltipshoing=FALSE;
                    ReleaseCapture();
                };
            }
            else if (wParam==TM_STATUSBAR)
            {
                POINT pt;
                KillTimer(hwnd,TM_STATUSBAR);
                GetCursorPos(&pt);
                if (pt.x==lastpnt.x&&pt.y==lastpnt.y)
                {
                    int i;
                    RECT rc;
                    ScreenToClient(hwnd,&pt);
                    for (i=0; i<allocedItemData; i++)
                    {
                        rc=ProtosData[i].protoRect;
                        if(PtInRect(&rc,pt))
                        {
                            NotifyEventHooks(hStatusBarShowToolTipEvent,(WPARAM)ProtosData[i].ProtoName,0);
                            CLUI_SafeSetTimer(hwnd,TM_STATUSBARHIDE,DBGetContactSettingWord(NULL,"CLUIFrames","HideToolTipTime",SETTING_HIDETOOLTIPTIME_DEFAULT),0);
                            tooltipshoing=TRUE;
                            ClientToScreen(hwnd,&pt);
                            ptToolTipShow=pt;
                            SetCapture(hwnd);
                            return 0;
                        }
                    }
                    return 0;
                }
            }
            return 0;
        }
    case WM_MOUSEMOVE:
        if (tooltipshoing)
        {
            POINT pt;
            GetCursorPos(&pt);
            if (abs(pt.x-ptToolTipShow.x)>TOOLTIP_TOLERANCE || abs(pt.y-ptToolTipShow.y)>TOOLTIP_TOLERANCE)
            {
                KillTimer(hwnd,TM_STATUSBARHIDE);
                NotifyEventHooks(hStatusBarHideToolTipEvent,0,0);
                tooltipshoing=FALSE;
                ReleaseCapture();
            }
        }
        break;
    case WM_SETCURSOR:
        {
            if (g_CluiData.bBehindEdgeSettings) CLUI_UpdateTimer(0);
            {
                POINT pt;
                GetCursorPos(&pt);
                SendMessage(GetParent(hwnd),msg,wParam,lParam);
                if (pt.x==lastpnt.x&&pt.y==lastpnt.y)
                {
                    return(CLUI_TestCursorOnBorders());
                };
                lastpnt=pt;
                if (tooltipshoing)
                    if  (abs(pt.x-ptToolTipShow.x)>TOOLTIP_TOLERANCE || abs(pt.y-ptToolTipShow.y)>TOOLTIP_TOLERANCE)
                    {
                        KillTimer(hwnd,TM_STATUSBARHIDE);
                        NotifyEventHooks(hStatusBarHideToolTipEvent,0,0);
                        tooltipshoing=FALSE;
                        ReleaseCapture();
                    };
                KillTimer(hwnd,TM_STATUSBAR);
                CLUI_SafeSetTimer(hwnd,TM_STATUSBAR,DBGetContactSettingWord(NULL,"CLC","InfoTipHoverTime",CLCDEFAULT_INFOTIPTIME),0);

                return(CLUI_TestCursorOnBorders());
            }
        }
    case WM_LBUTTONDOWN:
    case WM_MBUTTONDOWN:
    case WM_RBUTTONDOWN:
        {
            RECT rc;
            POINT pt;
            int i;
            BOOL showXStatusMenu=FALSE;
            pt.x=(short)LOWORD(lParam);
            pt.y=(short)HIWORD(lParam);
            KillTimer(hwnd,TM_STATUSBARHIDE);
            KillTimer(hwnd,TM_STATUSBAR);

            if (tooltipshoing){
                NotifyEventHooks(hStatusBarHideToolTipEvent,0,0);
            };
            tooltipshoing=FALSE;
            for (i=0; i<allocedItemData; i++)
            {
                RECT rc1;
                BOOL isOnExtra=FALSE;
                rc=ProtosData[i].protoRect;
                rc1=rc;
                rc1.left=rc.left+16;
                rc1.right=rc1.left+16;
                if (PtInRect(&rc,pt) && PtInRect(&rc1,pt)&&ProtosData[i].DoubleIcons)
                    isOnExtra=TRUE;
                if(PtInRect(&rc,pt))
                {
                    HMENU hMenu=NULL;
                    SHORT a=GetKeyState(VK_CONTROL);
                    if (msg==WM_MBUTTONDOWN || (a&0x8000) || isOnExtra) 
                    {
                        showXStatusMenu=TRUE;
                        hMenu=(HMENU)CallService(MS_CLIST_MENUGETSTATUS,0,0);
                        if(allocedItemData>1 && pcli->menuProtos && i<pcli->menuProtoCount)// && GetSubMenu(hMenu,i)) hMenu=GetSubMenu(hMenu,i);
                        {           
                            TMO_IntMenuItem * it=pcli->pfnMOGetMenuItemByGlobalID((int)pcli->menuProtos[i].menuID);
                            if (it)
                                hMenu=it->hSubMenu;
                            else
                            {
                                HMENU hSubmenu= GetSubMenu(hMenu,i);
                                if (hSubmenu) hMenu=hSubmenu;
                            }
                        } 
                        if (hMenu)
                        {
                            HMENU tm=hMenu;
                            hMenu=GetSubMenu(tm,0); 
                            if (!hMenu) hMenu=GetSubMenu(tm,1); 
                        }
                    }        
                    if (!hMenu)
                    {
                        if (msg==WM_RBUTTONDOWN)
                        {
                            if( DBGetContactSettingByte(NULL,"CLUI","SBarRightClk",SETTING_SBARRIGHTCLK_DEFAULT))
                                hMenu=(HMENU)CallService(MS_CLIST_MENUGETMAIN,0,0);
                            else
                                hMenu=(HMENU)CallService(MS_CLIST_MENUGETSTATUS,0,0);
                        }
                        else
                        {
                            hMenu=(HMENU)CallService(MS_CLIST_MENUGETSTATUS,0,0);       
                            if(allocedItemData>1 && pcli->menuProtos && i<pcli->menuProtoCount)// && GetSubMenu(hMenu,i)) hMenu=GetSubMenu(hMenu,i);
                            {           
                                TMO_IntMenuItem * it=pcli->pfnMOGetMenuItemByGlobalID((int)pcli->menuProtos[i].menuID);
                                if (it && it->hSubMenu)
                                    hMenu=it->hSubMenu;
                                else
                                    hMenu=GetSubMenu(hMenu,i);
                            }
                        }
                    }
                    ClientToScreen(hwnd,&pt);
                    {
                        HWND parent=GetParent(hwnd);
                        if (parent!=pcli->hwndContactList) parent=GetParent(parent);
                        TrackPopupMenu(hMenu,TPM_TOPALIGN|TPM_LEFTALIGN|TPM_LEFTBUTTON,pt.x,pt.y,0,parent,NULL);
                    }
                    return 0;
                }
            }
        }
    }
    return DefWindowProc(hwnd, msg, wParam, lParam);
}

int StatusBar_Create(HWND parent)
{
    WNDCLASS wndclass={0};
    TCHAR pluginname[]=TEXT("ModernStatusBar");
    int h=GetSystemMetrics(SM_CYSMICON)+2;
    if (GetClassInfo(g_hInst,pluginname,&wndclass) ==0)
    {
        wndclass.style         = 0;
        wndclass.lpfnWndProc   = ModernStatusProc;
        wndclass.cbClsExtra    = 0;
        wndclass.cbWndExtra    = 0;
        wndclass.hInstance     = g_hInst;
        wndclass.hIcon         = NULL;
        wndclass.hCursor       = LoadCursor (NULL, IDC_ARROW);
        wndclass.hbrBackground = GetSysColorBrush(COLOR_3DFACE);
        wndclass.lpszMenuName  = NULL;
        wndclass.lpszClassName = pluginname;
        RegisterClass(&wndclass);
    }
    hModernStatusBar=CreateWindow(pluginname,pluginname,WS_CHILD|WS_VISIBLE|WS_CLIPCHILDREN,
        0,0,0,h,parent,NULL,g_hInst,NULL);
    // register frame
    {
        CLISTFrame Frame;
        memset(&Frame,0,sizeof(Frame));
        Frame.cbSize=sizeof(CLISTFrame);
        Frame.hWnd=hModernStatusBar;
        Frame.align=alBottom;
        Frame.hIcon=LoadSkinnedIcon (SKINICON_OTHER_MIRANDA);
        Frame.Flags=(DBGetContactSettingByte(NULL,"CLUI","ShowSBar",SETTING_SHOWSBAR_DEFAULT)?F_VISIBLE:0)|F_LOCKED|F_NOBORDER|F_NO_SUBCONTAINER|F_TCHAR;
        Frame.height=h;
        Frame.tname=_T("Status Bar");
        Frame.TBtname=TranslateT("Status Bar");
        hFramehModernStatusBar=(HANDLE)CallService(MS_CLIST_FRAMES_ADDFRAME,(WPARAM)&Frame,(LPARAM)0);
        CallService(MS_SKINENG_REGISTERPAINTSUB,(WPARAM)Frame.hWnd,(LPARAM)NewStatusPaintCallbackProc); //$$$$$ register sub for frame
    }

    LoadStatusBarData();
    hStatusBarShowToolTipEvent=CreateHookableEvent(ME_CLIST_FRAMES_SB_SHOW_TOOLTIP);
    hStatusBarHideToolTipEvent=CreateHookableEvent(ME_CLIST_FRAMES_SB_HIDE_TOOLTIP);
    CLUIServices_ProtocolStatusChanged(0,0);
    CallService(MS_CLIST_FRAMES_UPDATEFRAME,-1,0);
    return (int)hModernStatusBar;
}
