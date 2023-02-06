/*
 AcessOS Shell Version 2
- Based on IOOS CLI Shell
/// LIB.C \\\
*/
#include "header.h"

// STRINGS
int strcmp(char *s1, char *s2)
{
	while(*s1 == *s2 && *s1 != '\0') {
		s1++; s2++;
	}
	return (int)*s1 - (int)*s2;
}

void strcpy(char *dst, char *src)
{
	while(*src) {
		*dst = *src;
		src++; dst++;
	}
	*dst = '\0';
}

int strlen(char *str)
{
	int retval;
	for(retval = 0; *str != '\0'; str++)
		retval++;
	return retval;
}

// FILE PATHS
int GeneratePath(char *file, char *base, char *tmpPath)
{
	char	*pathComps[64];
	char	*tmpStr;
	int		iPos = 0;
	int		iPos2 = 0;
	
	//Print("GeneratePath: (file=\"");Print(file);Print("\", base=\"");Print(base);Print("\")\n");
	
	// Parse Base Path
	if(file[0] != '/')
	{
		pathComps[iPos++] = tmpStr = base+1;
		while(*tmpStr)
		{
			if(*tmpStr++ == '/')
			{
				pathComps[iPos] = tmpStr;
				iPos ++;
			}
		}
	}
	
	//Print(" GeneratePath: Base Done\n");
	
	// Parse Argument Path
	pathComps[iPos++] = tmpStr = file;
	while(*tmpStr)
	{
		if(*tmpStr++ == '/')
		{
			pathComps[iPos] = tmpStr;
			iPos ++;
		}
	}
	pathComps[iPos] = NULL;
	
	//Print(" GeneratePath: Path Done\n");
	
	// Cleanup
	iPos2 = iPos = 0;
	while(pathComps[iPos])
	{
		tmpStr = pathComps[iPos];
		// Always Increment iPos
		iPos++;
		// ..
		if(tmpStr[0] == '.' && tmpStr[1] == '.'	&& (tmpStr[2] == '/' || tmpStr[2] == '\0') )
		{
			if(iPos2 != 0)
				iPos2 --;
			continue;
		}
		// .
		if(tmpStr[0] == '.' && (tmpStr[1] == '/' || tmpStr[1] == '\0') )
		{
			continue;
		}
		// Empty
		if(tmpStr[0] == '/' || tmpStr[0] == '\0')
		{
			continue;
		}
		
		// Set New Position
		pathComps[iPos2] = tmpStr;
		iPos2++;
	}
	pathComps[iPos2] = NULL;
	
	//Print(" GeneratePath: Cleaned\n");
	//Print(" GeneratePath: New\n");
	
	// Build New Path
	iPos2 = 1;	iPos = 0;
	tmpPath[0] = '/';
	while(pathComps[iPos])
	{
		tmpStr = pathComps[iPos];
		while(*tmpStr && *tmpStr != '/')
		{
			tmpPath[iPos2++] = *tmpStr;
			tmpStr++;
		}
		tmpPath[iPos2++] = '/';
		iPos++;
	}
	if(iPos2 > 1)
		tmpPath[iPos2-1] = 0;
	else
		tmpPath[iPos2] = 0;
	
	//Print(" GeneratePath: Complete\n");
	
	return iPos2;	//Length
}
