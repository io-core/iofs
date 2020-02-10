// SPDX-License-Identifier: GPL-2.0
/*
 * dir.c
 *
 * Copyright (c) 2020 Charles Perkins
 * 
 * Portions derived from work (c) 1999 Al Smith
 */

#include <linux/buffer_head.h>
#include "iofs.h"

static int iofs_readdir(struct file *, struct dir_context *);

const struct file_operations iofs_dir_operations = {
	.llseek		= generic_file_llseek,
	.read		= generic_read_dir,
	.iterate_shared	= iofs_readdir,
};

const struct inode_operations iofs_dir_inode_operations = {
	.lookup		= iofs_lookup,
};

static int do_iofs_readdir(struct file *file, uint64_t ino, struct dir_context *ctx, int start)
{
	
        struct inode *finode = file_inode(file);
	struct buffer_head *bh;

	int			slot, here, namelen;
	char			*nameptr;
	struct iofs_dinode	*dinode;
        struct iofs_dinode      dinode_buf;
	struct iofs_de		*dirslot;
	
	here = start;
//if (here == 0){
	bh = sb_bread(finode->i_sb, (ino/29)-1);

	if (!bh) {
		pr_err("%s(): failed to read dir inode %llu\n",
		       __func__, ino);
		return 0;	
	}

	dinode = &dinode_buf; 
        memcpy(dinode,bh->b_data,sizeof(dinode_buf));
        brelse(bh);

	if (le32_to_cpu(dinode->origin) != IOFS_DIRMARK) {
		pr_err("%s(): invalid directory inode %llu\n", __func__,ino);
		return here;
	}


	if (here==0){
		if (ctx->pos==0){
		  dir_emit_dot(file, ctx);
                  ctx->pos++;
                }
		here++;
	}

        if (dinode->dirb.p0 != 0){
                pr_debug("%s(): pre recursive read dir inode %llu\n",
                       __func__, ino);
                here = do_iofs_readdir( file, dinode->dirb.p0, ctx, here );
        }


	for (slot = 0; slot < dinode->dirb.m && slot < 24; slot++) {
		dirslot  = &dinode->dirb.e[slot];
		namelen  = strnlen(dirslot->name,24);
		nameptr  = dirslot->name;
		if( here >= ctx->pos) {
                  pr_debug("%s(): slot %llu:%d name \"%s\", namelen %u inode %u\n",
                         __func__, ino, slot,
                         nameptr, namelen, dirslot->adr);

                  ctx->pos++;
                  if (!dir_emit(ctx, nameptr, namelen, dirslot->adr, DT_UNKNOWN)) {
                        brelse(bh);
                        return here;
                  }

                  if (dirslot->p != 0){
                        pr_debug("%s(): descending inode %llu (children of %s)\n",
                               __func__, ino,nameptr);
                        here = do_iofs_readdir( file, dirslot->p, ctx, here );
                  }


		}
		here++;
	}

	return here;
}

static int iofs_readdir(struct file *file, struct dir_context *ctx)
{
    struct inode *inode = file_inode(file);
    int ret;
    ret = do_iofs_readdir( file, inode->i_ino, ctx, 0 ); 
    ctx->pos = INT_MAX;
    return 0;
}
