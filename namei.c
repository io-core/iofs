// SPDX-License-Identifier: GPL-2.0
/*
 * namei.c
 *
 * Copyright (c) 2020 Charles Perkins
 *
 * Portions derived from work (c) 1999 Al Smith
 * Portions derived from work (c) 1995,1996 Christian Vogelgsang.
 */

#include <linux/buffer_head.h>
#include <linux/string.h>
#include <linux/exportfs.h>
#include "iofs.h"


static iofs_ino_t iofs_find_entry(struct super_block *sb, iofs_ino_t ino, const char *name, int len)
{

	struct buffer_head *bh;

	int			slot, namelen;
	char			*nameptr;
	struct iofs_dinode	*dinode;
        struct iofs_dinode      dinode_buf;
	struct iofs_de		*dirslot;

	iofs_ino_t		inodenum;
        iofs_ino_t		lower;

        bh = sb_bread(sb, (ino/29)-1);
        if (!bh) {
                pr_err("%s(): failed to read dir inode %d\n",
                       __func__, 29);
                return 0;
        }

        dinode = &dinode_buf;
        memcpy(dinode,bh->b_data,sizeof(dinode_buf));
        brelse(bh);

        if (le32_to_cpu(dinode->origin) != IOFS_DIRMARK) {
                pr_err("%s(): invalid directory inode %d\n", __func__,ino);
                return 0;
        }

        lower = dinode->dirb.p0;
	for (slot = 0; slot < dinode->dirb.m && slot < 24; slot++) {
		dirslot  = &dinode->dirb.e[slot];
		namelen  = strnlen(dirslot->name,24);
		nameptr  = dirslot->name;

		if ((namelen == len) && (!memcmp(name, nameptr, len))) {
			inodenum = le32_to_cpu(dirslot->adr);
			return inodenum;
		}else if(strncmp(name,nameptr,len) < 0 ){
			return iofs_find_entry(sb,lower,name,len);
                }
                lower = dirslot->p;
	}
	return 0;
}

struct dentry *iofs_lookup(struct inode *dir, struct dentry *dentry, unsigned int flags)
{
	iofs_ino_t inodenum;
	struct inode *inode = NULL;

	inodenum = iofs_find_entry(dir->i_sb, dir->i_ino, dentry->d_name.name, dentry->d_name.len);
	if (inodenum)
		inode = iofs_iget(dir->i_sb, inodenum);

	return d_splice_alias(inode, dentry);
}

static struct inode *iofs_nfs_get_inode(struct super_block *sb, u64 ino,
		u32 generation)
{
	struct inode *inode;

	if (ino == 0)
		return ERR_PTR(-ESTALE);
	inode = iofs_iget(sb, ino);
	if (IS_ERR(inode))
		return ERR_CAST(inode);

	if (generation && inode->i_generation != generation) {
		iput(inode);
		return ERR_PTR(-ESTALE);
	}

	return inode;
}

struct dentry *iofs_fh_to_dentry(struct super_block *sb, struct fid *fid,
		int fh_len, int fh_type)
{
	return generic_fh_to_dentry(sb, fid, fh_len, fh_type,
				    iofs_nfs_get_inode);
}

struct dentry *iofs_fh_to_parent(struct super_block *sb, struct fid *fid,
		int fh_len, int fh_type)
{
	return generic_fh_to_parent(sb, fid, fh_len, fh_type,
				    iofs_nfs_get_inode);
}

struct dentry *iofs_get_parent(struct dentry *child)
{
	struct dentry *parent = ERR_PTR(-ENOENT);
	iofs_ino_t ino;

	ino = iofs_find_entry(d_inode(child)->i_sb, d_inode(child)->i_ino, "..", 2);
	if (ino)
		parent = d_obtain_alias(iofs_iget(child->d_sb, ino));

	return parent;
}
