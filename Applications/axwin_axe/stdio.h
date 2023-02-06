/*
Acess OS Command Prompt
*/

//fstat structure
typedef struct {
	int		st_dev;	//dev_t
	int		st_ino;	//ino_t
	int		st_mode;	//mode_t
	unsigned int	st_nlink;
	unsigned int	st_uid;
	unsigned int	st_gid;
	int		st_rdev;	//dev_t
	unsigned int	st_size;
	long	st_atime;	//time_t
	long	st_mtime;
	long	st_ctime;
} t_fstat;

extern void k_puts(char *string);
extern void k_printf(char *format, ...);

extern int k_mount(char *dev, char *mount, char fs);
extern int k_fopen(char *path, int flags);
extern int k_opendir(char *path);
extern int k_readdir(int dp, char *file);
extern int k_fread(int handle, int len, void *buf);
extern int k_fwrite(int handle, int len, void *buf);
extern void	k_fseek(int handle, int dist, int flag);
extern int	k_ftell(int handle);
extern int	k_fstat(int handle, t_fstat *stat);
extern void	k_fclose(int handle);
extern void	k_exec(char *cmd);
extern int	k_ioctl(int handle, int id, void *data);

extern int strcmp(char *s1, char *s2);
extern void strcpy(char *dst, char *src);

extern int stdout;
extern int stdin;

#define	S_IFMT		0170000	/* type of file */
#define		S_IFDIR	0040000	/* directory */
#define		S_IFCHR	0020000	/* character special */
#define		S_IFBLK	0060000	/* block special */
#define		S_IFREG	0100000	/* regular */
#define		S_IFLNK	0120000	/* symbolic link */
#define		S_IFSOCK	0140000	/* socket */
#define		S_IFIFO	0010000	/* fifo */

