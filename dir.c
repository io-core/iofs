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

static uint32_t setbit( uint32_t n, int b){
	uint32_t ret = n;
        if (b < 32) {
	   ret = ret | (1 << b);
        }
	return ret;
}


void markfile (struct inode *inode, uint32_t ino)
{
        struct iofs_dinode * finode;
        struct buffer_head *bh;
        struct iofs_bm  *bm;
	struct iofs_ep	*ep;
	int i;
	uint32_t iino, sec[IOFS_SECTABSIZE], ext[IOFS_EXTABSIZE];
	

        bm = inode->i_sb->s_fs_info;



        bh = sb_bread(inode->i_sb, (ino/29)-1);
        finode = (struct iofs_dinode *)bh->b_data;
        memcpy(sec,finode->fhb.sec,IOFS_SECTABSIZE);
        memcpy(ext,finode->fhb.ext,IOFS_EXTABSIZE);
        brelse(bh);

	for(i=0;i<finode->fhb.aleng;i++){
	   if (i < IOFS_SECTABSIZE){
		iino=sec[i];
	        bm->s[(iino/29)/32]=setbit(bm->s[(iino/29)/32],bm->s[(iino/29)%32]);
	   }else{
                bh = sb_bread(inode->i_sb, (ext[(i-IOFS_SECTABSIZE)/256]/29)-1);
		ep = (struct iofs_ep *)bh->b_data;
		if ((i-IOFS_SECTABSIZE)%256 < IOFS_EXTABSIZE){
//		    iino=ep->x[(i-IOFS_SECTABSIZE)%256];
//		    if (iino < 65536*29){
//                      bm->s[(iino/29)/32]=setbit(bm->s[(iino/29)/32],bm->s[(iino/29)%32]);
//		    }else{
//		    }
		}
                brelse(bh);
	     
	   }
        } 
}


int do_iofs_readdir(struct inode *file, uint64_t ino, struct dir_context *ctx, int start, bool mark)
{
        struct inode *finode = file; //file_inode(file);
	struct buffer_head *bh;

	int			slot, here, namelen;
	char			*nameptr;
	struct iofs_dinode	*dinode;
        struct iofs_dinode      dinode_buf;
	struct iofs_de		*dirslot;
	struct iofs_bm		*bm;
	
	here = start;

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

	bm = finode->i_sb->s_fs_info;
	if (mark) {
		bm->s[(ino/29)/32]=setbit(bm->s[(ino/29)/32],bm->s[(ino/29)%32]);
	}

//	if ((!mark) && (here==0)){
//		if (ctx->pos==0){
//		  dir_emit_dot(file, ctx);
//                  ctx->pos++;
//                }
//		here++;
//	}

        if (dinode->dirb.p0 != 0){
                here = do_iofs_readdir( file, dinode->dirb.p0, ctx, here, mark );
        }


	for (slot = 0; slot < dinode->dirb.m && slot < 24; slot++) {
		dirslot  = &dinode->dirb.e[slot];
		namelen  = strnlen(dirslot->name,24);
		nameptr  = dirslot->name;
		if((!mark) && (here >= ctx->pos)) {
                  ctx->pos++;		  
                  if (!dir_emit(ctx, nameptr, namelen, dirslot->adr, DT_UNKNOWN)) {
                        return here;
		  }
                  if (dirslot->p != 0){
                        here = do_iofs_readdir( file, dirslot->p, ctx, here, mark );
                  }
		}else{

		  markfile(file,dirslot->adr);         

                  if (dirslot->p != 0){
                        here = do_iofs_readdir( file, dirslot->p, ctx, here, mark );
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
    ret = do_iofs_readdir( file_inode(file), inode->i_ino, ctx, 0, false ); 
    ctx->pos = INT_MAX;
    return 0;
}
