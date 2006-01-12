#define FILE_VERSION	0, 4, 1, 16
#define PRODUCT_VERSION	0, 4, 1, 16

#ifdef UNICODE
#ifndef _DEBUG
	#define FILE_VERSION_STR	"0, 4, 1, UNICODE build 16"
	#define PRODUCT_VERSION_STR	"0, 4, 1, UNICODE build 16"
#else
	#define FILE_VERSION_STR	"0, 4, 1, DEBUG UNICODE build 16"
	#define PRODUCT_VERSION_STR	"0, 4, 1, DEBUG UNICODE build 16"
#endif
#else
#ifndef _DEBUG
	#define FILE_VERSION_STR	"0, 4, 1, build 16"
	#define PRODUCT_VERSION_STR	"0, 4, 1, build 16"
#else
	#define FILE_VERSION_STR	"0, 4, 1, DEBUG build 16"
	#define PRODUCT_VERSION_STR	"0, 4, 1, DEBUG build 16"
#endif
#endif
