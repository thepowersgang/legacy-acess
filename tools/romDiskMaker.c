/*
Acess OS
Init ROM Driver
Creater
*/
#include <stdio.h>

typedef unsigned int	Uint32;

typedef struct {
	char	id[4];	//'I','R','F','0'
	Uint32	rootOff;
	Uint32	count;
} __attribute__((packed)) header;

typedef struct {
	char	namelen;
	char	name[32];
	Uint32	offset;
	Uint32	size;
} __attribute__((packed)) dirent;

int main(int argc, char *argv[])
{
	int		count = argc - 2;
	int		i=0, j=0;
	dirent	*dirDat;
	header	hdr;
	FILE	*fpOut, *fpIn;
	int		curOffset = 0;
	int		tmpLen = 0;
	int		slashPos = 0;
	Uint32	*tmpData;
	
	if(argc < 3) {
		printf("Usage %s <romdisk> <file1> <file2> ...\n", argv[0]);
		return 1;
	}
	
	fpOut = fopen(argv[1], "wb");
	
	//Header
	hdr.id[0] = 'I';	hdr.id[1] = 'R';
	hdr.id[2] = 'F';	hdr.id[3] = 0;
	hdr.rootOff = sizeof(header);
	hdr.count = count;
	fwrite(&hdr, sizeof(header), 1, fpOut);
	
	printf("%i files\n", count);
	
	//File Table
	dirDat = (dirent*) calloc(count, sizeof(dirent));	
	curOffset = sizeof(header)+count*sizeof(dirent);
	for(i=0;i<count;i++) {
		//Get Name Length
		tmpLen = strlen(argv[i+2]);
		//Swap out slashes
		for(slashPos=0;slashPos<tmpLen;slashPos++) {
			if(argv[i+2][slashPos] == '/' ||
				argv[i+2][slashPos] == '\\')
				break;
		}
		slashPos++;
		if(slashPos >= tmpLen) {
			slashPos = 0;
		}
		//Check name length
		dirDat[i].namelen = tmpLen - slashPos;
		//printf("Name Length - %i\n", dirDat[i].namelen);
		if(dirDat[i].namelen > 32) {
			printf("Error: Filenames below 32 characters only.\n");
			return 1;
		}
		
		//Copy name
		//printf("Copy Name\n");
		memcpy(dirDat[i].name, (char*)((argv[i+2])+slashPos), dirDat[i].namelen);
		
		dirDat[i].offset = curOffset;
		
		//Open File
		//printf("Getting file size\n");
		fpIn = fopen(argv[i+2], "rb");
		if(fpIn == NULL) {
			printf("Unable to open %s\n", argv[i+2]);
			return 1;
		}
		fseek(fpIn, 0, SEEK_END);
		dirDat[i].size = ftell(fpIn);
		//printf("File Size = %i\n", dirDat[i].size);
		fseek(fpIn, 0, SEEK_SET);
		fclose(fpIn);
		
		//Read Size
		printf("'%s': Size: %i, Offset: %i\n", dirDat[i].name, dirDat[i].size, dirDat[i].offset);
		
		curOffset += dirDat[i].size;
	}
	
	fwrite(dirDat, sizeof(dirent), count, fpOut);
	
	for(i=0;i<count;i++) {
		fpIn = fopen(argv[i+2], "rb");
		for(j=0;j<dirDat[i].size;j++)
			fputc( fgetc(fpIn), fpOut);
		fclose(fpIn);
		free(tmpData);
	}
	fclose(fpOut);
	free(dirDat);
	printf("Done.\n");
	return 0;
}
