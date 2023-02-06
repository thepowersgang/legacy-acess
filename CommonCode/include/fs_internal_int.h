/*
Acess OS
Internal Filesystems Version 1
Includes RootFS and DevFS
HEADER
*/

//DEFINES
#define MAX_ROOT	32
#define	CHILD_ARR_STEP	8

//STRUCTURES
typedef struct {
	char	name[32];
	int		nameLen;
} virtualFs_file;

typedef struct {
	 int	alloc;	//Boolean - Allocated?
	vfs_node	node;
	 int	childCount;
	 int	childMax;
	 int	*children;
} ramfs_file;

typedef struct sRamFS_File {
	vfs_node	Node;
	union {
		struct sRamFS_File	*FirstChild;
		char	*FileData;
	};
	struct sRamFS_File	*NextSibling;
	struct sRamFS_File	*Parent;
} tRamFS_File;
