// 0, 4, 3, 68
#define BUILD_NUM 68
#define BUILD_NUM_STR  "68"

#define FILE_VERSION	0, 4, 3, BUILD_NUM
#define PRODUCT_VERSION	0, 4, 3, BUILD_NUM

#ifdef UNICODE
#ifndef _DEBUG
#define FILE_VERSION_STR	"0, 4, 3, UNICODE build " BUILD_NUM_STR
	#define PRODUCT_VERSION_STR	"0, 4, 3, UNICODE build " BUILD_NUM_STR 
#else
#define FILE_VERSION_STR	"0, 4, 3, DEBUG UNICODE build " BUILD_NUM_STR
#define PRODUCT_VERSION_STR	"0, 4, 3, DEBUG UNICODE build " BUILD_NUM_STR
#endif
#else
#ifndef _DEBUG
	#define FILE_VERSION_STR	"0, 4, 3, build " BUILD_NUM_STR
	#define PRODUCT_VERSION_STR	"0, 4, 3, build " BUILD_NUM_STR
#else
	#define FILE_VERSION_STR	"0, 4, 3, DEBUG build " BUILD_NUM_STR
	#define PRODUCT_VERSION_STR	"0, 4, 3, DEBUG build " BUILD_NUM_STR
#endif
#endif
