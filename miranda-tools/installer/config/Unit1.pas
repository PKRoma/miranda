unit Unit1;

interface

uses
  XPTheme, Windows, Messages, SysUtils, Classes, Controls, Forms, Dialogs,
  StdCtrls, ExtCtrls, ComCtrls, IniFiles, ShellAPI;

type

  TForm1 = class(TForm)
    ListView1: TListView;
    ImageList1: TImageList;
    Bevel1: TBevel;
    panelWelcome: TPanel;
    panelICQ: TPanel;
    Memo1: TMemo;
    Label1: TLabel;
    Panel1: TPanel;
    Bevel2: TBevel;
    btnBack: TButton;
    btnNext: TButton;
    Label2: TLabel;
    setIcqOnline: TCheckBox;
    icqUIN: TEdit;
    icqPass: TEdit;
    Label3: TLabel;
    Label4: TLabel;
    Label5: TLabel;
    Label6: TLabel;
    Bevel3: TBevel;
    Label7: TLabel;
    Label8: TLabel;
    Bevel4: TBevel;
    Label9: TLabel;
    Label10: TLabel;
    Label11: TLabel;
    btnFinished: TButton;
    panelAIM: TPanel;
    setAIMOnline: TCheckBox;
    Label12: TLabel;
    Bevel5: TBevel;
    setAIM_SN: TEdit;
    Label13: TLabel;
    setAIM_PW: TEdit;
    Label14: TLabel;
    Label15: TLabel;
    Label16: TLabel;
    setAIM_DN: TEdit;
    Label17: TLabel;
    Label18: TLabel;
    Bevel6: TBevel;
    Label19: TLabel;
    Label20: TLabel;
    Label21: TLabel;
    Label22: TLabel;
    panelMSN: TPanel;
    setMSNOnline: TCheckBox;
    Label23: TLabel;
    Bevel7: TBevel;
    setMSN_UN: TEdit;
    setMSN_PW: TEdit;
    Label24: TLabel;
    Label25: TLabel;
    Label26: TLabel;
    Label27: TLabel;
    setMSN_DN: TEdit;
    Label28: TLabel;
    Label29: TLabel;
    Bevel8: TBevel;
    Label30: TLabel;
    Label31: TLabel;
    Label32: TLabel;
    Label33: TLabel;
    panelYahoo: TPanel;
    Bevel9: TBevel;
    Label34: TLabel;
    setYahooOnline: TCheckBox;
    setYahoo_UN: TEdit;
    Label35: TLabel;
    Bevel10: TBevel;
    Label36: TLabel;
    Label37: TLabel;
    setYahoo_PW: TEdit;
    Label38: TLabel;
    Label39: TLabel;
    Label40: TLabel;
    Label41: TLabel;
    Label42: TLabel;
    panelJabber: TPanel;
    Bevel11: TBevel;
    Label43: TLabel;
    setJabberOnline: TCheckBox;
    setJabber_UN: TEdit;
    Label44: TLabel;
    Label45: TLabel;
    setJabber_PW: TEdit;
    Label46: TLabel;
    setJabber_SV: TEdit;
    Label47: TLabel;
    Label48: TLabel;
    Label49: TLabel;
    Bevel12: TBevel;
    Label50: TLabel;
    Label51: TLabel;
    Label52: TLabel;
    Label53: TLabel;
    Label54: TLabel;
    panelReport: TPanel;
    Label56: TLabel;
    panelIRC: TPanel;
    Bevel13: TBevel;
    Label55: TLabel;
    Label57: TLabel;
    Label58: TLabel;
    Label60: TLabel;
    Label61: TLabel;
    setIRCOnline: TCheckBox;
    setIRC_DN: TEdit;
    setIRC_SV: TEdit;
    procedure FormCreate(Sender: TObject);
    procedure FormResize(Sender: TObject);
    procedure btnNextClick(Sender: TObject);
    procedure ListView1Change(Sender: TObject; Item: TListItem;
      Change: TItemChange);
    procedure FormActivate(Sender: TObject);
    procedure btnBackClick(Sender: TObject);
    procedure setIcqOnlineClick(Sender: TObject);
    procedure icqUINKeyPress(Sender: TObject; var Key: Char);
    procedure Label6Click(Sender: TObject);
    procedure Label10Click(Sender: TObject);
    procedure setAIMOnlineClick(Sender: TObject);
    procedure Label15Click(Sender: TObject);
    procedure ListView1Click(Sender: TObject);
    procedure Label20Click(Sender: TObject);
    procedure setMSNOnlineClick(Sender: TObject);
    procedure Label27Click(Sender: TObject);
    procedure Label32Click(Sender: TObject);
    procedure setYahooOnlineClick(Sender: TObject);
    procedure Label41Click(Sender: TObject);
    procedure Label39Click(Sender: TObject);
    procedure setJabberOnlineClick(Sender: TObject);
    procedure Label49Click(Sender: TObject);
    procedure Label51Click(Sender: TObject);
    procedure Label54Click(Sender: TObject);
    procedure setJabber_UNChange(Sender: TObject);
    procedure setIRCOnlineClick(Sender: TObject);
    procedure btnFinishedClick(Sender: TObject);
    procedure ListView1MouseMove(Sender: TObject; Shift: TShiftState; X,
      Y: Integer);
  private
    { Private declarations }
  public
    { Public declarations }
  end;

var
  Form1: TForm1;

implementation

{$R *.DFM}

function miranda_password_encode(s: string): string;
var
   sz: PChar;
begin
     sz := PChar(s);
     while sz^ <> #0 do
     begin
          Byte(sz^) := Ord(sz^) + 5;
          inc(sz);
     end;
     result := s;
end;

function doICQReport(var report: TStringList): Integer;
var
   uname, pass: string;
begin
     Result := 0;

     uname := Form1.ICQUIN.Text;
     pass := Form1.ICQPass.Text;

     if (Length(uname) = 0) or (Length(pass) = 0) then Form1.setICQOnline.Checked := False;

     if not Form1.setICQOnline.Checked then // no icq?
     begin
          report.Add('; No ICQ');
          report.Add('[PluginDisable]');
          report.Add('icq.dll=b1');
          report.Add('import.dll=b1');
          exit;
     end;

     report.Add('[ICQ]');
     report.Add('UIN=d' + uname);
     report.Add('Password=s' + miranda_password_encode(pass));
     report.Add('FirstRun=b1');

     report.Add('; Make sure all the ICQ related components are enabled (jic)');
     report.Add('[PluginDisable]');
     report.Add('icq.dll=l');
     report.Add('import.dll=l');
     Result := 1;
end;

function doAIMReport(var report: TStringList): Integer;
var
   uname, pass, dn: string;
begin
     Result := 0;

     uname := Form1.setAIM_SN.Text;
     pass := Form1.setAIM_PW.Text;
     dn := Form1.setAIM_DN.Text;

     if (Length(uname) = 0) or (Length(pass) = 0) then Form1.setAIMOnline.Checked := False;

     if not Form1.setAIMOnline.Checked then
     begin
          report.Add('; No AIM');
          report.Add('[PluginDisable]');
          report.Add('aim.dll=b1');
          Exit;
     end;

     report.Add('[AIM]');
     report.Add('SN=s' + uname);
     report.Add('Password=s' + miranda_password_encode(pass));
     report.Add('Nick=s' + DN);

     report.Add('; Make sure AIM is enabled (jic)');
     report.Add('[PluginDisable]');
     report.Add('aim.dll=l');
     Result := 1;
end;

function doMSNReport(var report: TStringList): Integer;
var
   uname, pass, dn: string;
begin
     Result := 0;

     uname := Form1.setMSN_UN.Text;
     pass := Form1.setMSN_PW.Text;
     dn := Form1.setMSN_DN.Text;

     if (Length(uname) = 0 ) or (Length(pass) = 0) then Form1.setMSNOnline.Checked := False;

     if not Form1.setMSNOnline.Checked then
     begin
          report.Add('; No MSN');
          report.Add('[PluginDisable]');
          report.Add('msn.dll=b1');
          exit;
     end;

     report.Add('[MSN]');
     report.Add('e-mail=s' + uname);
     report.Add('Password=s' + miranda_password_encode(pass));
     report.Add('Nick=s' + dn);

     report.Add('; Make sure MSN is enabled (jic)');
     report.Add('[PluginDisable]');
     report.Add('msn.dll=l');

     Result := 1;
end;

function doYahooReport(var report: TStringList): Integer;
var
   uname, pass, dn: string;
begin
     Result := 0;

     uname := Form1.setYahoo_UN.Text;
     pass := Form1.setYahoo_PW.Text;

     if (Length(uname) = 0) or (Length(pass) = 0) then Form1.setYahooOnline.Checked := False;

     if not Form1.setYahooOnline.Checked then
     begin
          report.Add('; No Yahoo');
          report.Add('[PluginDisable]');
          report.Add('yahoo.dll=b1');
          exit;
     end;

     report.Add('[YAHOO]');
     report.Add('yahoo_id=s' + uname);
     report.Add('Password=s' + miranda_password_encode(pass));

     report.Add('; Make sure Yahoo is enabled (jic)');
     report.Add('[PluginDisable]');
     report.Add('yahoo.dll=l');

     Result := 1;
end;

function doJabberReport(var report: TStringList): Integer;
var
   uname, pass, server: string;
   j: integer;
begin
     Result := 0;

     uname := Form1.setJabber_UN.Text;
     pass := Form1.setJabber_PW.Text;
     server := Form1.setJabber_SV.Text;

     if (Length(uname) = 0) or (Length(pass) = 0) or (Length(server) = 0) then Form1.setJabberOnline.Checked := False;

     if not Form1.setJabberOnline.Checked then
     begin
          report.Add('; No Jabber');
          report.Add('[PluginDisable]');
          report.Add('jabber.dll=b1');
          exit;
     end;

     j := Pos('@', uname); // remove domain from username
     if j <> 0 then
     begin
          uname := Copy(uname, 1, j - 1);
     end;

     report.Add('[JABBER]');
     report.Add('LoginName=s' + uname);
     report.Add('Password=s' + miranda_password_encode(pass));
     report.Add('LoginServer=s' + server);

     report.Add('; Make sure Jabber is enabled (jic)');
     report.Add('[PluginDisable]');
     report.Add('jabber.dll=l');

     Result := 1;
end;

function doIRCReport(var report: TStringList): Integer;
var
   nick, server: string;
begin
     Result := 0;
     nick := Form1.setIRC_DN.Text;
     server := Form1.setIRC_SV.Text;

     if (Length(nick) = 0) or (Length(server) = 0) then Form1.setIRCOnline.Checked := False;

     if not Form1.setIRCOnline.Checked then
     begin
          report.Add('; no IRC');
          report.Add('[PluginDisable]');
          report.Add('irc.dll=b1');
          exit;
     end;

     report.Add('[IRC]');
     report.Add('Nick=s' + nick);
     report.Add('ServerName=s' + server);
     report.Add('Name=s' + '...');

     report.Add('; Make sure irc is enabled (jic)');
     report.Add('[PluginDisable]');
     report.Add('irc.dll=l');

     Result := 1;

end;

procedure MirandaINI(szSection, szName, szValue: PChar);
var
   inf: TIniFile;
   szBuf: array[0..MAX_PATH] of Char;
   path: string;
   p: PChar;
begin
     try
          GetModuleFileName(0, szBuf,sizeof(szBuf));
          p := StrRScan(szBuf, '\'); // find the last \
          if p <> nil then // found, replace with null
          begin
               p^ := #0;
               Path := string(PChar(@szBuf)) + '\mirandaboot.ini';
          end;
          inf := TIniFile.Create(path);
     finally
     end;
     inf.WriteString(szSection, szName, szValue);
     inf.Free;
end;

function iffb(expr, a, b: Boolean): Boolean;
begin
     if expr then Result := a else Result := b;
end;

function iffi(expr: Boolean; a, b: Integer): Integer;
begin
     if expr then Result := a else Result := b;
end;

procedure TForm1.FormCreate(Sender: TObject);
var
   cur: HCURSOR;
begin

     cur := LoadCursor(0,MAKEINTRESOURCE(32649));
     if cur = 0 then cur := LoadCursor(0,MAKEINTRESOURCE(IDC_UPARROW)); // will fail on 95
     Screen.Cursors[-50] := cur; // cursor hand

     // Make the panes align to the client area
     panelWelcome.Align := alClient;
     panelICQ.Align := alClient;
     panelAIM.Align := alClient;
     panelMSN.Align := alClient;
     panelYahoo.Align := alClient;
     panelJabber.Align := alClient;
     panelIRC.Align := alClient;
     panelReport.Align := alClient;

     // Make sure the Welcome page is first
     panelWelcome.BringToFront();

     panelWelcome.BorderStyle := bsNone;
     panelICQ.BorderStyle := bsNone;
     panelAIM.BorderStyle := bsNone;
     panelMSN.BorderStyle := bsNone;
     panelYahoo.BorderStyle := bsNone;
     panelJabber.BorderStyle := bsNone;
     panelIRC.BorderStyle := bsNone;
     panelReport.BorderStyle := bsNone;

     Panel1.Color := GetSysColor(COLOR_BTNFACE) + $101010;

end;

procedure TForm1.FormResize(Sender: TObject);
begin
     ShowScrollBar(ListView1.Handle,SB_BOTH,FALSE); // stupid scrollbars, go away
end;

procedure TForm1.btnNextClick(Sender: TObject);
var
   i: Integer;
begin
     if ListView1.Selected <> nil then
     begin
          i := ListView1.Selected.Index;
          if i < ListView1.Items.Count-1 then ListView1.Items [ i + 1 ] . Selected := True;
     end else begin
         ListView1.Items[0].Selected := TRUE;
     end;
end;

procedure TForm1.btnBackClick(Sender: TObject);
var
   i: Integer;
begin
     if ListView1.Selected <> nil then
     begin
          i := ListView1.Selected.Index;
          if i > 0 then ListView1.Items [ i - 1 ] . Selected := True;
     end else begin
         ListView1.Items[0].Selected := TRUE;
     end;
end;

procedure TForm1.ListView1Change(Sender: TObject; Item: TListItem;
  Change: TItemChange);
var
   l: TListItem;
begin
     // set btnBack as needed
     btnBack.Enabled := iffb(Item.Index > 0, TRUE, FALSE);
     // which page should we be on?
     l := ListView1.Selected;
     // last page?
     btnNext.Enabled := iffb(Item.Index <> ListView1.Items.Count -1, TRUE, FALSE);
     if l <> nil then
     begin
          if l.index = 0 then // Welcome
          begin
               panelWelcome.BringToFront();
               Exit;
          end; //if
          if l.index = 1 then // ICQ
          begin
               panelICQ.BringToFront();
               Exit;
          end;
          if l.index = 2 then // AIM
          begin
               panelAIM.BringToFront();
               Exit;
          end; //if
          if l.Index = 3 then // MSN
          begin
               panelMSN.BringToFront();
               Exit;
          end; //if
          if l.Index = 4 then // Yahoo
          begin
               //panelYahoo.BringToFront();
               //Exit;
          end; //if
          if l.Index = 4 then // Jabber
          begin
               panelJabber.BringToFront();
               Exit;
          end; //if
          if l.Index = 5 then // IRC
          begin
               panelIRC.BringToFront();
               Exit;
          end; //if
          if l.Index = 6 then // Report
          begin
               panelReport.BringToFront();
               Exit;
          end; //if
     end;
end;

procedure TForm1.FormActivate(Sender: TObject);
begin
     if ListView1.Selected = nil then
     begin
          ListView1.Items[0].Selected := True;
     end;
end;

procedure TForm1.setIcqOnlineClick(Sender: TObject);
begin
     ListView1.Items[1].ImageIndex := iffi(setIcqOnline.Checked, 6, 0); // if enabled change icon
     icqUIN.Enabled := setIcqOnline.Checked; // disable form if icq is not online
     icqPass.Enabled := icqUIN.Enabled;
     label3.Enabled := icqPass.Enabled;
     label4.Enabled := label3.Enabled;
     label5.Enabled := label4.Enabled;
     label6.Enabled := label5.Enabled;
     icqUIN.Color := iffi(setIcqOnline.Checked, GetSysColor(COLOR_WINDOW), GetSysColor(COLOR_BTNFACE));
     icqPass.Color := icqUIN.Color;
end;

procedure TForm1.setAIMOnlineClick(Sender: TObject);
begin
     ListView1.Items[2].ImageIndex := iffi(setAIMOnline.Checked, 7, 1); // if enabled change icon
     setAIM_SN.Enabled := setAIMOnline.Checked;
     label13.Enabled := setAIM_SN.Enabled;
     label14.Enabled := label13.Enabled;
     label15.Enabled := label14.Enabled;
     label16.Enabled := label15.Enabled;
     label17.Enabled := label16.Enabled;
     label22.Enabled := label17.Enabled;
     setAIM_PW.Enabled := setAIM_SN.Enabled;
     setAIM_DN.Enabled := setAIM_PW.Enabled;
     setAIM_SN.Color := iffi(setAIMOnline.Checked, GetSysColor(COLOR_WINDOW), GetSysColor(COLOR_BTNFACE));
     setAIM_PW.Color := setAIM_SN.Color;
     setAIM_DN.Color := setAIM_SN.Color;
end;

procedure TForm1.setMSNOnlineClick(Sender: TObject);
begin
     ListView1.Items[3].ImageIndex := iffi(setMSNOnline.Checked, 8, 2); // if enabled change icon
     setMSN_UN.Enabled := setMSNOnline.Checked;
     setMSN_PW.Enabled := setMSN_UN.Enabled;
     setMSN_DN.Enabled := setMSN_PW.Enabled;
     label24.Enabled := setMSN_UN.Enabled;
     label25.Enabled := label24.Enabled;
     label26.Enabled := label25.Enabled;
     label27.Enabled := label26.Enabled;
     label28.Enabled := label27.Enabled;
     label29.Enabled := label28.Enabled;
     setMSN_UN.Color := iffi(setMSNOnline.Checked, GetSysColor(COLOR_WINDOW), GetSysColor(COLOR_BTNFACE));
     setMSN_PW.Color := setMSN_UN.Color;
     setMSN_DN.Color := setMSN_PW.Color;
end;

procedure TForm1.setYahooOnlineClick(Sender: TObject);
begin
     ListView1.Items[4].ImageIndex := iffi(setYahooOnline.Checked, 9, 3); // if enabled change icon
     setYahoo_UN.Enabled := setYahooOnline.Checked;
     setYahoo_PW.Enabled := setYahoo_UN.Enabled;
     label36.Enabled := setYahoo_UN.Enabled;
     label37.Enabled := label36.Enabled;
     label38.Enabled := label37.Enabled;
     label39.Enabled := label38.Enabled;
     setYahoo_UN.Color := iffi(setYahooOnline.Checked, GetSysColor(COLOR_WINDOW), GetSysColor(COLOR_BTNFACE));
     setYahoo_PW.Color := setYahoo_UN.Color;
end;

procedure TForm1.setJabberOnlineClick(Sender: TObject);
begin
     ListView1.Items[5-1].ImageIndex := iffi(setJabberOnline.Checked, 10, 4); // if enabled change icon
     setJabber_UN.Enabled := setJabberOnline.Checked;
     setJabber_PW.Enabled := setJabber_UN.Enabled;
     setJabber_SV.Enabled := setJabber_PW.Enabled;
     label44.Enabled := setJabber_UN.Enabled;
     label45.Enabled := label44.Enabled;
     label46.Enabled := label45.Enabled;
     label47.Enabled := label46.Enabled;
     label48.Enabled := label47.Enabled;
     label49.Enabled := label48.Enabled;
     setJabber_UN.Color := iffi(setJabberOnline.Checked, GetSysColor(COLOR_WINDOW), GetSysColor(COLOR_BTNFACE));
     setJabber_PW.Color := setJabber_UN.Color;
     setJabber_SV.Color := setJabber_UN.Color;
end;

procedure TForm1.setIRCOnlineClick(Sender: TObject);
begin
     ListView1.Items[6-1].ImageIndex := iffi(setIRCOnline.Checked, 11 , 5); // if enabled change icon
     setIRC_DN.Enabled := setIRCOnline.Checked;
     setIRC_SV.Enabled := setIRC_DN.Enabled;
     label57.Enabled := setIRC_DN.Enabled;
     label58.Enabled := label57.Enabled;
     label60.Enabled := label57.Enabled;
     label61.Enabled := label60.Enabled;
     setIRC_DN.Color := iffi(setIRCOnline.Checked, GetSysColor(COLOR_WINDOW), GetSysColor(COLOR_BTNFACE));
     setIRC_SV.Color := setIRC_DN.Color;
end;

procedure TForm1.icqUINKeyPress(Sender: TObject; var Key: Char);
begin
    if (GetAsyncKeyState(VK_LCONTROL) = 0) and (not (Key in ['0'..'9'])) then Key := #0; // don't allow anything but numbers
end;

procedure TForm1.Label6Click(Sender: TObject);
begin
     ShellExecute(0,'open','https://web.icq.com/secure/password',nil,nil,SW_SHOW);
end;

procedure TForm1.Label10Click(Sender: TObject);
begin
     ShellExecute(0,'open','https://lite.icq.com/register',nil,nil,SW_SHOW);
end;

procedure TForm1.Label15Click(Sender: TObject);
begin
     ShellExecute(0,'open','http://www.aim.com/help_faq/forgot_password/password.adp',nil,nil,SW_SHOW);
end;

procedure TForm1.ListView1Click(Sender: TObject);
begin
     ShowScrollBar(ListView1.Handle,SB_BOTH,FALSE); // stupid scrollbars, go away
end;

procedure TForm1.Label20Click(Sender: TObject);
begin
     ShellExecute(0,'open','http://my.screenname.aol.com/_cqr/login/login.psp?seamless=n&siteState=http://www.aim.aol.com/get_aim/congratsd2.adp&triedAimAuth=y&siteId=aimregistrationPROD&mcState=initialized&createSn=1',nil,nil,SW_SHOW);
end;

procedure TForm1.Label27Click(Sender: TObject);
begin
     ShellExecute(0,'open','https://memberservices.passport.net/ppsecure/MSRV_ResetPW.srf?lc=2057',nil,nil,SW_SHOW);
end;

procedure TForm1.Label32Click(Sender: TObject);
begin
     ShellExecute(0,'open','https://register.passport.net/',nil,nil,SW_SHOW);
end;

procedure TForm1.Label41Click(Sender: TObject);
begin
     ShellExecute(0,'open','http://edit.yahoo.com/config/eval_register?.intl=us&new=1&.done=http%3a//messenger.',nil,nil,SW_SHOW);
end;

procedure TForm1.Label39Click(Sender: TObject);
begin
     ShellExecute(0,'open','http://us.rd.yahoo.com/reg/fpflib/*https://edit.yahoo.com/config/eval_forgot_pw?new=1&.done=http://messenger.yahoo.com&.src=pg&partner=&.partner=&.intl=us&pkg=&stepid=&.last=',nil,nil,SW_SHOW);
end;

procedure TForm1.Label49Click(Sender: TObject);
var
   msg: string;
begin
     msg := 'The jabber server you request your lost password from depends on which jabber server you registered at.'#13#10#13#10;
     msg := msg + 'Do you want to see the lost password/contact request form for jabber.org?';
     if MessageBox(0,PChar(msg),'Jabber information',MB_YESNO or MB_ICONINFORMATION) = IDNO then Exit;
     ShellExecute(0,'open','http://www.jabber.org/jsf/info.php',nil,nil,SW_SHOW);
end;

procedure TForm1.Label51Click(Sender: TObject);
begin
     ShellExecute(0,'open','http://www.jabber.org/user/publicservers.php',nil,nil,SW_SHOW);
end;

procedure TForm1.Label54Click(Sender: TObject);
var
   S: string;
begin
     S := 'There are no web based jabber signup forms, therefore you have to do this from the Jabber options dialog ';
     S := S + '(Miranda->Options->Network->Jabber which you will be able to access after setup).'#13#10#13#10'Every jabber server differs on how it allows new accounts but all offer sign-up via the service.'#13#10;
     S := S + 'For example, some allow access to other IMs (called transports) others do not. So you have to pick depending on which services you may want.';
     MessageBox(0,PChar(S),'',MB_OK or MB_ICONINFORMATION)
end;

procedure TForm1.setJabber_UNChange(Sender: TObject);
var
   i: integer;
begin
     i := Pos('@', setJabber_UN.Text); // if there is a server portion copy it to the server field
     if i > 0 then
     begin
          setJabber_SV.Text := Copy(setJabber_UN.Text, i+1, Length(setJabber_UN.Text) - i);
     end;
end;

procedure TForm1.btnFinishedClick(Sender: TObject);
var
     report: TStringList;
     rc: Integer;
begin
     panelReport.BringToFront();
     ShowWindow(Application.Handle,SW_HIDE);
     Application.ProcessMessages;
     btnFinished.Enabled := False;
     try
          report := TStringList.Create;
          rc := doICQReport(report);
          rc := doAIMReport(report);
          rc := doMSNReport(report);
          //rc := doYahooReport(report);
          rc := doJabberReport(report);
          rc := doIRCReport(report);

          MirandaINI('Database','AutoCreate','yes');
          MirandaINI('AutoExec','OverrideSecurityFilename','autoexec_proto_config.ini');

          report.SaveToFile('autoexec_proto_config.ini');

          ShellExecute(0,'open','miranda32.exe','profile',nil,SW_SHOW);
          Form1.Hide;
          Sleep(1000 * 5);

          MirandaINI('Database','AutoCreate','no');
          MirandaINI('AutoExec','OverrideSecurityFilename','');

          Application.Terminate;
     finally
          report.Free;
     end;
end;

procedure TForm1.ListView1MouseMove(Sender: TObject; Shift: TShiftState; X,
  Y: Integer);
begin
     ShowScrollBar(ListView1.Handle,SB_BOTH,FALSE); // stupid scrollbars, go away
end;

end.


