library talk2;

{
Talk2 Plugin
Version 0.1 Alpha
by Christian Kästner
for Miranda ICQ 0.1.1.0+
written with Delphi 5 Pro
converted from the testplug by cyreve


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


For more information, e-mail christian.k@stner.de
}

uses
  Windows,
  Classes,
  messages,
  Commctrl,
  ComObj,
  sysutils,
  newpluginapi in '..\units\newpluginapi.pas',
  globals in '..\units\globals.pas',
  m_clist in '..\headerfiles\m_clist.pas',
  statusmodes in '..\units\statusmodes.pas',
  langpacktools in '..\units\langpacktools.pas',
  m_langpack in '..\headerfiles\m_langpack.pas',
  m_system in '..\headerfiles\m_system.pas',
  m_plugins in '..\headerfiles\m_plugins.pas',
  m_skin in '..\headerfiles\m_skin.pas',
  skintools in '..\units\skintools.pas',
  OLE2 in '..\..\..\..\..\borland\Delphi5\Source\Rtl\Win\ole2.pas',
  m_database in '..\headerfiles\m_database.pas',
  clisttools in '..\units\clisttools.pas',
  m_icq in '..\headerfiles\m_icq.pas',
  talktools in '..\units\talktools.pas',
  m_talk in 'm_talk.pas',
  HTTSLib_TLB in '..\..\..\..\..\borland\Delphi5\Imports\HTTSLib_TLB.pas',
  m_options in '..\headerfiles\m_options.pas',
  speechapi in 'speechapi.pas';

{$R *.RES}



var
  TextToSpeech:TTextToSpeech;
  hmenuitem:THandle;
  readincoming,
  readstatuschange,
  readcontactchange:Boolean;


function OnModulesLoad(wParam{0},lParam{0}:DWord):integer;cdecl;forward;
procedure LoadSettings;forward;

function MirandaPluginInfo(mirandaVersion:DWord):PPLUGININFO;cdecl;
//Tell Miranda about this plugin
begin
  PluginInfo.cbSize:=sizeof(TPLUGININFO);
  PluginInfo.shortName:='Talk2 Plugin';
  PluginInfo.version:=PLUGIN_MAKE_VERSION(0,1,0,0);
  PluginInfo.description:='Announces online users and reads incoming messages.';
  PluginInfo.author:='Christian Kästner';
  PluginInfo.authorEmail:='christian@kaestnerpro.de';
  PluginInfo.copyright:='© 2001 by Christian Kästner';
  PluginInfo.homepage:='http://www.nortiq.com/miranda';
  PluginInfo.isTransient:=0;
  PluginInfo.replacesDefaultModule:=0;  //doesn't replace anything built-in

  Result:=@PluginInfo;

  globals.mirandaVersion:=mirandaVersion;
end;

function Load(link:PPLUGINLINK):Integer;cdecl;
//load function called by miranda
begin
  PluginLink:=link^;

  //init history functions later
  PluginLink.HookEvent(ME_SYSTEM_MODULESLOADED,OnModulesLoad);

  CoInitialize(nil);
  TextToSpeech:=TTextToSpeech.Create(nil);
  LoadSettings;

  Result:=0;
end;

function Unload:Integer;cdecl;
//unload
begin
  TextToSpeech.Free;
  CoUninitialize;

  Result:=0;
end;

procedure Speak(Text:string);
begin
  Text:=StringReplace(Text,'ä','ae',[rfReplaceAll, rfIgnoreCase]);
  Text:=StringReplace(Text,'ö','oe',[rfReplaceAll, rfIgnoreCase]);
  Text:=StringReplace(Text,'ü','ue',[rfReplaceAll, rfIgnoreCase]);
  Text:=StringReplace(Text,'ß','ss',[rfReplaceAll, rfIgnoreCase]);
try
  TextToSpeech.Speak(Text);
except end;
end;

function GetMyStatus:integer;
begin
  Result:=ReadSettingWord(pluginlink,0,'ICQ','Status',0);
end;
function GetNick(hContact: THandle): string;
//get Nickname from hContact
begin
  Result:=string(PChar(PluginLink.CallService(MS_CLIST_GETCONTACTDISPLAYNAME,DWord(hContact),0)));
end;

function OnUserSettingChange(wParam{hContact},lParam{DBCONTACTWRITESETTING}:DWord):integer;cdecl;
//used to watch user status changes
var
  newstatus:string;
  nick:string;
  Text:string;
begin
  Result:=0;

  if readcontactchange then
    if StrComp(PDBCONTACTWRITESETTING(lParam)^.szModule,'ICQ')=0 then
      if StrComp(PDBCONTACTWRITESETTING(lParam)^.szSetting,'Status')=0 then
        begin
        newstatus:=miranda_ConvertStatus2Str(Word(PDBCONTACTWRITESETTING(lParam)^.Value.val));
        nick:=GetNick(wParam);
        Text:=Translate('%USER% is %STATUS%');
        Text:=StringReplace(Text,'%USER%',nick,[]);
        Text:=StringReplace(Text,'%STATUS%',newstatus,[]);
        Speak(text);
        end;
end;


function OnStatusModeChange(wParam{NewStatus},lParam{0}:DWord):integer;cdecl;
var
  statusmode:string;
  sm:integer;
begin
  Result:=0;
  sm:=getmystatus;
  if sm<MAX_CONNECT_RETRIES then
    Exit;

  if not readstatuschange then
    Exit;


  statusmode:=miranda_ConvertStatus2Str(wParam);

  Speak(StringReplace(translate('I''m %STATUS%'),'%STATUS%',statusmode,[]));
end;

function DoSpeakText(wParam{Text:PChar},lParam{0}:DWord):integer;cdecl;
var
  Text:string;
begin
  Text:=string(PChar(wParam));
  Speak(Text);
  Result:=0;
end;

function DoStopSpeak(wParam{0},lParam{0}:DWord):integer;cdecl;
begin
  Result:=TextToSpeech.IsSpeaking;
  if Result>0 then
    TextToSpeech.StopSpeaking;
end;


function OnMessageEventAdded(wParam{hContact},lParam{hdbEvent}:DWord):integer;cdecl;
//when a new item was added to the database read it
var
  BlobSize:Integer;
  dbei:TDBEVENTINFO;

  Text:string;
  msgtext:string;
  nick:string;
begin
  Result:=0;

  if not readincoming then
    Exit;

  dbei.cbSize:=sizeof(dbei);
  //alloc memory for message
  blobsize:=PluginLink.CallService(MS_DB_EVENT_GETBLOBSIZE,lParam{hDbEvent},0);
  getmem(dbei.pBlob,blobsize);
  try
  dbei.cbBlob:=BlobSize;
  //getmessage
  PluginLink.CallService(MS_DB_EVENT_GET,lParam{hdbEvent},DWord(@dbei));


  if ((dbei.flags and DBEF_SENT)<>0) or //send yourself
    ((dbei.flags and DBEF_READ)<>0) or //already read?
    (StrComp(dbei.szModule,'ICQ')<>0) or
    (dbei.eventType<>EVENTTYPE_MESSAGE) then
    Exit;

  Text:=translate('Message from %USER%: %TEXT%');

  msgtext:=string(PChar(dbei.pBlob));
  nick:=GetNick(wParam);

  Text:=StringReplace(Text,'%USER%',nick,[]);
  Text:=StringReplace(Text,'%TEXT%',msgtext,[]);
  Speak(text);


  finally
  freemem(dbei.pBlob);
  end;
end;


function OnOptInitialise(wParam{0},lParam{0}:DWord):integer;cdecl;forward;

function OnModulesLoad(wParam{0},lParam{0}:DWord):integer;cdecl;
//init plugin
var
  menuitem:TCLISTMENUITEM;
begin
  //register service for showing history
  PluginLink.HookEvent(ME_CLIST_STATUSMODECHANGE,OnStatusModeChange);
  PluginLink.HookEvent(ME_DB_CONTACT_SETTINGCHANGED,OnUserSettingChange);
  PluginLink.HookEvent(ME_DB_EVENT_ADDED,OnMessageEventAdded);
  PluginLink.HookEvent(ME_OPT_INITIALISE,OnOptInitialise);

  PluginLink.CreateServiceFunction(MS_TALK_SPEAKTEXT,DoSpeakText);
  PluginLink.CreateServiceFunction(MS_TALK_STOPSPEAK,DoStopSpeak);

  //create menu item in contact menu
  menuitem.cbSize:=sizeof(menuitem);
  menuitem.Position:=1;
  menuitem.flags:=0;
  menuitem.hIcon:=0;
  menuitem.pszContactOwner:=nil;    //all contacts
  menuitem.pszName:=PChar(translate('Stop Talk!'));
  menuitem.pszService:=MS_TALK_STOPSPEAK;
  hmenuitem:=PluginLink.CallService(MS_CLIST_ADDMAINMENUITEM,0,DWord(@menuitem));

  Result:=0;
end;


(*************************OPTIONS**********************)
const
  IDC_testbtn	        =110;
  IDC_lexiconbtn	=109;
  IDC_VOICEINFO	        =106;
  IDC_aboutvoice	=108;
  IDC_voicecombo	=107;
  IDC_statuschange	=103;
  IDC_ContactChange	=102;
  IDC_Readincoming	=101;

{$R optdlgs.res}


function DlgProcOptions(Dialog: HWnd; Message, wParam, lParam: DWord): Boolean; cdecl;forward;

function OnOptInitialise(wParam{addinfo},lParam{0}:DWord):integer;cdecl;
var
  odp:TOPTIONSDIALOGPAGE;
begin
  ZeroMemory(@odp,sizeof(odp));
  odp.cbSize:=sizeof(odp);
  odp.Position:=900000000;
  odp.hInstance:=hInstance;
  odp.pszTemplate:='DLG_TALKOPTIONS';
  odp.pszTitle:=PChar(translate('Talk'));
  odp.pfnDlgProc:=@DlgProcOptions;
  PluginLink.CallService(MS_OPT_ADDPAGE,wParam,dword(@odp));
  Result:=0;
end;

function GenderToStr(Gender:integer):string;
begin
  case gender of
    GENDER_FEMALE:Result:=translate('Female');
    GENDER_MALE:Result:=translate('Male');
  else
    Result:=translate('Neutral');
  end;
end;
function AgeToStr(Age:integer):string;
begin
  Result:=IntToStr(age);
end;
function LanguageToStr(LangID:integer):string;
begin
  Result:=languages.NameFromLocaleID[langid];
end;

function DlgProcOptions(Dialog: HWnd; Message, wParam, lParam: DWord): Boolean; cdecl;
var changed:Boolean;

  procedure Change;
  begin
    if not changed then
      SendMessage(GetParent(dialog), PSM_CHANGED, 0, 0);
    changed:=True;
  end;

var
  modname:string;
  modidx:integer;
  voiceidx:integer;
  infotext:string;
begin
  Result:=False;

  case message of
    WM_INITDIALOG:
      begin
      //load options
      TranslateDialogDefault(dialog);
      if ReadSettingBool(PluginLink,0,'Talk2','ReadIncoming',True) then
        CheckDlgButton(Dialog,IDC_Readincoming,BST_CHECKED);
      if ReadSettingBool(PluginLink,0,'Talk2','ReadStatusChange',True) then
        CheckDlgButton(Dialog,IDC_statuschange,BST_CHECKED);
      if ReadSettingBool(PluginLink,0,'Talk2','ReadContactChange',True) then
        CheckDlgButton(Dialog,IDC_ContactChange,BST_CHECKED);

      for modidx:=1 to TextToSpeech.CountEngines do
        begin
        modname:=TextToSpeech.ModeName(modidx);
	SendDlgItemMessage (dialog, IDC_voicecombo, CB_ADDSTRING, 0, dword(PChar(modname)));
        end;
      voiceidx:=ReadSettingInt(PluginLink,0,'Talk2','VoiceIdx',0);
      SendDlgItemMessage (dialog, IDC_voicecombo, CB_SETCURSEL, voiceidx, 0);

      changed:=False;

      Result:=True;
      end;
    WM_COMMAND:
      begin
      case LOWORD(wParam) of
        IDC_lexiconbtn:
          TextToSpeech.LexiconDlg(dialog,translate('Lexicon'));
	IDC_aboutvoice:
          TextToSpeech.GeneralDlg(dialog,translate('About'));
	IDC_testbtn:
          begin
          TextToSpeech.Select(SendDlgItemMessage(dialog, IDC_voicecombo, CB_GETCURSEL, 0, 0)+1);
          case Random(5) of
            0:speak(translate('mike check 1 2 1 2 we in the house'));
            1:speak(translate('its tricky to rock a rhyme, to rock a rhyme thats right on time'));
            2:speak(translate('my rhymes make me wealthy, and the funky bunch helps me'));
            3:speak(translate('and if your name happens to be herb, just say i''m not the herb your looking for'));
            4:speak(translate('man makes the money, money never makes the man'));
          end;
          end;
        IDC_Readincoming:
          change;
        IDC_statuschange:
          change;
        IDC_ContactChange:
          change;
        IDC_voicecombo:
          begin
          if (HIWORD(wParam)=LBN_SELCHANGE) then
            change;
          if (HIWORD(wParam) in [LBN_SELCHANGE,LBN_SETFOCUS]) then
            begin
            voiceidx := SendDlgItemMessage (dialog, IDC_voicecombo, CB_GETCURSEL, 0, 0)+1;
            if (voiceidx>=0) and (voiceidx<TextToSpeech.CountEngines) then
              begin
              infotext:='';
              infotext:=infotext+translate('Product Name:')+' '+TextToSpeech.ProductName(voiceidx)+#13#10;
              infotext:=infotext+translate('Manufacturer:')+' '+TextToSpeech.MfgName(voiceidx)+#13#10;
              infotext:=infotext+translate('Gender, Age:')+' '+GenderToStr(TextToSpeech.Gender(voiceidx))+', '+AgeToStr(TextToSpeech.Age(voiceidx))+#13#10;
              infotext:=infotext+translate('Language:')+' '+LanguageToStr(TextToSpeech.LanguageID(voiceidx))+#13#10;
              infotext:=infotext+translate('Style:')+' '+TextToSpeech.Style(voiceidx);
              SetDlgItemText(dialog,IDC_VOICEINFO,PChar(infotext));
              end;
            end;
          end;
      end;
      end;
    WM_NOTIFY:
      begin
      if PNMHdr(lParam)^.code = PSN_APPLY then//save settings
        begin
        //save new settings
        WriteSettingBool(PluginLink,0,'Talk2','ReadIncoming',IsDlgButtonChecked(dialog,IDC_Readincoming)=BST_CHECKED);
        WriteSettingBool(PluginLink,0,'Talk2','ReadStatusChange',IsDlgButtonChecked(dialog,IDC_statuschange)=BST_CHECKED);
        WriteSettingBool(PluginLink,0,'Talk2','ReadContactChange',IsDlgButtonChecked(dialog,IDC_ContactChange)=BST_CHECKED);
        voiceidx := SendDlgItemMessage (dialog, IDC_voicecombo, CB_GETCURSEL, 0, 0)+1;
        WriteSettingInt(PluginLink,0,'Talk2','VoiceIdx',voiceidx);
        LoadSettings;

        Result:=True;
        end;
      end;
   end;
end;


procedure LoadSettings;
var
  voiceidx:integer;
begin
try
  voiceidx:=ReadSettingInt(PluginLink,0,'Talk2','VoiceIdx',0);
  if voiceidx>0 then
    TextToSpeech.Select(voiceidx);
except end;
  readincoming:=ReadSettingBool(PluginLink,0,'Talk2','ReadIncoming',True);
  readstatuschange:=ReadSettingBool(PluginLink,0,'Talk2','ReadStatusChange',True);
  readcontactchange:=ReadSettingBool(PluginLink,0,'Talk2','ReadContactChange',True);
end;


exports
  MirandaPluginInfo,
  Load,
  Unload;


begin
end.

