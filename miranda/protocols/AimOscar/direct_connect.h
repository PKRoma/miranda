#ifndef DIRECT_CONNECT_H
#define DIRECT_CONNECT_H

struct aim_dc_helper_param
{
	aim_dc_helper_param( CAimProto* _ppro, HANDLE _h ) :
		ppro( _ppro ),
		hContact( _h )
	{}

	CAimProto* ppro;
	HANDLE hContact;
};

void aim_dc_helper( aim_dc_helper_param* );
void aim_direct_connection_initiated(HANDLE hNewConnection, DWORD dwRemoteIP, CAimProto*);

#endif
