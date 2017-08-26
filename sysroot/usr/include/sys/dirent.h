
#ifndef _SYS_DIRENT_H
#define _SYS_DIRENT_H

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


DIR *opendir(const char *);
struct dirent *readdir(DIR *);
void rewinddir(DIR *);
long telldir (DIR *);
void seekdir (DIR *, off_t loc);
int closedir(DIR *);

#endif
