{***************************************************************
 * Project     : Miranda Plugin API for Delphi
 * Unit Name   : globals
 * Description : Contains globals for every plugin.
 *
 * Author      : Christian K�stner
 * Date        : 28.04.2001
 *
 * Copyright � 2001 by Christian K�stner (christian.k@stner.de)
 ****************************************************************}

unit globals;
{Global variables for the whole plugin}

interface

uses
  newpluginapi, windows;

var
  PluginInfo:TPluginInfo;
  PluginLink:TPluginLink;
  PluginInfoVersion:string;
  MirandaVersion:DWord;

implementation

end.
