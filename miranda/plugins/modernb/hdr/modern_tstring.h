#include <string>

#ifndef M_SYSTEM_H__
#error <m_system.h> have to be included before
#endif

typedef std::string astring;

#ifdef UNICODE
typedef std::wstring ustring;

class utfstring : public std::string 
{
	// It is prohibited to initialize by char* outside, use L"xxx"
private:
	utfstring( const char * pChar )		: astring( pChar ) {};
	utfstring& operator=( const char * pChar  )	{ this->operator =( pChar ); return *this; }

public:
	utfstring()							: astring() {};
	utfstring( const utfstring& uStr )	: astring( uStr ) {};
	utfstring( const wchar_t * wChar )	{ char * astr = mir_u2a_cp( wChar, CP_UTF8 ); this->assign( astr );	mir_free( astr );	};
	utfstring( const ustring&  tStr )	{ *this = tStr.c_str(); }
	utfstring& operator=( const ustring&  tStr  )	{ this->operator =( tStr.c_str() ); return *this; }
	utfstring& operator=( const astring&  aStr  )	{ wchar_t * wstr = mir_a2u( aStr.c_str() );  char * astr = mir_u2a_cp( wstr, CP_UTF8);  this->assign( astr );   mir_free( astr ); mir_free( wstr ); return *this; }
	utfstring& operator=( const wchar_t * wChar )	{ if ( wChar != NULL ) {  char * astr = mir_u2a_cp( wChar, CP_UTF8 );   this->assign( astr );   mir_free( astr );   }	else   this->assign( "" ); return *this; }
	
	operator ustring()					{ wchar_t * ustr = mir_a2u_cp( this->c_str(), CP_UTF8 );   ustring res( ustr );  mir_free( ustr );  return res;   }
	operator astring()					{ wchar_t * ustr = mir_a2u_cp( this->c_str(), CP_UTF8 );   char * astr = mir_u2a( ustr );    astring res( astr );  mir_free( astr ); mir_free( ustr );  return res;   }
};

class tstring : public ustring
{
public:
	   tstring()							: ustring() {};
	   tstring(const wchar_t * pwChar)		: ustring( pwChar ) {};
	   tstring(const char * pChar)			{	if ( pChar ){ wchar_t * wstr = mir_a2u( pChar ); this->assign( wstr );	mir_free( wstr ); } 	}
	   tstring(const astring& aStr)			{	*this = aStr.c_str(); }
	   tstring(const utfstring& utfStr)		{	*this = utfStr; }
	   
	   tstring& operator=( const char * pChar )		{ if ( pChar != NULL )	 {  wchar_t * wstr = mir_a2u( pChar );   this->assign( wstr );   mir_free( wstr );   }   else   this->assign( L"" );  return *this; }
	   tstring& operator=( const utfstring& uStr )	{ wchar_t * wstr = mir_a2u_cp( uStr.c_str(), CP_UTF8 );   this->assign( wstr );   mir_free( wstr ); return *this; }
	   tstring& operator=( const astring& aStr )	{ this->operator =( aStr.c_str() ); return *this; }  

	   operator astring()					{  char * astr = mir_u2a( this->c_str() );	   astring res( astr );	   mir_free( astr );   return res;	   }
	   operator utfstring()					{  return  utfstring( *this ); }
};

#else
typedef std::string tstring;
typedef std::string utfstring;
#endif
