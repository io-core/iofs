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
//        .create         = iofs_create,
        .lookup         = iofs_lookup,
//        .link           = iofs_link,
//        .unlink         = iofs_unlink,
//        .symlink        = iofs_symlink,
//        .mkdir          = iofs_mkdir,
//        .rmdir          = iofs_rmdir,
//        .mknod          = iofs_mknod,
//        .rename         = iofs_rename,
//        .getattr        = iofs_getattr,
//        .tmpfile        = iofs_tmpfile,
};


void markfile (struct inode *inode, uint32_t ino,struct iofs_bm *bm, bool state)
{
        struct iofs_dinode * finode;
        struct buffer_head *bh;
      
	struct iofs_ep	*ep;
	int i,ii;
	uint32_t iino, sec[IOFS_SECTABSIZE], ext[IOFS_EXTABSIZE];
	

        bh = sb_bread(inode->i_sb, (ino/29)-1);
        finode = (struct iofs_dinode *)bh->b_data;
	for(i=0;i<IOFS_SECTABSIZE;i++){ sec[i]=finode->fhb.sec[i];}
	if (finode->fhb.aleng > IOFS_SECTABSIZE){
          for(i=0;i<IOFS_EXTABSIZE;i++){ ext[i]=finode->fhb.ext[i];}
	}
        brelse(bh);

	for(i=0;i<finode->fhb.aleng;i++){
	   if (i < IOFS_SECTABSIZE){
		iino=sec[i]/29;
		if (bm!=0){
		  if (state){
	            BITSET(bm->s[iino/32],iino%32);
		  }else{
                    BITCLEAR(bm->s[iino/32],iino%32);
		  }
		}
	   }else{
		 ii=i-IOFS_SECTABSIZE;
		 if (ii/256 < IOFS_EXTABSIZE){

                    bh = sb_bread(inode->i_sb, (ext[ii/256]/29)-1);
		    ep = (struct iofs_ep *)bh->b_data;
		
		    iino=ep->x[ii%256]/29;
		    if (iino < 65536){
		      iino = sec[0]/29;
			if (bm!=0){
                          if (state){
                            BITSET(bm->s[iino/32],iino%32);
			  }else{
                            BITCLEAR(bm->s[iino/32],iino%32);
			  }
			}
		    }
                    brelse(bh);
                }	     
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
        bm = SUPER_INFO(finode->i_sb);

	if (mark) {
		BITSET(bm->s[(ino/29)/32],(ino/29)%32);
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

		  markfile(file,dirslot->adr,bm,true);         

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
