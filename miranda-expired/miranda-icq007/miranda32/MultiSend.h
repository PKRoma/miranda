

#define MAX_MULTISEND 50
typedef struct{
	unsigned long	destuin;
	HWND			sendwnd;
	unsigned long   sendto[MAX_MULTISEND];

}_MULTISEND;

