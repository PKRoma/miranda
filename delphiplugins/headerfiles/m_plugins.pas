{***************************************************************
 * Project     : Miranda Plugin API for Delphi
 * Unit Name   : m_plugins
 * Description : Converted Headerfile from m_plugins.h
 *
 * Author      : Christian Kästner
 * Date        : 28.04.2001
 *
 * Copyright © 2001 by Christian Kästner (christian.k@stner.de)
 ****************************************************************}

unit m_plugins;

interface

const
  DEFMOD_PROTOCOLICQ	 =	1;
  DEFMOD_PROTOCOLMSN	 =	2;
  DEFMOD_UIFINDADD	 =	3;
  DEFMOD_UIUSERINFO	 =	4;
  DEFMOD_SRMESSAGE	 =	5;
  DEFMOD_SRURL		 =	6;
  DEFMOD_SREMAIL	 =	7;
  DEFMOD_SRAUTH		 =	8;
  DEFMOD_SRFILE		 =	9;
  DEFMOD_UIHELP		 =	10;
  DEFMOD_UIHISTORY	 =	11;
  DEFMOD_RNDCHECKUPD	 =	12;
  DEFMOD_RNDICQIMPORT	 =	13;
  DEFMOD_RNDAUTOAWAY	 =	14;
  DEFMOD_RNDUSERONLINE	 =      15;
  DEFMOD_RNDCRYPT        =      16;

  DEFMOD_HIGHEST         =      16;

//plugins/getdisabledefaultarray
//gets an array of the modules that the plugins report they want to replace
//wParam=lParam=0
//returns a pointer to an array of ints, with elements 1 or 0 indexed by the
//DEFMOD_ constants. 1 to signify that the default module shouldn't be loaded.
//this is primarily for use by the core's module initialiser, but could also
//be used by modules that are doing naughty things that are very
//feature-dependent
const
  MS_PLUGINS_GETDISABLEDEFAULTARRAY = 'Plugins/GetDisableDefaultArray';


implementation




end.
