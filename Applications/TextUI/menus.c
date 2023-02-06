/*
AcessOS/AcessBasic
Text Mode Shell
*/
#include "header.h"

typedef struct sMenuEnt
{
	char *name;
	void (*fcn)(int,int);
} tMenuEnt;

typedef struct sMenuDef
{
	char	*name;
	void	(*fcn)(int);
	char	**ents;
	 int	count;
	 int	width;
} tMenuDef;


// Menu Definitions
const char	*cMENU_ACESS[] = {"About", "", "Exit", NULL};
const char	*cMENU_EDIT[] = {"Copy", NULL};
};
const tMenuDef cMENUS[] = {
	{"(&0)", Menu_Acess, cMENU_ACESS, sizeof(cMENU_ACESS)/sizeof(cMENU_ACESS[0]), 6},
	{"&Edit", Menu_Edit, cMENU_EDIT, sizeof(cMENU_EDIT)/sizeof(cMENU_EDIT[0]), 5}
	};
	
// Globals
static int	giMenuCount = sizeof(cMENUS)/sizeof(cMENUS[0]);
static int	giMenuState = 0;	// 0: Base, 1: Alt Held, 2: Select Menu, 3: Browse Menu
static int	giMenuSelected = 0;
static int	giMenuItemSel = -1;


// Code	
void MenusUpdate()
{
	int i, j=0, x=0;
	char	*s;
	char	posTpl[9] = "\x1B[00;00H";
	
	Print("\x1B[H");	//Restore Cursor to Top Left
	Print("\x1B[0;37;41m");	// Set White on Red Attrib
	
	for(i=0;i<giMenuCount;i++)
	{
		s = cMENUS[i].name;
		if(giMenuState >= 2 && giMenuSelected == i)
		{
			x = j;
			Print("\x1B[42m");	// Green BG
		}
		
		while(*s) {
			if(*s == '&')
			{
				s++;
				if(giMenuState > 1) {	// Hotkey is underlined
					Print("\x1B[4m"); Putc(*s); Print("\x1B[0m");
				} else
					Putc(*s);
			}
			else
				Putc(*s);
			s ++;
			j ++;
		}
		Putc(' ');
		j ++;
		
		if(giMenuState >= 2 && giMenuSelected == i)
			Print("\x1B[41m");	// Red BG Again
	}
	
	if(giMenuState == 3)
	{
		Print("\x1B[42m");
		Print("\x1B[30m");
		posTpl[5] = '0' + (x / 10);
		posTpl[6] = '0' + (x % 10);
		for(i=0;i<cMENUS[giMenuSelected].count;i++)
		{
			posTpl[2] = '0' + (i / 10);
			posTpl[3] = '1' + (i % 10);
			Print(posTpl);
			
			// Set New Text Colour
			if(giMenuItemSel == i)	Print("\x1B[37m");
			
			s = cMENUS[giMenuSelected].ents[i];
			if(*s)
			{
				// Print Padded String
				for(j=0;s[j];j++)
					Putc(s[j]);
				for(;j<cMENUS[giMenuSelected].width;j++)
					Putc(' ');
			}
			else
			{
				for(j=0;j<cMENUS[giMenuSelected].width;j++)
					Putc('-');
			}
				
			//Restore Text Colour
			if(giMenuItemSel == i)	Print("\x1B[30m");
		}
	}
}
