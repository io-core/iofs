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
#define IOFS_DIRMARK 0x9B1EA38D
#define IOFS_HEADERMARK 0x9BA71D86

#define IOFS_FNLENGTH 32
#define IOFS_SECTABSIZE 64
#define IOFS_EXTABSIZE 12
#define IOFS_SECTORSIZE 1024
#define IOFS_INDEXSIZE (IOFS_SECTORSIZE / 4)
#define IOFS_HEADERSIZE 352
#define IOFS_INDATASIZE (IOFS_SECTORSIZE - IOFS_HEADERSIZE)
#define IOFS_SMALLFILELIMIT (IOFS_SECTORSIZE * IOFS_SECTABSIZE)
#define IOFS_DIRROOTADR 29
#define IOFS_DIRPGSIZE 24
#define IOFS_FILLERSIZE 52
#define IOFS_FILENAME_MAXLEN 63


static const char cprt[] = "IOFS: "IOFS_VERSION"  ";


/* 1 block is 512 bytes */
#define	IOFS_BLOCKSIZE_BITS	10
#define	IOFS_BLOCKSIZE		(1 << IOFS_BLOCKSIZE_BITS)

typedef	int32_t		iofs_block_t;
typedef uint32_t	iofs_ino_t;

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
} iofs_extent;

typedef struct edevs {
	__be16		odev;
	__be32		ndev;
} iofs_devs;

/*
 * extent based filesystem inode as it appears on disk.  The efs inode
 * is exactly 128 bytes long.
 */

struct  iofs_de {  // directory entry B-tree node
    char name[IOFS_FNLENGTH];
    uint32_t  adr;       // sec no of file header
    uint32_t  p;         // sec no of descendant in directory
}__attribute__((packed));


struct iofs_fh {    // file header
    char name[IOFS_FNLENGTH];
    uint32_t aleng;
    uint32_t bleng;
    uint32_t date;
    uint32_t ext[IOFS_EXTABSIZE];                     // ExtensionTable
    uint32_t sec[IOFS_SECTABSIZE];                    // SectorTable;
    char fill[IOFS_SECTORSIZE - IOFS_HEADERSIZE];     // File Data
}__attribute__((packed));

struct iofs_dp {    // directory page
    uint32_t m;
    uint32_t p0;         //sec no of left descendant in directory
    char fill[IOFS_FILLERSIZE];
    struct iofs_de e[24];
}__attribute__((packed));

struct iofs_ep {    // extended page
    uint32_t x[256];
}__attribute__((packed)); 


struct iofs_dir_record {
    char filename[IOFS_FILENAME_MAXLEN];
    uint64_t inode_no;
}__attribute__((packed));

struct iofs_dinode {
    uint32_t origin;     // magic number on disk, inode type | sector number in memory
    union {
       struct iofs_fh fhb;
       struct iofs_dp dirb;
    };
//    struct inode vfs_inode;
}__attribute__((packed));


struct	old_iofs_dinode {
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
		iofs_extent	di_extents[IOFS_DIRECTEXTENTS];
		iofs_devs	di_dev;	/* device for IFCHR/IFBLK */
	} di_u;
};

/* efs inode storage in memory */
struct iofs_inode_info {
	int		numextents;
	int		lastextent;

	iofs_extent	extents[IOFS_DIRECTEXTENTS];
	struct inode	vfs_inode;
};

#include "iofs_fs_sb.h"

#define IOFS_DIRBSIZE_BITS	IOFS_BLOCKSIZE_BITS
#define IOFS_DIRBSIZE		(1 << IOFS_DIRBSIZE_BITS)

struct iofs_dentry {
	__be32		inode;
	unsigned char	namelen;
	char		name[3];
};

#define IOFS_DENTSIZE	(sizeof(struct iofs_dentry) - 3 + 1)
#define IOFS_MAXNAMELEN  ((1 << (sizeof(char) * 8)) - 1)

#define IOFS_DIRBLK_HEADERSIZE	4
#define IOFS_DIRBLK_MAGIC	0xbeef	/* moo */

struct iofs_dir {
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

extern const struct inode_operations iofs_dir_inode_operations;
extern const struct file_operations iofs_dir_operations;
extern const struct address_space_operations iofs_symlink_aops;

extern struct inode *iofs_iget(struct super_block *, unsigned long);
extern iofs_block_t iofs_map_block(struct inode *, iofs_block_t);
extern int iofs_get_block(struct inode *, sector_t, struct buffer_head *, int);

extern struct dentry *iofs_lookup(struct inode *, struct dentry *, unsigned int);
extern struct dentry *iofs_fh_to_dentry(struct super_block *sb, struct fid *fid,
		int fh_len, int fh_type);
extern struct dentry *iofs_fh_to_parent(struct super_block *sb, struct fid *fid,
		int fh_len, int fh_type);
extern struct dentry *iofs_get_parent(struct dentry *);
extern int iofs_bmap(struct inode *, int);

#endif /* _IOFS_IOFS_H_ */
