/*
Acess OS
Init ROM Driver
HEADER
*/


//On Disk Structures
struct rom_header_s {
	char	id[4];	//'I','R','F',0
	Uint32	rootOff;
	Uint32	count;
} __attribute__((packed));

struct rom_file_entry_s {
	char	namelen;
	char	name[32];
	Uint32	offset;
	Uint32	size;
} __attribute__((packed));

typedef struct rom_header_s rom_header;
typedef struct rom_file_entry_s rom_dirent;

//Memory Structures
struct drv_rom_volinfo_s {
	Uint32	start;
	Uint32	*data;
	rom_dirent	*dir;
	rom_header	header;
	vfs_node	*node;
	vfs_node	*dirNodes;
};

typedef struct drv_rom_volinfo_s drv_rom_volinfo;
