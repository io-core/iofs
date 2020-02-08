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

static int iofs_readdir(struct file *file, struct dir_context *ctx)
{
	struct inode *inode = file_inode(file);
	struct buffer_head *bh;

	int			slot, namelen;
	char			*nameptr;
	struct iofs_dinode	*dinode;
	struct iofs_de		*dirslot;

	bh = sb_bread(inode->i_sb, 0); //iofs_bmap(inode, block));

	if (!bh) {
		pr_err("%s(): failed to read dir inode %d\n",
		       __func__, 29);
		
	}

	dinode = (struct iofs_dinode *) bh->b_data; 


	if (le32_to_cpu(dinode->origin) != IOFS_DIRMARK) {
		pr_err("%s(): invalid directory inode\n", __func__);
		brelse(bh);
		return 0;
	}

	for (slot = ctx->pos; slot < dinode->dirb.m && slot < 24; slot++) {
		dirslot  = &dinode->dirb.e[slot];
		namelen  = strnlen(dirslot->name,24);
		nameptr  = dirslot->name;

                pr_debug("%s(): slot %d name \"%s\", namelen %u\n",
                         __func__, slot, 
                         nameptr, namelen);
                ctx->pos = slot;
                if (!dir_emit(ctx, nameptr, namelen, dirslot->adr, DT_UNKNOWN)) {
                        brelse(bh);
                        return 0;
                }

	}


        brelse(bh);
        ctx->pos = dinode->dirb.m;

	return 0;
}
