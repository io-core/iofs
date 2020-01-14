#include "kiofs.h"
#include <linux/vfs.h>



int iofs_read_dirpage(struct super_block *sb, uint32_t ino, int depth) {
        struct iofs_inode *inode;
        struct iofs_dpblock *dirpage;
        struct buffer_head *bh;
        int i;
        
       
        uint32_t down;
	int ret = 0;

        printk(KERN_INFO "examining directory page:%u (%d)", ino/29, depth);

        
        

        bh = sb_bread(sb, (ino/29) - 1);
        BUG_ON(!bh);


        inode = (struct iofs_inode *)bh->b_data;
        dirpage = &(inode->dirb);
       


  if (depth < 2){


        if (inode->origin == IOFS_DIRMARK) {
          printk(KERN_INFO "readdir directory page:%u (%d)", ino/29, depth);
          for( i=0; i <= dirpage->m; i++){
             down = 0;
             if (i == 0) {
               down = dirpage->p0;
             }else{
               down = dirpage->e[i-1].p;
               dirpage->e[i-1].name[31]=0;
               printk(KERN_INFO "        %8d:%8u:%8u %s", i, dirpage->e[i-1].adr/29, dirpage->e[i-1].p/29, dirpage->e[i-1].name);
             }
             if ( down != 0 && (down % 29 == 0)) {
//               ret = iofs_read_dirpage(sb, down, depth + 1);
             }
          }

        }else if (inode->origin == IOFS_HEADERMARK) {

          printk(KERN_INFO "found IOFS_HEADERMARK on sector #%u (%d)", ino/29, depth);

	}else{

          printk(KERN_INFO "no IOFS_DIRMARK or IOFS_HEADERMARK on sector #%u (%d)", ino/29, depth);



	}

   }
        brelse(bh);
	return ret;
}

int iofs_iterate_shared(struct file *filp, struct dir_context *dc) { 
	struct inode *inode;

	struct super_block *sb;
	int i;

	mutex_lock(&iofs_d_lock);
	inode = filp->f_path.dentry->d_inode;
	sb = inode->i_sb;

	i = iofs_read_dirpage(sb,inode->i_ino*29,1);

	mutex_unlock(&iofs_d_lock);
	return 0;

}

int iofs_statfs(struct dentry *dentry, struct kstatfs *buf) {
	struct super_block *sb = dentry->d_sb;
	u64 id = 0;

	if (sb->s_bdev)
		id = huge_encode_dev(sb->s_bdev->bd_dev);
	else if (sb->s_dev)
		id = huge_encode_dev(sb->s_dev);


	buf->f_type = IOFS_MAGIC;
	buf->f_bsize = IOFS_DEFAULT_BLOCKSIZE;
	buf->f_blocks = 0; //CRAMFS_SB(sb)->blocks;
	buf->f_bfree = 0;
	buf->f_bavail = 0;
	buf->f_files = 0;
	buf->f_ffree = 0;
	buf->f_fsid.val[0] = (u32)id;
	buf->f_fsid.val[1] = (u32)(id >> 32);
	buf->f_namelen = IOFS_FILENAME_MAXLEN;

	return 0;
}

int iofs_readdir(struct file *filp, void *dirent, filldir_t filldir) {
/*
    loff_t pos;
    struct inode *inode;
    struct super_block *sb;
    struct buffer_head *bh;
    struct iofs_inode *iofs_inode;
    struct iofs_dir_record *dir_record;
    uint64_t i;

    pos = filp->f_pos;
    inode = filp->f_path.dentry->d_inode;
    sb = inode->i_sb;
    iofs_inode = IOFS_INODE(inode);

    if (pos) {
        // TODO @Sankar: we use a hack of reading pos to figure if we have filled in data.
        return 0;
    }

    printk(KERN_INFO "readdir: iofs_inode->inode_no=%u", iofs_inode->origin & 0x0FFFFFFF);

    if (unlikely(!S_ISDIR(inode->i_mode))) {
        printk(KERN_ERR
               "Inode %llu of dentry %s is not a directory\n",
               iofs_inode->origin & 0x0FFFFFFF,
               filp->f_path.dentry->d_name.name);
        return -ENOTDIR;
    }

    bh = sb_bread(sb, iofs_inode->data_block_no);
    BUG_ON(!bh);

    dir_record = (struct iofs_dir_record *)bh->b_data;
    for (i = 0; i < iofs_inode->dir_children_count; i++) {
        filldir(dirent, dir_record->filename, IOFS_FILENAME_MAXLEN, pos,
                dir_record->inode_no, DT_UNKNOWN);
        filp->f_pos += sizeof(struct iofs_dir_record);
        pos += sizeof(struct iofs_dir_record);
        dir_record++;
    }
    brelse(bh);
*/
    return 0;
}
