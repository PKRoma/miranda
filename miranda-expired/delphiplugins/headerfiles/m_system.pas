{***************************************************************
 * Project     : Miranda Plugin API for Delphi
 * Unit Name   : m_system
 * Description : Converted Headerfile from m_system.h
 *
 * Author      : Christian Kästner
 * Date        : 28.04.2001
 *
 * Copyright © 2001 by Christian Kästner (christian.k@stner.de)
 ****************************************************************}

unit m_system;

interface

const
  MIRANDANAME='Miranda';

//miranda/system/modulesloaded
//called after all modules have been successfully initialised
//wParam=lParam=0
//used to resolve double-dependencies in the module load order
ME_SYSTEM_MODULESLOADED='Miranda/System/ModulesLoaded';

//miranda/system/shutdown event
//called just before the application terminates
//the database is still guaranteed to be running during this hook.
//wParam=lParam=0
ME_SYSTEM_SHUTDOWN='Miranda/System/Shutdown';

//miranda/system/oktoexit event
//called before the app goes into shutdown routine to make sure everyone is
//happy to exit
//wParam=lParam=0
//return nonzero to stop the exit cycle
ME_SYSTEM_OKTOEXIT='Miranda/System/OkToExitEvent';

//miranda/system/oktoexit service
//Check if everyone is happy to exit
//wParam=lParam=0
//if everyone acknowleges OK to exit then returns true, otherwise false
MS_SYSTEM_OKTOEXIT='Miranda/System/OkToExit';

implementation

end.
