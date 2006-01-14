// ---------------------------------------------------------------------------80
//                ICQ plugin for Miranda Instant Messenger
//                ________________________________________
// 
// Copyright © 2000,2001 Richard Hughes, Roland Rabien, Tristan Van de Vreede
// Copyright © 2001,2002 Jon Keating, Richard Hughes
// Copyright © 2002,2003,2004 Martin Öberg, Sam Kothari, Robert Rainwater
// Copyright © 2004,2005 Joe Kucera
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
//  Describe me here please...
//
// -----------------------------------------------------------------------------

#include "icqoscar.h"



struct fieldnames_t interestsField[]={
  {100, _T("Art")},
  {101, _T("Cars")},
  {102, _T("Celebrity Fans")},
  {103, _T("Collections")},
  {104, _T("Computers")},
  {105, _T("Culture & Literature")},
  {106, _T("Fitness")},
  {107, _T("Games")},
  {108, _T("Hobbies")},
  {109, _T("ICQ - Providing Help")},
  {110, _T("Internet")},
  {111, _T("Lifestyle")},
  {112, _T("Movies/TV")},
  {113, _T("Music")},
  {114, _T("Outdoor Activities")},
  {115, _T("Parenting")},
  {116, _T("Pets/Animals")},
  {117, _T("Religion")},
  {118, _T("Science/Technology")},
  {119, _T("Skills")},
  {120, _T("Sports")},
  {121, _T("Web Design")},
  {122, _T("Nature and Environment")},
  {123, _T("News & Media")},
  {124, _T("Government")},
  {125, _T("Business & Economy")},
  {126, _T("Mystics")},
  {127, _T("Travel")},
  {128, _T("Astronomy")},
  {129, _T("Space")},
  {130, _T("Clothing")},
  {131, _T("Parties")},
  {132, _T("Women")},
  {133, _T("Social science")},
  {134, _T("60's")},
  {135, _T("70's")},
  {136, _T("80's")},
  {137, _T("50's")},
  {138, _T("Finance and corporate")},
  {139, _T("Entertainment")},
  {140, _T("Consumer electronics")},
  {141, _T("Retail stores")},
  {142, _T("Health and beauty")},
  {143, _T("Media")},
  {144, _T("Household products")},
  {145, _T("Mail order catalog")},
  {146, _T("Business services")},
  {147, _T("Audio and visual")},
  {148, _T("Sporting and athletic")},
  {149, _T("Publishing")},
  {150, _T("Home automation")},
  {-1,  NULL}};

struct fieldnames_t languageField[]={
  {1, _T("Arabic")},
  {2, _T("Bhojpuri")},
  {3, _T("Bulgarian")},
  {4, _T("Burmese")},
  {5, _T("Cantonese")},
  {6, _T("Catalan")},
  {7, _T("Chinese")},
  {8, _T("Croatian")},
  {9, _T("Czech")},
  {10, _T("Danish")},
  {11, _T("Dutch")},
  {12, _T("English")},
  {13, _T("Esperanto")},
  {14, _T("Estonian")},
  {15, _T("Farci")},
  {16, _T("Finnish")},
  {17, _T("French")},
  {18, _T("Gaelic")},
  {19, _T("German")},
  {20, _T("Greek")},
  {21, _T("Hebrew")},
  {22, _T("Hindi")},
  {23, _T("Hungarian")},
  {24, _T("Icelandic")},
  {25, _T("Indonesian")},
  {26, _T("Italian")},
  {27, _T("Japanese")},
  {28, _T("Khmer")},
  {29, _T("Korean")},
  {30, _T("Lao")},
  {31, _T("Latvian")},
  {32, _T("Lithuanian")},
  {33, _T("Malay")},
  {34, _T("Norwegian")},
  {35, _T("Polish")},
  {36, _T("Portuguese")},
  {37, _T("Romanian")},
  {38, _T("Russian")},
  {39, _T("Serbo-Croatian")},
  {40, _T("Slovak")},
  {41, _T("Slovenian")},
  {42, _T("Somali")},
  {43, _T("Spanish")},
  {44, _T("Swahili")},
  {45, _T("Swedish")},
  {46, _T("Tagalog")},
  {47, _T("Tatar")},
  {48, _T("Thai")},
  {49, _T("Turkish")},
  {50, _T("Ukrainian")},
  {51, _T("Urdu")},
  {52, _T("Vietnamese")},
  {53, _T("Yiddish")},
  {54, _T("Yoruba")},
  {55, _T("Afrikaans")},
  {56, _T("Bosnian")},
  {57, _T("Persian")},
  {58, _T("Albanian")},
  {59, _T("Armenian")},
  {60, _T("Punjabi")},
  {61, _T("Chamorro")},
  {62, _T("Mongolian")},
  {63, _T("Mandarin")},
  {64, _T("Taiwaness")},
  {65, _T("Macedonian")},
  {66, _T("Sindhi")},
  {67, _T("Welsh")},
  {68, _T("Azerbaijani")},
  {69, _T("Kurdish")},
  {70, _T("Gujarati")},
  {71, _T("Tamil")},
  {72, _T("Belorussian")},
  {-1, NULL}};

struct fieldnames_t pastField[]={
  {300, _T("Elementary School")},
  {301, _T("High School")},
  {302, _T("College")},
  {303, _T("University")},
  {304, _T("Military")},
  {305, _T("Past Work Place")},
  {306, _T("Past Organization")},
  {399, _T("Other")},
  {-1,  NULL}};

struct fieldnames_t genderField[]={
  {1, _T("Female")},
  {2, _T("Male")},
  {-1,  NULL}};

struct fieldnames_t workField[]={
  {1, _T("Academic")},
  {2, _T("Administrative")},
  {3, _T("Art/Entertainment")},
  {4, _T("College Student")},
  {5, _T("Computers")},
  {6, _T("Community & Social")},
  {7, _T("Education")},
  {8, _T("Engineering")},
  {9, _T("Financial Services")},
  {10, _T("Government")},
  {11, _T("High School Student")},
  {12, _T("Home")},
  {13, _T("ICQ - Providing Help")},
  {14, _T("Law")},
  {15, _T("Managerial")},
  {16, _T("Manufacturing")},
  {17, _T("Medical/Health")},
  {18, _T("Military")},
  {19, _T("Non-Government Organization")},
  {20, _T("Professional")},
  {21, _T("Retail")},
  {22, _T("Retired")},
  {23, _T("Science & Research")},
  {24, _T("Sports")},
  {25, _T("Technical")},
  {26, _T("University Student")},
  {27, _T("Web building")},
  {99, _T("Other services")},
  {-1,  NULL}};

struct fieldnames_t affiliationField[]={
  {200, _T("Alumni Org.")},
  {201, _T("Charity Org.")},
  {202, _T("Club/Social Org.")},
  {203, _T("Community Org.")},
  {204, _T("Cultural Org.")},
  {205, _T("Fan Clubs")},
  {206, _T("Fraternity/Sorority")},
  {207, _T("Hobbyists Org.")},
  {208, _T("International Org.")},
  {209, _T("Nature and Environment Org.")},
  {210, _T("Professional Org.")},
  {211, _T("Scientific/Technical Org.")},
  {212, _T("Self Improvement Group")},
  {213, _T("Spiritual/Religious Org.")},
  {214, _T("Sports Org.")},
  {215, _T("Support Org.")},
  {216, _T("Trade and Business Org.")},
  {217, _T("Union")},
  {218, _T("Volunteer Org.")},
  {299, _T("Other")},
  {-1,  NULL}};

struct fieldnames_t agesField[]={
  {0x0011000D, _T("13-17")},
  {0x00160012, _T("18-22")},
  {0x001D0017, _T("23-29")},
  {0x0027001E, _T("30-39")},
  {0x00310028, _T("40-49")},
  {0x003B0032, _T("50-59")},
  {0x2710003C, _T("60-above")},
  {-1,  NULL}};

struct fieldnames_t maritalField[]={
  {10, _T("Single")},
  {11, _T("Close relationships")},
  {12, _T("Engaged")},
  {20, _T("Married")},
  {30, _T("Divorced")},
  {31, _T("Separated")},
  {40, _T("Widowed")},
  {-1,  NULL}};


  
TCHAR* LookupFieldName(struct fieldnames_t* table, int code)
{
  int i;

  if (code != 0)
  {
    for(i = 0; table[i].code != -1 && table[i].text; i++)
    {
      if (table[i].code == code)
        return TranslateTS(table[i].text);
    }
    
    // Tried to get unexisting field name, you have an
    // error in the data or in the table
    _ASSERT(FALSE);
  }

  return NULL;
}
