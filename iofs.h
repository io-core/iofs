/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2020 Charles Perkins
 *
 * Portions derived from work (c) 1999 Al Smith
 * Portions derived from work (c) 1995,1996 Christian Vogelgsang
 * Portions derived from IRIX header files (c) 1988 Silicon Graphics
 */
#ifndef _IOFS_IOFS_H_
#define _IOFS_IOFS_H_

#ifdef pr_fmt
#undef pr_fmt
#endif

#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt

#include <linux/fs.h>
#include <linux/uaccess.h>

#define IOFS_VERSION "2013a"

static const char cprt[] = "IOFS: "IOFS_VERSION"  ";


/* 1 block is 512 bytes */
#define	IOFS_BLOCKSIZE_BITS	9
#define	IOFS_BLOCKSIZE		(1 << IOFS_BLOCKSIZE_BITS)

typedef	int32_t		efs_block_t;
typedef uint32_t	efs_ino_t;

#define	IOFS_DIRECTEXTENTS	12

/*
 * layout of an extent, in memory and on disk. 8 bytes exactly.
 */
typedef union extent_u {
	unsigned char raw[8];
	struct extent_s {
		unsigned int	ex_magic:8;	/* magic # (zero) */
		unsigned int	ex_bn:24;	/* basic block */
		unsigned int	ex_length:8;	/* numblocks in this extent */
		unsigned int	ex_offset:24;	/* logical offset into file */
	} cooked;
} efs_extent;

typedef struct edevs {
	__be16		odev;
	__be32		ndev;
} efs_devs;

/*
 * extent based filesystem inode as it appears on disk.  The efs inode
 * is exactly 128 bytes long.
 */
struct	iofs_dinode {
	__be16		di_mode;	/* mode and type of file */
	__be16		di_nlink;	/* number of links to file */
	__be16		di_uid;		/* owner's user id */
	__be16		di_gid;		/* owner's group id */
	__be32		di_size;	/* number of bytes in file */
	__be32		di_atime;	/* time last accessed */
	__be32		di_mtime;	/* time last modified */
	__be32		di_ctime;	/* time created */
	__be32		di_gen;		/* generation number */
	__be16		di_numextents;	/* # of extents */
	u_char		di_version;	/* version of inode */
	u_char		di_spare;	/* spare - used by AFS */
	union di_addr {
		efs_extent	di_extents[IOFS_DIRECTEXTENTS];
		efs_devs	di_dev;	/* device for IFCHR/IFBLK */
	} di_u;
};

/* efs inode storage in memory */
struct iofs_inode_info {
	int		numextents;
	int		lastextent;

	efs_extent	extents[IOFS_DIRECTEXTENTS];
	struct inode	vfs_inode;
};

#include "iofs_fs_sb.h"

#define IOFS_DIRBSIZE_BITS	IOFS_BLOCKSIZE_BITS
#define IOFS_DIRBSIZE		(1 << IOFS_DIRBSIZE_BITS)

struct efs_dentry {
	__be32		inode;
	unsigned char	namelen;
	char		name[3];
};

#define IOFS_DENTSIZE	(sizeof(struct efs_dentry) - 3 + 1)
#define IOFS_MAXNAMELEN  ((1 << (sizeof(char) * 8)) - 1)

#define IOFS_DIRBLK_HEADERSIZE	4
#define IOFS_DIRBLK_MAGIC	0xbeef	/* moo */

struct efs_dir {
	__be16	magic;
	unsigned char	firstused;
	unsigned char	slots;

	unsigned char	space[IOFS_DIRBSIZE - IOFS_DIRBLK_HEADERSIZE];
};

#define IOFS_MAXENTS \
	((IOFS_DIRBSIZE - IOFS_DIRBLK_HEADERSIZE) / \
	 (IOFS_DENTSIZE + sizeof(char)))

#define IOFS_SLOTAT(dir, slot) IOFS_REALOFF((dir)->space[slot])

#define IOFS_REALOFF(offset) ((offset << 1))


static inline struct iofs_inode_info *INODE_INFO(struct inode *inode)
{
	return container_of(inode, struct iofs_inode_info, vfs_inode);
}

static inline struct iofs_sb_info *SUPER_INFO(struct super_block *sb)
{
	return sb->s_fs_info;
}

struct statfs;
struct fid;

extern const struct inode_operations efs_dir_inode_operations;
extern const struct file_operations efs_dir_operations;
extern const struct address_space_operations efs_symlink_aops;

extern struct inode *efs_iget(struct super_block *, unsigned long);
extern efs_block_t efs_map_block(struct inode *, efs_block_t);
extern int efs_get_block(struct inode *, sector_t, struct buffer_head *, int);

extern struct dentry *efs_lookup(struct inode *, struct dentry *, unsigned int);
extern struct dentry *efs_fh_to_dentry(struct super_block *sb, struct fid *fid,
		int fh_len, int fh_type);
extern struct dentry *efs_fh_to_parent(struct super_block *sb, struct fid *fid,
		int fh_len, int fh_type);
extern struct dentry *efs_get_parent(struct dentry *);
extern int efs_bmap(struct inode *, int);

#endif /* _IOFS_IOFS_H_ */
