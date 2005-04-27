/*

Miranda IM: the free IM client for Microsoft* Windows*

Copyright 2000-2003 Miranda ICQ/IM project, 
all portions of this codebase are copyrighted to the people 
listed in contributors.txt.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details .

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

$Id$

/*
 * templates for the message log...
 */

#include "commonheaders.h"
#pragma hdrstop
#include "msgs.h"
#include "m_popup.h"
#include "nen.h"
#include "functions.h"
#include "templates.h"

/*
 * hardcoded default set of templates for both LTR and RTL.
 * cannot be changed and may be used at any time to "revert" to a working layout
 */
 
TemplateSet LTR_Default = { TRUE, 
    "%I%S  %_%N %~said %D%_%n%H%S  %h:%m:%s:%|%M",
    "%I%S  %_%N %~said %D%_%n%H%S  %h:%m:%s:%|%M",
    "%I%S  %_%N %~said %D%_%n%H%S  %h:%m:%s:%|%M",
    "%I%S  %_%N %~said %D%_%n%H%S  %h:%m:%s:%|%M",
    "%S  %h:%m:%s:%|%M",
    "%S  %h:%m:%s:%|%M",
    "%I%S%D, %h:%m:%s, %N %M",
    "Default LTR"
};

TemplateSet RTL_Default = { TRUE, 
    "%I%S  %_%N %~said %D%_%n%H%S  %h:%m:%s:%|%M",
    "%I%S  %_%N %~said %D%_%n%H%S  %h:%m:%s:%|%M",
    "%I%S  %_%N %~said %D%_%n%H%S  %h:%m:%s:%|%M",
    "%I%S  %_%N %~said %D%_%n%H%S  %h:%m:%s:%|%M",
    "%S  %h:%m:%s:%|%M",
    "%S  %h:%m:%s:%|%M",
    "%I%S%D, %h:%m:%s, %N %M",
    "Default LTR"
};

