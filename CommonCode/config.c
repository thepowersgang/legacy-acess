/*
AcessOS/Acess Basic v1
Configuration Settings
*/
#include <acess.h>
#include <config.h>

char	*gsInitProgram;
char	*gsRootDevice;
char	*gsBootDevice;

void Config_LoadMBoot(mboot_info *mboot)
{
	 int	i, argc, j;
	char	*str, *start;
	char	*argv[18];
	char	*newInit = NULL;
	char	*newRootDev = NULL;
	
	str = (char*)(mboot->cmdline += 0xC0000000);
	LogF(" MBoot Command Line: '%s'\n", str);
	Config_LoadDefaults();
	
	// Create `argc` and `argv`
	start = str;	argc = 0;
	for( i = 0; str[i]; i ++)
	{
		if(str[i] == ' ')
		{
			argv[argc++] = start;
			str[i] = '\0';
			start = str+i+1;
		}
	}
	argv[argc++] = start;
	
	// Parse Arguments
	for( i = 1; i < argc; i ++ )
	{
		// Check for Equals
		for( j = 0; argv[i][j] && argv[i][j] != '='; j ++ );
		if(argv[i][j] == '\0')	continue;
		
		// Mount Assignment
		if( argv[i][0] == '/' )
		{
			argv[i][j] = '\0';
			LogF(" Config_LoadMBoot: Mount '%s' to '%s'\n", argv[i]+j+1, argv[i]);
		}
		else
		{
			argv[i][j] = '\0';
			if( strcmp("INIT", argv[i]) == 0 )		newInit = argv[i]+j+1;
			else if( strcmp("ROOT", argv[i]) == 0 )	newRootDev = argv[i]+j+1;
			else
				warning("Config_LoadMBoot - Unknown boot option '%s', Arg '%s'\n", argv[i], argv[i]+j+1);
		}
	}
	
	// Apply New Settings
	if(newInit != NULL) {
		LogF(" Config_LoadMBoot: New Init Path '%s'\n", newInit);
		gsInitProgram = malloc( strlen(newInit) + 1 );
		strcpy(gsInitProgram, newInit);
	}
	if(newRootDev != NULL) {
		gsRootDevice = malloc( strlen(newRootDev) + 1 );
		strcpy(gsRootDevice, newRootDev);
	}
}

void Config_LoadDefaults()
{
	gsInitProgram = "/Mount/Root/bin/sh2.axe";
	gsRootDevice = "/Devices/ide/A1";
	gsBootDevice = "/Devices/fdd/0";
}

void Config_PrintConfig()
{
	LogF("System Configuration:\n");
	LogF(" INIT = '%s'\n", gsInitProgram);
	LogF(" ROOT = '%s'\n", gsRootDevice);
	LogF(" BOOT = '%s'\n", gsBootDevice);
}
