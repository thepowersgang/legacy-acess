/*
AcessOS
XML Application Parser

header.h

(c) John Hodge (thePowersGang) 2008
*/

typedef struct {
	char	*id;
	
	int		functionCount;
	char	**functonNames;
	int		*functionOffsets;
	
	int		variableCount;
	char	**variableNames;
	int		*variableOffsets;
	
	char	*code;
}	xmla_code;

typedef struct s_xmla_element {
	int		type;
	int		x,y, w,h;
	int		flags;
	char	*id;
	char	*value;
	
	//int		childCount;
	//struct s_xmla_element	*children;
	struct s_xmla_element	*parent;
	xmla_code	*code;
} xmla_element;

typedef struct {
	char	*id;
	char	*title;
	int		x,y,w,h;
	int		flags;
	
	int		eleCount;
	xmla_element	*elements;
	xmla_code	*code;
} xmla_window;

typedef struct {
	char	*name;
	char	*versionString;
	char	*author;
	char	*category;
	int		versionMajor, versionMinor;
	xmla_code	*code;
	
	int	windowCount;
	xmla_window	*windows;
} xmla_application;


extern xmla_application	gApplication;
