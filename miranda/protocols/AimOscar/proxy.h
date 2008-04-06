#ifndef PROXY_H
#define PROXY_H

struct aim_proxy_helper_param
{
	aim_proxy_helper_param( CAimProto* _ppro, HANDLE _h ) :
		ppro( _ppro ),
		hContact( _h )
	{}

	CAimProto* ppro;
	HANDLE hContact;
};

void aim_proxy_helper( aim_proxy_helper_param* );

int proxy_initialize_send(HANDLE connection,char* sn, char* cookie);
int proxy_initialize_recv(HANDLE connection,char* sn, char* cookie,unsigned short port_check);

#endif
