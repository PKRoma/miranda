// ---------------------------------------------------------------------------80
//                ICQ plugin for Miranda Instant Messenger
//                ________________________________________
// 
// Copyright � 2000,2001 Richard Hughes, Roland Rabien, Tristan Van de Vreede
// Copyright � 2001,2002 Jon Keating, Richard Hughes
// Copyright � 2002,2003,2004 Martin �berg, Sam Kothari, Robert Rainwater
// Copyright � 2004 Joe Kucera
// 
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
//
// -----------------------------------------------------------------------------
//
// File name      : $Source$
// Revision       : $Revision$
// Last change on : $Date$
// Last change by : $Author$
//
// DESCRIPTION:
//
// Includes all header files that should be precompiled to speed up compilation.
//
// -----------------------------------------------------------------------------



// Windows includes
#include <windows.h>
#include <commctrl.h>

// Standard includes
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <io.h>
#include <direct.h>
#include <fcntl.h>
#include <crtdbg.h>
#include <process.h>

// Miranda IM SDK includes
#include <newpluginapi.h> // This must be included first
#include <m_clc.h>
#include <m_clist.h>
#include <m_database.h>
#include <m_langpack.h>
#include <m_message.h>
#include <m_netlib.h>
#include <m_protocols.h>
#include <m_protomod.h>
#include <m_protosvc.h>
#include <m_options.h>
#include <m_system.h>
#include <m_userinfo.h>
#include <m_utils.h>
#include <m_idle.h>

// Project resources
#include "resource.h"

// ICQ plugin includes
#include "capabilities.h"
#include "i18n.h"
#include "icq_packet.h"
#include "icq_direct.h"
#include "icq_server.h"
#include "icqosc_svcs.h"
#include "icq_opts.h"
#include "icq_servlist.h"
#include "icq_http.h"
#include "icq_fieldnames.h"
#include "icq_constants.h"
#include "icq_infoupdate.h"
#include "icq_avatar.h"
#include "init.h"
#include "stdpackets.h"
#include "tlv.h"
#include "families.h"
#include "utilities.h"
#include "m_icq.h"
#include "icq_advsearch.h"
#include "icq_uploadui.h"
#include "log.h"
#include "channels.h"
#include "forkthread.h"
#include "ui/askauthentication.h"
#include "ui/userinfotab.h"
#include "ui/loginpassword.h"

// :TODO: This should not be here :p
void icq_FirstRunCheck(void);