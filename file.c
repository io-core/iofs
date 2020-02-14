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
/*
	int error = -EROFS;
	long phys;

	if (create)
		return error;
	if (iblock >= inode->i_blocks) {
		pr_warn("%s(): block %llu >= %llu (filesize %llu)\n",
			__func__, iblock, inode->i_blocks, inode->i_size);

		return 0;
	}
	phys = iofs_map_block(inode, iblock);
	if (phys)
		map_bh(bh_result, inode->i_sb, phys);
*/
	return 0;
}

int iofs_bmap(struct inode *inode, iofs_block_t block) {
/*
	if (block < 0) {
		pr_warn("%s(): block < 0\n", __func__);
		return 0;
	}


	if (!(block < inode->i_blocks)) {
		pr_warn("%s(): block %d >= %llu (filesize %llu)\n",
			__func__, block, inode->i_blocks, inode->i_size);

		return 0;
	}
*/
	return 0; //iofs_map_block(inode, block);
}
