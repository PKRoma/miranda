unit flowerform;

interface

uses
  Windows, Messages, SysUtils, Classes, Graphics, Controls, Forms, Dialogs,
  ExtCtrls;

type
  TFlowerFrm = class(TForm)
    Image1: TImage;
    procedure FormCreate(Sender: TObject);
  private
    { Private declarations }
  public
    { Public declarations }
  end;


implementation

{$R *.DFM}

procedure TFlowerFrm.FormCreate(Sender: TObject);
var h:tHandle;
begin
  h:=CreateEllipticRgn(0,0,123,123);
  SetWindowRgn(handle,h,true);
end;

end.
