object OptionForm: TOptionForm
  Left = 504
  Top = 292
  BorderStyle = bsDialog
  Caption = 'Conversation Style Messaging - Options'
  ClientHeight = 368
  ClientWidth = 339
  Color = clBtnFace
  Font.Charset = DEFAULT_CHARSET
  Font.Color = clWindowText
  Font.Height = -11
  Font.Name = 'MS Sans Serif'
  Font.Style = []
  OldCreateOrder = False
  Position = poScreenCenter
  OnShow = FormShow
  PixelsPerInch = 96
  TextHeight = 13
  object Bevel1: TBevel
    Left = -8
    Top = 320
    Width = 353
    Height = 9
    Shape = bsTopLine
  end
  object OKBtn: TButton
    Left = 43
    Top = 336
    Width = 75
    Height = 25
    Caption = 'OK'
    Default = True
    ModalResult = 1
    TabOrder = 0
    OnClick = OKBtnClick
  end
  object CancelBtn: TButton
    Left = 131
    Top = 336
    Width = 75
    Height = 25
    Cancel = True
    Caption = 'Cancel'
    ModalResult = 2
    TabOrder = 1
  end
  object AboutBtn: TButton
    Left = 219
    Top = 336
    Width = 75
    Height = 25
    Caption = 'About...'
    TabOrder = 2
    OnClick = AboutBtnClick
  end
  object PageControl1: TPageControl
    Left = 8
    Top = 8
    Width = 321
    Height = 305
    ActivePage = TabSheet1
    TabOrder = 3
    object TabSheet1: TTabSheet
      Caption = 'Misc'
      object StorePositionsCheck: TCheckBox
        Left = 8
        Top = 40
        Width = 289
        Height = 17
        Caption = 'Store options for every user (position, size, send method)'
        TabOrder = 0
      end
      object LoadRecentMsgCheck: TCheckBox
        Left = 8
        Top = 8
        Width = 305
        Height = 17
        Caption = 'Load History on top (last              messages)'
        TabOrder = 1
      end
      object LoadRecentMsgCountEdit: TEdit
        Left = 144
        Top = 6
        Width = 33
        Height = 21
        TabOrder = 2
        Text = '10'
        OnExit = LoadRecentMsgCountEditExit
      end
      object HandleIncomingGroup: TRadioGroup
        Left = 8
        Top = 72
        Width = 265
        Height = 105
        Caption = 'Incoming Messages'
        ItemIndex = 0
        Items.Strings = (
          'Popup, steal focus'
          'Show window without focus'
          'Flash window'
          'Flash icon on contactlist only')
        TabOrder = 3
      end
    end
    object TabSheet5: TTabSheet
      Caption = 'Send'
      ImageIndex = 4
      object Label1: TLabel
        Left = 8
        Top = 161
        Width = 110
        Height = 13
        Caption = 'Special send shortcuts:'
      end
      object Label2: TLabel
        Left = 8
        Top = 80
        Width = 108
        Height = 13
        Caption = 'Timeout:              msec'
      end
      object Label3: TLabel
        Left = 8
        Top = 120
        Width = 219
        Height = 13
        Caption = 'Auto-retry sending through server             times'
      end
      object DoubleEnter: TCheckBox
        Left = 128
        Top = 160
        Width = 89
        Height = 17
        Caption = 'Double Enter'
        TabOrder = 0
      end
      object SingleEnter: TCheckBox
        Left = 128
        Top = 184
        Width = 81
        Height = 17
        Caption = 'Single Enter'
        TabOrder = 1
      end
      object TimeoutEdit: TEdit
        Left = 54
        Top = 76
        Width = 33
        Height = 21
        TabOrder = 2
        Text = '8'
        OnExit = TimeoutEditExit
      end
      object CloseWindowCheck: TCheckBox
        Left = 8
        Top = 40
        Width = 161
        Height = 17
        Caption = 'Close window after sending'
        TabOrder = 3
      end
      object SplitLargeMessagesCheck: TCheckBox
        Left = 8
        Top = 8
        Width = 305
        Height = 17
        Caption = 'Split messages larger than %d characters.'
        TabOrder = 4
      end
      object AutoRetryEdit: TEdit
        Left = 168
        Top = 116
        Width = 33
        Height = 21
        TabOrder = 5
        Text = '2'
        OnExit = AutoRetryEditExit
      end
      object TabEnter: TCheckBox
        Left = 128
        Top = 208
        Width = 81
        Height = 17
        Caption = 'Tab + Enter'
        TabOrder = 6
      end
    end
    object TabSheet2: TTabSheet
      Caption = 'Memo'
      ImageIndex = 1
      object m_texteditlabel: TLabel
        Left = 16
        Top = 72
        Width = 24
        Height = 13
        Caption = 'Text:'
      end
      object m_textedithint: TLabel
        Left = 16
        Top = 112
        Width = 257
        Height = 33
        AutoSize = False
        Caption = 'Use %NAME%, %DATE%, %TIME% and %TEXT% as possible fields.'
        Color = clBtnFace
        Font.Charset = DEFAULT_CHARSET
        Font.Color = clGray
        Font.Height = -11
        Font.Name = 'MS Sans Serif'
        Font.Style = []
        ParentColor = False
        ParentFont = False
        WordWrap = True
      end
      object m_textedit: TComboBox
        Left = 16
        Top = 88
        Width = 257
        Height = 21
        ItemHeight = 13
        TabOrder = 0
        Text = 'm_textedit'
        Items.Strings = (
          '%NAME% (%DATE% %TIME%): %TEXT%'
          '%NAME% (%TIME%): %TEXT%'
          '%NAME%: %TEXT%'
          '%TIME%: %TEXT%'
          '%TEXT%')
      end
      object m_changefont: TButton
        Left = 16
        Top = 32
        Width = 89
        Height = 25
        Caption = 'Change Font'
        TabOrder = 1
        OnClick = m_changefontClick
      end
      object UseMemoCheck: TCheckBox
        Left = 8
        Top = 8
        Width = 217
        Height = 17
        Caption = 'Use simple memo as historybox'
        TabOrder = 2
        OnClick = RadionBtnClick
      end
    end
    object TabSheet3: TTabSheet
      Caption = 'RichEdit'
      ImageIndex = 2
      object r_texteditlabel: TLabel
        Left = 16
        Top = 72
        Width = 24
        Height = 13
        Caption = 'Text:'
      end
      object r_textedithint: TLabel
        Left = 16
        Top = 112
        Width = 257
        Height = 33
        AutoSize = False
        Caption = 'Use %NAME%, %DATE%, %TIME% and %TEXT% as possible fields.'
        Color = clBtnFace
        Font.Charset = DEFAULT_CHARSET
        Font.Color = clGray
        Font.Height = -11
        Font.Name = 'MS Sans Serif'
        Font.Style = []
        ParentColor = False
        ParentFont = False
        WordWrap = True
      end
      object r_textedit: TComboBox
        Left = 16
        Top = 88
        Width = 257
        Height = 21
        ItemHeight = 13
        TabOrder = 0
        Text = 'ComboBox1'
        Items.Strings = (
          '%NAME% (%DATE% %TIME%): %TEXT%'
          '%NAME% (%TIME%): %TEXT%'
          '%NAME%: %TEXT%'
          '%TIME%: %TEXT%'
          '%TEXT%')
      end
      object r_grayrecent: TCheckBox
        Left = 16
        Top = 256
        Width = 137
        Height = 17
        Caption = 'Gray Recent Messages'
        TabOrder = 1
        OnClick = r_grayrecentClick
      end
      object r_changefont: TButton
        Left = 16
        Top = 32
        Width = 89
        Height = 25
        Caption = 'Change Font'
        TabOrder = 2
        OnClick = r_changefontClick
      end
      object r_textbold: TCheckBox
        Left = 168
        Top = 152
        Width = 97
        Height = 17
        Caption = 'Bold'
        TabOrder = 3
        OnClick = r_textboldClick
      end
      object r_textitalic: TCheckBox
        Left = 168
        Top = 176
        Width = 97
        Height = 17
        Caption = 'Italic'
        TabOrder = 4
        OnClick = r_textitalicClick
      end
      object r_changecolor: TButton
        Left = 168
        Top = 200
        Width = 75
        Height = 25
        Caption = 'Change Color'
        TabOrder = 5
        OnClick = r_changecolorClick
      end
      object UseRicheditCheck: TCheckBox
        Left = 8
        Top = 8
        Width = 217
        Height = 17
        Caption = 'Use richedit as historybox'
        TabOrder = 6
        OnClick = RadionBtnClick
      end
      object r_texttypeselect: TListBox
        Left = 16
        Top = 152
        Width = 137
        Height = 97
        ItemHeight = 13
        Items.Strings = (
          'SENDMESSAGE'
          'INCOMING: NAME'
          'INCOMING: DATE/TIME'
          'INCOMING: TEXT'
          'OUTGOING: NAME'
          'OUTGOING: DATE/TIME'
          'OUTGOING: TEXT')
        TabOrder = 7
        OnClick = r_texttypeselectChange
      end
    end
    object TabSheet4: TTabSheet
      Caption = 'Grid'
      ImageIndex = 3
      object g_changefont: TButton
        Left = 16
        Top = 32
        Width = 89
        Height = 25
        Caption = 'Change Font'
        TabOrder = 0
        OnClick = g_changefontClick
      end
      object g_showtime: TCheckBox
        Left = 16
        Top = 80
        Width = 121
        Height = 17
        Caption = 'Show Time Column'
        TabOrder = 1
        OnClick = g_showtimeClick
      end
      object g_showdate: TCheckBox
        Left = 16
        Top = 104
        Width = 97
        Height = 17
        Caption = 'Include Date'
        TabOrder = 2
        OnClick = g_showdateClick
      end
      object g_changecolor: TButton
        Left = 16
        Top = 136
        Width = 105
        Height = 25
        Caption = 'Incoming Color'
        Enabled = False
        TabOrder = 3
        OnClick = g_changecolorClick
      end
      object g_grayrecent: TCheckBox
        Left = 16
        Top = 208
        Width = 137
        Height = 17
        Caption = 'Gray Recent Messages'
        TabOrder = 4
        OnClick = g_grayrecentClick
      end
      object UseGridCheck: TCheckBox
        Left = 8
        Top = 8
        Width = 257
        Height = 17
        Caption = 'Use Grid as Historybox (like the History+ Plugin)'
        TabOrder = 5
        OnClick = RadionBtnClick
      end
      object Button1: TButton
        Left = 16
        Top = 168
        Width = 105
        Height = 25
        Caption = 'Outgoing Color'
        TabOrder = 6
        OnClick = Button1Click
      end
    end
  end
  object FontDialog: TFontDialog
    Font.Charset = DEFAULT_CHARSET
    Font.Color = clWindowText
    Font.Height = -11
    Font.Name = 'MS Sans Serif'
    Font.Style = []
    MinFontSize = 0
    MaxFontSize = 0
    Options = [fdForceFontExist, fdNoStyleSel]
    Left = 152
    Top = 328
  end
  object ColorDialog: TColorDialog
    Ctl3D = True
    Options = [cdFullOpen, cdAnyColor]
    Left = 216
    Top = 328
  end
end
