// SPDX-License-Identifier: GPL-2.0
/*
 * file.c
 *
 * Copyright 2020 Charles Perkins
 *
 * Portions derived from work (c) 1999 Al Smith
 * Portions derived from work (c) 1995,1996 Christian Vogelgsang.
 */

#include <linux/buffer_head.h>
#include "iofs.h"

int iofs_get_block(struct inode *inode, sector_t iblock,
		  struct buffer_head *bh_result, int create)
{
	int error = -EROFS;
	long phys;

	if (create)
		return error;
	if (iblock >= inode->i_blocks) {
#ifdef DEBUG
		/*
		 * i have no idea why this happens as often as it does
		 */
		pr_warn("%s(): block %llu >= %llu (filesize %llu)\n",
			__func__, iblock, inode->i_blocks, inode->i_size);
#endif
		return 0;
	}
	phys = iofs_map_block(inode, iblock);
	if (phys)
		map_bh(bh_result, inode->i_sb, phys);
	return 0;
}

int iofs_bmap(struct inode *inode, iofs_block_t block) {

	if (block < 0) {
		pr_warn("%s(): block < 0\n", __func__);
		return 0;
	}

	/* are we about to read past the end of a file ? */
	if (!(block < inode->i_blocks)) {
#ifdef DEBUG
		/*
		 * i have no idea why this happens as often as it does
		 */
		pr_warn("%s(): block %d >= %llu (filesize %llu)\n",
			__func__, block, inode->i_blocks, inode->i_size);
#endif
		return 0;
	}

	return iofs_map_block(inode, block);
}
