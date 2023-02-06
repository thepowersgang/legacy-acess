/*
 AcessOS Shell Version 2
- Based on IOOS CLI Shell
*/
#include "../syscalls.h"
#include "header.h"

// ==== PROTOTYPES ====
void Parse_Args(char *str, char **dest);
void Command_Colour(int argc, char **argv);
void Command_Clear(int argc, char **argv);
void Command_Ls(int argc, char **argv);
void Command_Cd(int argc, char **argv);
void Command_Cat(int argc, char **argv);

// ==== CONSTANT GLOBALS ====
char	*cCOLOUR_NAMES[8] = {"black", "red", "green", "yellow", "blue", "magenta", "cyan", "white"};
struct	{
	char	*name;
	void	(*fcn)(int argc, char **argv);
}	cBUILTINS[] = {
	{"colour", Command_Colour}, {"clear", Command_Clear}, {"ls", Command_Ls}, {"cd", Command_Cd}, {"cat", Command_Cat}
	};
#define	BUILTIN_COUNT	(sizeof(cBUILTINS)/sizeof(cBUILTINS[0]))

// ==== LOCAL VARIABLES ====
char	gsCommandBuffer[1024];
char	gsCurrentDirectory[1024] = "/";
char	gsTmpBuffer[1024];

// ==== CODE ====
int main(int argc, char *argv[], char *envp[])
{
	char	ch = 0;
	char	*sCommandStr;
	char	*saArgs[32];
	int		length = 0;
	int		pid = -1;
	int		iArgCount = 0;
	t_fstat	stats;
	
	Command_Clear(0, NULL);
	write(_stdout, 36, "AcessOS/AcessBasic Shell Version 2\n");
	write(_stdout, 30, " Based on CLI Shell for IOOS\n");
	write(_stdout,  2, "\n");
	for(;;)
	{
		write(_stdout, strlen(gsCurrentDirectory), gsCurrentDirectory);
		write(_stdout, 3, "$ ");
		
		// Preset Variables
		sCommandStr = gsCommandBuffer;
		length = 0;
		
		// Read In Command Line
		do {
			read(_stdin, 1, &ch);	// Read Character from stdin
			if(ch < 0) {
				yield();
				continue;
			}
			if(ch == '\b') {
				if(length > 0) {
					length--;
					sCommandStr--;
					write(_stdout, 1, &ch);
				}
				continue;
			}
			if(length < 1024)
			{
				write(_stdout, 1, &ch);
				*sCommandStr++ = ch;
				length ++;
			}
		} while(ch != '\n');
		sCommandStr--;
		*sCommandStr = '\0';
		
		// Parse Command Line into arguments
		sCommandStr = gsCommandBuffer;
		Parse_Args(sCommandStr, saArgs);
		
		// Count Arguments
		iArgCount = 0;
		while(saArgs[iArgCount])
			iArgCount++;
		
		// Silently Ignore all empty commands
		if(saArgs[0][0] == '\0')	continue;
		
		// Check Built-In Commands
		//  [HACK] Mem Usage - Use Length in place of `i'
		for(length=0;length<BUILTIN_COUNT;length++)
		{
			if(strcmp(saArgs[0], cBUILTINS[length].name) == 0)
			{
				cBUILTINS[length].fcn(iArgCount, saArgs);
				break;
			}
		}
		if(length == BUILTIN_COUNT)
		{
			GeneratePath(saArgs[0], gsCurrentDirectory, gsTmpBuffer);
			// Use length in place of fp
			length = open(gsTmpBuffer, OPEN_FLAG_READ, 0);
			// Check file existence
			if(length == -1) {
				Print("Unknown Command: `");Print(saArgs[0]);Print("'\n");	// Error Message
				continue;
			}
			// Check if the file is a directory
			stat(length, &stats);
			if(stats.st_mode & S_IFDIR) {
				Print("Unknown Command: `");Print(saArgs[0]);Print("'\n");	// Error Message
				Print(" Attempt to execute a directory.\n");
				continue;
			}
			pid = kexec(gsTmpBuffer, saArgs, NULL);
			if(pid <= 0) {
				Print("Unknown Command: `");Print(saArgs[0]);Print("'\n");	// Error Message
			}
			else
				waitpid(pid, K_WAITPID_DIE);
		}
	}
}

void Parse_Args(char *str, char **dest)
{
	int i = 0;
	for(;;)
	{
		dest[i] = str;
		i++;
		while(*str && *str != ' ')
		{
			if(*str++ == '"')
			{
				while(*str && *str != '"')	str++;
			}
		}
		if(*str == '\0')
			break;
		*str = '\0';
		str++;
	}
	dest[i] = NULL;
}

void Command_Colour(int argc, char **argv)
{
	int fg, bg;
	char	clrStr[6] = "\x1B[37m";
	
	// Verify Arg Count
	if(argc < 2)
	{
		Print("Please specify a colour\n");
		goto usage;
	}
	
	for(fg=0;fg<8;fg++)
		if(strcmp(cCOLOUR_NAMES[fg], argv[1]) == 0)
			break;

	// Foreground a valid colour
	if(fg == 8)
	{
		Print("Unknown Colour '");Print(argv[1]);Print("'\n");
		goto usage;
	}
	// Set Foreground
	clrStr[3] = '0' + fg;
	write(_stdout, 6, clrStr);
	
	// Need to Set Background?
	if(argc > 2)
	{
		for(bg=0;bg<8;bg++)
			if(strcmp(cCOLOUR_NAMES[bg], argv[2]) == 0)
				break;
	
		// Valid colour
		if(bg == 8)
		{
			Print("Unknown Colour '");Print(argv[2]);Print("'\n");
			goto usage;
		}
	
		clrStr[2] = '4';
		clrStr[3] = '0' + bg;
		write(_stdout, 6, clrStr);
	}
	// Return
	return;

	// Function Usage (Requested via a Goto (I know it's ugly))
usage:
	Print("Valid Colours are ");
	for(fg=0;fg<8;fg++)
	{
		Print(cCOLOUR_NAMES[fg]);
		write(_stdout, 3, ", ");
	}
	write(_stdout, 4, "\b\b\n");
	return;
}

void Command_Clear(int argc, char **argv)
{
	write(_stdout, 4, "\x1B[2J");	//Clear Screen
}

void Command_Ls(int argc, char **argv)
{
	int 	dp, fp, dirLen;
	char	modeStr[11] = "RWXrwxRWX ";
	char	tmpPath[1024];
	char	fileName[256];
	t_fstat	stats;
	
	// Generate Directory Path
	if(argc > 1)
		dirLen = GeneratePath(argv[1], gsCurrentDirectory, tmpPath);
	else
	{
		strcpy(tmpPath, gsCurrentDirectory);
	}
	dirLen = strlen(tmpPath);
	
	// Open Directory
	dp = open(tmpPath, OPEN_FLAG_READ|OPEN_FLAG_EXEC, 0);
	// Check if file opened
	if(dp == -1)
	{
		write(_stdout, 27, "Unable to open directory `");
		write(_stdout, strlen(tmpPath)+1, tmpPath);
		write(_stdout, 25, "', File cannot be found\n");
		return;
	}
	// Get File Stats
	if( stat(dp, &stats) == -1 )
	{
		close(dp);
		write(_stdout, 34, "stat Failed, Bad File Descriptor\n");
		return;
	}
	// Check if it's a directory
	if(!(stats.st_mode & S_IFDIR))
	{
		close(dp);
		write(_stdout, 27, "Unable to open directory `");
		write(_stdout, strlen(tmpPath)+1, tmpPath);
		write(_stdout, 20, "', Not a directory\n");
		return;
	}
	
	// Append Shash for file paths
	if(tmpPath[dirLen-1] != '/')
	{
		tmpPath[dirLen++] = '/';
		tmpPath[dirLen] = '\0';
	}
	// Read Directory Content
	while( (fp = readdir(dp, fileName)) )
	{
		if(fp < 0)
		{
			if(fp == -3)
				write(_stdout, 42, "Invalid Permissions to traverse directory\n");
			break;
		}
		// Create File Path
		strcpy((char*)(tmpPath+dirLen), fileName);
		// Open File
		fp = open(tmpPath, 0, 0);
		if(fp == -1)
			continue;
		// Get File Stats
		stat(fp, &stats);
		close(fp);
		
		//Colour Code
		write(_stdout, 6, "\x1B[37m");	//White
		if(stats.st_mode & 0111)	//Executable Yellow
			write(_stdout, 6, "\x1B[33m");
		if(stats.st_mode & S_IFDIR)	//Directory Green
			write(_stdout, 6, "\x1B[32m");
		
		//Print Mode
		if(stats.st_mode & 0400)	modeStr[0] = 'R';	else	modeStr[0] = '-';
		if(stats.st_mode & 0200)	modeStr[1] = 'W';	else	modeStr[1] = '-';
		if(stats.st_mode & 0100)	modeStr[2] = 'X';	else	modeStr[2] = '-';
		if(stats.st_mode & 0040)	modeStr[3] = 'R';	else	modeStr[3] = '-';
		if(stats.st_mode & 0020)	modeStr[4] = 'W';	else	modeStr[4] = '-';
		if(stats.st_mode & 0010)	modeStr[5] = 'X';	else	modeStr[5] = '-';
		if(stats.st_mode & 0004)	modeStr[6] = 'R';	else	modeStr[6] = '-';
		if(stats.st_mode & 0002)	modeStr[7] = 'W';	else	modeStr[7] = '-';
		if(stats.st_mode & 0001)	modeStr[8] = 'X';	else	modeStr[8] = '-';
		write(_stdout, 10, modeStr);
		
		//Print Name
		write(_stdout, strlen(fileName), fileName);
		//Print slash if applicable
		if(stats.st_mode & S_IFDIR)
			write(_stdout, 2, "/");
		
		// Revert Colout and end line
		write(_stdout, 6, "\x1B[37m");
		write(_stdout, 2, "\n");
	}
	// Close Directory
	close(dp);
}

void Command_Cd(int argc, char **argv)
{
	char	tmpPath[1024];
	int		fp;
	t_fstat	stats;
	
	if(argc < 2)
	{
		Print(gsCurrentDirectory);Print("\n");
		return;
	}
	
	GeneratePath(argv[1], gsCurrentDirectory, tmpPath);
	
	fp = open(tmpPath, 0, 0);
	if(fp == -1) {
		write(_stdout, 26, "Directory does not exist\n");
		return;
	}
	stat(fp, &stats);
	close(fp);
	if(!(stats.st_mode & S_IFDIR)) {
		write(_stdout, 17, "Not a Directory\n");
		return;
	}
	
	strcpy(gsCurrentDirectory, tmpPath);
}


void Command_Cat(int argc, char **argv)
{
	char	tmpPath[1024];
	int		fp, len;
	char	ch[33] = {0};
	
	if(argc < 2) {
		Print("Usage: cat <filename>\n");
		return;
	}
	
	GeneratePath(argv[1], gsCurrentDirectory, tmpPath);
	
	fp = open(tmpPath, OPEN_FLAG_READ, 0);
	if(fp == -1) {
		write(_stdout, 26, "File does not exist\n");
		return;
	}
	
	seek(fp, 0, -1);
	len = tell(fp);
	seek(fp, 0, 1);
	
	for(;len>32;len-=32) {
		read(fp, 32, &ch[0]);
		Print(ch);
	}
	ch[1] = '\0';
	while(len--) {
		read(fp, 1, &ch[0]);
		Print(ch);
	}
	
	close(fp);
	Print("\n\n");
}
