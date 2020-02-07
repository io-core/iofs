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
/*
	struct inode *inode = file_inode(file);
	iofs_block_t		block;
	int			slot;

	if (inode->i_size & (IOFS_DIRBSIZE-1))
		pr_warn("%s(): directory size not a multiple of IOFS_DIRBSIZE\n",
			__func__);

	/* work out where this entry can be found */
	block = ctx->pos >> IOFS_DIRBSIZE_BITS;

	/* each block contains at most 256 slots */
	slot  = ctx->pos & 0xff;

	/* look at all blocks */
	while (block < inode->i_blocks) {
		struct iofs_dir		*dirblock;
		struct buffer_head *bh;

		/* read the dir block */
		bh = sb_bread(inode->i_sb, iofs_bmap(inode, block));

		if (!bh) {
			pr_err("%s(): failed to read dir block %d\n",
			       __func__, block);
			break;
		}

		dirblock = (struct iofs_dir *) bh->b_data; 

		if (be16_to_cpu(dirblock->magic) != IOFS_DIRBLK_MAGIC) {
			pr_err("%s(): invalid directory block\n", __func__);
			brelse(bh);
			break;
		}

		for (; slot < dirblock->slots; slot++) {
			struct iofs_dentry *dirslot;
			iofs_ino_t inodenum;
			const char *nameptr;
			int namelen;

			if (dirblock->space[slot] == 0)
				continue;

			dirslot  = (struct iofs_dentry *) (((char *) bh->b_data) + IOFS_SLOTAT(dirblock, slot));

			inodenum = be32_to_cpu(dirslot->inode);
			namelen  = dirslot->namelen;
			nameptr  = dirslot->name;
			pr_debug("%s(): block %d slot %d/%d: inode %u, name \"%s\", namelen %u\n",
				 __func__, block, slot, dirblock->slots-1,
				 inodenum, nameptr, namelen);
			if (!namelen)
				continue;
			/* found the next entry */
			ctx->pos = (block << IOFS_DIRBSIZE_BITS) | slot;

			/* sanity check */
			if (nameptr - (char *) dirblock + namelen > IOFS_DIRBSIZE) {
				pr_warn("directory entry %d exceeds directory block\n",
					slot);
				continue;
			}

			/* copy filename and data in dirslot */
			if (!dir_emit(ctx, nameptr, namelen, inodenum, DT_UNKNOWN)) {
				brelse(bh);
				return 0;
			}
		}
		brelse(bh);

		slot = 0;
		block++;
	}
	ctx->pos = (block << IOFS_DIRBSIZE_BITS) | slot;
*/
	return 0;
}
