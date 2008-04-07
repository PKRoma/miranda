#include "aim.h"
#include "avatars.h"

struct avatar_request_thread_param
{
	avatar_request_thread_param( CAimProto* _ppro, char* _data ) :
		ppro( _ppro ),
		data( _data )
	{}

	~avatar_request_thread_param()
	{	delete[] data;
	}

	CAimProto* ppro;
	char* data;
};

void avatar_request_thread(avatar_request_thread_param* p)
{
	EnterCriticalSection( &p->ppro->avatarMutex );
	if ( !p->ppro->hAvatarConn && p->ppro->hServerConn ) {
		p->ppro->LOG("Starting Avatar Connection.");
		ResetEvent( p->ppro->hAvatarEvent ); //reset incase a disconnection occured and state is now set following the removal of queued threads
		p->ppro->hAvatarConn = (HANDLE)1;    //set so no additional service request attempts are made while aim is still processing the request
		p->ppro->aim_new_service_request( p->ppro->hServerConn, p->ppro->seqno, 0x0010 ) ;//avatar service connection!
	}
	LeaveCriticalSection( &p->ppro->avatarMutex );//not further below because the thread will become trapped if the connection dies.

	if ( WaitForSingleObject( p->ppro->hAvatarEvent, INFINITE ) == WAIT_OBJECT_0 ) 	{
		if (Miranda_Terminated()) {
			LeaveCriticalSection( &p->ppro->avatarMutex );
			delete p;
			return;
		}
		
		if ( p->ppro->hServerConn ) {
			char* sn = strtok(p->data,";");
			char* hash_string = strtok(NULL,";");
			char* hash = new char[lstrlenA(hash_string)/2];
			string_to_bytes( hash_string, hash );
			p->ppro->LOG("Requesting an Avatar: %s(Hash: %s)", sn, hash_string );
			p->ppro->aim_request_avatar( p->ppro->hAvatarConn, p->ppro->avatar_seqno, sn, hash, (unsigned short)lstrlenA(hash_string)/2 );
		}
		else {
			SetEvent( p->ppro->hAvatarEvent );
			LeaveCriticalSection( &p->ppro->avatarMutex );
			return;
		}
	}
	delete p;
}

void CAimProto::avatar_request_handler(TLV &tlv, HANDLE &hContact,char* sn,int &offset)//checks to see if the avatar needs requested
{
	if(!getByte( AIM_KEY_DA, 0))
	{
		if(char* norm_sn=normalize_name(sn))
		{
			int avatar_exist=1;
			int hash_size=(int)tlv.ubyte(offset+3);
			char* hash=tlv.part(offset+4,hash_size);
			char* hash_string=bytes_to_string(hash,hash_size);
			if(lstrcmpA(hash_string,"0201d20472"))//gaim default icon fix- we don't want their blank icon displaying.
			{
				
				if(char* photo_path=getSetting(hContact,"ContactPhoto","File"))//check for image type in db 
				{
					char filetype[5];
					memcpy(&filetype,&photo_path[lstrlenA(photo_path)]-4,5);
					char* filename=strlcat(norm_sn,filetype);
					char* path;
					FILE* out=open_contact_file(norm_sn,filename,"rb",path,0);
					if(out!=NULL)//make sure file exist then check hash in db
					{
						fclose(out);
						if(char* hash=getSetting(hContact,m_szModuleName,AIM_KEY_AH))
						{
							avatar_exist=lstrcmpA(hash_string,hash);
							if(!avatar_exist)//NULL means it exist
							{
								LOG("Avatar exist for %s. So attempting to apply it",norm_sn);
								avatar_apply(hContact,norm_sn,path);
							}
							delete[] hash;
						}
						delete[] path;
					}
					delete[] photo_path;
					delete[] filename;
				}
				if(avatar_exist)//NULL means it exist
				{
					int length=lstrlenA(sn)+2+hash_size*2;
					char* blob= new char[length];
					mir_snprintf(blob,length,"%s;%s",sn,hash_string);
					LOG("Starting avatar request thread for %s)",sn);
					mir_forkthread((pThreadFunc)avatar_request_thread, new avatar_request_thread_param( this, blob ));
				}
			}
			delete[] hash;
			delete[] hash_string;
			delete[] norm_sn;
		}
	}
}

void avatar_request_limit_thread( CAimProto* ppro )
{
	ppro->LOG("Avatar Request Limit thread begin");
	while( ppro->AvatarLimitThread ) {
		Sleep(1000);
		ppro->LOG("Setting Avatar Request Event...");
		SetEvent( ppro->hAvatarEvent );
	}
	ppro->LOG("Avatar Request Limit Thread has ended");
}

void CAimProto::avatar_retrieval_handler(SNAC &snac)
{
	int sn_length=(int)snac.ubyte(0);
	char* sn=snac.part(1,sn_length);
	if(char* norm_sn=normalize_name(sn))
	{
		HANDLE hContact=find_contact(norm_sn);
		int hash_size=snac.ubyte(4+sn_length);
		unsigned short icon_length=snac.ushort(5+sn_length+hash_size);
		if(icon_length>0)
		{
			char* hash=snac.part(5+sn_length,hash_size);
			char* hash_string=bytes_to_string(hash,hash_size);
			DBWriteContactSettingString(hContact,m_szModuleName,AIM_KEY_AH,hash_string);
			char* file_data=snac.val(7+sn_length+hash_size);
			char type[5]; 
			detect_image_type(file_data,type);
			char* filename=strlcat(norm_sn,type);
			char* path;
			FILE* out=open_contact_file(norm_sn,filename,"wb",path,0);
			LOG("Retrieving an avatar for %s(Hash: %s Length: %u)",norm_sn,hash_string,icon_length);
			if ( out != NULL )
			{
				fwrite( file_data, icon_length, 1, out );
				fclose( out );
				avatar_apply(hContact,norm_sn,path);
				delete[] path;
			}
			else
				LOG("Error saving avatar to file for %s",norm_sn);
			delete[] hash;
			delete[] hash_string;
			delete[] filename;
		}
		else
			LOG("AIM sent avatar of zero length for %s.(Usually caused by repeated request for the same icon)",norm_sn);
		delete[] norm_sn;	
	}
	delete[] sn;
}

void CAimProto::avatar_apply(HANDLE &hContact,char* sn,char* filename)
{
	if(char* photo_path=getSetting(hContact,"ContactPhoto","File"))//compare filenames if different and exist then ignore icon change
	{
		filename[lstrlenA(filename)-4]='\0';
		/*if(lstrcmpiA(filename,photo_path))
		{
			char* path;
			FILE* out=open_contact_file(sn,photo_path,"rb",path);
			if(out!=NULL)
			{
				fclose(out);
				delete[] photo_path;
				delete[] path;
				return;
			}
		}*/
		delete[] photo_path;
		filename[lstrlenA(filename)]='.';
	}
	PROTO_AVATAR_INFORMATION AI;
	AI.cbSize = sizeof AI;
	AI.hContact = hContact;
	strlcpy(AI.filename,filename,lstrlenA(filename)+1);
	char filetype[4];
	memcpy(&filetype,&filename[lstrlenA(filename)]-3,4);
	if(!lstrcmpiA(filetype,"jpg")||!lstrcmpiA(filetype,"jpeg"))
		AI.format=PA_FORMAT_JPEG;
	else if(!lstrcmpiA(filetype,"gif"))
		AI.format=PA_FORMAT_GIF;
	else if(!lstrcmpiA(filetype,"png"))
		AI.format=PA_FORMAT_PNG;
	else if(!lstrcmpiA(filetype,"bmp"))
		AI.format=PA_FORMAT_BMP;
	DBWriteContactSettingString( hContact, "ContactPhoto", "File", AI.filename );
	LOG("Successfully added avatar for %s(%s)",sn,AI.filename);
	ProtoBroadcastAck(m_szModuleName, hContact, ACKTYPE_AVATAR, ACKRESULT_SUCCESS,&AI,0);
}

void detect_image_type(char* stream,char* type_ret)
{
	if(stream[0]=='G'&&stream[1]=='I'&&stream[2]=='F')
		strlcpy(type_ret,".gif",5);
	else if(stream[1]=='P'&&stream[2]=='N'&&stream[3]=='G')
		strlcpy(type_ret,".png",5);
	else if(stream[0]=='B'&&stream[1]=='M')
		strlcpy(type_ret,".bmp",5);
	else//assume jpg
		strlcpy(type_ret,".jpg",5);
}
