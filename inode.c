// SPDX-License-Identifier: GPL-2.0-only
/*
 * inode.c
 *
 * Copyright 2020 Charles Perkins
 *
 * Portions derived from work (c) 1999 Al Smith
 * Portions derived from work (c) 1995,1996 Christian Vogelgsang,
 *              and from work (c) 1998 Mike Shaver.
 */

#include <linux/buffer_head.h>
#include <linux/module.h>
#include <linux/fs.h>
#include "iofs.h"
#include "iofs_fs_sb.h"

static int iofs_readpage(struct file *file, struct page *page)
{
	return block_read_full_page(page,iofs_get_block);
}
static sector_t _iofs_bmap(struct address_space *mapping, sector_t block)
{
	return generic_block_bmap(mapping,block,iofs_get_block);
}
static const struct address_space_operations iofs_aops = {
	.readpage = iofs_readpage,
	.bmap = _iofs_bmap
};

static inline void extent_copy(iofs_extent *src, iofs_extent *dst) {
	/*
	 * this is slightly evil. it doesn't just copy
	 * iofs_extent from src to dst, it also mangles
	 * the bits so that dst ends up in cpu byte-order.
	 */

	dst->cooked.ex_magic  =  (unsigned int) src->raw[0];
	dst->cooked.ex_bn     = ((unsigned int) src->raw[1] << 16) |
				((unsigned int) src->raw[2] <<  8) |
				((unsigned int) src->raw[3] <<  0);
	dst->cooked.ex_length =  (unsigned int) src->raw[4];
	dst->cooked.ex_offset = ((unsigned int) src->raw[5] << 16) |
				((unsigned int) src->raw[6] <<  8) |
				((unsigned int) src->raw[7] <<  0);
	return;
}

struct inode *iofs_iget(struct super_block *super, unsigned long ino)
{
/*
	int i, inode_index;
	dev_t device;
	u32 rdev;
	struct buffer_head *bh;
	struct iofs_sb_info    *sb = SUPER_INFO(super);
	struct iofs_inode_info *in;
	iofs_block_t block, offset;
	struct iofs_dinode *iofs_inode;
	struct inode *inode;

	inode = iget_locked(super, ino);
	if (!inode)
		return ERR_PTR(-ENOMEM);
	if (!(inode->i_state & I_NEW))
		return inode;

	in = INODE_INFO(inode);

	
	// EFS layout:
	//
	// |   cylinder group    |   cylinder group    |   cylinder group ..etc
	// |inodes|data          |inodes|data          |inodes|data       ..etc
	//
	// work out the inode block index, (considering initially that the
	// inodes are stored as consecutive blocks). then work out the block
	// number of that inode given the above layout, and finally the
	// offset of the inode within that block.
	

	inode_index = inode->i_ino /
		(IOFS_BLOCKSIZE / sizeof(struct iofs_dinode));

	block = 1; //sb->fs_start + sb->first_block + 
		//(sb->group_size * (inode_index / sb->inode_blocks)) +
		//(inode_index % sb->inode_blocks);

	offset = (inode->i_ino %
			(IOFS_BLOCKSIZE / sizeof(struct iofs_dinode))) *
		sizeof(struct iofs_dinode);

	bh = sb_bread(inode->i_sb, block);
	if (!bh) {
		pr_warn("%s() failed at block %d\n", __func__, block);
		goto read_inode_error;
	}

	iofs_inode = (struct iofs_dinode *) (bh->b_data + offset);
    
	inode->i_mode  = be16_to_cpu(iofs_inode->di_mode);
	set_nlink(inode, be16_to_cpu(iofs_inode->di_nlink));
	i_uid_write(inode, (uid_t)be16_to_cpu(iofs_inode->di_uid));
	i_gid_write(inode, (gid_t)be16_to_cpu(iofs_inode->di_gid));
	inode->i_size  = be32_to_cpu(iofs_inode->di_size);
	inode->i_atime.tv_sec = be32_to_cpu(iofs_inode->di_atime);
	inode->i_mtime.tv_sec = be32_to_cpu(iofs_inode->di_mtime);
	inode->i_ctime.tv_sec = be32_to_cpu(iofs_inode->di_ctime);
	inode->i_atime.tv_nsec = inode->i_mtime.tv_nsec = inode->i_ctime.tv_nsec = 0;

	// this is the number of blocks in the file 
	if (inode->i_size == 0) {
		inode->i_blocks = 0;
	} else {
		inode->i_blocks = ((inode->i_size - 1) >> IOFS_BLOCKSIZE_BITS) + 1;
	}

	rdev = be16_to_cpu(iofs_inode->di_u.di_dev.odev);
	if (rdev == 0xffff) {
		rdev = be32_to_cpu(iofs_inode->di_u.di_dev.ndev);
		if (sysv_major(rdev) > 0xfff)
			device = 0;
		else
			device = MKDEV(sysv_major(rdev), sysv_minor(rdev));
	} else
		device = old_decode_dev(rdev);

	// get the number of extents for this object 
	in->numextents = be16_to_cpu(iofs_inode->di_numextents);
	in->lastextent = 0;

	// copy the extents contained within the inode to memory 
	for(i = 0; i < IOFS_DIRECTEXTENTS; i++) {
		extent_copy(&(iofs_inode->di_u.di_extents[i]), &(in->extents[i]));
		if (i < in->numextents && in->extents[i].cooked.ex_magic != 0) {
			pr_warn("extent %d has bad magic number in inode %lu\n",
				i, inode->i_ino);
			brelse(bh);
			goto read_inode_error;
		}
	}

	brelse(bh);
	pr_debug("iofs_iget(): inode %lu, extents %d, mode %o\n",
		 inode->i_ino, in->numextents, inode->i_mode);
	switch (inode->i_mode & S_IFMT) {
		case S_IFDIR: 
			inode->i_op = &iofs_dir_inode_operations; 
			inode->i_fop = &iofs_dir_operations; 
			break;
		case S_IFREG:
			inode->i_fop = &generic_ro_fops;
			inode->i_data.a_ops = &iofs_aops;
			break;
		case S_IFLNK:
			inode->i_op = &page_symlink_inode_operations;
			inode_nohighmem(inode);
			inode->i_data.a_ops = &iofs_symlink_aops;
			break;
		case S_IFCHR:
		case S_IFBLK:
		case S_IFIFO:
			init_special_inode(inode, inode->i_mode, device);
			break;
		default:
			pr_warn("unsupported inode mode %o\n", inode->i_mode);
			goto read_inode_error;
			break;
	}

	unlock_new_inode(inode);
	return inode;
        
read_inode_error:
	pr_warn("failed to read inode %lu\n", inode->i_ino);
	iget_failed(inode);
*/
	return ERR_PTR(-EIO);
}

static inline iofs_block_t
iofs_extent_check(iofs_extent *ptr, iofs_block_t block, struct iofs_sb_info *sb) {
	iofs_block_t start;
	iofs_block_t length;
	iofs_block_t offset;

	/*
	 * given an extent and a logical block within a file,
	 * can this block be found within this extent ?
	 */
	start  = ptr->cooked.ex_bn;
	length = ptr->cooked.ex_length;
	offset = ptr->cooked.ex_offset;

	if ((block >= offset) && (block < offset+length)) {
		return(sb->fs_start + start + block - offset);
	} else {
		return 0;
	}
}

iofs_block_t iofs_map_block(struct inode *inode, iofs_block_t block) {
	struct iofs_sb_info    *sb = SUPER_INFO(inode->i_sb);
	struct iofs_inode_info *in = INODE_INFO(inode);
	struct buffer_head    *bh = NULL;

	int cur, last, first = 1;
	int ibase, ioffset, dirext, direxts, indext, indexts;
	iofs_block_t iblock, result = 0, lastblock = 0;
	iofs_extent ext, *exts;

	last = in->lastextent;

	if (in->numextents <= IOFS_DIRECTEXTENTS) {
		/* first check the last extent we returned */
		if ((result = iofs_extent_check(&in->extents[last], block, sb)))
			return result;
    
		/* if we only have one extent then nothing can be found */
		if (in->numextents == 1) {
			pr_err("%s() failed to map (1 extent)\n", __func__);
			return 0;
		}

		direxts = in->numextents;

		/*
		 * check the stored extents in the inode
		 * start with next extent and check forwards
		 */
		for(dirext = 1; dirext < direxts; dirext++) {
			cur = (last + dirext) % in->numextents;
			if ((result = iofs_extent_check(&in->extents[cur], block, sb))) {
				in->lastextent = cur;
				return result;
			}
		}

		pr_err("%s() failed to map block %u (dir)\n", __func__, block);
		return 0;
	}

	pr_debug("%s(): indirect search for logical block %u\n",
		 __func__, block);
	direxts = in->extents[0].cooked.ex_offset;
	indexts = in->numextents;

	for(indext = 0; indext < indexts; indext++) {
		cur = (last + indext) % indexts;

		/*
		 * work out which direct extent contains `cur'.
		 *
		 * also compute ibase: i.e. the number of the first
		 * indirect extent contained within direct extent `cur'.
		 *
		 */
		ibase = 0;
		for(dirext = 0; cur < ibase && dirext < direxts; dirext++) {
			ibase += in->extents[dirext].cooked.ex_length *
				(IOFS_BLOCKSIZE / sizeof(iofs_extent));
		}

		if (dirext == direxts) {
			/* should never happen */
			pr_err("couldn't find direct extent for indirect extent %d (block %u)\n",
			       cur, block);
			if (bh) brelse(bh);
			return 0;
		}
		
		/* work out block number and offset of this indirect extent */
		iblock = sb->fs_start + in->extents[dirext].cooked.ex_bn +
			(cur - ibase) /
			(IOFS_BLOCKSIZE / sizeof(iofs_extent));
		ioffset = (cur - ibase) %
			(IOFS_BLOCKSIZE / sizeof(iofs_extent));

		if (first || lastblock != iblock) {
			if (bh) brelse(bh);

			bh = sb_bread(inode->i_sb, iblock);
			if (!bh) {
				pr_err("%s() failed at block %d\n",
				       __func__, iblock);
				return 0;
			}
			pr_debug("%s(): read indirect extent block %d\n",
				 __func__, iblock);
			first = 0;
			lastblock = iblock;
		}

		exts = (iofs_extent *) bh->b_data;

		extent_copy(&(exts[ioffset]), &ext);

		if (ext.cooked.ex_magic != 0) {
			pr_err("extent %d has bad magic number in block %d\n",
			       cur, iblock);
			if (bh) brelse(bh);
			return 0;
		}

		if ((result = iofs_extent_check(&ext, block, sb))) {
			if (bh) brelse(bh);
			in->lastextent = cur;
			return result;
		}
	}
	if (bh) brelse(bh);
	pr_err("%s() failed to map block %u (indir)\n", __func__, block);
	return 0;
}  

MODULE_LICENSE("GPL");
