{***************************************************************
 * Project     : convers
 * Unit Name   : timeoutfrm
 * Description : Timeout window for convers plugin
 *               
 * Author      : Christian Kästner
 * Date        : 24.05.2001
 *
 * Copyright © 2001 by Christian Kästner
 ****************************************************************}

unit timeoutfrm;

interface

uses
  Windows, Messages, SysUtils, Classes, Graphics, Controls, Forms, Dialogs,
  StdCtrls;

type
  TTimeoutAction=(taRetry,taRetryServer,taCancel);
  TTimeoutForm = class(TForm)
    RetryBtn: TButton;
    RetryServerBtn: TButton;
    CancelBtn: TButton;
    CapLabel: TLabel;
    MessageTextLabel: TMemo;
  private
    { Private declarations }
  public
    { Public declarations }
  end;


function MessageTimeout(tText:string):TTimeoutAction;

implementation

{$R *.DFM}

function MessageTimeout(tText:string):TTimeoutAction;
begin
  Result:=taCancel;

  with TTimeoutForm.Create(nil) do
    try
    MessageTextLabel.Lines.Text:=tText;
    case ShowModal of
      ID_OK:Result:=taRetry;
      ID_RETRY:Result:=taRetryServer;
    end;
    finally
    Free;
    end;
end;

end.
