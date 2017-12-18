
#ifndef	MNTENT_H
#define	MNTENT_H

#include <stdio.h>

/* File listing canonical interesting mount points. */
#define	MNTTAB				"/etc/mnttab"

/* File listing currently active mount points. */
#define	MOUNTED				"/etc/mount"

/* General filesystem types. */
#define MNTTYPE_IGNORE		"ignore"		/* Ignore this entry. */
#define MNTTYPE_NFS			"nfs"			/* Network file system. */
#define MNTTYPE_SWAP		"swap"			/* Swap device. */

/* Generic mount options. */
#define MNTOPT_DEFAULTS		"defaults"		/* Use all default options. */
#define MNTOPT_RO			"ro"			/* Read only. */
#define MNTOPT_RW			"rw"			/* Read/write. */
#define MNTOPT_SUID			"suid"			/* Set uid allowed. */
#define MNTOPT_NOSUID		"nosuid"		/* No set uid allowed. */
#define MNTOPT_NOAUTO		"noauto"		/* Do not auto mount. */

/* Structure describing a mount table entry. */
struct mntent {
	char *mnt_fsname;	/* Device or server for filesystem. */
	char *mnt_dir;		/* Directory mounted on. */
	char *mnt_type;		/* Type of filesystem: ufs, nfs, etc. */
	char *mnt_opts;		/* Comma-separated options for fs. */
	int mnt_freq;		/* Dump frequency (in days). */
	int mnt_passno;		/* Pass number for `fsck'. */
};

FILE *setmntent(const char *filename, const char *type);
struct mntent *getmntent(FILE *fp);
int endmntent(FILE * filep);

#endif
