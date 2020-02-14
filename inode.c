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
#include <linux/time.h>

static int iofs_readpage(struct file *file, struct page *page)
{
        struct inode *inode = file_inode(file);
        struct buffer_head *bh;
        void * buf;
	struct iofs_dinode * finode;
	struct iofs_dinode finode_buf;
        int ret = -EIO;


        loff_t offset, size, lim;
	
        unsigned long fillsize, idx, iidx, sector, sofar;

        buf = kmap(page);
        if (!buf)
                return -ENOMEM;

        offset = page_offset(page);
        size = i_size_read(inode);

        bh = sb_bread(inode->i_sb, (inode->i_ino/29)-1);
        finode = &finode_buf;
        memcpy(&finode_buf,bh->b_data,IOFS_SECTORSIZE);
	brelse(bh);

        fillsize = 0;
	sofar = 0;
	ret = 0;

        while( (offset < size) && (sofar < PAGE_SIZE) ){

            if (offset < IOFS_INDATASIZE){
                memcpy(buf,finode->fhb.fill+offset,IOFS_INDATASIZE-offset);
	        sofar = IOFS_INDATASIZE - offset;
		offset += sofar;
	    }else if (offset < IOFS_SMALLFILELIMIT ) {
		idx = ((offset - IOFS_INDATASIZE) / IOFS_SECTORSIZE)+1;
		
	        bh = sb_bread(inode->i_sb, (finode->fhb.sec[idx]/29)-1);
	        finode = &finode_buf;
		lim=IOFS_SECTORSIZE;
		if (sofar+IOFS_SECTORSIZE > PAGE_SIZE) {lim -= PAGE_SIZE-(sofar+IOFS_SECTORSIZE);}
	        memcpy(buf+sofar,bh->b_data,lim);
	        brelse(bh);
		sofar+=lim;
		offset +=lim;
	    }else{
//                iidx = (offset - IOFS_INDATASIZE - (IOFS_SECTORSIZE * (IOFS_SECTABSIZE - 1))) / (IOFS_SECTORSIZE * 256);
//                idx = (offset - IOFS_INDATASIZE - (IOFS_SECTORSIZE * (IOFS_SECTABSIZE - 1))) / (IOFS_SECTORSIZE * 256);
//                zone = 2;
//                lim = IOFS_SECTORSIZE;
	          offset = size;
	    }
	   
	}

        if (sofar < PAGE_SIZE)
                memset(buf + sofar, 0, PAGE_SIZE - sofar);
        if (ret == 0)
                SetPageUptodate(page);

        flush_dcache_page(page);
        kunmap(page);
        unlock_page(page);
        return ret;


//	return block_read_full_page(page,iofs_get_block);
}
/*
static sector_t _iofs_bmap(struct address_space *mapping, sector_t block)
{
	return 0; //generic_block_bmap(mapping,block,iofs_get_block);
}
*/
static const struct address_space_operations iofs_aops = {
	.readpage = iofs_readpage  //,
//	.bmap = _iofs_bmap
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
	struct buffer_head *bh;
	struct iofs_dinode *iofs_inode;
	struct inode *inode;
        uint32_t tv;
        time_t t_of_day;

	inode = iget_locked(super, ino);
	if (!inode)
		return ERR_PTR(-ENOMEM);
	if (!(inode->i_state & I_NEW))
		return inode;

	bh = sb_bread(inode->i_sb, inode->i_ino/29 -1);
	if (!bh) {
		pr_warn("%s() failed at inode %lu\n", __func__, inode->i_ino);
		goto read_inode_error;
	}

	iofs_inode = (struct iofs_dinode *) (bh->b_data); 

        if (iofs_inode->origin == IOFS_DIRMARK) {
          inode->i_mode = 0040777; //octal
        }else{
          inode->i_mode = 0100777; //octal
        }
    

//	set_nlink(inode, be16_to_cpu(iofs_inode->di_nlink));
//	i_uid_write(inode, (uid_t)be16_to_cpu(iofs_inode->di_uid));
//	i_gid_write(inode, (gid_t)be16_to_cpu(iofs_inode->di_gid));

	inode->i_size  = iofs_inode->fhb.aleng * 1024 + iofs_inode->fhb.bleng - 352;  //be32_to_cpu(iofs_inode->di_size);

        tv = iofs_inode->fhb.date;
    //                  year        month             day                  hour                 minute            second
    t_of_day = mktime((uint32_t)((tv >> 26) & 0x3FF)+2000, (tv >> 22) & 0xFF , (tv >> 18) & 0x1FF, (tv >> 12) & 0x1FF, ( tv >> 6) & 0x3FF, tv & 0x3FF);



	inode->i_atime.tv_sec = t_of_day; 
	inode->i_mtime.tv_sec = t_of_day; 
	inode->i_ctime.tv_sec = t_of_day; 
	inode->i_atime.tv_nsec = inode->i_mtime.tv_nsec = inode->i_ctime.tv_nsec = 0;

//        inode->i_sb = super;
//        inode->i_ino = ino;
//        inode->i_op = &iofs_inode_ops;

	inode->i_blocks = 0;

	brelse(bh);

//	pr_debug("iofs_iget(): inode %lu, mode %o\n",
//		 inode->i_ino, inode->i_mode);

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
//		case S_IFIFO:
//			init_special_inode(inode, inode->i_mode, device);
//			break;
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

	return ERR_PTR(-EIO);
}


/*
static inline efs_block_t
efs_extent_check(efs_extent *ptr, efs_block_t block, struct efs_sb_info *sb) {
	efs_block_t start;
	efs_block_t length;
	efs_block_t offset;

	//
	// given an extent and a logical block within a file,
	// can this block be found within this extent ?
	//
	start  = ptr->cooked.ex_bn;
	length = ptr->cooked.ex_length;
	offset = ptr->cooked.ex_offset;

	if ((block >= offset) && (block < offset+length)) {
		return(sb->fs_start + start + block - offset);
	} else {
		return 0;
	}
}
*/



iofs_block_t iofs_map_block(struct inode *inode, iofs_block_t block) {
/*
	struct efs_sb_info    *sb = SUPER_INFO(inode->i_sb);
	struct efs_inode_info *in = INODE_INFO(inode);
	struct buffer_head    *bh = NULL;

	int cur, last, first = 1;
	int ibase, ioffset, dirext, direxts, indext, indexts;
	efs_block_t iblock, result = 0, lastblock = 0;
	efs_extent ext, *exts;

	last = in->lastextent;

	if (in->numextents <= EFS_DIRECTEXTENTS) {
		// first check the last extent we returned 
		if ((result = efs_extent_check(&in->extents[last], block, sb)))
			return result;
    
		// if we only have one extent then nothing can be found 
		if (in->numextents == 1) {
			pr_err("%s() failed to map (1 extent)\n", __func__);
			return 0;
		}

		direxts = in->numextents;

		//
		// check the stored extents in the inode
		// start with next extent and check forwards
		//
		for(dirext = 1; dirext < direxts; dirext++) {
			cur = (last + dirext) % in->numextents;
			if ((result = efs_extent_check(&in->extents[cur], block, sb))) {
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

		//
		// work out which direct extent contains `cur'.
		//
		// also compute ibase: i.e. the number of the first
		// indirect extent contained within direct extent `cur'.
		//
		//
		ibase = 0;
		for(dirext = 0; cur < ibase && dirext < direxts; dirext++) {
			ibase += in->extents[dirext].cooked.ex_length *
				(EFS_BLOCKSIZE / sizeof(efs_extent));
		}

		if (dirext == direxts) {
			// should never happen 
			pr_err("couldn't find direct extent for indirect extent %d (block %u)\n",
			       cur, block);
			if (bh) brelse(bh);
			return 0;
		}
		
		// work out block number and offset of this indirect extent 
		iblock = sb->fs_start + in->extents[dirext].cooked.ex_bn +
			(cur - ibase) /
			(EFS_BLOCKSIZE / sizeof(efs_extent));
		ioffset = (cur - ibase) %
			(EFS_BLOCKSIZE / sizeof(efs_extent));

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

		exts = (efs_extent *) bh->b_data;

		extent_copy(&(exts[ioffset]), &ext);

		if (ext.cooked.ex_magic != 0) {
			pr_err("extent %d has bad magic number in block %d\n",
			       cur, iblock);
			if (bh) brelse(bh);
			return 0;
		}

		if ((result = efs_extent_check(&ext, block, sb))) {
			if (bh) brelse(bh);
			in->lastextent = cur;
			return result;
		}
	}
	if (bh) brelse(bh);


	pr_err("%s() failed to map block %u (indir)\n", __func__, block);
*/
        pr_err("%s() failed to map inode %lu block %d\n", __func__,inode->i_ino, block);
	return 0;
}  



MODULE_LICENSE("GPL");
