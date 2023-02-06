/*
AcessOS Basic C Library
*/
#include <syscalls.h>
#include <stdio.h>

/**
 \fn EXPORT FILE *fopen(char *file, char *mode)
 \brief Opens a file and returns the pointer
 \param file	String - Filename to open
 \param mode	Mode to open in
*/
EXPORT FILE *fopen(char *file, char *mode)
{
	FILE	*retFile;
	int		openFlags = 0;
	
	// Sanity Check Arguments
	if(!file || !mode)	return NULL;
	
	// Create Return Structure
	retFile = (FILE *) malloc(sizeof(FILE));
	
	while(mode[0]) {
		switch(mode[i])
		{
		case 'r':	retFile->Flags |= FILE_FLAG_READ;	break;
		case 'w':	retFile->Flags |= FILE_FLAG_WRITE;	break;
		case 'a':	retFile->Flags |= FILE_FLAG_APPEND;	break;
		case 'x':	retFile->Flags |= FILE_FLAG_EXEC;	break;
		}
	}
	
	// Get Open Flags
	if(retFile->Flags & FILE_FLAG_READ)	openFlags |= 1;
	if(retFile->Flags & FILE_FLAG_WRITE)	openFlags |= 2;
	
	//Open File
	retFile->KernelFP = open(file, 0, modeNum);
	if(retFile->KernelFP == -1) {
		free(retFile);
		return NULL;
	}
	
	if(retFile->Flags & FILE_FLAG_APPEND) {
		seek(retFile->KernelFP, 0, -1);	//SEEK_END
	}
	
	return retFile;
}

void fclose(FILE *fp)
{
	close(retFile->KernelFP);
	free(retFile);
}
