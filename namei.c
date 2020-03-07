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
//#include <linux/ctype.h>
#include "iofs.h"

static iofs_ino_t iofs_find_entry(struct super_block *sb, iofs_ino_t ino, const char *name, int len, iofs_ino_t *dpino, int *di)
{

	struct buffer_head *bh;

	int			slot, namelen, ret;
	char			*nameptr;
	struct iofs_dinode	*dinode;
        struct iofs_dinode      dinode_buf;
	struct iofs_de		*dirslot;

	iofs_ino_t		inodenum;
        iofs_ino_t		lower;

   if (ino != 0){

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
			*dpino=ino;
			*di=slot;
			return inodenum;
//		}else if(strncmp(name,nameptr,len) < 0 ){
//			return iofs_find_entry(sb,lower,name,len,dpino,di);
                }else{
                      ret=0;
		      if(strncmp(name,nameptr,len) < 0 )
                         ret = iofs_find_entry(sb,lower,name,len,dpino,di);
		      if (ret!=0) return ret;
                }
                lower = le32_to_cpu(dirslot->p);
	}
	return iofs_find_entry(sb,lower,name,len,dpino,di);
   }
   return 0;
}

struct dentry *iofs_lookup(struct inode *dir, struct dentry *dentry, unsigned int flags)
{
	iofs_ino_t inodenum;
	struct inode *inode = NULL;
        iofs_ino_t dpino = 0;
        int di = 0;

	inodenum = iofs_find_entry(dir->i_sb, dir->i_ino, dentry->d_name.name, dentry->d_name.len, &dpino, &di);
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
        iofs_ino_t dpino = 0;
        int di = 0;

	ino = iofs_find_entry(d_inode(child)->i_sb, d_inode(child)->i_ino, "..", 2, &dpino, &di);
	if (ino)
		parent = d_obtain_alias(iofs_iget(child->d_sb, ino));

	return parent;
}


/*


static int add_nondir(struct dentry *dentry, struct inode *inode)
{
	int err = minix_add_link(dentry, inode);
	if (!err) {
		d_instantiate(dentry, inode);
		return 0;
	}
	inode_dec_link_count(inode);
	iput(inode);
	return err;
}

static struct dentry *minix_lookup(struct inode * dir, struct dentry *dentry, unsigned int flags)
{
	struct inode * inode = NULL;
	ino_t ino;

	if (dentry->d_name.len > minix_sb(dir->i_sb)->s_namelen)
		return ERR_PTR(-ENAMETOOLONG);

	ino = minix_inode_by_name(dentry);
	if (ino)
		inode = minix_iget(dir->i_sb, ino);
	return d_splice_alias(inode, dentry);
}

static int minix_mknod(struct inode * dir, struct dentry *dentry, umode_t mode, dev_t rdev)
{
	int error;
	struct inode *inode;

	if (!old_valid_dev(rdev))
		return -EINVAL;

	inode = minix_new_inode(dir, mode, &error);

	if (inode) {
		minix_set_inode(inode, rdev);
		mark_inode_dirty(inode);
		error = add_nondir(dentry, inode);
	}
	return error;
}

static int minix_tmpfile(struct inode *dir, struct dentry *dentry, umode_t mode)
{
	int error;
	struct inode *inode = minix_new_inode(dir, mode, &error);
	if (inode) {
		minix_set_inode(inode, 0);
		mark_inode_dirty(inode);
		d_tmpfile(dentry, inode);
	}
	return error;
}

static int minix_create(struct inode *dir, struct dentry *dentry, umode_t mode,
		bool excl)
{
	return minix_mknod(dir, dentry, mode, 0);
}

static int minix_symlink(struct inode * dir, struct dentry *dentry,
	  const char * symname)
{
	int err = -ENAMETOOLONG;
	int i = strlen(symname)+1;
	struct inode * inode;

	if (i > dir->i_sb->s_blocksize)
		goto out;

	inode = minix_new_inode(dir, S_IFLNK | 0777, &err);
	if (!inode)
		goto out;

	minix_set_inode(inode, 0);
	err = page_symlink(inode, symname, i);
	if (err)
		goto out_fail;

	err = add_nondir(dentry, inode);
out:
	return err;

out_fail:
	inode_dec_link_count(inode);
	iput(inode);
	goto out;
}

static int minix_link(struct dentry * old_dentry, struct inode * dir,
	struct dentry *dentry)
{
	struct inode *inode = d_inode(old_dentry);

	inode->i_ctime = current_time(inode);
	inode_inc_link_count(inode);
	ihold(inode);
	return add_nondir(dentry, inode);
}

static int minix_mkdir(struct inode * dir, struct dentry *dentry, umode_t mode)
{
	struct inode * inode;
	int err;

	inode_inc_link_count(dir);

	inode = minix_new_inode(dir, S_IFDIR | mode, &err);
	if (!inode)
		goto out_dir;

	minix_set_inode(inode, 0);

	inode_inc_link_count(inode);

	err = minix_make_empty(inode, dir);
	if (err)
		goto out_fail;

	err = minix_add_link(dentry, inode);
	if (err)
		goto out_fail;

	d_instantiate(dentry, inode);
out:
	return err;

out_fail:
	inode_dec_link_count(inode);
	inode_dec_link_count(inode);
	iput(inode);
out_dir:
	inode_dec_link_count(dir);
	goto out;
}
*/

int iofs_unlink(struct inode * dir, struct dentry *dentry)
{
	int err = -ENOENT;
        iofs_ino_t dpino = 0;
        int di = 0;

	struct inode * inode = d_inode(dentry);


        iofs_ino_t inodenum = iofs_find_entry(dir->i_sb, dir->i_ino, dentry->d_name.name, dentry->d_name.len, &dpino, &di);
	if (!inodenum)
		goto end_unlink;

	err = iofs_delete_entry(dir, dpino, di);
	if (err)
		goto end_unlink;

	inode->i_ctime = dir->i_ctime;
	inode_dec_link_count(inode);

end_unlink:
	return err;
}


/*
static int minix_rmdir(struct inode * dir, struct dentry *dentry)
{
	struct inode * inode = d_inode(dentry);
	int err = -ENOTEMPTY;

	if (minix_empty_dir(inode)) {
		err = minix_unlink(dir, dentry);
		if (!err) {
			inode_dec_link_count(dir);
			inode_dec_link_count(inode);
		}
	}
	return err;
}

static int minix_rename(struct inode * old_dir, struct dentry *old_dentry,
			struct inode * new_dir, struct dentry *new_dentry,
			unsigned int flags)
{
	struct inode * old_inode = d_inode(old_dentry);
	struct inode * new_inode = d_inode(new_dentry);
	struct page * dir_page = NULL;
	struct minix_dir_entry * dir_de = NULL;
	struct page * old_page;
	struct minix_dir_entry * old_de;
	int err = -ENOENT;

	if (flags & ~RENAME_NOREPLACE)
		return -EINVAL;

	old_de = minix_find_entry(old_dentry, &old_page);
	if (!old_de)
		goto out;

	if (S_ISDIR(old_inode->i_mode)) {
		err = -EIO;
		dir_de = minix_dotdot(old_inode, &dir_page);
		if (!dir_de)
			goto out_old;
	}

	if (new_inode) {
		struct page * new_page;
		struct minix_dir_entry * new_de;

		err = -ENOTEMPTY;
		if (dir_de && !minix_empty_dir(new_inode))
			goto out_dir;

		err = -ENOENT;
		new_de = minix_find_entry(new_dentry, &new_page);
		if (!new_de)
			goto out_dir;
		minix_set_link(new_de, new_page, old_inode);
		new_inode->i_ctime = current_time(new_inode);
		if (dir_de)
			drop_nlink(new_inode);
		inode_dec_link_count(new_inode);
	} else {
		err = minix_add_link(new_dentry, old_inode);
		if (err)
			goto out_dir;
		if (dir_de)
			inode_inc_link_count(new_dir);
	}

	minix_delete_entry(old_de, old_page);
	mark_inode_dirty(old_inode);

	if (dir_de) {
		minix_set_link(dir_de, dir_page, new_dir);
		inode_dec_link_count(old_dir);
	}
	return 0;

out_dir:
	if (dir_de) {
		kunmap(dir_page);
		put_page(dir_page);
	}
out_old:
	kunmap(old_page);
	put_page(old_page);
out:
	return err;
}

*/
