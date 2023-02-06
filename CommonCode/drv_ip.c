/*
AcessOS Version 1
- Internet Protocol Driver
*/
#include <kmod.h>
#include <drv_ip.h>

// === CONSTANTS ===
#define FILES_PER_INTERFACE	5

// === PROTOTYPES ===
 int	IP_Initialise();
vfs_node	*IP_Root_ReadDir(vfs_node *node, int pos);
vfs_node	*IP_Root_FindDir(vfs_node *node, char *name);
 int	IP_IOCtl(vfs_node *node, int id, void *data);

// === GLOBALS ===
 int	giIP_DevfsId = 0;
vfs_node	gIP_RootNode = {0};
devfs_driver	gIP_DriverInfo = {"ip", 2, &gIP_RootNode, IP_IOCtl};
tInterface	gpIP_Loopback = {0};
tInterface	*gpIP_Interfaces = NULL;
tInterface	*gaIP_InterfaceIndex = NULL;
 int	giIP_InterfaceCount = 0;

// === CODE ===
int IP_Initialise()
{
	// Initialise VFS Root
	gIP_RootNode.name = "ip";
	gIP_RootNode.nameLength = 2;
	gIP_RootNode.mode = 0111;	// Directory (--X--X--X)
	gIP_RootNode.flags = VFS_FFLAG_DIRECTORY;	// Directory
	gIP_RootNode.readdir = IP_Root_ReadDir;
	gIP_RootNode.finddir = IP_Root_FindDir;
	// Initialise Loopback Adapter
	gpIP_Loopback.Node.name = "ipL";	gpIP_Loopback.Node.nameLength = 3;
	gpIP_Loopback.Node.mode = 0111;	// Directory (--X--X--X)
	gpIP_Loopback.Node.flags = VFS_FFLAG_DIRECTORY;	// Directory
	gpIP_Loopback.Node.readdir = IP_Sub_ReadDir;
	gpIP_Loopback.Node.finddir = IP_Sub_FindDir;
	
	// Register Driver
	giIP_DevfsId = DevFS_AddDevice( &gIP_DriverInfo );
	if(giIP_DevfsId == -1)	return 0;
	
	return 1;
}

vfs_node *IP_Root_ReadDir(vfs_node *node, int pos)
{
	tInterface	*interface;
	// Check for Root Node
	if(node != &gIP_RootNode)	return NULL;
	// Sanity Check `pos`
	if(pos < 0 || pos >= giIP_InterfaceCount+1)	return NULL;
	
	// Get Interface
	if(pos == 0)
		interface = &gpIP_Loopback;	// Loopback is first
	else
		interface = gaIP_InterfaceIndex[ pos-1 ];
	
	// Increase Reference Count
	interface->ReferenceCount ++;
	// Get File
	return &interface->Node;
}

/**
 
*/
vfs_node *IP_Root_FindDir(vfs_node *node, char *name)
{
	int id=0;
	if(name == NULL)	return NULL;
	if(name[0] != 'i' && name[1] != 'p')	return NULL;
	
	if(name[2] == 'L' && name[3] == '\0')	return &gpIP_Loopback.Node;
	
	name += 2;
	while(*name)
	{
		if(*name < '0' || *name > '9')	return NULL;
		id *= 10;
		id += *name-'0';
		name++;
	}
	return &gaIP_InterfaceIndex[ id ].Node;
}

/**
 \fn vfs_node *IP_Sub_ReadDir(vfs_node *node, int pos)
 \brief Fetch the `pos`th item from the directory
 \param node	VFS Node - Parent Node
 \param pos	Integer - Position of required item
 \return Item Node or NULL
 
 This function fetches the nth sub-item from a
 `/Devices/ip/ipX/` directory.
*/
vfs_node *IP_Sub_ReadDir(vfs_node *node, int pos)
{
	tInterface *interface = (void *)node->inode;
	
	switch( pos )
	{
	case 0:	return &interface->NodeTcpc;	// TCP Client (tcpc)
	case 1:	return &interface->NodeTcps;	// TCP Sever (tcps)
	case 2:	return &interface->NodeUdpc;	// UDP Client (udpc)
	case 3:	return &interface->NodeUdps;	// UDP Server (udps)
	return NULL;
	}
}

/**
 \fn vfs_node *IP_Sub_FindDir(vfs_node *node, char *name)
 \brief Finds the named item in an ip directory
 \param node	VFS Node - Parent Node
 \param name	String - Item to locate
 \return Node of required item OR NULL on error
 
 This function validates and parses the passed string
 and returns the required VFS node based on that.
 Used for the `/Devices/ip/ipX/` directories.
*/
vfs_node *IP_Sub_FindDir(vfs_node *node, char *name)
{
	tInterface *interface = (void *)node->inode;
	
	if(name == NULL)	return NULL;	// NULL Check
	// Match ??p?
	if(name[2] != 'p' && name[4] != '\0')	return NULL;
	
	// TCP
	if(name[0] == 't')
	{
		if(name[1] != 'c')	return NULL;
		if(name[3] == 'c')	return &interface->NodeTcpc;	// TCP Client
		if(name[3] == 's')	return &interface->NodeTcps;	// TCP Server
		return NULL;
	}
	// UDP
	if(name[0] == 'u')
	{
		if(name[1] != 'd')	return NULL;
		if(name[3] == 'c')	return &interface->NodeUdpc;	// UDP Client
		if(name[3] == 's')	return &interface->NodeUdps;	// UDP Server
		return NULL;
	}
	return NULL;
}
