
#ifndef	_SYS_MMAN_H_
#define _SYS_MMAN_H_

#ifdef __cplusplus
extern "C" {
#endif

#define	PROT_NONE	0x00
#define	PROT_READ	0x01
#define	PROT_WRITE	0x02
#define	PROT_EXEC	0x04

#define	MAP_SHARED	0x0001
#define	MAP_PRIVATE	0x0002
#define	MAP_COPY	MAP_PRIVATE

#define	MAP_FIXED	 0x0010
#define	MAP_RENAME	 0x0020
#define	MAP_NORESERVE	 0x0040
#define	MAP_INHERIT	 0x0080
#define	MAP_NOEXTEND	 0x0100
#define	MAP_HASSEMAPHORE 0x0200

#ifdef _P1003_1B_VISIBLE
#define MCL_CURRENT	0x0001
#define MCL_FUTURE	0x0002

#endif /* _P1003_1B_VISIBLE */

#define MAP_FAILED	((void *)-1)

#define	MS_SYNC		0x0000
#define MS_ASYNC	0x0001
#define MS_INVALIDATE	0x0002

#ifndef _POSIX_SOURCE
#define MS_KILLPAGES    0x0004
#define MS_DEACTIVATE   0x0008
#endif

#define	MAP_FILE		0x0000
#define	MAP_ANON		0x1000
#define	MAP_ANONYMOUS	0x1000

#define	MADV_NORMAL	0
#define	MADV_RANDOM	1
#define	MADV_SEQUENTIAL	2
#define	MADV_WILLNEED	3
#define	MADV_DONTNEED	4
#define	MADV_FREE		5
#define	POSIX_MADV_NORMAL	MADV_NORMAL
#define	POSIX_MADV_RANDOM	MADV_RANDOM
#define	POSIX_MADV_SEQUENTIAL	MADV_SEQUENTIAL
#define	POSIX_MADV_WILLNEED	MADV_WILLNEED
#define	POSIX_MADV_DONTNEED	MADV_DONTNEED

#define	MINCORE_INCORE	 	 0x
#define	MINCORE_REFERENCED	 0x2
#define	MINCORE_MODIFIED	 0x4
#define	MINCORE_REFERENCED_OTHER 0x8
#define	MINCORE_MODIFIED_OTHER	0x10

#include <sys/cdefs.h>

__BEGIN_DECLS
#ifdef _P1003_1B_VISIBLE
int	mlockall __P((int));
int	munlockall __P((void));
#endif /* _P1003_1B_VISIBLE */
int	mlock __P((const void *, size_t));
void *	mmap __P((void *, size_t, int, int, int, off_t));
int	mprotect __P((const void *, size_t, int));
int	msync __P((void *, size_t, int));
int	munlock __P((const void *, size_t));
int	munmap __P((void *, size_t));
int	shm_open __P((const char *, int, ...));
int	shm_unlink __P((const char *));
int	posix_madvise __P((void *, size_t, int));
#ifndef _POSIX_SOURCE
int	madvise __P((void *, size_t, int));
int	mincore __P((const void *, size_t, char *));
int	minherit __P((void *, size_t, int));
#endif
__END_DECLS
	
#ifdef __cplusplus
}
#endif

#endif /* !_SYS_MMAN_H_ */
