// 0, 4, 2, 34
#define BUILD_NUM 34
#define BUILD_NUM_STR  "34"

#define FILE_VERSION	0, 4, 2, BUILD_NUM
#define PRODUCT_VERSION	0, 4, 2, BUILD_NUM

#ifdef UNICODE
#ifndef _DEBUG
#define FILE_VERSION_STR	"0, 4, 2, UNICODE build " BUILD_NUM_STR
	#define PRODUCT_VERSION_STR	"0, 4, 2, UNICODE build " BUILD_NUM_STR 
#else
#define FILE_VERSION_STR	"0, 4, 2, DEBUG UNICODE build " BUILD_NUM_STR
#define PRODUCT_VERSION_STR	"0, 4, 2, DEBUG UNICODE build " BUILD_NUM_STR
#endif
#else
#ifndef _DEBUG
	#define FILE_VERSION_STR	"0, 4, 2, build " BUILD_NUM_STR
	#define PRODUCT_VERSION_STR	"0, 4, 2, build " BUILD_NUM_STR
#else
	#define FILE_VERSION_STR	"0, 4, 2, DEBUG build " BUILD_NUM_STR
	#define PRODUCT_VERSION_STR	"0, 4, 2, DEBUG build " BUILD_NUM_STR
#endif
#endif
