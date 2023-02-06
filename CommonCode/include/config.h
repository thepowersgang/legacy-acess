/*
AcessOS/Acess Basic v1
Configuration Settings
HEADER
*/
#ifndef _CONFIG_H
#define _CONFIG_H
#include <mboot.h>

extern char	*gsInitProgram;
extern char *gsRootDevice;
extern char	*gsBootDevice;

extern void Config_LoadMBoot(mboot_info *mboot);
extern void Config_LoadDefaults();
extern void Config_PrintConfig();

#endif
