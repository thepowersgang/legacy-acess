/*
 Syscall Definitions
*/

typedef unsigned char	Uint8;
typedef unsigned short	Uint16;
typedef unsigned long	Uint32;
typedef unsigned long long	Uint64;
typedef unsigned int	Uint;

//syscall stat structure
typedef struct {
	int		st_dev;		//dev_t
	int		st_ino;		//ino_t
	int		st_mode;	//mode_t
	Uint	st_nlink;
	Uint	st_uid;
	Uint	st_gid;
	int		st_rdev;	//dev_t
	Uint	st_size;
	long	st_atime;	//time_t
	long	st_mtime;
	long	st_ctime;
} t_fstat;
#define	S_IFMT		0170000	/* type of file */
#define		S_IFDIR	0040000	/* directory */
#define		S_IFCHR	0020000	/* character special */
#define		S_IFBLK	0060000	/* block special */
#define		S_IFREG	0100000	/* regular */
#define		S_IFLNK	0120000	/* symbolic link */
#define		S_IFSOCK	0140000	/* socket */
#define		S_IFIFO	0010000	/* fifo */

#define	OPEN_FLAG_READ	1
#define	OPEN_FLAG_WRITE	2
#define	OPEN_FLAG_EXEC	4

enum {
	K_WAITPID_DIE = 0
};

extern int	open(char *file, int flags, int mode);
extern int	close(int fp);
extern int	read(int fp, int len, void *buf);
extern int	write(int fp, int len, void *buf);
extern int	tell(int fp);
extern void seek(int fp, int dist, int flag);
extern int	stat(int fp, t_fstat *st);
extern int	ioctl(int fp, int call, void *arg);
extern int	readdir(int fp, char *file);
extern int	brk(int bssend);
extern int	kexec(char *file, char *args[], char *envp[]);
extern int	fork();
extern int	yield();
extern int	kdebug(char *fmt, ...);
extern int	waitpid(int pid, int action);
