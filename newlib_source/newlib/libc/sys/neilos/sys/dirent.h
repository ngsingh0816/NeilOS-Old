
#ifndef _SYS_DIRENT_H
#define _SYS_DIRENT_H

#ifdef __cplusplus
extern "C" {
#endif

#include <sys/types.h>
#include <bits/dirent.h>
#define _LIBC 1
#define  NOT_IN_libc 1
#undef _LIBC

#define HAVE_NO_D_NAMLEN	/* no struct dirent->d_namlen */

#define MAXNAMLEN 255		/* sizeof(struct dirent.d_name)-1 */

typedef struct {
    int dd_fd;		/* directory file */
    int dd_loc;		/* position in buffer */
    int dd_seek;
    char *dd_buf;	/* buffer */
    int dd_len;		/* buffer length */
    int dd_size;	/* amount of data in buffer */
} DIR;

# define __dirfd(dp)	((dp)->dd_fd)

DIR *opendir(const char *);
struct dirent *readdir(DIR *);
void rewinddir(DIR *);
long telldir (DIR *);
void seekdir (DIR *, off_t loc);
int closedir(DIR *);
	
#ifdef __cplusplus
}
#endif

#endif
