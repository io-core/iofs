// SPDX-License-Identifier: GPL-2.0
/*
 * symlink.c
 *
 * Copyright (c) 2020 Charles Perkins
 *
 * Portions derived from work (c) 1999 Al Smith
 * Portions derived from work (c) 1995,1996 Christian Vogelgsang.
 */

#include <linux/string.h>
#include <linux/pagemap.h>
#include <linux/buffer_head.h>
#include "iofs.h"

static int iofs_symlink_readpage(struct file *file, struct page *page)
{
	char *link = page_address(page);
	struct buffer_head * bh;
	struct inode * inode = page->mapping->host;
	iofs_block_t size = inode->i_size;
	int err;
  
	err = -ENAMETOOLONG;
	if (size > 2 * IOFS_BLOCKSIZE)
		goto fail;
  
	/* read first 512 bytes of link target */
	err = -EIO;
	bh = sb_bread(inode->i_sb, iofs_bmap(inode, 0));
	if (!bh)
		goto fail;
	memcpy(link, bh->b_data, (size > IOFS_BLOCKSIZE) ? IOFS_BLOCKSIZE : size);
	brelse(bh);
	if (size > IOFS_BLOCKSIZE) {
		bh = sb_bread(inode->i_sb, iofs_bmap(inode, 1));
		if (!bh)
			goto fail;
		memcpy(link + IOFS_BLOCKSIZE, bh->b_data, size - IOFS_BLOCKSIZE);
		brelse(bh);
	}
	link[size] = '\0';
	SetPageUptodate(page);
	unlock_page(page);
	return 0;
fail:
	SetPageError(page);
	unlock_page(page);
	return err;
}

const struct address_space_operations iofs_symlink_aops = {
	.readpage	= iofs_symlink_readpage
};
