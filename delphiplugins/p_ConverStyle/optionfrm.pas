{***************************************************************
 * Project     : convers
 * Unit Name   : optionfrm
 * Description : Optionsdialog for this plugin
 * Author      : Christian Kästner
 * Date        : 24.05.2001
 *
 * Copyright © 2001 by Christian Kästner
 ****************************************************************}

unit optionfrm;

interface

uses
  Windows, Messages, SysUtils, Classes, Graphics, Controls, Forms, Dialogs,
  ExtCtrls, StdCtrls, globals, clisttools, Menus, TB97Ctls,
  ComCtrls,misc,optionsd,commctrl;

type
  TOptionType = (otAll, otMisc, otSend, otMemo, otRich, otGrid);
  TOptionForm = class(TForm)
    OKBtn: TButton;
    CancelBtn: TButton;
    AboutBtn: TButton;
    Bevel1: TBevel;
    FontDialog: TFontDialog;
    PageControl1: TPageControl;
    TabSheet1: TTabSheet;
    TabSheet2: TTabSheet;
    TabSheet3: TTabSheet;
    TabSheet4: TTabSheet;
    StorePositionsCheck: TCheckBox;
    LoadRecentMsgCheck: TCheckBox;
    LoadRecentMsgCountEdit: TEdit;
    m_textedit: TComboBox;
    m_changefont: TButton;
    m_texteditlabel: TLabel;
    m_textedithint: TLabel;
    r_texteditlabel: TLabel;
    r_textedit: TComboBox;
    r_textedithint: TLabel;
    r_grayrecent: TCheckBox;
    r_changefont: TButton;
    r_textbold: TCheckBox;
    r_textitalic: TCheckBox;
    r_changecolor: TButton;
    g_changefont: TButton;
    g_showtime: TCheckBox;
    g_showdate: TCheckBox;
    g_changecolor: TButton;
    g_grayrecent: TCheckBox;
    HandleIncomingGroup: TRadioGroup;
    UseMemoCheck: TCheckBox;
    UseRicheditCheck: TCheckBox;
    UseGridCheck: TCheckBox;
    ColorDialog: TColorDialog;
    r_texttypeselect: TListBox;
    Button1: TButton;
    TabSheet5: TTabSheet;
    Label1: TLabel;
    DoubleEnter: TCheckBox;
    SingleEnter: TCheckBox;
    TimeoutEdit: TEdit;
    Label2: TLabel;
    CloseWindowCheck: TCheckBox;
    SplitLargeMessagesCheck: TCheckBox;
    procedure AboutBtnClick(Sender: TObject);
    procedure OKBtnClick(Sender: TObject);
    procedure FormShow(Sender: TObject);
    procedure LoadRecentMsgCountEditExit(Sender: TObject);
    procedure RadionBtnClick(Sender: TObject);
    procedure g_showtimeClick(Sender: TObject);
    procedure m_changefontClick(Sender: TObject);
    procedure g_changefontClick(Sender: TObject);
    procedure r_texttypeselectChange(Sender: TObject);
    procedure r_changecolorClick(Sender: TObject);
    procedure r_textboldClick(Sender: TObject);
    procedure r_textitalicClick(Sender: TObject);
    procedure r_grayrecentClick(Sender: TObject);
    procedure g_grayrecentClick(Sender: TObject);
    procedure g_showdateClick(Sender: TObject);
    procedure g_changecolorClick(Sender: TObject);
    procedure Button1Click(Sender: TObject);
    procedure r_changefontClick(Sender: TObject);
    procedure TimeoutEditExit(Sender: TObject);
  private
    richsettings:TRichEditSettings;
    gridsettings:TGridEditSettings;
  public
    hContact:DWord;
  end;


function DlgProcMiscOptions(Dialog: HWnd; Message, wParam, lParam: DWord): Boolean; cdecl;
function DlgProcSendOptions(Dialog: HWnd; Message, wParam, lParam: DWord): Boolean; cdecl;


implementation

uses windowmanager;

{$R *.DFM}

procedure TOptionForm.AboutBtnClick(Sender: TObject);
begin
    MessageDlg(PluginInfo.shortname+' V '+PluginInfoVersion+#13#10#13#10+PluginInfo.description+
      #13#10#13#10'by '+PluginInfo.author+' ('+PluginInfo.authorEmail+')'#13#10+
      PluginInfo.copyright+#13#10#13#10+PluginInfo.homepage,mtInformation,
      [mbok],0);
end;

procedure TOptionForm.OKBtnClick(Sender: TObject);
//save changes
var
  LogFont:TLogFont;
  val:integer;
begin
  TimeoutEditExit(Self);
  LoadRecentMsgCountEditExit(Self);

  //misc
  WriteSettingInt(PluginLink,0,'Convers','SplitLargeMessages',integer(SplitLargeMessagesCheck.Checked));
  WriteSettingInt(PluginLink,0,'Convers','StorePositions',integer(StorePositionsCheck.Checked));
  WriteSettingInt(PluginLink,0,'Convers','LoadRecentMsgs',integer(LoadRecentMsgCheck.Checked));
  WriteSettingInt(PluginLink,0,'Convers','LoadRecentMsgCount',LoadRecentMsgCountEdit.tag);
  WriteSettingInt(PluginLink,0,'Convers','DoubleEnterSend',Integer(DoubleEnter.Checked));
  WriteSettingInt(PluginLink,0,'Convers','SingleEnterSend',Integer(SingleEnter.Checked));
  WriteSettingInt(PluginLink,0,'Convers','CloseWindowAfterSend',integer(CloseWindowCheck.Checked));
  WriteSettingInt(PluginLink,0,'Convers','HandleIncoming',HandleIncomingGroup.ItemIndex);
  WriteSettingInt(PluginLink,0,'Convers','SendTimeout',TimeoutEdit.tag);

  //displaytype
  Val:=integer(dmmemo);
  if UseRicheditCheck.Checked then
    Val:=integer(dmrich);
  if UseGridCheck.Checked then
    Val:=integer(dmgrid);
  WriteSettingInt(PluginLink,0,'Convers','DisplayType',val);

  //save settings for all display modes (font, fontcolor)
  if GetObject(FontDialog.Font.Handle, SizeOf(LogFont), @LogFont) <> 0 then
    WriteSettingBlob(PluginLink,0,'Convers','Font',SizeOf(LogFont), @LogFont);
  WriteSettingInt(PluginLink,0,'Convers','FontCol',Integer(FontDialog.Font.Color));

  //save textpattern settings
  WriteSettingStr(PluginLink,0,'Convers','MemoTextPattern',PChar(m_textedit.Text));
  WriteSettingStr(PluginLink,0,'Convers','RichTextPattern',PChar(r_textedit.Text));

  //save grid and rich settings
  WriteSettingBlob(PluginLink,0,'Convers','RichSettings',SizeOf(richsettings), @richsettings);
  WriteSettingBlob(PluginLink,0,'Convers','GridSettings',SizeOf(gridsettings), @gridsettings);
end;

procedure TOptionForm.FormShow(Sender: TObject);
//load options
var
  val:integer;
  s:Word;
  p:Pointer;
  LogFont:TLogFont;
begin
  //misc options
  val:=ReadSettingInt(PluginLink,0,'Convers','SplitLargeMessages', integer(False));
  SplitLargeMessagesCheck.Checked:=Boolean(val);
  val:=ReadSettingInt(PluginLink,0,'Convers','StorePositions',integer(True));
  StorePositionsCheck.Checked:=Boolean(val);
  val:=ReadSettingInt(PluginLink,0,'Convers','LoadRecentMsgs',integer(false));
  LoadRecentMsgCheck.Checked:=Boolean(val);
  val:=ReadSettingInt(PluginLink,0,'Convers','LoadRecentMsgCount',10);
  LoadRecentMsgCountEdit.text:=IntToStr(val);
  LoadRecentMsgCountEdit.tag:=val;
  val:=ReadSettingInt(PluginLink,0,'Convers','DoubleEnterSend',integer(False));
  DoubleEnter.Checked:=Boolean(val);
  val:=ReadSettingInt(PluginLink,0,'Convers','SingleEnterSend',integer(False));
  SingleEnter.Checked:=Boolean(val);
  val:=ReadSettingInt(PluginLink,0,'Convers','CloseWindowAfterSend',integer(False));
  CloseWindowCheck.Checked:=Boolean(val);
  val:=ReadSettingInt(PluginLink,0,'Convers','HandleIncoming',0);
  HandleIncomingGroup.ItemIndex:=val;
  val:=ReadSettingInt(PluginLink,0,'Convers','SendTimeout',DEFAULT_TIMEOUT_MSGSEND);
  TimeoutEdit.Text:=IntToStr(val);
  TimeoutEdit.tag:=val;

  //load display type
  val:=ReadSettingInt(PluginLink,0,'Convers','DisplayType',Integer(DefaultDisplayMode));
  case TDisplayMode(val) of
    dmrich:UseRicheditCheck.Checked:=True;
    dmgrid:UseGridCheck.Checked:=True
    else UseMemoCheck.Checked:=True;
  end;

  //load font for all displayt.
  s:=SizeOf(LogFont);
  ReadSettingBlob(PluginLink,0,'Convers','Font',s, p);
  if Assigned(p) then
    begin
    LogFont:=PLogFont(p)^;
    FontDialog.Font.Handle:=CreateFontIndirect(LogFont);
    FontDialog.Font.Color:=TColor(ReadSettingInt(PluginLink,0,'Convers','FontCol',Integer(clBlack)));
    FreeSettingBlob(PluginLink,s, p);
    end;

  //load settings for memo
  m_textedit.Text:=string(ReadSettingStr(PluginLink,0,'Convers','MemoTextPattern','%NAME% (%TIME%): %TEXT%'));

  //load rich settings
  r_textedit.Text:=string(ReadSettingStr(PluginLink,0,'Convers','RichTextPattern','%NAME% (%TIME%): %TEXT%'));

  s:=SizeOf(RichSettings);
  ReadSettingBlob(PluginLink,0,'Convers','RichSettings',s, p);
  if Assigned(p) then
    begin
    RichSettings:=PRichEditSettings(p)^;
    FreeSettingBlob(PluginLink,s, p);
    end
  else
    RichSettings:=DefaultRichEditSettings;
  r_grayrecent.Checked:=richsettings.RecentGray;
  r_texttypeselectChange(self);

  //load grid settings
  s:=SizeOf(GridSettings);
  ReadSettingBlob(PluginLink,0,'Convers','GridSettings',s, p);
  if Assigned(p) then
    begin
    GridSettings:=PGridEditSettings(p)^;
    FreeSettingBlob(PluginLink,s, p);
    end
  else
    GridSettings:=DefaultGridEditSettings;
  g_showtime.Checked:=gridsettings.InclTime;
  g_showdate.Checked:=gridsettings.InclDate;
  g_grayrecent.Checked:=gridsettings.GrayRecent;
  //colors still missing

  //misc
  SplitLargeMessagesCheck.Caption:=StringReplace(SplitLargeMessagesCheck.Caption,'%d',IntToStr(MaxMessageLength),[]);
end;


procedure TOptionForm.RadionBtnClick(Sender: TObject);
//enable appropriated tab for selected display type
begin
  if not (Sender is TCheckBox) then
    Exit;
    
  if TCheckBox(Sender).Checked then
    begin
    if (Sender = UseMemoCheck) then
      begin
      UseRicheditCheck.Checked:=False;
      UseGridCheck.Checked:=False;
      end;
    if (Sender = UseGridCheck) then
      begin
      UseRicheditCheck.Checked:=False;
      UseMemoCheck.Checked:=False;
      end;
    if (Sender = UseRicheditCheck) then
      begin
      UseMemoCheck.Checked:=False;
      UseGridCheck.Checked:=False;
      end;
    end;
  //always check at least one!
  if not (UseMemoCheck.Checked or UseRicheditCheck.Checked or UseGridCheck.Checked) then
    UseMemoCheck.Checked:=True;

  m_changefont.Enabled:=UseMemoCheck.Checked;
  m_textedit.Enabled:=UseMemoCheck.Checked;
  m_texteditlabel.Enabled:=UseMemoCheck.Checked;
  m_textedithint.Enabled:=UseMemoCheck.Checked;

  r_textedit.Enabled:=UseRicheditCheck.Checked;
  r_texteditlabel.Enabled:=UseRicheditCheck.Checked;
  r_textedithint.Enabled:=UseRicheditCheck.Checked;
  r_texttypeselect.Enabled:=UseRicheditCheck.Checked;
  r_grayrecent.Enabled:=UseRicheditCheck.Checked;
  r_changefont.Enabled:=UseRicheditCheck.Checked;
  r_textbold.Enabled:=UseRicheditCheck.Checked;
  r_textitalic.Enabled:=UseRicheditCheck.Checked;
  r_changecolor.Enabled:=UseRicheditCheck.Checked;

  g_changefont.Enabled:=UseGridCheck.Checked;
  g_showtime.Enabled:=UseGridCheck.Checked;
  g_showdate.Enabled:=UseGridCheck.Checked and g_showtime.checked;
  g_changecolor.Enabled:=UseGridCheck.Checked;
  g_grayrecent.Enabled:=UseGridCheck.Checked;
end;


//******************* Misc frontend functions **********************************
procedure TOptionForm.LoadRecentMsgCountEditExit(Sender: TObject);
var
  v:integer;
begin
  try
  v:=StrToInt(LoadRecentMsgCountEdit.Text);
  if v>0 then
    LoadRecentMsgCountEdit.tag:=v
  else
    ShowMessage(LoadRecentMsgCountEdit.Text+' is not a valid number greater than 0');
  except
  ShowMessage('"'+LoadRecentMsgCountEdit.text+'" is not a valid number');
  end;
end;

procedure TOptionForm.g_showtimeClick(Sender: TObject);
begin
  g_showdate.Enabled:=g_showtime.Checked;

  gridsettings.InclTime:=g_showtime.Checked;
end;

procedure TOptionForm.m_changefontClick(Sender: TObject);
begin
  FontDialog.Options:=FontDialog.Options-[fdNoStyleSel];
  FontDialog.Execute;
end;

procedure TOptionForm.g_changefontClick(Sender: TObject);
begin
  FontDialog.Options:=FontDialog.Options-[fdNoStyleSel];
  FontDialog.Execute;
end;

procedure TOptionForm.r_texttypeselectChange(Sender: TObject);
begin
  r_changecolor.Enabled:=(r_texttypeselect.ItemIndex in [0..6]);
  r_textbold.Enabled:=r_changecolor.Enabled;
  r_textitalic.Enabled:=r_changecolor.Enabled;

  if not r_changecolor.Enabled then
    Exit;

  r_textbold.Checked:=richsettings.RichStyles[r_texttypeselect.ItemIndex].Bold;
  r_textitalic.Checked:=richsettings.RichStyles[r_texttypeselect.ItemIndex].Italic;
end;

procedure TOptionForm.r_changecolorClick(Sender: TObject);
begin
  if not (r_texttypeselect.ItemIndex in [0..6]) then
    Exit;
  ColorDialog.Color:=richsettings.RichStyles[r_texttypeselect.ItemIndex].Color;
  if ColorDialog.Execute then
    richsettings.RichStyles[r_texttypeselect.ItemIndex].Color:=ColorDialog.Color;
end;

procedure TOptionForm.r_textboldClick(Sender: TObject);
begin
  if not (r_texttypeselect.ItemIndex in [0..6]) then
    Exit;
  richsettings.RichStyles[r_texttypeselect.ItemIndex].Bold:=r_textbold.Checked;
end;

procedure TOptionForm.r_textitalicClick(Sender: TObject);
begin
  if not (r_texttypeselect.ItemIndex in [0..6]) then
    Exit;
  richsettings.RichStyles[r_texttypeselect.ItemIndex].Italic:=r_textitalic.Checked;
end;

procedure TOptionForm.r_grayrecentClick(Sender: TObject);
begin
  richsettings.RecentGray:=r_grayrecent.Checked;
end;

procedure TOptionForm.g_grayrecentClick(Sender: TObject);
begin
  gridsettings.GrayRecent:=g_grayrecent.Checked;
end;

procedure TOptionForm.g_showdateClick(Sender: TObject);
begin
  gridsettings.InclDate:=g_showdate.Checked and g_showdate.enabled;
end;

procedure TOptionForm.g_changecolorClick(Sender: TObject);
begin
  ColorDialog.Color:=gridsettings.BGColorIn;
  if ColorDialog.Execute then
    gridsettings.BGColorIn:=ColorDialog.Color;
end;

procedure TOptionForm.Button1Click(Sender: TObject);
begin
  ColorDialog.Color:=gridsettings.BGColorOut;
  if ColorDialog.Execute then
    gridsettings.BGColorOut:=ColorDialog.Color;
end;

procedure TOptionForm.r_changefontClick(Sender: TObject);
begin
  FontDialog.Options:=FontDialog.Options+[fdNoStyleSel];
  FontDialog.Execute;
end;

procedure TOptionForm.TimeoutEditExit(Sender: TObject);
var
  v:integer;
begin
  try
  v:=StrToInt(TimeoutEdit.Text);
  if v>0 then
    TimeoutEdit.tag:=v
  else
    ShowMessage(TimeoutEdit.Text+' is not a valid number greater than 0');
  except
  ShowMessage('"'+TimeoutEdit.text+'" is not a valid number');
  end;
end;

function Checked(c:Boolean):Cardinal;
//converts boolean value to  BST_CHECKED
begin
  if c then Result:=BST_CHECKED	else Result:=BST_UNCHECKED;
end;

function DlgProcMiscOptions(Dialog: HWnd; Message, wParam, lParam: DWord): Boolean; cdecl;
var
  str:string;
  pc:PChar;
  val:integer;
begin
  Result:=False;

  case message of
    WM_INITDIALOG:
      begin
      //load options
      CheckDlgButton(dialog, IDC_LOADRECENT, Checked(ReadSettingBool(PluginLink,0,'Convers','LoadRecentMsgs',false)));
      str:=IntToStr(ReadSettingInt(PluginLink,0,'Convers','LoadRecentMsgCount',10));pc:=PChar(str);
      SendDlgItemMessage(dialog,IDC_RECENTCOUNT,CB_ADDSTRING,0,integer(pc));
      if IsDlgButtonChecked(Dialog,IDC_LOADRECENT)<>BST_CHECKED then
        EnableWindow(GetDlgItem(Dialog,IDC_RECENTCOUNT),FALSE);
      CheckDlgButton(dialog, IDC_STOREPOS, Checked(ReadSettingBool(PluginLink,0,'Convers','StorePositions',True)));

      case ReadSettingInt(PluginLink,0,'Convers','HandleIncoming',0) of
        0:CheckDlgButton(Dialog,IDC_STEALFOCUS,BST_CHECKED);
        1:CheckDlgButton(Dialog,IDC_SHOWINACTIVE,BST_CHECKED);
        3:CheckDlgButton(Dialog,IDC_FLASHTRAY,BST_CHECKED);
        else
          CheckDlgButton(Dialog,IDC_FLASHWINDOW,BST_CHECKED);
      end;

      //make sure we can save the dialog...
      SendMessage(GetParent(dialog), PSM_CHANGED, 0, 0);

      Result:=True;
      end;
    WM_COMMAND:
      begin
      if (LOWORD(wParam)=IDC_LOADRECENT) then
        begin
        EnableWindow(GetDlgItem(Dialog,IDC_RECENTCOUNT),IsDlgButtonChecked(dialog,IDC_LOADRECENT)=BST_CHECKED);
        SendMessage(GetParent(dialog), PSM_CHANGED, 0, 0);
        end;
      end;
    WM_NOTIFY:
      begin
      if PNMHdr(lParam)^.code = PSN_KILLACTIVE then//check if all values are ok
        if IsDlgButtonChecked(Dialog,IDC_LOADRECENT)=BST_CHECKED then
          try
          setlength(str,256);pc:=PChar(str);
          GetDlgItemText(dialog,IDC_RECENTCOUNT,pc,sizeof(str));
          val:=StrToInt(str);
          if val<0 then
            raise Exception.Create('');
          except
            MessageDlg('Invalid value for recent messages count.', mtError, [mbOK], 0);
            SetWindowLong(dialog,DWL_MSGRESULT,integer(true));
            Result:=true
          end;
      if PNMHdr(lParam)^.code = PSN_APPLY then//save settings
        begin
        WriteSettingBool(PluginLink,0,'Convers','LoadRecentMsgs',IsDlgButtonChecked(Dialog,IDC_LOADRECENT)=BST_CHECKED);
        WriteSettingBool(PluginLink,0,'Convers','StorePositions',IsDlgButtonChecked(Dialog,IDC_STOREPOS)=BST_CHECKED);

        if IsDlgButtonChecked(Dialog,IDC_LOADRECENT)=BST_CHECKED then
          try
          setlength(str,256);pc:=PChar(str);
          GetDlgItemText(dialog,IDC_RECENTCOUNT,pc,sizeof(str));
          val:=StrToInt(str);
          WriteSettingInt(PluginLink,0,'Convers','LoadRecentMsgCount',val);
          except end;

        if IsDlgButtonChecked(dialog,IDC_STEALFOCUS)=BST_CHECKED then WriteSettingInt(PluginLink,0,'Convers','HandleIncoming',0);
        if IsDlgButtonChecked(dialog,IDC_SHOWINACTIVE)=BST_CHECKED then WriteSettingInt(PluginLink,0,'Convers','HandleIncoming',1);
        if IsDlgButtonChecked(dialog,IDC_FLASHWINDOW)=BST_CHECKED then WriteSettingInt(PluginLink,0,'Convers','HandleIncoming',2);
        if IsDlgButtonChecked(dialog,IDC_FLASHTRAY)=BST_CHECKED then WriteSettingInt(PluginLink,0,'Convers','HandleIncoming',3);

        Result:=True;

        windowmgr.ReloadOptions(otMisc);
        end;
      end;
   end;
end;

function DlgProcSendOptions(Dialog: HWnd; Message, wParam, lParam: DWord): Boolean; cdecl;
//var
//  str:string;
begin
  Result:=False;

  case message of
    WM_INITDIALOG:
      begin
      //load options

      //make sure we can save the dialog...
      SendMessage(GetParent(dialog), PSM_CHANGED, 0, 0);

      Result:=True;
      end;
    WM_COMMAND:
      begin
      end;
    WM_NOTIFY:
      begin
      if PNMHdr(lParam)^.code = PSN_KILLACTIVE then//check if all values are ok
        ;
      if PNMHdr(lParam)^.code = PSN_APPLY then//save settings
        begin
        Result:=True;

        windowmgr.ReloadOptions(otSend);
        end;
      end;
   end;
end;

end.
