// ---------------------------------------------------------------------------80
//                ICQ plugin for Miranda Instant Messenger
//                ________________________________________
// 
// Copyright © 2000,2001 Richard Hughes, Roland Rabien, Tristan Van de Vreede
// Copyright © 2001,2002 Jon Keating, Richard Hughes
// Copyright © 2002,2003,2004 Martin Öberg, Sam Kothari, Robert Rainwater
// Copyright © 2004,2005,2006,2007 Joe Kucera
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
// File name      : $Source: /cvsroot/miranda/miranda/protocols/IcqOscarJ/capabilities.c,v $
// Revision       : $Revision$
// Last change on : $Date$
// Last change by : $Author$
//
// DESCRIPTION:
//
//  Contains helper functions to handle oscar user capabilities. Scanning and
//  adding capabilities are assumed to be more timecritical than looking up
//  capabilites. During the login sequence there could possibly be many hundred
//  scans but only a few lookups. So when you add or change something in this
//  code you must have this in mind, dont do anything that will slow down the
//  adding process too much.
//
// -----------------------------------------------------------------------------

#include "icqoscar.h"


typedef struct icq_capability_s
{
  DWORD fdwMirandaID;              // A bitmask, we use it in order to save database space
  BYTE  CapCLSID[BINARY_CAP_SIZE]; // A binary representation of a oscar capability
} icq_capability;

static icq_capability CapabilityRecord[] =
{
  {CAPF_SRV_RELAY, {CAP_SRV_RELAY }},
  {CAPF_UTF,       {CAP_UTF       }},
  {CAPF_RTF,       {CAP_RTF       }},
  {CAPF_ICQDIRECT, {CAP_ICQDIRECT }},
  {CAPF_TYPING,    {CAP_TYPING    }},
  {CAPF_XTRAZ,     {CAP_XTRAZ     }},
  {CAPF_OSCAR_FILE,{CAP_OSCAR_FILE}}
};



// Deletes all oscar capabilities for a given contact
void ClearAllContactCapabilities(HANDLE hContact)
{
  ICQWriteContactSettingDword(hContact, DBSETTING_CAPABILITIES, 0);
}



// Deletes one or many oscar capabilities for a given contact
void ClearContactCapabilities(HANDLE hContact, DWORD fdwCapabilities)
{
  DWORD fdwContactCaps;


  // Get current capability flags
  fdwContactCaps =  ICQGetContactSettingDword(hContact, DBSETTING_CAPABILITIES, 0);

  // Clear unwanted capabilities
  fdwContactCaps &= ~fdwCapabilities;

  // And write it back to disk
  ICQWriteContactSettingDword(hContact, DBSETTING_CAPABILITIES, fdwContactCaps);
}



// Sets one or many oscar capabilities for a given contact
void SetContactCapabilities(HANDLE hContact, DWORD fdwCapabilities)
{
  DWORD fdwContactCaps;


  // Get current capability flags
  fdwContactCaps =  ICQGetContactSettingDword(hContact, DBSETTING_CAPABILITIES, 0);

  // Update them
  fdwContactCaps |= fdwCapabilities;

  // And write it back to disk
  ICQWriteContactSettingDword(hContact, DBSETTING_CAPABILITIES, fdwContactCaps);
}



// Returns true if the given contact supports the requested capabilites
BOOL CheckContactCapabilities(HANDLE hContact, DWORD fdwCapabilities)
{
  DWORD fdwContactCaps;
  
  
  // Get current capability flags
  fdwContactCaps =  ICQGetContactSettingDword(hContact, DBSETTING_CAPABILITIES, 0);
  
  // Check if all requested capabilities are supported
  if ((fdwContactCaps & fdwCapabilities) == fdwCapabilities)
  {
    return TRUE;
  }
  else
  {
    return FALSE;
  }
}



// Scans a binary buffer for oscar capabilities and adds them to the contact.
// You probably want to call ClearAllContactCapabilities() first.
void AddCapabilitiesFromBuffer(HANDLE hContact, BYTE* pbyBuffer, int nLength)
{
  DWORD fdwContactCaps;
  int iCapability;
  int nIndex;
  int nRecordSize;


  // Calculate the number of records
  nRecordSize = sizeof(CapabilityRecord)/sizeof(icq_capability);

  // Get current capability flags
  fdwContactCaps =  ICQGetContactSettingDword(hContact, DBSETTING_CAPABILITIES, 0);

  // Loop over all capabilities in the buffer and
  // compare them to our own record of capabilities
  for (iCapability = 0; (iCapability + BINARY_CAP_SIZE) <= nLength; iCapability += BINARY_CAP_SIZE)
  {
    for (nIndex = 0; nIndex < nRecordSize; nIndex++)
    {
      if (!memcmp(pbyBuffer + iCapability, CapabilityRecord[nIndex].CapCLSID, BINARY_CAP_SIZE))
      {
        // Match
        fdwContactCaps |= CapabilityRecord[nIndex].fdwMirandaID;
        break;
      }
    }
  }

  // And write it back to disk
  ICQWriteContactSettingDword(hContact, DBSETTING_CAPABILITIES, fdwContactCaps);
}
