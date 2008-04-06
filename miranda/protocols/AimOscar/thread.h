#ifndef THREAD_H
#define THREAD_H

struct CAimProto;

struct file_thread_param
{
	file_thread_param( CAimProto* _ppro, char* _arg ) :
		ppro( _ppro ),
		blob( _arg )
	{}

	CAimProto* ppro;
	char* blob;
};

void aim_keepalive_thread( CAimProto* fa );

void accept_file_thread( file_thread_param* );
void redirected_file_thread( file_thread_param* );
void proxy_file_thread( file_thread_param* );
#endif
