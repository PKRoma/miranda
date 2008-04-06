#ifndef SNAC_H
#define SNAC_H
#define SNAC_SIZE 10
class SNAC
{
private:
	unsigned short service_;
	unsigned short subgroup_;
	unsigned short length_;
	unsigned short id_;
	char* value_;
public:
	SNAC(char* buf, unsigned short length);
	int cmp(unsigned short service);
	int subcmp(unsigned short subgroup);
	unsigned short ushort(int pos=0);
	unsigned long ulong(int pos=0);
	unsigned char ubyte(int pos=0);
	char* part(int pos, int length);
	char* val(int pos=0);
	unsigned short len();
	unsigned short id();
};
#endif
