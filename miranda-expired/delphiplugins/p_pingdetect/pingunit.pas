unit pingunit;

interface

uses windows,classes,sysutils,ping;

type
  TPingObject=class;

  TTimerThread = class(TThread)
  private
    fPingObject : TPingObject;
    fTime:Integer;
  protected
    procedure Execute; override;
  Public
    Active:Boolean;
    constructor Create(PingObject : TPingObject; Time: Integer);
  end;

  TPingReadyCallback = procedure(Sender:Pointer);
  TPingObject=class
  private
    Ping:TPing;

    fTtlPings:Integer;
    fRemainingPings:Integer;
    fSuccessfulPings:Integer;
    fTimeout:Integer;//in msec
    FOnPingReady: TNotifyEvent;
    fTimerThread:TTimerThread;
    fTtlPingTime:integer;
    FCallback: TPingReadyCallback;
    function GetSuccess: Double;

    procedure PingDnsLookupDone(Sender: TObject; Error: Word);
    procedure PingEchoReply(Sender: TObject; Icmp: TObject; Error: Integer);
    procedure Timeout;
    procedure DoPing;
    function GetAvPingTime: Double;

  public
    procedure StartPingHost(Host:string;Timeout,Count:Integer);

    property OnPingReady:TNotifyEvent read FOnPingReady write FOnPingReady;
    property Callback:TPingReadyCallback read FCallback write FCallback;
    property Success:Double read GetSuccess;
    property AvPingTime:Double read GetAvPingTime;

    constructor Create;
    destructor Destroy;override;
  end;


implementation

{ TPingObject }


Constructor TTimerThread.Create;
Begin
  Inherited Create(False);
  Priority := tpNormal;
  FPingObject := PingObject;
  fTime:=Time;
end;

Procedure TTimerThread.Execute;
var i:integer;
Begin
  try
  for i:=0 to FPingObject.fTimeout div 50 do
    if not Terminated and Active then
      Sleep(50);
  if Active and not Terminated then
    Synchronize(FPingObject.Timeout);
  except
  end;
end;


constructor TPingObject.Create;
begin
  ping:=TPing.Create(nil);
  ping.OnEchoReply:=PingEchoReply;
  ping.OnDnsLookupDone:=PingDnsLookupDone;
end;

destructor TPingObject.Destroy;
begin
  if Assigned(fTimerThread) then
    begin
    fTimerThread.Active:=False;
    fTimerThread.Terminate;
    FreeAndNil(fTimerThread);
    end;

  FreeAndNil(ping);
end;


procedure TPingObject.DoPing;
begin
  if fRemainingPings<=0 then
    begin
    if Assigned(FOnPingReady) then
      FOnPingReady(Self);
    if Assigned(Callback) then
      Callback(Self);
    Exit;
    end;

  fTimerThread:=TTimerThread.Create(Self,fTimeout);
  fTimerThread.Resume;

  Dec(fRemainingPings);
  Ping.Ping;
end;

function TPingObject.GetAvPingTime: Double;
begin
  Result:=0;
  if fTtlPings<>0 then
    Result:=fTtlPingTime/fTtlPings;
end;

function TPingObject.GetSuccess: Double;
//Returns the percentage of successful requests (0<=x<=1)
begin
  Result:=0;
  if fTtlPings<>0 then
    Result:=fSuccessfulPings/fTtlPings;
end;

procedure TPingObject.PingDnsLookupDone(Sender: TObject; Error: Word);
begin
try

  if Assigned(fTimerThread) then
    begin
    fTimerThread.Active:=False;
    fTimerThread.Terminate;
    FreeAndNil(fTimerThread);
    end;

  Ping.Address := Ping.DnsResult;

  //don't even ping when lookup failes
  if error<>0 then
    fRemainingPings:=0;

  DoPing;

except end;
end;

procedure TPingObject.PingEchoReply(Sender, Icmp: TObject; Error: Integer);
begin
  if (Error=1) and (Ping.Reply.DataSize<>0) then
    begin
    Inc(fSuccessfulPings);
    fTtlPingTime:=fTtlPingTime+Ping.Reply.RTT;
    end;

  if Assigned(fTimerThread) then
    begin
    fTimerThread.Active:=False;
    fTimerThread.Terminate;
    FreeAndNil(fTimerThread);
    end;

  DoPing;
end;

procedure TPingObject.StartPingHost(Host: string; Timeout, Count: Integer);
{Pings the host count times... Times out after timeout msecs}
begin
  fTtlPings:=Count;
  fRemainingPings:=Count;
  fTimeout:=timeout;
  fSuccessfulPings:=0;
  fTtlPingTime:=0;

  fTimerThread:=TTimerThread.Create(Self,fTimeout);
  fTimerThread.Resume;

  ping.DnsLookup(host);
end;

procedure TPingObject.Timeout;
begin
  Ping.CancelDnsLookup;

  DoPing;
end;


end.
