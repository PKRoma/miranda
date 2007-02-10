#include "avatars.h"
CRITICAL_SECTION avatarMutex;
static int avatar_module_size=0;
static char* avatar_module_ptr=NULL;
void avatar_request_handler(TLV &tlv, HANDLE &hContact,char* sn,int &offset)//checks to see if the avatar needs requested
{
	if(!DBGetContactSettingByte(NULL, AIM_PROTOCOL_NAME, AIM_KEY_DA, 0))
	{
		int avatar_exist=1;
		int hash_size=(int)tlv.ubyte(offset+3);
		char* hash=tlv.part(offset+4,hash_size);
		char* hash_string=bytes_to_string(hash,hash_size);
		if(lstrcmp(hash_string,"0201d20472"))//gaim default icon fix- we don't want their blank icon displaying.
		{
			
			if(char* photo_path=getSetting(hContact,"ContactPhoto","File"))//check for image type in db 
			{
				char filetype[5];
				memcpy(&filetype,&photo_path[lstrlen(photo_path)]-4,5);
				char* filename=strlcat(sn,filetype);
				char* path;
				FILE* out=open_contact_file(sn,filename,"rb",path,0);
				if(out!=NULL)//make sure file exist then check hash in db
				{
					fclose(out);
					if(char* hash=getSetting(hContact,AIM_PROTOCOL_NAME,AIM_KEY_AH))
					{
						avatar_exist=lstrcmp(hash_string,hash);
						if(!avatar_exist)//NULL means it exist
						{
							LOG("Avatar exist for %s. So attempting to apply it",sn);
							avatar_apply(hContact,sn,path);
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
				int length=lstrlen(sn)+2+hash_size*2;
				char* blob= new char[length];
				mir_snprintf(blob,length,"%s;%s",sn,hash_string);
				LOG("Starting avatar request thread for %s)",sn);
				ForkThread((pThreadFunc)avatar_request_thread,blob);
			}
		}
		delete[] hash;
		delete[] hash_string;
	}
}
void avatar_request_thread(char* data)
{
	EnterCriticalSection(&avatarMutex);
	if(!conn.hAvatarConn&&conn.hServerConn)
	{
		LOG("Starting Avatar Connection.");
		ResetEvent(conn.hAvatarEvent);//reset incase a disconnection occured and state is now set following the removal of queued threads
		conn.hAvatarConn=(HANDLE)1;//set so no additional service request attempts are made while aim is still processing the request
		aim_new_service_request(conn.hServerConn,conn.seqno,0x0010);//avatar service connection!
	}
	LeaveCriticalSection(&avatarMutex);//not further below because the thread will become trapped if the connection dies.
	if(WaitForSingleObject(conn.hAvatarEvent ,  INFINITE )==WAIT_OBJECT_0)
	{
		if (Miranda_Terminated())
		{
			LeaveCriticalSection(&avatarMutex);
			delete[] data;
			return;
		}
		if(conn.hServerConn)
		{
			char* sn=strtok(data,";");
			char* hash_string=strtok(NULL,";");
			char* hash= new char[lstrlen(hash_string)/2];
			string_to_bytes(hash_string,hash);
			LOG("Requesting an Avatar: %s(Hash: %s)",sn,hash_string);
			aim_request_avatar(conn.hAvatarConn,conn.avatar_seqno,sn,hash,(unsigned short)lstrlen(hash_string)/2);
		}
		else
		{
			SetEvent(conn.hAvatarEvent);
			LeaveCriticalSection(&avatarMutex);
			return;
		}
	}
	delete[] data;
}
void avatar_request_limit_thread()
{
	LOG("Avatar Request Limit thread begin");
	while(conn.AvatarLimitThread)
	{
		Sleep(1000);
		LOG("Setting Avatar Request Event...");
		SetEvent(conn.hAvatarEvent);
	}
	LOG("Avatar Request Limit Thread has ended");
}
void avatar_retrieval_handler(SNAC &snac)
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
			DBWriteContactSettingString(hContact,AIM_PROTOCOL_NAME,AIM_KEY_AH,hash_string);
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
void avatar_apply(HANDLE &hContact,char* sn,char* filename)
{
	if(char* photo_path=getSetting(hContact,"ContactPhoto","File"))//compare filenames if different and exist then ignore icon change
	{
		filename[lstrlen(filename)-4]='\0';
		/*if(lstrcmpi(filename,photo_path))
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
		filename[lstrlen(filename)]='.';
	}
	PROTO_AVATAR_INFORMATION AI;
	AI.cbSize = sizeof AI;
	AI.hContact = hContact;
	strlcpy(AI.filename,filename,lstrlen(filename)+1);
	char filetype[4];
	memcpy(&filetype,&filename[lstrlen(filename)]-3,4);
	if(!lstrcmpi(filetype,"jpg")||!lstrcmpi(filetype,"jpeg"))
		AI.format=PA_FORMAT_JPEG;
	else if(!lstrcmpi(filetype,"gif"))
		AI.format=PA_FORMAT_GIF;
	else if(!lstrcmpi(filetype,"png"))
		AI.format=PA_FORMAT_PNG;
	else if(!lstrcmpi(filetype,"bmp"))
		AI.format=PA_FORMAT_BMP;
	DBWriteContactSettingString( hContact, "ContactPhoto", "File", AI.filename );
	LOG("Successfully added avatar for %s(%s)",sn,AI.filename);
	ProtoBroadcastAck(AIM_PROTOCOL_NAME, hContact, ACKTYPE_AVATAR, ACKRESULT_SUCCESS,&AI,0);
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
