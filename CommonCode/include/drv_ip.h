/**
 AcessOS Version 1
 \file drv_ip.h
 \brief Internet Protocol Driver
*/
#ifndef _DRV_IP_H
#define _DRV_IP_H

// === STRUCTURES and TYPES ===
/**
 \union uIPv4
 \brief IP Version 4 Address
*/
union uIPv4 {
	Uint32	v;	//!< 32-Bit Address component
	struct {
		Uint8	a,b,c,d;	//!< Individual Bytes
	};
};
/**
 \union uIPv6
 \brief IP Version 6 Address
*/
union uIPv6 {
	Uint32	v[4];	//!< 32-Bit Components
	Uint8	b[16];	//!< Individual Bytes
};

/**
 \struct sInterface
 \brief Interface Structure used by drv_ip
*/
struct sInterface {
	/// \name Device Settings
	/// \{
	char	*Device;	//!< Device Path
	 int	DeviceFP;	//!< Device Handle
	char	DeviceMAC[6];	//!< Physical Address
	/// \}
	/// \name IPv6 Settings
	/// \{
	tIPv6	IP;	//!< IPv6 Address
	tIPv6	Gateway;	//!< IPv6 Gateway (Is this needed?)
	 int	Subnet;	//!< Subnet bits for IPv6
	/// \}
	/// \name IPv4 Settings
	/// \{
	tIPv4	IP4;	//!< IPv4 Address
	tIPv4	Gateway4;	//!< IPv4 Gateway
	 int	Subnet4;	//!< IPv4 Subnet Bits
	/// \}
	/// \name Filesystem
	/// \{
	vfs_node	Node;	//!< /ipX Node
	vfs_node	NodeTcpc;	//!< TCP Client Node
	vfs_node	NodeTcps;	//!< TCP Server Node
	vfs_node	NodeUdpc;	//!< UDP Client Node
	vfs_node	NodeUdps;	//!< UDP Server Node
	 int	ReferenceCount;	//!< Number of open connectons
	/// \}
	/// \name Link
	struct sInterface	*Next;	//!< Next in list
};
typedef union uIPv4	tIPv4;	//!< IPv4 Data Type
typedef union uIPv6	tIPv6;	//!< IPv6 Data Type
typedef struct sInterface	tInterface;	//!< Interface Data Type

#endif
