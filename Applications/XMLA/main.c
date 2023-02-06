/*
AcessOS
XML Application Parser

(c) John Hodge (thePowersGang) 2008
*/
#include <stdio.h>
#include "header.h"
#include "/projects/Libraries/xmlparse/xmlparse.h"

//======
// GLOBALS
//======
char	*gsArgv0 = NULL;
char	*gsFilename = NULL;
xmla_application	gApplication;
XMLElement		*gRootElement;

void ParseCmd(int argc, char *argv[]);
void print_help();
void LoadXML();

//======
// CODE
//======
/* int main(int argc, char *argv[])
 Entrypoint & Core of application
 */
int main(int argc, char *argv[])
{
	
	ParseCmd(argc, argv);
	
	if(gsFilename == NULL) {
		print_help();
	}
	
	gRootElement = XML_ParseFile(gsFilename);
	printf("Parsing XML...\n");
	LoadXML();
	printf("Done.\n");
	XML_FreeElement(gRootElement);
}

/* void ParseCmd(int argc, char *argv[])
 Parses command line arguments into global
 variables
 */
void ParseCmd(int argc, char *argv[])
{
	int i;
	
	gsArgv0 = argv[0];
	for(i=1;i<argc;i++)
	{
		if(argv[i][0] == '-') {
			switch(argv[i][1]) {
			case 'h':
			case '?':
				print_help();
				exit(0);
				break;
			}
		} else {
			gsFilename = argv[i];
		}
	}
}

/* void print_help()
*/
void print_help()
{
	printf("Usage: %s <filename>\n", gsArgv0);
	exit(0);
}

/* void LoadXML()
 Loads the application from the XML file
*/
void LoadXML()
{
	int	i = 0;
	
	//Check Root Tag
	if( strcmp(gRootElement->tag, "Application") != 0 )
	{
		printf("Expected 'Application' Tag, found '%s'\n", gRootElement->tag);
		exit(1);
	}
	
	gApplication.name = XML_GetAttribute(gRootElement, "Name");
	gApplication.versionString = XML_GetAttribute(gRootElement, "Version");
	gApplication.category = XML_GetAttribute(gRootElement, "Category");
	gApplication.author = XML_GetAttribute(gRootElement, "Author");
	
	printf("Application Stats:\n");
	printf(" Name: '%s'\n", gApplication.name);
	printf(" Version: '%s'\n", gApplication.versionString);
	printf(" Category: '%s'\n", gApplication.category);
	printf(" Author: '%s'\n", gApplication.author);
	
	//---------------------
	//Parse Root Attributes
	//---------------------
	/*for( i = 0; i < gRootElement->attribCount; i++ )
	{
		XMLAttrib	*tmpA;
		tmpA = &gRootElement->attribs[i];
		//Application Name
		if( strcmp(tmpA->name, "Name") == 0 ) {
			//Allocate and Copy
			gApplication.name = malloc(tmpA->valueLen + 1);
			strcpy(gApplication.name, tmpA->value);
			continue;
		}
		//Version
		if( strcmp(tmpA->name, "Version") == 0 ) {
			//Allocate and Copy
			gApplication.versionString = malloc(tmpA->valueLen + 1);
			strcpy(gApplication.versionString, tmpA->value);
			continue;
		}
		//Category
		if( strcmp(tmpA->name, "Category") == 0 ) {
			//Allocate and Copy
			gApplication.category = malloc(tmpA->valueLen + 1);
			strcpy(gApplication.category, tmpA->value);
			continue;
		}
		//Author Name
		if( strcmp(tmpA->name, "Author") == 0 ) {
			//Allocate and Copy
			gApplication.author = malloc(tmpA->valueLen + 1);
			strcpy(gApplication.author, tmpA->value);
			continue;
		}
		
		printf("WARNING: Unknown Attribute name '%s'\n", tmpA->name);
	}*/
	
	//--------------------------
	//Parse Application Children
	//--------------------------
	for( i = 0; i < gRootElement->childCount; i++ )
	{
		XMLElement	*tmpE;
		tmpE = gRootElement->children[i];
		
		//Code
		if( strcmp(tmpE->tag, "Code") == 0 ) {
			if( tmpE->childCount == 0 )
				continue;
			if( tmpE->childCount > 1 ) {
				continue;
			}
			if( strcmp(tmpE->children[0]->tag, ".TEXT") != 0 ) {
				continue;
			}
			
			//ParseCode2App(tmpE->children[0]->innerText);
			continue;
		}
		
		//Window
		if( strcmp(tmpE->tag, "Window") == 0 ) {
			gApplication.windowCount ++;
			gApplication.windows = (xmla_window	*) realloc(gApplication.windows, sizeof(xmla_window)*gApplication.windowCount);
			
		}
	}
	
}
