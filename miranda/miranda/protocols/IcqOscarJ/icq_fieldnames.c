// ---------------------------------------------------------------------------80
//                ICQ plugin for Miranda Instant Messenger
//                ________________________________________
// 
// Copyright © 2000,2001 Richard Hughes, Roland Rabien, Tristan Van de Vreede
// Copyright © 2001,2002 Jon Keating, Richard Hughes
// Copyright © 2002,2003,2004 Martin  berg, Sam Kothari, Robert Rainwater
// Copyright © 2004,2005,2006 Joe Kucera
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
// File name      : $Source: /cvsroot/miranda/miranda/protocols/IcqOscarJ/icq_fieldnames.c,v $
// Revision       : $Revision$
// Last change on : $Date$
// Last change by : $Author$
//
// DESCRIPTION:
//
//  Describe me here please...
//
// -----------------------------------------------------------------------------

#include "icqoscar.h"



struct fieldnames_t interestsField[]={
  {100, "Art"},
  {101, "Cars"},
  {102, "Celebrity Fans"},
  {103, "Collections"},
  {104, "Computers"},
  {105, "Culture & Literature"},
  {106, "Fitness"},
  {107, "Games"},
  {108, "Hobbies"},
  {109, "ICQ - Providing Help"},
  {110, "Internet"},
  {111, "Lifestyle"},
  {112, "Movies/TV"},
  {113, "Music"},
  {114, "Outdoor Activities"},
  {115, "Parenting"},
  {116, "Pets/Animals"},
  {117, "Religion"},
  {118, "Science/Technology"},
  {119, "Skills"},
  {120, "Sports"},
  {121, "Web Design"},
  {122, "Nature and Environment"},
  {123, "News & Media"},
  {124, "Government"},
  {125, "Business & Economy"},
  {126, "Mystics"},
  {127, "Travel"},
  {128, "Astronomy"},
  {129, "Space"},
  {130, "Clothing"},
  {131, "Parties"},
  {132, "Women"},
  {133, "Social science"},
  {134, "60's"},
  {135, "70's"},
  {136, "80's"},
  {137, "50's"},
  {138, "Finance and corporate"},
  {139, "Entertainment"},
  {140, "Consumer electronics"},
  {141, "Retail stores"},
  {142, "Health and beauty"},
  {143, "Media"},
  {144, "Household products"},
  {145, "Mail order catalog"},
  {146, "Business services"},
  {147, "Audio and visual"},
  {148, "Sporting and athletic"},
  {149, "Publishing"},
  {150, "Home automation"},
  {-1,  NULL}};

struct fieldnames_t languageField[]={
  {1, "Arabic"},
  {2, "Bhojpuri"},
  {3, "Bulgarian"},
  {4, "Burmese"},
  {5, "Cantonese"},
  {6, "Catalan"},
  {7, "Chinese"},
  {8, "Croatian"},
  {9, "Czech"},
  {10, "Danish"},
  {11, "Dutch"},
  {12, "English"},
  {13, "Esperanto"},
  {14, "Estonian"},
  {15, "Farci"},
  {16, "Finnish"},
  {17, "French"},
  {18, "Gaelic"},
  {19, "German"},
  {20, "Greek"},
  {21, "Hebrew"},
  {22, "Hindi"},
  {23, "Hungarian"},
  {24, "Icelandic"},
  {25, "Indonesian"},
  {26, "Italian"},
  {27, "Japanese"},
  {28, "Khmer"},
  {29, "Korean"},
  {30, "Lao"},
  {31, "Latvian"},
  {32, "Lithuanian"},
  {33, "Malay"},
  {34, "Norwegian"},
  {35, "Polish"},
  {36, "Portuguese"},
  {37, "Romanian"},
  {38, "Russian"},
  {39, "Serbo-Croatian"},
  {40, "Slovak"},
  {41, "Slovenian"},
  {42, "Somali"},
  {43, "Spanish"},
  {44, "Swahili"},
  {45, "Swedish"},
  {46, "Tagalog"},
  {47, "Tatar"},
  {48, "Thai"},
  {49, "Turkish"},
  {50, "Ukrainian"},
  {51, "Urdu"},
  {52, "Vietnamese"},
  {53, "Yiddish"},
  {54, "Yoruba"},
  {55, "Afrikaans"},
  {56, "Bosnian"},
  {57, "Persian"},
  {58, "Albanian"},
  {59, "Armenian"},
  {60, "Punjabi"},
  {61, "Chamorro"},
  {62, "Mongolian"},
  {63, "Mandarin"},
  {64, "Taiwaness"},
  {65, "Macedonian"},
  {66, "Sindhi"},
  {67, "Welsh"},
  {68, "Azerbaijani"},
  {69, "Kurdish"},
  {70, "Gujarati"},
  {71, "Tamil"},
  {72, "Belorussian"},
  {-1, NULL}};

struct fieldnames_t pastField[]={
  {300, "Elementary School"},
  {301, "High School"},
  {302, "College"},
  {303, "University"},
  {304, "Military"},
  {305, "Past Work Place"},
  {306, "Past Organization"},
  {399, "Other"},
  {-1,  NULL}};

struct fieldnames_t genderField[]={
  {1, "Female"},
  {2, "Male"},
  {-1,  NULL}};

struct fieldnames_t workField[]={
  {1, "Academic"},
  {2, "Administrative"},
  {3, "Art/Entertainment"},
  {4, "College Student"},
  {5, "Computers"},
  {6, "Community & Social"},
  {7, "Education"},
  {8, "Engineering"},
  {9, "Financial Services"},
  {10, "Government"},
  {11, "High School Student"},
  {12, "Home"},
  {13, "ICQ - Providing Help"},
  {14, "Law"},
  {15, "Managerial"},
  {16, "Manufacturing"},
  {17, "Medical/Health"},
  {18, "Military"},
  {19, "Non-Government Organization"},
  {20, "Professional"},
  {21, "Retail"},
  {22, "Retired"},
  {23, "Science & Research"},
  {24, "Sports"},
  {25, "Technical"},
  {26, "University Student"},
  {27, "Web building"},
  {99, "Other services"},
  {-1,  NULL}};

struct fieldnames_t affiliationField[]={
  {200, "Alumni Org."},
  {201, "Charity Org."},
  {202, "Club/Social Org."},
  {203, "Community Org."},
  {204, "Cultural Org."},
  {205, "Fan Clubs"},
  {206, "Fraternity/Sorority"},
  {207, "Hobbyists Org."},
  {208, "International Org."},
  {209, "Nature and Environment Org."},
  {210, "Professional Org."},
  {211, "Scientific/Technical Org."},
  {212, "Self Improvement Group"},
  {213, "Spiritual/Religious Org."},
  {214, "Sports Org."},
  {215, "Support Org."},
  {216, "Trade and Business Org."},
  {217, "Union"},
  {218, "Volunteer Org."},
  {299, "Other"},
  {-1,  NULL}};

struct fieldnames_t agesField[]={
  {0x0011000D, "13-17"},
  {0x00160012, "18-22"},
  {0x001D0017, "23-29"},
  {0x0027001E, "30-39"},
  {0x00310028, "40-49"},
  {0x003B0032, "50-59"},
  {0x2710003C, "60-above"},
  {-1,  NULL}};

struct fieldnames_t maritalField[]={
  {10, "Single"},
  {11, "Close relationships"},
  {12, "Engaged"},
  {20, "Married"},
  {30, "Divorced"},
  {31, "Separated"},
  {40, "Widowed"},
  {-1,  NULL}};


  
char *LookupFieldNameUtf(struct fieldnames_t *table, int code, char *str)
{
  int i;

  if (code != 0)
  {
    for(i = 0; table[i].code != -1 && table[i].text; i++)
    {
      if (table[i].code == code)
        return ICQTranslateUtfStatic(table[i].text, str);
    }
    
    // Tried to get unexisting field name, you have an
    // error in the data or in the table
    _ASSERT(FALSE);
  }

  return NULL;
}
