object MsgWindow: TMsgWindow
  Left = 427
  Top = 292
  ActiveControl = SendMemo
  AutoScroll = False
  BorderWidth = 4
  Caption = '%Username% (%UID%) %STATUS%'
  ClientHeight = 193
  ClientWidth = 345
  Color = clBtnFace
  Font.Charset = DEFAULT_CHARSET
  Font.Color = clWindowText
  Font.Height = -11
  Font.Name = 'MS Sans Serif'
  Font.Style = []
  KeyPreview = True
  OldCreateOrder = False
  Position = poDefaultPosOnly
  OnClose = FormClose
  OnCloseQuery = FormCloseQuery
  OnCreate = FormCreate
  OnDestroy = FormDestroy
  OnKeyDown = FormKeyDown
  OnKeyUp = FormKeyUp
  OnResize = FormResize
  PixelsPerInch = 96
  TextHeight = 13
  object Splitter: TSplitter
    Left = 0
    Top = 0
    Width = 345
    Height = 4
    Cursor = crVSplit
    Align = alTop
    Beveled = True
  end
  object TabEnterWorkAroundBtn: TButton
    Left = 160
    Top = -40
    Width = 75
    Height = 25
    Caption = 'TabEnterWorkAroundBtn'
    TabOrder = 2
    OnClick = TabEnterWorkAroundBtnClick
  end
  object SendMemo: TMemo
    Left = 0
    Top = 4
    Width = 345
    Height = 159
    Align = alClient
    Lines.Strings = (
      'SendMemo')
    TabOrder = 0
    OnChange = SendMemoChange
    OnKeyDown = SendMemoKeyDown
    OnKeyUp = SendMemoKeyUp
  end
  object Panel1: TPanel
    Left = 0
    Top = 163
    Width = 345
    Height = 30
    Align = alBottom
    BevelOuter = bvNone
    Caption = ' '
    TabOrder = 1
    object SplitLabel: TLabel
      Left = 176
      Top = 16
      Width = 20
      Height = 13
      Caption = 'Split'
      Visible = False
    end
    object CharCountLabel: TLabel
      Left = 176
      Top = 8
      Width = 42
      Height = 13
      Caption = 'Char: %d'
    end
    object CancelBtn: TToolbarButton97
      Left = 4
      Top = 6
      Width = 75
      Height = 23
      Cancel = True
      Caption = '&Cancel'
      Flat = False
      OnClick = CancelBtnClick
    end
    object UserBtn: TToolbarButton97
      Left = 92
      Top = 6
      Width = 53
      Height = 23
      DropdownArrowWidth = 12
      DropdownMenu = UserMenu
      Caption = '&User'
      Flat = False
    end
    object Panel2: TPanel
      Left = 250
      Top = 0
      Width = 95
      Height = 30
      Align = alRight
      BevelOuter = bvNone
      Caption = ' '
      TabOrder = 0
      object QLabel: TLabel
        Left = 2
        Top = 11
        Width = 8
        Height = 13
        Caption = 'Q'
        Font.Charset = DEFAULT_CHARSET
        Font.Color = clGray
        Font.Height = -11
        Font.Name = 'MS Sans Serif'
        Font.Style = []
        ParentFont = False
        Visible = False
      end
      object SendBtn: TToolbarButton97
        Left = 14
        Top = 6
        Width = 77
        Height = 23
        DropdownArrowWidth = 12
        DropdownCombo = True
        DropdownMenu = SendMenu
        Caption = '&Send'
        Flat = False
        OnClick = SendBtnClick
      end
    end
  end
  object SendTimer: TTimer
    Enabled = False
    OnTimer = SendTimerTimer
    Left = 72
    Top = 8
  end
  object UserMenu: TPopupMenu
    Left = 144
    Top = 8
    object HistoryMenuItem: TMenuItem
      Caption = 'History...'
      OnClick = HistoryMenuItemClick
    end
    object UserDetailsMenuItem: TMenuItem
      Caption = 'User &Details...'
      OnClick = UserDetailsMenuItemClick
    end
    object UserSendEMailMenuItem: TMenuItem
      Caption = 'Send &EMail...'
      OnClick = UserSendEMailMenuItemClick
    end
    object SendFiles1: TMenuItem
      Caption = 'Send &File(s)...'
      Visible = False
    end
    object AddUserMenuItem: TMenuItem
      Caption = 'Add User...'
      OnClick = AddUserMenuItemClick
    end
  end
  object SendMenu: TPopupMenu
    Left = 208
    Top = 8
    object SendDefaultWayItem: TMenuItem
      Caption = 'Send using default way'
      Default = True
      OnClick = SendWayItemClick
    end
    object SendBestWayItem: TMenuItem
      Caption = 'Send using best way'
      OnClick = SendWayItemClick
    end
    object SendThroughServerItem: TMenuItem
      Caption = 'Send through server'
      OnClick = SendWayItemClick
    end
    object SendDirectItem: TMenuItem
      Caption = 'Send direct'
      OnClick = SendWayItemClick
    end
  end
  object FlashTimer: TTimer
    Enabled = False
    Interval = 400
    OnTimer = FlashTimerTimer
    Left = 72
    Top = 56
  end
end
