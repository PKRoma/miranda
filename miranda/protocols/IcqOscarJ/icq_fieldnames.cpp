// ---------------------------------------------------------------------------80
//                ICQ plugin for Miranda Instant Messenger
//                ________________________________________
// 
// Copyright © 2000-2001 Richard Hughes, Roland Rabien, Tristan Van de Vreede
// Copyright © 2001-2002 Jon Keating, Richard Hughes
// Copyright © 2002-2004 Martin  berg, Sam Kothari, Robert Rainwater
// Copyright © 2004-2008 Joe Kucera
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
// File name      : $URL$
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
  {100, LPGENUTF("Art")},
  {101, LPGENUTF("Cars")},
  {102, LPGENUTF("Celebrity Fans")},
  {103, LPGENUTF("Collections")},
  {104, LPGENUTF("Computers")},
  {105, LPGENUTF("Culture & Literature")},
  {106, LPGENUTF("Fitness")},
  {107, LPGENUTF("Games")},
  {108, LPGENUTF("Hobbies")},
  {109, LPGENUTF("ICQ - Providing Help")},
  {110, LPGENUTF("Internet")},
  {111, LPGENUTF("Lifestyle")},
  {112, LPGENUTF("Movies/TV")},
  {113, LPGENUTF("Music")},
  {114, LPGENUTF("Outdoor Activities")},
  {115, LPGENUTF("Parenting")},
  {116, LPGENUTF("Pets/Animals")},
  {117, LPGENUTF("Religion")},
  {118, LPGENUTF("Science/Technology")},
  {119, LPGENUTF("Skills")},
  {120, LPGENUTF("Sports")},
  {121, LPGENUTF("Web Design")},
  {122, LPGENUTF("Nature and Environment")},
  {123, LPGENUTF("News & Media")},
  {124, LPGENUTF("Government")},
  {125, LPGENUTF("Business & Economy")},
  {126, LPGENUTF("Mystics")},
  {127, LPGENUTF("Travel")},
  {128, LPGENUTF("Astronomy")},
  {129, LPGENUTF("Space")},
  {130, LPGENUTF("Clothing")},
  {131, LPGENUTF("Parties")},
  {132, LPGENUTF("Women")},
  {133, LPGENUTF("Social science")},
  {134, LPGENUTF("60's")},
  {135, LPGENUTF("70's")},
  {136, LPGENUTF("80's")},
  {137, LPGENUTF("50's")},
  {138, LPGENUTF("Finance and corporate")},
  {139, LPGENUTF("Entertainment")},
  {140, LPGENUTF("Consumer electronics")},
  {141, LPGENUTF("Retail stores")},
  {142, LPGENUTF("Health and beauty")},
  {143, LPGENUTF("Media")},
  {144, LPGENUTF("Household products")},
  {145, LPGENUTF("Mail order catalog")},
  {146, LPGENUTF("Business services")},
  {147, LPGENUTF("Audio and visual")},
  {148, LPGENUTF("Sporting and athletic")},
  {149, LPGENUTF("Publishing")},
  {150, LPGENUTF("Home automation")},
  {-1,  NULL}};

struct fieldnames_t languageField[]={
  {1, LPGENUTF("Arabic")},
  {2, LPGENUTF("Bhojpuri")},
  {3, LPGENUTF("Bulgarian")},
  {4, LPGENUTF("Burmese")},
  {5, LPGENUTF("Cantonese")},
  {6, LPGENUTF("Catalan")},
  {7, LPGENUTF("Chinese")},
  {8, LPGENUTF("Croatian")},
  {9, LPGENUTF("Czech")},
  {10, LPGENUTF("Danish")},
  {11, LPGENUTF("Dutch")},
  {12, LPGENUTF("English")},
  {13, LPGENUTF("Esperanto")},
  {14, LPGENUTF("Estonian")},
  {15, LPGENUTF("Farci")},
  {16, LPGENUTF("Finnish")},
  {17, LPGENUTF("French")},
  {18, LPGENUTF("Gaelic")},
  {19, LPGENUTF("German")},
  {20, LPGENUTF("Greek")},
  {21, LPGENUTF("Hebrew")},
  {22, LPGENUTF("Hindi")},
  {23, LPGENUTF("Hungarian")},
  {24, LPGENUTF("Icelandic")},
  {25, LPGENUTF("Indonesian")},
  {26, LPGENUTF("Italian")},
  {27, LPGENUTF("Japanese")},
  {28, LPGENUTF("Khmer")},
  {29, LPGENUTF("Korean")},
  {30, LPGENUTF("Lao")},
  {31, LPGENUTF("Latvian")},
  {32, LPGENUTF("Lithuanian")},
  {33, LPGENUTF("Malay")},
  {34, LPGENUTF("Norwegian")},
  {35, LPGENUTF("Polish")},
  {36, LPGENUTF("Portuguese")},
  {37, LPGENUTF("Romanian")},
  {38, LPGENUTF("Russian")},
  {39, LPGENUTF("Serbo-Croatian")},
  {40, LPGENUTF("Slovak")},
  {41, LPGENUTF("Slovenian")},
  {42, LPGENUTF("Somali")},
  {43, LPGENUTF("Spanish")},
  {44, LPGENUTF("Swahili")},
  {45, LPGENUTF("Swedish")},
  {46, LPGENUTF("Tagalog")},
  {47, LPGENUTF("Tatar")},
  {48, LPGENUTF("Thai")},
  {49, LPGENUTF("Turkish")},
  {50, LPGENUTF("Ukrainian")},
  {51, LPGENUTF("Urdu")},
  {52, LPGENUTF("Vietnamese")},
  {53, LPGENUTF("Yiddish")},
  {54, LPGENUTF("Yoruba")},
  {55, LPGENUTF("Afrikaans")},
  {56, LPGENUTF("Bosnian")},
  {57, LPGENUTF("Persian")},
  {58, LPGENUTF("Albanian")},
  {59, LPGENUTF("Armenian")},
  {60, LPGENUTF("Punjabi")},
  {61, LPGENUTF("Chamorro")},
  {62, LPGENUTF("Mongolian")},
  {63, LPGENUTF("Mandarin")},
  {64, LPGENUTF("Taiwaness")},
  {65, LPGENUTF("Macedonian")},
  {66, LPGENUTF("Sindhi")},
  {67, LPGENUTF("Welsh")},
  {68, LPGENUTF("Azerbaijani")},
  {69, LPGENUTF("Kurdish")},
  {70, LPGENUTF("Gujarati")},
  {71, LPGENUTF("Tamil")},
  {72, LPGENUTF("Belorussian")},
  {-1, NULL}};

struct fieldnames_t pastField[]={
  {300, LPGENUTF("Elementary School")},
  {301, LPGENUTF("High School")},
  {302, LPGENUTF("College")},
  {303, LPGENUTF("University")},
  {304, LPGENUTF("Military")},
  {305, LPGENUTF("Past Work Place")},
  {306, LPGENUTF("Past Organization")},
  {399, LPGENUTF("Other")},
  {-1,  NULL}};

struct fieldnames_t genderField[]={
  {1, LPGENUTF("Female")},
  {2, LPGENUTF("Male")},
  {-1,  NULL}};

struct fieldnames_t workField[]={
  {1, LPGENUTF("Academic")},
  {2, LPGENUTF("Administrative")},
  {3, LPGENUTF("Art/Entertainment")},
  {4, LPGENUTF("College Student")},
  {5, LPGENUTF("Computers")},
  {6, LPGENUTF("Community & Social")},
  {7, LPGENUTF("Education")},
  {8, LPGENUTF("Engineering")},
  {9, LPGENUTF("Financial Services")},
  {10, LPGENUTF("Government")},
  {11, LPGENUTF("High School Student")},
  {12, LPGENUTF("Home")},
  {13, LPGENUTF("ICQ - Providing Help")},
  {14, LPGENUTF("Law")},
  {15, LPGENUTF("Managerial")},
  {16, LPGENUTF("Manufacturing")},
  {17, LPGENUTF("Medical/Health")},
  {18, LPGENUTF("Military")},
  {19, LPGENUTF("Non-Government Organization")},
  {20, LPGENUTF("Professional")},
  {21, LPGENUTF("Retail")},
  {22, LPGENUTF("Retired")},
  {23, LPGENUTF("Science & Research")},
  {24, LPGENUTF("Sports")},
  {25, LPGENUTF("Technical")},
  {26, LPGENUTF("University Student")},
  {27, LPGENUTF("Web building")},
  {99, LPGENUTF("Other services")},
  {-1,  NULL}};

struct fieldnames_t affiliationField[]={
  {200, LPGENUTF("Alumni Org.")},
  {201, LPGENUTF("Charity Org.")},
  {202, LPGENUTF("Club/Social Org.")},
  {203, LPGENUTF("Community Org.")},
  {204, LPGENUTF("Cultural Org.")},
  {205, LPGENUTF("Fan Clubs")},
  {206, LPGENUTF("Fraternity/Sorority")},
  {207, LPGENUTF("Hobbyists Org.")},
  {208, LPGENUTF("International Org.")},
  {209, LPGENUTF("Nature and Environment Org.")},
  {210, LPGENUTF("Professional Org.")},
  {211, LPGENUTF("Scientific/Technical Org.")},
  {212, LPGENUTF("Self Improvement Group")},
  {213, LPGENUTF("Spiritual/Religious Org.")},
  {214, LPGENUTF("Sports Org.")},
  {215, LPGENUTF("Support Org.")},
  {216, LPGENUTF("Trade and Business Org.")},
  {217, LPGENUTF("Union")},
  {218, LPGENUTF("Volunteer Org.")},
  {299, LPGENUTF("Other")},
  {-1,  NULL}};

struct fieldnames_t agesField[]={
  {0x0011000D, LPGENUTF("13-17")},
  {0x00160012, LPGENUTF("18-22")},
  {0x001D0017, LPGENUTF("23-29")},
  {0x0027001E, LPGENUTF("30-39")},
  {0x00310028, LPGENUTF("40-49")},
  {0x003B0032, LPGENUTF("50-59")},
  {0x2710003C, LPGENUTF("60-above")},
  {-1,         NULL}};

struct fieldnames_t maritalField[]={
  {10, LPGENUTF("Single")},
  {11, LPGENUTF("Close relationships")},
  {12, LPGENUTF("Engaged")},
  {20, LPGENUTF("Married")},
  {30, LPGENUTF("Divorced")},
  {31, LPGENUTF("Separated")},
  {40, LPGENUTF("Widowed")},
  {-1, NULL}};


  
unsigned char *LookupFieldNameUtf(struct fieldnames_t *table, int code, unsigned char *str, size_t strsize)
{
  int i;

  if (code != 0)
  {
    for(i = 0; table[i].code != -1 && table[i].text; i++)
    {
      if (table[i].code == code)
        return ICQTranslateUtfStatic(table[i].text, str, strsize);
    }
    
    // Tried to get unexisting field name, you have an
    // error in the data or in the table
    _ASSERT(FALSE);
  }

  return NULL;
}
