object TimeoutForm: TTimeoutForm
  Left = 309
  Top = 103
  ActiveControl = Button1
  BorderStyle = bsDialog
  Caption = 'Timeout'
  ClientHeight = 122
  ClientWidth = 319
  Color = clBtnFace
  Font.Charset = DEFAULT_CHARSET
  Font.Color = clWindowText
  Font.Height = -11
  Font.Name = 'MS Sans Serif'
  Font.Style = []
  OldCreateOrder = False
  PixelsPerInch = 96
  TextHeight = 13
  object CapLabel: TLabel
    Left = 8
    Top = 8
    Width = 111
    Height = 13
    Caption = 'Message timed out:'
    Font.Charset = DEFAULT_CHARSET
    Font.Color = clWindowText
    Font.Height = -11
    Font.Name = 'MS Sans Serif'
    Font.Style = [fsBold]
    ParentFont = False
  end
  object RetryBtn: TButton
    Left = 8
    Top = 88
    Width = 75
    Height = 25
    Caption = 'Retry'
    ModalResult = 1
    TabOrder = 0
  end
  object RetryServerBtn: TButton
    Left = 96
    Top = 88
    Width = 121
    Height = 25
    Caption = 'Retry Through Server'
    ModalResult = 4
    TabOrder = 1
  end
  object CancelBtn: TButton
    Left = 232
    Top = 88
    Width = 75
    Height = 25
    Cancel = True
    Caption = 'Cancel'
    ModalResult = 2
    TabOrder = 2
  end
  object MessageTextLabel: TMemo
    Left = 8
    Top = 32
    Width = 297
    Height = 49
    TabStop = False
    BorderStyle = bsNone
    Color = clBtnFace
    Lines.Strings = (
      'MessageTextLabel')
    ReadOnly = True
    TabOrder = 3
    WantReturns = False
  end
  object Button1: TButton
    Left = 328
    Top = 112
    Width = 9
    Height = 9
    Caption = 'Button1'
    Default = True
    TabOrder = 4
  end
end
