/*
AcessOS 0.1
General Disk Controller
HEADER
*/

//STRUCTURES
/**
 \struct sPartitionInfo
 \brief Partion Information Structure
*/
struct sPartitionInfo {
	Uint8	fs;		//!< Filesystem ID
	Uint	lba_offset;	//!< LBA Offset
	Uint	lba_length;	//!< LBA Length
};
typedef struct sPartitionInfo	t_part_info;	//!< Partition Type

/**
 \struct sDiskInfo
 \brief Disk Information Structure
*/
struct sDiskInfo {
	Uint	part_count;
	t_part_info	parts[4];
};

typedef struct sDiskInfo	t_disk_info;	//!< Disk Information
