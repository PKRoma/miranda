#ifndef SERVICES_H
#define SERVICES_H
#include "defines.h"
int ModulesLoaded(WPARAM wParam,LPARAM lParam);
int PreBuildContactMenu(WPARAM wParam,LPARAM lParam);
int PreShutdown(WPARAM wParam,LPARAM lParam);
int ContactDeleted(WPARAM wParam,LPARAM lParam);
int IdleChanged(WPARAM wParam, LPARAM lParam);
void CreateServices();
#endif
