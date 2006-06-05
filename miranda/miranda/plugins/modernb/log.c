#include "log.h"

#include "commonheaders.h"

#include <stdarg.h>
#include <stdio.h>
#include <string.h>


void Log(const char *file,int line,const char *fmt,...)
{
	FILE *fp;
	va_list vararg;
	const char *file_tmp;
	char str[1024];

	file_tmp = strrchr(file, '\\');
	if (file_tmp == NULL)
		file_tmp = file;
	else
		file_tmp++;

	va_start(vararg,fmt);
	mir_vsnprintf(str,sizeof(str),fmt,vararg);
	va_end(vararg);
	fp=fopen("c:\\miranda_clist_modern_pescuma.log.txt","at");
	//fprintf(fp,"[%u - %u] %s %d: %s\n",GetCurrentThreadId(),GetTickCount(),file_tmp,line,str);
	fprintf(fp,"[%u - %u]: %s \t\t[%s %d]\n",GetCurrentThreadId(),GetTickCount(),str,file_tmp,line);
	fclose(fp);
}

