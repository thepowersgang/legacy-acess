/*
AcessOS/AcessBasic
Text Mode Shell
*/
#include "header.h"
#include "../syscalls.h"

// === GLOBALS ===
int giState = 1;

// === CODE ===
int main(int argc, char *argv[])
{
	RenderUI();
	while(giState)
	{
		GetInput();
		RenderUI();
	}
}
void RenderUI()
{
	MenusUpdate();
}

