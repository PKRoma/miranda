set Version=0.7.15
set FileVer=miranda-im-v%Version%-ansi
rd /Q /S %Temp%\pdba >nul
md %Temp%\pdba
md %Temp%\pdba\plugins

copy ..\src\Release\miranda32.pdb                   %Temp%\pdba
rem  Protocols
copy ..\protocols\AimOscar\Release\Aim.pdb          %Temp%\pdba\plugins
copy ..\protocols\IcqOscarJ\Release\ICQ.pdb         %Temp%\pdba\plugins
copy ..\protocols\IRCG\Release\IRC.pdb               %Temp%\pdba\plugins
copy ..\protocols\JabberG\Release\jabber.pdb        %Temp%\pdba\plugins
copy ..\protocols\MSN\Release\MSN.pdb               %Temp%\pdba\plugins
copy ..\protocols\Yahoo\Release\Yahoo.pdb           %Temp%\pdba\plugins
copy ..\protocols\Gadu-Gadu\Release\GG.pdb          %Temp%\pdba\plugins
rem  Plugins
copy ..\plugins\avs\Release\avs.pdb                 %Temp%\pdba\plugins
copy ..\plugins\chat\Release\chat.pdb               %Temp%\pdba\plugins
copy ..\plugins\clist\Release\clist_classic.pdb     %Temp%\pdba\plugins
copy ..\plugins\db3x\Release\dbx_3x.pdb             %Temp%\pdba\plugins
copy ..\plugins\import\Release\import.pdb           %Temp%\pdba\plugins
copy ..\plugins\srmm\Release\srmm.pdb               %Temp%\pdba\plugins

del /Q /F "%FileVer%-pdb.zip"
7z.exe a -tzip -r -mx=9 "%FileVer%-pdb.zip" %Temp%\pdba/*
rd /Q /S %Temp%\pdba

rd /Q /S %Temp%\miransi >nul
md %Temp%\miransi
md %Temp%\miransi\Plugins
md %Temp%\miransi\Icons

copy Release\miranda32.exe              %Temp%\miransi
copy Release\dbtool.exe                 %Temp%\miransi
rem  Protocols
copy Release\plugins\aim.dll            %Temp%\miransi\Plugins
copy Release\plugins\ICQ.dll            %Temp%\miransi\Plugins
copy Release\plugins\IRC.dll            %Temp%\miransi\Plugins
copy Release\plugins\jabber.dll         %Temp%\miransi\Plugins
copy Release\plugins\MSN.dll            %Temp%\miransi\Plugins
copy Release\plugins\Yahoo.dll          %Temp%\miransi\Plugins
copy Release\plugins\GG.dll             %Temp%\miransi\Plugins
rem  Plugins
copy Release\plugins\avs.dll            %Temp%\miransi\Plugins
copy Release\plugins\chat.dll           %Temp%\miransi\Plugins
copy Release\plugins\clist_classic.dll  %Temp%\miransi\Plugins
copy Release\plugins\dbx_3x.dll         %Temp%\miransi\Plugins
copy Release\plugins\advaimg.dll        %Temp%\miransi\Plugins
copy Release\plugins\import.dll         %Temp%\miransi\Plugins
copy Release\plugins\srmm.dll           %Temp%\miransi\Plugins
rem misc
copy Release\zlib.dll                             %Temp%\miransi
copy Release\winssl.dll                             %Temp%\miransi
copy ..\docs\Readme.txt                           %Temp%\miransi
copy ..\docs\mirandaboot.ini                      %Temp%\miransi
copy ..\protocols\IRCG\docs\IRC_Servers.ini       %Temp%\miransi\plugins

copy Release\Icons\xstatus_ICQ.dll                                      %Temp%\miransi\Icons
copy ..\..\miranda-tools\installer\icons\bin\locolor\proto_AIM.dll      %Temp%\miransi\Icons
copy ..\..\miranda-tools\installer\icons\bin\locolor\proto_ICQ.dll      %Temp%\miransi\Icons
copy ..\..\miranda-tools\installer\icons\bin\locolor\proto_IRC.dll      %Temp%\miransi\Icons
copy ..\..\miranda-tools\installer\icons\bin\locolor\proto_JABBER.dll   %Temp%\miransi\Icons
copy ..\..\miranda-tools\installer\icons\bin\locolor\proto_MSN.dll      %Temp%\miransi\Icons
copy ..\..\miranda-tools\installer\icons\bin\locolor\proto_YAHOO.dll    %Temp%\miransi\Icons

del /Q /F "%FileVer%.zip"
7z.exe a -tzip -r -mx=9 "%FileVer%.zip" %Temp%\miransi/* ChangeLog.txt
rd /Q /S %Temp%\miransi

set FileVer=miranda-im-v%Version%-unicode
rd /Q /S %Temp%\pdbw >nul
md %Temp%\pdbw
md %Temp%\pdbw\plugins

copy ..\src\Release_Unicode\miranda32.pdb                   %Temp%\pdbw
rem  Protocols
copy ..\protocols\AimOscar\Release\Aim.pdb                  %Temp%\pdbw\plugins
copy ..\protocols\IcqOscarJ\Release\ICQ.pdb                 %Temp%\pdbw\plugins
copy ..\protocols\IRCG\Release_Unicode\IRC.pdb              %Temp%\pdbw\plugins
copy ..\protocols\JabberG\Release_Unicode\jabber.pdb        %Temp%\pdbw\plugins
copy ..\protocols\MSN\Release_Unicode\MSN.pdb               %Temp%\pdbw\plugins
copy ..\protocols\Yahoo\Release\Yahoo.pdb                   %Temp%\pdbw\plugins
copy ..\protocols\Gadu-Gadu\Release\GG.pdb                  %Temp%\pdbw\plugins
rem  Unicode plugins
copy ..\plugins\avs\Release_Unicode\avs.pdb                 %Temp%\pdbw\plugins
copy ..\plugins\chat\Release_Unicode\chat.pdb               %Temp%\pdbw\plugins
copy ..\plugins\clist\Release_Unicode\clist_classic.pdb     %Temp%\pdbw\plugins
copy ..\plugins\db3x_mmap\Release\dbx_mmap.pdb              %Temp%\pdbw\plugins
copy ..\plugins\srmm\Release_Unicode\srmm.pdb               %Temp%\pdbw\plugins
rem  Non-Unicode plugins
copy ..\plugins\import\Release_Unicode\import.pdb           %Temp%\pdbw\plugins

del /Q /F "%FileVer%-pdb.zip"
7z.exe a -tzip -r -mx=9 "%FileVer%-pdb.zip" %Temp%\pdbw/*
rd /Q /S %Temp%\pdbw

rd /Q /S %Temp%\miransiw >nul
md %Temp%\miransiw
md %Temp%\miransiw\Plugins
md %Temp%\miransiw\Icons

copy "Release Unicode\miranda32.exe"              %Temp%\miransiw
copy "Release Unicode\dbtool.exe"                 %Temp%\miransiw
rem  Protocols
copy "Release\plugins\aim.dll"                    %Temp%\miransiw\Plugins
copy "Release\plugins\ICQ.dll"                    %Temp%\miransiw\Plugins
copy "Release Unicode\plugins\IRC.dll"            %Temp%\miransiw\Plugins
copy "Release Unicode\plugins\jabber.dll"         %Temp%\miransiw\Plugins
copy "Release Unicode\plugins\MSN.dll"            %Temp%\miransiw\Plugins
copy "Release\plugins\Yahoo.dll"                  %Temp%\miransiw\Plugins
copy "Release\plugins\GG.dll"                     %Temp%\miransiw\Plugins
rem  Plugins
copy "Release Unicode\plugins\avs.dll"            %Temp%\miransiw\Plugins
copy "Release Unicode\plugins\chat.dll"           %Temp%\miransiw\Plugins
copy "Release Unicode\plugins\clist_classic.dll"  %Temp%\miransiw\Plugins
copy "Release\plugins\dbx_mmap.dll"               %Temp%\miransiw\Plugins
copy "Release\plugins\advaimg.dll"                %Temp%\miransiw\Plugins
copy "Release Unicode\plugins\import.dll"         %Temp%\miransiw\Plugins
copy "Release Unicode\plugins\srmm.dll"           %Temp%\miransiw\Plugins
rem misc
copy "Release Unicode\zlib.dll"                   %Temp%\miransiw
copy "Release\winssl.dll"                         %Temp%\miransiw
copy ..\docs\Readme.txt                           %Temp%\miransiw
copy ..\docs\mirandaboot.ini                      %Temp%\miransiw
copy ..\protocols\IRCG\docs\IRC_Servers.ini       %Temp%\miransiw\plugins

copy "Release\Icons\xstatus_ICQ.dll"                                    %Temp%\miransiw\Icons
copy ..\..\miranda-tools\installer\icons\bin\hicolor\proto_AIM.dll      %Temp%\miransiw\Icons
copy ..\..\miranda-tools\installer\icons\bin\hicolor\proto_ICQ.dll      %Temp%\miransiw\Icons
copy ..\..\miranda-tools\installer\icons\bin\hicolor\proto_IRC.dll      %Temp%\miransiw\Icons
copy ..\..\miranda-tools\installer\icons\bin\hicolor\proto_JABBER.dll   %Temp%\miransiw\Icons
copy ..\..\miranda-tools\installer\icons\bin\hicolor\proto_MSN.dll      %Temp%\miransiw\Icons
copy ..\..\miranda-tools\installer\icons\bin\hicolor\proto_YAHOO.dll    %Temp%\miransiw\Icons

del /Q /F "%FileVer%.zip"
7z.exe a -tzip -r -mx=9 "%FileVer%.zip" %Temp%\miransiw/*
rd /Q /S %Temp%\miransiw

makensis ../../miranda-tools/installer/miranda.nsi
makensis ../../miranda-tools/installer/miranda-unicode.nsi

