/*
XML Parser
HEADER
*/

#ifdef BUILD_XMLPARSE
# ifdef __WIN32__
#  define IMPORT __declspec(dllimport)
# else
#  define IMPORT extern
# endif
#elif defined(__WIN32__)
# define IMPORT __declspec(dllexport)
#endif

typedef struct {
	char	*name;
	char	*value;
	short	nameLen;
	short	valueLen;
} XMLAttrib;

typedef struct _sXMLElement {
	char	*tag;
	char	*innerText;
	int		innerTextLen;
	
	int		attribCount;
	int		childCount;
	XMLAttrib	*attribs;
	struct _sXMLElement	*parent;
	struct _sXMLElement	**children;
} XMLElement;


IMPORT XMLElement *XML_ParseFile(char *filename);
IMPORT XMLElement *XML_ParseString(char *string);
IMPORT void XML_FreeElement(XMLElement *element);
IMPORT void XML_DumpElement(XMLElement *element);
