{***************************************************************
 * Project     : convers
 * Unit Name   : msgform
 * Description : The Message Form
 * Author      : Christian Kästner
 * Date        : 24.05.2001
 *
 * Copyright © 2001 by Christian Kästner
 ****************************************************************}

unit msgform;

interface

uses
  Windows, Messages, SysUtils, Classes, Graphics, Controls, Forms, Dialogs,
  StdCtrls, ExtCtrls,Placemnt,globals,m_clist,clisttools,statusmodes,skintools,
  m_skin,databasetools,m_icq,m_database, Menus,m_history,newpluginapi,
  m_userinfo, TB97Ctls, ComCtrls, Aligrid,misc,timeoutfrm,m_email;



type
  TMsgWindow = class(TForm)
    Panel1: TPanel;
    Panel2: TPanel;
    SendMemo: TMemo;
    Splitter: TSplitter;
    CharCountLabel: TLabel;
    SplitLabel: TLabel;
    SendTimer: TTimer;
    UserMenu: TPopupMenu;
    HistoryMenuItem: TMenuItem;
    UserDetailsMenuItem: TMenuItem;
    AddUserMenuItem: TMenuItem;
    QLabel: TLabel;
    CancelBtn: TToolbarButton97;
    UserBtn: TToolbarButton97;
    SendBtn: TToolbarButton97;
    SendMenu: TPopupMenu;
    SendDefaultWayItem: TMenuItem;
    SendThroughServerItem: TMenuItem;
    SendDirectItem: TMenuItem;
    SendBestWayItem: TMenuItem;
    FlashTimer: TTimer;
    SendFiles1: TMenuItem;
    UserSendEMailMenuItem: TMenuItem;
    TabEnterWorkAroundBtn: TButton;
    //misc frontend
    procedure SendTimerTimer(Sender: TObject);
    procedure HistoryMenuItemClick(Sender: TObject);
    procedure UserDetailsMenuItemClick(Sender: TObject);
    procedure AddUserMenuItemClick(Sender: TObject);
    procedure CancelBtnClick(Sender: TObject);
    procedure SendBtnClick(Sender: TObject);
    procedure SendMemoChange(Sender: TObject);
    procedure SetSplitLargeMessages(const Value: Boolean);
    procedure SendWayItemClick(Sender: TObject);
    procedure FlashTimerTimer(Sender: TObject);
    procedure UserSendEMailMenuItemClick(Sender: TObject);
    procedure FormKeyDown(Sender: TObject; var Key: Word;
      Shift: TShiftState);
    procedure FormKeyUp(Sender: TObject; var Key: Word;
      Shift: TShiftState);
    procedure SendMemoKeyDown(Sender: TObject; var Key: Word;
      Shift: TShiftState);
    procedure SendMemoKeyUp(Sender: TObject; var Key: Word;
      Shift: TShiftState);

    //create close free
    procedure FormCreate(Sender: TObject);
    procedure FormCloseQuery(Sender: TObject; var CanClose: Boolean);
    procedure FormClose(Sender: TObject; var Action: TCloseAction);
    procedure FormDestroy(Sender: TObject);
    procedure FormResize(Sender: TObject);
    procedure TabEnterWorkAroundBtnClick(Sender: TObject);
  private
    HistoryMemo:TWinControl;//basic class for memo, richedit and grid
  private//options
    FSplitLargeMessages: Boolean;
    fSavePosition:Boolean;
    fHandleIncoming:Integer;
    fDoubleEnter:Boolean;
    fSingleEnter:Boolean;
    fSendTimeout:Integer;

    fDisplayMode:TDisplayMode;

    fMemoTextPattern:string;
    fRichTextPattern:string;
    fRichSettings:TRichEditSettings;
    fGridSettings:TGridEditSettings;
  private
    SendMessageQueue:TList;
    fAutoRetry:Integer;
    fLastSendID:Integer;
    fHookSendMessage:THandle;
    fUIN:integer;
    fEnterCount:Integer;
    fTabCount:Integer;
    fLastEnter:TDateTime;
    property SplitLargeMessages:Boolean read FSplitLargeMessages write SetSplitLargeMessages;
    function GetUserNick(hContact:THandle):string;
    procedure ShowSplit(show:Boolean);

    procedure WMSysCommand(var Message: TWMSysCommand); message WM_SYSCOMMAND;
    procedure OnMessageSend(var Message: TMessage); message HM_EVENTSENT;
    procedure OnMouseActivate(var Message: TMessage); message WM_MOUSEACTIVATE;
    procedure OnActivate(var Message: TWMActivate); message WM_ACTIVATE;
    procedure OnCNChar(var Message: TWMChar); message WM_CHAR;


    procedure pShowMessage(Text: string; time: Cardinal; username:string;incoming:Boolean;isRecent:Boolean=False);

    procedure AddMessageToSendQueue(text:PChar;SendWay:Integer=ISMF_ROUTE_DEFAULT);
    procedure SendMessageFromSendQueue(ForceThroughServer:Boolean=False);
    procedure DeleteFirstSendMessageQueueItem;
    procedure SendedFirstSendMessageQueueItem;
    procedure AllMessagesSended;
  public
    hContact:THandle;
    fCloseWindowAfterSend:Boolean;
    WindowManager:Pointer;
    function UIN:integer;

    procedure LoadRecentMessages(ExceptLast:Boolean=False);

    procedure ShowUserStatus(ICQStatus:Integer);
    procedure ShowMessage(Msg:TIcqMessage);
    procedure AddEmptyLine;
    procedure ActivateWindow;
    procedure BringWindowToFront;

    procedure LoadOptions;
    procedure ReloadOptions;
    procedure SaveOptions;
    procedure SavePos;
    procedure LoadPos;

    constructor CreateUID(AOwner: TComponent; hContact:THandle);
  end;

function ServiceExists(PluginLink:TPluginLink;ServiceName:PChar):Boolean;

implementation

uses windowmanager,optionfrm;

{$R *.DFM}

//***************** Create and Free ********************************************

constructor TMsgWindow.CreateUID(AOwner: TComponent; hContact: THandle);
begin
  HistoryMemo:=nil;

  inherited Create(AOwner);
  Self.hContact:=hContact;

  fuin:=-1;
  fEnterCount:=0;
  fCloseWindowAfterSend:=False;
  fGridSettings.InclTime:=True;
  fSendTimeout:=DEFAULT_TIMEOUT_MSGSEND;

  LoadOptions;

  //init historymemo
  case fDisplayMode of
    dmRich:
      begin
      HistoryMemo:=TRichEdit.Create(Self);
      HistoryMemo.Parent:=Self;
      HistoryMemo.Align:=alTop;
      TRichEdit(HistoryMemo).ReadOnly:=True;
      TRichEdit(HistoryMemo).Lines.Clear;
      TRichEdit(HistoryMemo).Font.Assign(SendMemo.Font);
      TRichEdit(HistoryMemo).ScrollBars:=ssVertical;
      end;
    dmGrid:
      begin
      HistoryMemo:=TStringAlignGrid.Create(Self);
      HistoryMemo.Parent:=Self;
      HistoryMemo.Align:=alTop;
      TStringAlignGrid(HistoryMemo).Editable:=false;
      TStringAlignGrid(HistoryMemo).EditMultiline:=true;
      TStringAlignGrid(HistoryMemo).ScrollBars := ssVertical;
      TStringAlignGrid(HistoryMemo).Wordwrap := ww_wordwrap;
      TStringAlignGrid(HistoryMemo).AutoAdjustLastCol := True;
      if fGridSettings.InclTime then
        TStringAlignGrid(HistoryMemo).ColCount:=2
      else
        TStringAlignGrid(HistoryMemo).ColCount:=1;
      TStringAlignGrid(HistoryMemo).RowCount:=1;
      TStringAlignGrid(HistoryMemo).DefaultRowHeight:=16;
      TStringAlignGrid(HistoryMemo).FixedCols:=0;
      if not fGridSettings.InclTime then
        TStringAlignGrid(HistoryMemo).cells[0,0]:='Message'
      else
        begin
        TStringAlignGrid(HistoryMemo).cells[1,0]:='Message';
        if fGridSettings.InclDate then
          TStringAlignGrid(HistoryMemo).cells[0,0]:='Date/Time'
        else
          TStringAlignGrid(HistoryMemo).cells[0,0]:='Time';
        end;
      TStringAlignGrid(HistoryMemo).AllowCutnPaste:=True;
      TStringAlignGrid(HistoryMemo).Font.Assign(SendMemo.Font);
      end
    else //dmMemo
      begin
      HistoryMemo:=TMemo.Create(Self);
      HistoryMemo.Parent:=Self;
      HistoryMemo.Align:=alTop;
      TMemo(HistoryMemo).ReadOnly:=True;
      TMemo(HistoryMemo).Lines.Clear;
      TMemo(HistoryMemo).Font.Assign(SendMemo.Font);
      TMemo(HistoryMemo).ScrollBars:=ssVertical;
      end;
  end;
  Twincontrol(HistoryMemo).TabStop:=False;
  SendMemo.Height:=57;
end;

procedure TMsgWindow.FormCreate(Sender: TObject);
//OnCreate initializing and loading options
var
  email:PChar;
begin
  //add menuitems to system menu
  AppendMenu(GetSystemMenu(Handle, False), MF_SEPARATOR, $F201, '-');
  AppendMenu(GetSystemMenu(Handle, False), MF_STRING, $F210, '&Options...');
  AppendMenu(GetSystemMenu(Handle, False), MF_SEPARATOR, $F201, '-');
  AppendMenu(GetSystemMenu(Handle, False), MF_STRING, $F200, 'Plugin &Info...');

  //init message queue
  SendMessageQueue:=TList.Create;

  //hook event when message send
  fHookSendMessage:=PluginLink.HookEventMessage(ME_ICQ_EVENTSENT,Self.Handle,HM_EVENTSENT);

  //check if other modules exists for history and userdetail etc.
  HistoryMenuItem.Enabled:=ServiceExists(PluginLink,MS_HISTORY_SHOWCONTACTHISTORY);
  UserDetailsMenuItem.Enabled:=ServiceExists(PluginLink,MS_USERINFO_SHOWDIALOG);
  UserSendEMailMenuItem.Enabled:=ServiceExists(PluginLink,MS_EMAIL_SENDEMAIL);

  //check if user has specified an email, if not, don't enable send email item...
  email:='';
  if fUIN<>0 then
    email:=ReadSettingStr(PluginLink,Self.hContact,'ICQ','e-mail','');
  UserSendEMailMenuItem.Enabled:=UserSendEMailMenuItem.Enabled and (strlen(email)>0);


  //init sendmemo
  sendmemo.Lines.Clear;
  SendMemoChange(sender);

  //load options, recentmessages and pos
  LoadOptions;
  LoadPos;
end;

procedure TMsgWindow.FormCloseQuery(Sender: TObject;
  var CanClose: Boolean);
//ask if really close when there are still messages in the msgqueue.
begin
  if SendMessageQueue.Count=1 then
    CanClose:=MessageDlg('There is still a messages in the message queue. This message wont be send if you close the message window.'#13#10'Close message window nevertheless?',mtWarning,[mbyes,mbno],0)=idyes;
  if SendMessageQueue.Count>1 then
    CanClose:=MessageDlg(format('There are still %d messages in the message queue. These messages wont be send if you close the message window.'#13#10'Close message window nevertheless?',[SendMessageQueue.Count]),mtWarning,[mbyes,mbno],0)=idyes;
end;

procedure TMsgWindow.FormClose(Sender: TObject; var Action: TCloseAction);
//when closing the from free it and save position
begin
  SavePos;
  if Action<>caFree then
    Action:=caFree;
end;

procedure TMsgWindow.FormDestroy(Sender: TObject);
//when destroying form remove it from the window manager
begin
  PluginLink.UnhookEvent(fHookSendMessage);

  while SendMessageQueue.Count>0 do
    DeleteFirstSendMessageQueueItem;
  SendMessageQueue.Free;

  if Assigned(WindowManager) then
    TWindowManager(WindowManager).DeleteMsgWindow(Self);
end;




//************************** Show incoming message *****************************

procedure TMsgWindow.ShowMessage(Msg: TIcqMessage);
//shows a message (only handles TICQMessage record)
begin
  pShowMessage(Msg.Msg,Msg.Time,GetUserNick(Msg.hContact),Msg.hContact<>0,Msg.Recent);
end;


function MixColor(Color1,Color2:TColor;Amount1:Double):TColor;
var
  r,g,b:Byte;
begin
  r:=Round(getRValue(Color1)*Amount1+getRValue(Color2)*(1-Amount1));
  g:=Round(getGValue(Color1)*Amount1+getGValue(Color2)*(1-Amount1));
  b:=Round(getBValue(Color1)*Amount1+getBValue(Color2)*(1-Amount1));
  Result:=RGB(r,g,b);
end;
procedure SetFont(Font:TTextAttributes;RichEditSettings:TRichEditSettings;Id:Integer;Recent:Boolean);
begin
  if (id < Low(RichEditSettings.RichStyles)) or (id > High(RichEditSettings.RichStyles)) then
    Exit;
  Font.Color:=RichEditSettings.RichStyles[id].Color;
  if Recent then
    Font.Color:=MixColor(Font.Color,clGray,0.5);
  if RichEditSettings.RichStyles[id].Bold then
    Font.Style:=Font.Style+[fsBold]
  else
    Font.Style:=Font.Style-[fsBold];
  if RichEditSettings.RichStyles[id].Italic then
    Font.Style:=Font.Style+[fsItalic]
  else
    Font.Style:=Font.Style-[fsItalic];
end;
procedure TMsgWindow.pShowMessage(Text:string;time:Cardinal;username:string;incoming:Boolean;isRecent:Boolean=False);
//adds a message to the history_memo
var
  Grid:TStringAlignGrid;
  dText:string;
  rowcolor:tColor;
  fid:integer;
  npos,dpos,tpos:integer;
  stsel:integer;
begin
  case fDisplayMode of
    dmRich:
      begin
      TRichEdit(HistoryMemo).Lines.BeginUpdate;
      try
      dText:=fRichTextPattern;
      if dText='' then
        dText:='%NAME% (%TIME%): %TEXT%';
      dText:=StringReplace(dText,'%TEXT%',Text,[rfIgnoreCase]);
      npos:=Pos('%NAME%',UpperCase(dtext));
      dpos:=Pos('%DATE%',UpperCase(dtext));
      tpos:=Pos('%TIME%',UpperCase(dtext));

      TRichEdit(HistoryMemo).SelStart:=MaxInt;
      TRichEdit(HistoryMemo).SelLength:=0;
      stsel:=TRichEdit(HistoryMemo).SelStart;

      fid:=3;//intext
      if not incoming then
        fid:=6;//outtext
      SetFont(TRichEdit(HistoryMemo).SelAttributes,fRichSettings,fid,isRecent);

      //showtext
      TRichEdit(HistoryMemo).Lines.Add(dText);

      //replace %NAME% etc.
      while npos+dpos+tpos<>0 do
        begin
        if (npos>dpos) and (npos>tpos) then//name
          begin
          TRichEdit(HistoryMemo).SelStart:=stsel+npos-1;
          TRichEdit(HistoryMemo).SelLength:=6;
          if incoming then
            SetFont(TRichEdit(HistoryMemo).SelAttributes,fRichSettings,1,isRecent)
          else
            SetFont(TRichEdit(HistoryMemo).SelAttributes,fRichSettings,4,isRecent);
          TRichEdit(HistoryMemo).SelText:=username;
          npos:=0;
          end;
        if (dpos>npos) and (dpos>tpos) then//date
          begin
          TRichEdit(HistoryMemo).SelStart:=stsel+dpos-1;
          TRichEdit(HistoryMemo).SelLength:=6;
          if incoming then
            SetFont(TRichEdit(HistoryMemo).SelAttributes,fRichSettings,2,isRecent)
          else
            SetFont(TRichEdit(HistoryMemo).SelAttributes,fRichSettings,5,isRecent);
          TRichEdit(HistoryMemo).SelText:=MirandaFormatDateTime(PluginLink,'d',time);
          dpos:=0;
          end;
        if (tpos>dpos) and (tpos>npos) then//time
          begin
          TRichEdit(HistoryMemo).SelStart:=stsel+tpos-1;
          TRichEdit(HistoryMemo).SelLength:=6;
          if incoming then
            SetFont(TRichEdit(HistoryMemo).SelAttributes,fRichSettings,2,isRecent)
          else
            SetFont(TRichEdit(HistoryMemo).SelAttributes,fRichSettings,5,isRecent);
          TRichEdit(HistoryMemo).SelText:=MirandaFormatDateTime(PluginLink,'t',time);
          tpos:=0;
          end;
        end;

      finally
      TRichEdit(HistoryMemo).Lines.EndUpdate;
      end;
      //scroll to end
      TRichEdit(HistoryMemo).SelStart:=Length(TRichEdit(HistoryMemo).Text);
      TRichEdit(HistoryMemo).SelLength:=0;
      //some problems here to scroll to the end in windows 95/98/me
//      TRichEdit(HistoryMemo).perform(em_linescroll,0,TRichEdit(HistoryMemo).lines.Count-1)
      TRichEdit(HistoryMemo).Perform(EM_ScrollCaret, 0, 0);
      end;
    dmGrid:
      begin
      Grid:=TStringAlignGrid(HistoryMemo);
      Grid.RowCount:=Grid.RowCount+1;

      //time?
      if fGridSettings.InclTime then
        begin
        if fGridSettings.InclDate then
          Grid.Cells[0,Grid.RowCount-1]:=MirandaFormatDateTime(PluginLink,'d t',time)
        else
          Grid.Cells[0,Grid.RowCount-1]:=MirandaFormatDateTime(PluginLink,'t',time);
        Grid.Cells[1,Grid.RowCount-1]:=Text;
        end
      else
        Grid.Cells[0,Grid.RowCount-1]:=Text;

      //color for incoming or outgoing?
      if not incoming then
        rowcolor:=fGridSettings.BGColorOut
      else
        rowcolor:=fGridSettings.BGColorIn;
      //gray recent?
      if isRecent and fGridSettings.GrayRecent then
        rowcolor:=MixColor(rowcolor,clgray,0.7);
      Grid.ColorRow[Grid.RowCount-1]:=rowcolor;

      //adjust grid layout
      if Grid.RowCount=1 then
        Grid.RowCount:=2;
      Grid.FixedRows:=1;
      Grid.Font.Assign(SendMemo.Font);
      Grid.AdjustRowHeights;

      //selektieren
      grid.row:=Grid.RowCount-1;
      grid.Col:=0;
      end
    else//dmMemo
      begin
      dText:=fMemoTextPattern;
      if dText='' then
        dText:='%NAME% (%TIME%): %TEXT%';
      dText:=StringReplace(dText,'%TEXT%',Text,[rfIgnoreCase]);
      dText:=StringReplace(dText,'%TIME%',MirandaFormatDateTime(PluginLink,'t',time),[rfIgnoreCase]);
      dText:=StringReplace(dText,'%DATE%',MirandaFormatDateTime(PluginLink,'d',time),[rfIgnoreCase]);
      dText:=StringReplace(dText,'%NAME%',UserName,[rfIgnoreCase]);
      //adjust font
      TMemo(HistoryMemo).Font.assign(SendMemo.Font);
      //showtext
      TMemo(HistoryMemo).Lines.Add(dText);
      //scroll to end
      TMemo(HistoryMemo).perform(em_linescroll,0,TMemo(HistoryMemo).lines.Count)
      end;
  end;
end;

procedure TMsgWindow.AddEmptyLine;
begin
  case fDisplayMode of
    dmGrid:
    else//memo und richedit
      TCustomMemo(HistoryMemo).Lines.Add('');
  end;
end;





//************************** Send message und Message queue ********************

procedure TMsgWindow.SendBtnClick(Sender: TObject);
//send text from inputmemo. Longer messages will be splitted
var
  Text:string;
  splitter:string;
  ptext:PChar;
  Sendway:integer;
  iMaxMessageLength:integer;
begin
  Assert(Sendmemo.Lines.Text<>'','No text inserted');

  Self.ActiveControl:=SendMemo;

  Text:=Sendmemo.Lines.Text;
  Text:=trimright(Text);

  sendway:=ISMF_ROUTE_DEFAULT;
  if SendThroughServerItem.default then
    sendway:=ISMF_ROUTE_THRUSERVER;
  if SendDirectItem.default then
    sendway:=ISMF_ROUTE_DIRECT;
  if SendBestWayItem.default then
    sendway:=ISMF_ROUTE_BESTWAY;

  if sendway=ISMF_ROUTE_DIRECT then
    iMaxMessageLength:=maxint
  else
    iMaxMessageLength:=MaxMessageLength;

  if not FSplitLargeMessages and (Length(text)>iMaxMessageLength) then
    begin
    dialogs.ShowMessage(text_texttoolong);
    Exit;
    end;

  try
  while Length(Text)>iMaxMessageLength do
    begin
    splitter:=Copy(Text,1,iMaxMessageLength);
    ptext:=PChar(splitter);

    AddMessageToSendQueue(ptext,sendway);

    Text:=Copy(Text,iMaxMessageLength+1,maxint);
    end;
  ptext:=PChar(Text);
  AddMessageToSendQueue(ptext,sendway);
  except end;

  SendMemo.Lines.Clear;
  SendMemoChange(Self);

  SendMessageFromSendQueue;

  //if fCloseWindowAfterSend then minimize window and close when sent
  if fCloseWindowAfterSend then
    WindowState:=wsMinimized;
end;

procedure TMsgWindow.AddMessageToSendQueue(text:PChar;SendWay:Integer=ISMF_ROUTE_DEFAULT);
//Add new message to messagequeue for later sending
var
  p:PICQSENDMESSAGE;
begin
  New(p);
  p^.cbSize:=SizeOf(p^);
  p^.uin:=uin;
  GetMem(p^.pszMessage,strlen(Text)+1);
  strcopy(p^.pszMessage,Text);

  //send way (by default use miranda settings)
  p^.routeOverride:=SendWay;

  SendMessageQueue.Add(p);

  QLabel.Visible:=SendMessageQueue.Count>0;
end;

procedure TMsgWindow.OnMessageSend(var Message: TMessage);
//Event called by Miranda (ICQ Module) when the message was send successfully
begin
  if message.lParam=0 then
    if message.wParam=fLastSendID then
      begin
      //disable timeout timer
      SendTimer.Enabled:=False;

      //message done
      SendedFirstSendMessageQueueItem;
      DeleteFirstSendMessageQueueItem;
      //send next message if still some in queue
      SendMessageFromSendQueue;
      end;
end;

procedure TMsgWindow.SendTimerTimer(Sender: TObject);
//Sendmessage timeout
var
  p:PICQSENDMESSAGE;
  mtext:string;
begin
  SendTimer.Enabled:=False;

  if fAutoRetry>0 then
    begin//autoretry
    Dec(fAutoRetry);
    SendMessageFromSendQueue(True);
    end
  else
    begin//ask what to do...

    //get message text
    mtext:='';
    if SendMessageQueue.Count>0 then
      try
      p:=SendMessageQueue[0];
      mtext:=string(p^.pszMessage);
      except
      end;

    case MessageTimeout(mtext) of
      taRetry:
        //just send first event in message again
        SendMessageFromSendQueue;
      taRetryServer:
        //just send first event in message again but this time through server
        SendMessageFromSendQueue(True)
      else//cancel
        //chancel this message
        begin
        DeleteFirstSendMessageQueueItem;
        SendMessageFromSendQueue;
        end;
    end;
    end;
end;



procedure TMsgWindow.SendMessageFromSendQueue;
{sends the first message from the message queue
this function is called when sending a message after adding it to the queue or
when first message finished to send the next one or when user clicks retry}
var
  ism:TICQSENDMESSAGE;
begin
  if SendMessageQueue.Count>0 then
    begin
    copymemory(@ism,PICQSENDMESSAGE(SendMessageQueue[0]),SizeOf(ism));
    
    if ForceThroughServer then
      ism.routeOverride:=ISMF_ROUTE_THRUSERVER;

    fLastSendID:=PluginLink.CallService(MS_ICQ_SENDMESSAGE,0,dword(@ism));

    SendTimer.Interval:=fSendTimeout;
    SendTimer.Enabled:=True;
    end;

  QLabel.Visible:=SendMessageQueue.Count>0;
end;

procedure TMsgWindow.SendedFirstSendMessageQueueItem;
//called when successfully sended. Adds text to history and to
//upper textfield (indirect)
var
  dbei:TDBEVENTINFO;
  p:PICQSENDMESSAGE;
begin
  if SendMessageQueue.Count>0 then
    try
    p:=SendMessageQueue[0];

    dbei.cbSize:=sizeof(dbei);
    dbei.eventType:=EVENTTYPE_MESSAGE;
    dbei.flags:=DBEF_SENT;
    dbei.szModule:='ICQ';
    //convert tdatetime to seconds since 1.1.1970
    dbei.timestamp:=round((LocalToGMTTime(now)-encodedate(1970,1,1))*SecsPerDay);
    dbei.cbBlob:=strlen(p^.pszMessage)+1;
    dbei.pBlob:=PByte(p^.pszMessage);
    PluginLink.CallService(MS_DB_EVENT_ADD,Self.hContact,dword(@dbei));
    except
    end;
end;

procedure TMsgWindow.DeleteFirstSendMessageQueueItem;
//deletes first item from queue when send or when given up sending
var
  p:PICQSENDMESSAGE;
begin
  if SendMessageQueue.Count>0 then
    try
    p:=SendMessageQueue[0];
    SendMessageQueue.Delete(0);
    freemem(p^.pszMessage);
    dispose(p);
    except
    end;
  QLabel.Visible:=SendMessageQueue.Count>0;
  if SendMessageQueue.Count=0 then
    AllMessagesSended;

  //reset autoretry for next send...
  fAutoRetry:=ReadSettingInt(PluginLink,0,'Convers','AutoRetry',2);
end;

procedure TMsgWindow.AllMessagesSended;
begin
  //close window after sending if user wishes it
  if fCloseWindowAfterSend then
    if WindowState=wsMinimized then
      Close;
end;




//********************* Load and Save options and window positions *************

procedure TMsgWindow.LoadOptions;
//loading options (only once at start!)
var
  val:integer;
  s:Word;
  p:Pointer;
  uin_identifier:string;
begin
  fDisplayMode:=TDisplayMode(ReadSettingInt(PluginLink,0,'Convers','DisplayType',integer(DefaultDisplayMode)));

  ReloadOptions;

  //load grid settings (only load InclTime once because cannot change while already displaying a second column)
  s:=SizeOf(fGridSettings);
  ReadSettingBlob(PluginLink,0,'Convers','GridSettings',s, p);
  if Assigned(p) then
    begin
    fGridSettings:=PGridEditSettings(p)^;
    FreeSettingBlob(PluginLink,s, p);
    end
  else
    fGridSettings:=DefaultGridEditSettings;


  //sendway
  uin_identifier:='';
  if fSavePosition then
    uin_identifier:=IntToStr(UIN);
  val:=ReadSettingInt(PluginLink,0,'Convers',pchar('SendWay'+uin_identifier),integer(False));
  case val of
    ISMF_ROUTE_DIRECT:
      SendDirectItem.default:=true;
    ISMF_ROUTE_THRUSERVER:
      SendThroughServerItem.default:=true;
    ISMF_ROUTE_BESTWAY:
      SendBestWayItem.default:=true;
  else
    SendDefaultWayItem.default:=true;
  end;
end;

procedure TMsgWindow.ReloadOptions;
//once at start and any time the options change
var
  val:integer;
  s:Word;
  p:Pointer;
  it:Boolean;
begin
  //misc
  val:=ReadSettingInt(PluginLink,0,'Convers','SplitLargeMessages',integer(False));
  SplitLargeMessages:=Boolean(val);
  val:=ReadSettingInt(PluginLink,0,'Convers','StorePositions',integer(True));
  fSavePosition:=Boolean(val);
  val:=ReadSettingInt(PluginLink,0,'Convers','DoubleEnterSend',integer(False));
  fDoubleEnter:=Boolean(val);
  val:=ReadSettingInt(PluginLink,0,'Convers','SingleEnterSend',integer(False));
  fSingleEnter:=Boolean(val);
  val:=ReadSettingInt(PluginLink,0,'Convers','CloseWindowAfterSend',integer(False));
  fCloseWindowAfterSend:=Boolean(val);
  val:=ReadSettingInt(PluginLink,0,'Convers','HandleIncoming',0);
  fHandleIncoming:=val;
  val:=ReadSettingInt(PluginLink,0,'Convers','SendTimeout',DEFAULT_TIMEOUT_MSGSEND);
  fSendTimeout:=val;



  //font for sendmemo
  s:=SizeOf(LogFont);
  ReadSettingBlob(PluginLink,0,'Convers','Font',s, p);
  if Assigned(p) then
    begin
    SendMemo.Font.Handle:=CreateFontIndirect(PLogFont(p)^);
    SendMemo.Font.Color:=TColor(ReadSettingInt(PluginLink,0,'Convers','FontCol',Integer(clBlack)));
    FreeSettingBlob(PluginLink,s, p);
    end;

  //load settings for memo
  fMemoTextPattern:=string(ReadSettingStr(PluginLink,0,'Convers','MemoTextPattern','%NAME% (%TIME%): %TEXT%'));

  //load rich settings
  fRichTextPattern:=string(ReadSettingStr(PluginLink,0,'Convers','RichTextPattern','%NAME% (%TIME%): %TEXT%'));

  s:=SizeOf(fRichSettings);
  ReadSettingBlob(PluginLink,0,'Convers','RichSettings',s, p);
  if Assigned(p) then
    begin
    fRichSettings:=PRichEditSettings(p)^;
    FreeSettingBlob(PluginLink,s, p);
    end
  else
    fRichSettings:=DefaultRichEditSettings;
  if fDisplayMode=dmRich then
    begin
    SendMemo.Font.Color:=fRichSettings.RichStyles[0].Color;
    if fRichSettings.RichStyles[0].Bold then
      SendMemo.Font.Style:=SendMemo.Font.Style+[fsBold]
    else
      SendMemo.Font.Style:=SendMemo.Font.Style-[fsBold];
    if fRichSettings.RichStyles[0].Italic then
      SendMemo.Font.Style:=SendMemo.Font.Style+[fsItalic]
    else
      SendMemo.Font.Style:=SendMemo.Font.Style-[fsItalic];
    end;

  //load grid settings (except incltime! incltime only load one time!)
  it:=fGridSettings.InclTime;
  s:=SizeOf(fGridSettings);
  ReadSettingBlob(PluginLink,0,'Convers','GridSettings',s, p);
  if Assigned(p) then
    begin
    fGridSettings:=PGridEditSettings(p)^;
    FreeSettingBlob(PluginLink,s, p);
    end
  else
    fGridSettings:=DefaultGridEditSettings;
  fGridSettings.InclTime:=it;
end;

procedure TMsgWindow.SaveOptions;
//save those very few options which can be selected directly in this form...
var
  SendWay:integer;
  uin_identifier:string;
begin
  uin_identifier:='';
  if fSavePosition then
    uin_identifier:=IntToStr(UIN);

  SendWay:=ISMF_ROUTE_DEFAULT;
  if SendThroughServerItem.default then
    SendWay:=ISMF_ROUTE_THRUSERVER;
  if SendDirectItem.default then
    SendWay:=ISMF_ROUTE_DIRECT;
  if SendBestWayItem.default then
    SendWay:=ISMF_ROUTE_BESTWAY;

  WriteSettingInt(PluginLink,hContact,'Convers',pchar('SendWay'+uin_identifier),SendWay);
end;

procedure TMsgWindow.LoadPos;
//load save position from message window
var
  val:integer;
  uin_identifier:string;
begin
  uin_identifier:='';
  if fSavePosition then
    uin_identifier:=IntToStr(UIN);

  val:=ReadSettingInt(PluginLink,hContact,'Convers',pchar('pos'+uin_identifier+'l'),maxint);
  if val<>MaxInt then
    Self.Left:=val;
  val:=ReadSettingInt(PluginLink,hContact,'Convers',pchar('pos'+uin_identifier+'t'),maxint);
  if val<>MaxInt then
    Self.Top:=val;
  val:=ReadSettingInt(PluginLink,hContact,'Convers',pchar('pos'+uin_identifier+'r'),maxint);
  if val<>MaxInt then
    Self.Width:=val-Self.Left;
  val:=ReadSettingInt(PluginLink,hContact,'Convers',pchar('pos'+uin_identifier+'b'),maxint);
  if val<>MaxInt then
    if val-Self.Top>100 then
      Self.Height:=val-Self.Top;
  val:=ReadSettingInt(PluginLink,hContact,'Convers',pchar('pos'+uin_identifier+'hm'),maxint);
  if val<>MaxInt then
    begin
    if (val>Self.Height) or (val<10) then
      val:=57;
    HistoryMemo.Height:=val;
    end;
end;

procedure TMsgWindow.SavePos;
//save message window position
var
  uin_identifier:string;
begin
  uin_identifier:='';
  if fSavePosition then
    uin_identifier:=IntToStr(UIN);

  WriteSettingInt(PluginLink,hContact,'Convers',pchar('pos'+uin_identifier+'l'),Self.left);
  WriteSettingInt(PluginLink,hContact,'Convers',pchar('pos'+uin_identifier+'t'),Self.Top);
  WriteSettingInt(PluginLink,hContact,'Convers',pchar('pos'+uin_identifier+'r'),Self.Left+Self.Width);
  WriteSettingInt(PluginLink,hContact,'Convers',pchar('pos'+uin_identifier+'b'),Self.Top+Self.Height);
  WriteSettingInt(PluginLink,hContact,'Convers',pchar('pos'+uin_identifier+'hm'),HistoryMemo.Height);
end;




//********************* Load recent messages ***********************************

procedure TMsgWindow.LoadRecentMessages(ExceptLast:Boolean=False);
{loads recent messages to the historymemo when showing the window if user
selects it in the options}
var
  RMCount:Integer;
  hDbEvent:THandle;
  BlobSize:Integer;
  DbEventInfo:TDBEVENTINFO;
  Msg:TIcqMessage;
  hDbEventList:TList;
  i:integer;
begin
  if Boolean(ReadSettingInt(PluginLink,0,'Convers','LoadRecentMsgs',integer(false))) then
    begin
    RMCount:=ReadSettingInt(PluginLink,0,'Convers','LoadRecentMsgCount',10);
    if RMCount<1 then
      RMCount:=10;

    //load list of all DBEventHandles to get the last messages later
    hDbEventList:=Tlist.Create;
    try
    hDbEvent:=PluginLink.CallService(MS_DB_EVENT_FINDFIRST,hContact,0);
    while Assigned(Pointer(hDbEvent)) do
      begin
      hDbEventList.Add(Pointer(hDbEvent));
      hDbEvent:=PluginLink.CallService(MS_DB_EVENT_FINDNEXT,hDbEvent,0);
      end;

    //add last RMCount messages to message window
    for i:=hDbEventList.Count-1-RMCount to hDbEventList.Count-1-Integer(ExceptLast) do
      if i>=0 then
      begin
      hDbEvent:=dword(hDbEventList[i]);
      //receive message
      DbEventInfo.cbSize:=SizeOf(DbEventInfo);
      DbEventInfo.pBlob:=nil;

      BlobSize:=PluginLink.CallService(MS_DB_EVENT_GETBLOBSIZE,hDbEvent,0);

      GetMem(DbEventInfo.pBlob,BlobSize);
      DbEventInfo.cbBlob:=BlobSize;

      PluginLink.CallService(MS_DB_EVENT_GET,hDbEvent,Integer(@DbEventInfo));

      Msg.MSGTYPE:=DbEventInfo.eventType;
      if (DbEventInfo.flags and DBEF_SENT)>0 then
        Msg.hContact:=0
      else
        Msg.hContact:=hContact;
      Msg.Msg:=string(PChar(DbEventInfo.pBlob));
      Msg.time:=DbEventInfo.timestamp;
      Msg.Recent:=True;
      ShowMessage(Msg);

      if Assigned(DbEventInfo.pBlob) then
        begin
        freemem(DbEventInfo.pBlob);
        DbEventInfo.pBlob:=nil;
        end;
      end;

    AddEmptyLine;

    finally
    hDbEventList.Free;
    end;
    end;
end;



//***************************** Interface stuff ********************************


procedure TMsgWindow.ShowUserStatus;
//Adjust Form.Caption and icon everytime the user status changes
//the parameter is optional, set to -1 if the plugin should find out itself
var
  statustext:string;
  nickname:string;
begin
  nickname:=GetUserNick(hContact);

  if ICQStatus<0 then
    begin
    statustext:=icq_ConvertStatus2Str(MirandaStatusToIcq(GetContactStatus(PluginLink,hContact)));
    //icon
    SendMessage(Self.Handle, WM_SETICON, ICON_BIG, LoadSkinnedIcon(PluginLink,MirandaStatusToIconID(GetContactStatus(PluginLink,hContact))));
    end
  else
    begin
    statustext:=icq_ConvertStatus2Str(MirandaStatusToICQ(ICQStatus));
    //icon
    SendMessage(Self.Handle, WM_SETICON, ICON_BIG, LoadSkinnedIcon(PluginLink,MirandaStatusToIconID(ICQStatus)));
    end;

  Self.Caption:=Format('%s (%d) %s',[
    nickname,uin,statustext]);

  AddUserMenuItem.Visible:=(ReadSettingInt(PluginLink,hContact,'CList','NotOnList',0)<>0);
end;


procedure TMsgWindow.SendMemoChange(Sender: TObject);
//Adjust Character Count field
var
  textlength:integer;
  Text:string;
begin
  Text:=sendmemo.Lines.Text;
  if fDoubleEnter or fSingleEnter then
    Text:=trimright(Text);
  textlength:=Length(text);
  CharCountLabel.Caption:=Format(text_chard,[textlength]);
  ShowSplit(textlength>MaxMessageLength);
  SendBtn.Enabled:=SendMemo.Enabled and (textlength<>0);
end;

procedure TMsgWindow.CancelBtnClick(Sender: TObject);
//cancel dialog
begin
  Close;
end;

procedure TMsgWindow.SetSplitLargeMessages(const Value: Boolean);
//can split larger messages?
//no maxlength in any case because might be send direct...
begin
  FSplitLargeMessages := Value;
  SendMemoChange(self);
end;

procedure TMsgWindow.ShowSplit(show: Boolean);
//shows "split" label or not
begin
  if show<>splitlabel.Visible then
    begin
    splitlabel.Visible:=show;
    if show then
      CharCountLabel.Top:=3
    else
      CharCountLabel.Top:=8;
    end;
end;

procedure TMsgWindow.BringWindowToFront;
//bring message to front (if set in options)
begin
  Visible:=True;
  if WindowState=wsMinimized then
    WindowState:=wsNormal;
  Self.bringtofront;
end;

procedure TMsgWindow.ActivateWindow;
//show window, depending on show method specified in the options...
var
  cle:TCLISTEVENT;
const
  FlashInterval=400;
begin
  case fHandleIncoming of
    0://popup steal focus
      begin
      Self.Visible:=True;
      ShowWindow(Self.Handle,SW_SHOWNORMAL);
      Self.BringToFront;
      end;
    1://show but without focus
      begin
      Self.Visible:=True;
      ShowWindow(Self.Handle,SW_SHOWNOACTIVATE);
      end;
    2://flash window
      begin
      Self.Visible:=True;
      ShowWindow(Self.Handle,SW_SHOWNOACTIVATE);

      FlashTimerTimer(FlashTimer);

      FlashTimer.Interval:=FlashInterval;
      FlashTimer.Enabled:=True;
      end;
    3://blink on contact list
      begin
      cle.cbSize:=sizeof(cle);
      cle.hContact:=Self.hContact;
      cle.hDbEvent:=blinkid;
      cle.lParam:=blinkid;
      cle.hIcon:=LoadSkinnedIcon(PluginLink,SKINICON_EVENT_MESSAGE);
      cle.pszService:=PluginService_ReadBlinkMessage;
      cle.pszTooltip:=pchar('Message from '+GetUserNick(Self.hContact));
      PluginLink.CallService(MS_CLIST_ADDEVENT,blinkid,dword(@cle));
      end;
  end;
end;

procedure TMsgWindow.FlashTimerTimer(Sender: TObject);
//flash window until disabled in OnMouseActivate or OnActivate
begin
  FlashWindow(Self.Handle,True);
end;

procedure TMsgWindow.OnMouseActivate(var Message: TMessage);
//disable flashing
begin
  inherited;
  if FlashTimer.Enabled then
    begin
    FlashTimer.Enabled:=False;
    FlashWindow(Self.Handle,False);
    end;
end;

procedure TMsgWindow.OnActivate(var Message: TWMActivate);
//disable flashing
begin
  inherited;
  if FlashTimer.Enabled then
    begin
    FlashTimer.Enabled:=False;
    FlashWindow(Self.Handle,False);
    end;
end;

procedure TMsgWindow.WMSysCommand(var Message: TWMSysCommand);
//show info- and optiondialog
begin
  inherited;
  if Message.CmdType and $FFF0 = $F200 then
    MessageDlg(PluginInfo.shortname+' V '+PluginInfoVersion+#13#10#13#10+PluginInfo.description+
      #13#10#13#10'by '+PluginInfo.author+' ('+PluginInfo.authorEmail+')'#13#10+
      PluginInfo.copyright+#13#10#13#10+PluginInfo.homepage,mtInformation,
      [mbok],0);
  if Message.CmdType and $FFF0 = $F210 then
    with TOptionForm.Create(nil) do
      try
      hContact:=Self.hContact;
      ShowModal;
      TWindowManager(WindowManager).ReloadOptions;
      finally
      Free;
      end;
end;

procedure TMsgWindow.HistoryMenuItemClick(Sender: TObject);
//show history window
begin
  PluginLink.CallService(MS_HISTORY_SHOWCONTACTHISTORY,Self.hContact,0);
end;

procedure TMsgWindow.UserDetailsMenuItemClick(Sender: TObject);
//show user details window
begin
  PluginLink.CallService(MS_USERINFO_SHOWDIALOG,Self.hContact,0);
end;

procedure TMsgWindow.AddUserMenuItemClick(Sender: TObject);
//add new user (incomplete)
//var
//  hnewcontact:DWord;
var
  cgs:TDBCONTACTGETSETTING;
begin
  cgs.szModule:='CList';
  cgs.szSetting:='NotOnList';
  PluginLink.CallService(MS_DB_CONTACT_DELETESETTING,Self.hContact,dword(@cgs));
  AddUserMenuItem.Visible:=False;
end;

procedure TMsgWindow.UserSendEMailMenuItemClick(Sender: TObject);
//send email menu item clicked.
begin
  PluginLink.CallService(MS_EMAIL_SENDEMAIL,Self.hContact,0);
end;

procedure TMsgWindow.FormResize(Sender: TObject);
//adjust histroymemo on window resize
begin
  if fDisplayMode=dmGrid then
    TStringAlignGrid(HistoryMemo).AdjustRowHeights;
  if fDisplayMode=dmRich then
    TRichEdit(HistoryMemo).Refresh;
end;

procedure TMsgWindow.SendWayItemClick(Sender: TObject);
//send in a special way, when menu item clicked
begin
  TMenuItem(Sender).default:=True;
  SendBtnClick(Self);
end;


//******************** Shortcuts for message sending (interface stuff) *********
{Unfortunatly when you make a form from a dll this form won't become the
normal messages specified by the VCL but only the basic windows messages.
Therefore neither tabs nor button shortcuts work on this form. As a workaround
i've make some functions:}

procedure TMsgWindow.SendMemoKeyDown(Sender: TObject; var Key: Word;
  Shift: TShiftState);
//count returns
begin
  if (key = vk_return) and not (ssShift in Shift) then
    begin
    //time between two enters is 3 seks max
    if (time-fLastEnter)*SecsPerDay>3 then
      fEnterCount:=0;

    Inc(fEnterCount);
    fLastEnter:=time;
    key:=0;
    end
  else
    if (key=vk_tab) then
      Inc(fTabCount)
    else
      begin
      fEnterCount:=0;
      fTabCount:=0;
      end;
end;

procedure TMsgWindow.SendMemoKeyUp(Sender: TObject; var Key: Word;
  Shift: TShiftState);
//handle single and double enter
begin
  if fSingleEnter and (fEnterCount=1) then
    begin
    key:=0;
    if SendBtn.Enabled then
      SendBtnClick(Sender);
    end;

  if fDoubleEnter and (fEnterCount=2) then
    begin
    key:=0;
    if SendBtn.Enabled then
      SendBtnClick(Sender);
    end;
end;

procedure TMsgWindow.FormKeyDown(Sender: TObject; var Key: Word;
  Shift: TShiftState);
var
  Mask: Integer;
begin
  with Sender as TWinControl do
    begin
      if Perform(CM_CHILDKEY, Key, Integer(Sender)) <> 0 then
        Exit;
      Mask := 0;
      case Key of
        VK_TAB:
          Mask := DLGC_WANTTAB;
        VK_RETURN, VK_EXECUTE, VK_ESCAPE, VK_CANCEL:
          Mask := DLGC_WANTALLKEYS;
      end;
      if (Mask <> 0)
        and (Perform(CM_WANTSPECIALKEY, Key, 0) = 0)
        and (Perform(WM_GETDLGCODE, 0, 0) and Mask = 0)
        and (Self.Perform(CM_DIALOGKEY, Key, 0) <> 0)
        then Exit;
    end;
end;

procedure TMsgWindow.OnCNChar(var Message: TWMChar);
begin
  if not (csDesigning in ComponentState) then
    with Message do
    begin
      Result := 1;
      if (Perform(WM_GETDLGCODE, 0, 0) and DLGC_WANTCHARS = 0) and
        (GetParentForm(Self).Perform(CM_DIALOGCHAR,
        CharCode, KeyData) <> 0) then Exit;
      Result := 0;
    end;
end;


procedure TMsgWindow.FormKeyUp(Sender: TObject; var Key: Word;
  Shift: TShiftState);
begin
  //STRG+ENTER and STRG+S shortcut for sending
  if (ssCtrl in Shift) then
    if (key in [vk_return,ord('S')]) then
      begin
      key:=0;
      if SendBtn.Enabled then
        SendBtnClick(Sender);
      end;

  if (ssAlt in Shift) then
    begin
    if key in [ord('s'),Ord('S')] then
      if SendBtn.Enabled then
        SendBtnClick(Sender);
    if key=Ord('U') then
      UserBtn.Click;
    if key=Ord('C') then
      CancelBtn.Click;
    key:=0;
    end;
end;


//************************** Misc tool functions *******************************

function TMsgWindow.GetUserNick(hContact: THandle): string;
//get Nickname from hContact
var
  contactname:PChar;
  p:integer;
begin
  Result:='';

  try
  //may change, therefore load everytime it's needed
  p:=PluginLink.CallService(MS_CLIST_GETCONTACTDISPLAYNAME,DWord(hContact),0);
  contactname:=PChar(p);
  while contactname^<>#0 do
    begin
    Result:=Result+contactname^;
    Inc(contactname);
    end;
  except
    on e:Exception do
      Result:='Error '+e.classname+'-'+e.message;
  end;
end;

function TMsgWindow.UIN: integer;
begin
  //load uin only once as it does not change
  if fuin=-1 then
    begin
    fuin:=GetContactUin(PluginLink,hContact);
    end;
  Result:=fUIN;
end;






function DummyService(wParam,lParam:DWord):integer;cdecl;begin result:=0;end;
function ServiceExists(PluginLink:TPluginLink;ServiceName:PChar):Boolean;
//test if service exists
var
  p:DWord;
begin
  //if version 0.1.0.1 then call serviceexists, otherwise work around
  if CompareVersion(mirandaversion,PLUGIN_MAKE_VERSION(0,1,0,1))>=0 then
    begin
    Result:=PluginLink.ServiceExists(ServiceName)<>0;
    end
  else
    begin
    Result:=True;
    p:=PluginLink.CreateServiceFunction(ServiceName,DummyService);
    if p<>0 then
      begin
      Result:=False;
      PluginLink.DestroyServiceFunction(p);
      end;
    end;
end;














procedure TMsgWindow.TabEnterWorkAroundBtnClick(Sender: TObject);
//just a work around that tabenter and tab space work because the send
//button cannot get the focus as it is a speedbutton
begin
  SendBtn.Click;
end;


function DlgProcIgnoreOpts(Dialog: HWnd; Message, WParam: LongWord;
  LParam: Longint): Boolean; export;
begin
  Result:=True;
end;



end.
