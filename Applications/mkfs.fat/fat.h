/*
Acess OS
FAT16 Driver Version 1
HEADER
*/

// === On Disk Structures ===
/**
 \struct fat_bootsect_s
 \brief Bootsector format
*/
struct fat_bootsect_s {
	Uint8	jmp[3];	//!< Jump Instruction
	char	oem_name[8];	//!< OEM Name. Typically MSDOS1.1
	Uint16	bps;	//!< Bytes per Sector. Assumed to be 512
	Uint8	spc;	//!< Sectors per Cluster
	Uint16	resvSectCount;	//!< Number of reserved sectors at beginning of volume
	Uint8	fatCount;	//!< Number of copies of the FAT
	Uint16	files_in_root;	//!< Count of files in the root directory
	Uint16	totalSect16;	//!< Total sector count (FAT12/16)
	Uint8	mediaDesc;	//!< Media Desctiptor
	Uint16	fatSz16;	//!< FAT Size (FAT12/16)
	Uint16	spt;	//!< Sectors per track. Ignored (Acess uses LBA)
	Uint16	heads;	//!< Heads. Ignored (Acess uses LBA)
	Uint32	hiddenCount;	//!< ???
	Uint32	totalSect32;	//!< Total sector count (FAT32)
	union {
		struct {
			Uint8	drvNum;	//!< Drive Number. BIOS Drive ID (E.g. 0x80)
			Uint8	resv;	//!< Reserved byte
			Uint8	bootSig;	//!< Boot Signature. ???
			Uint32	volId;	//!< Volume ID
			char	label[11];	//!< Disk Label
			char	fsType[8];	//!< FS Type. ???
		} __attribute__((packed)) fat16;	//!< FAT16 Specific information
		struct {
			Uint32	fatSz32;	//!< 32-Bit FAT Size
			Uint16	extFlags;	//!< Extended flags
			Uint16	fsVer;	//!< Filesystem Version
			Uint32	rootClust;	//!< Root Cluster ID
			Uint16	fsInfo;	//!< FS Info. ???
			Uint16	backupBS;	//!< Backup Bootsector Sector Offset
			char	resv[12];	//!< Reserved Data
			Uint8	drvNum;	//!< Drive Number
			char	resv2;	//!< Reserved Data
			Uint8	bootSig;	//!< Boot Signature. ???
			Uint32	volId;	//!< Volume ID
			char	label[11];	//!< Disk Label
			char	fsType[8];	//!< Filesystem Type. ???
		} __attribute__((packed)) fat32;	//!< FAT32 Specific Information
	}__attribute__((packed)) spec;	//!< Non Shared Data
	char pad[512-90];	//!< Bootsector Data (Code/Boot Signature 0xAA55)
} __attribute__((packed));

/**
 \struct fat_filetable_s
 \brief Format of a 8.3 file entry on disk
*/
struct fat_filetable_s {
	char	name[8];	//!< Name
	char	ext[3];	//!< Extention
	Uint8	attrib;	//!< File Attributes.
	Uint8	ntres;	//!< Reserved for NT - Set to 0
	Uint8	ctimems;	//!< 10ths of a second ranging from 0-199 (2 seconds)
	Uint16	ctime;	//!< Creation Time
	Uint16	cdate;	//!< Creation Date
	Uint16	adate;	//!< Accessed Data. No Time feild though
	Uint16	clusterHi;	//!< High Cluster. 0 for FAT12 and FAT16
	Uint16	mtime;	//!< Last Modified Time
	Uint16	mdate;	//!< Last Modified Date
	Uint16	cluster;	//!< Low Word of First cluster
	Uint32	size;	//!< Size of file
} __attribute__((packed));

/**
 \struct fat_longfilename_s
 \brief Format of a long file name entry on disk
*/
struct fat_longfilename_s {
	Uint8	id;	//!< ID of entry. Bit 6 is set for last entry
	Uint16	name1[5];	//!< 5 characters of name
	Uint8	attrib;	//!< Attributes. Must be ATTR_LFN
	Uint8	type;	//!< Type. ???
	Uint8	checksum;	//!< Checksum
	Uint16	name2[6];	//!< 6 characters of name
	Uint16	firstCluster;	//!< Used for non LFN compatability. Set to 0
	Uint16	name3[2];	//!< Last 2 characters of name
} __attribute__((packed));

#define ATTR_READONLY	0x01
#define ATTR_HIDDEN		0x02
#define ATTR_SYSTEM		0x04
#define ATTR_VOLUMEID	0x08
#define ATTR_DIRECTORY	0x10
#define ATTR_ARCHIVE	0x20
#define	ATTR_LFN		(ATTR_READONLY | ATTR_HIDDEN | ATTR_SYSTEM | ATTR_VOLUMEID)

#define	EOC_FAT12	0x0FFF
#define	EOC_FAT16	0xFFFF
#define	EOC_FAT32	0x0FFFFFF

typedef struct fat_bootsect_s fat_bootsect;
typedef struct fat_filetable_s fat_filetable;
typedef struct fat_longfilename_s fat_longfilename;
