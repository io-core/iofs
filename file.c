#include "kiofs.h"

ssize_t iofs_read(struct file *filp, char __user *buf, size_t len,
                     loff_t *ppos) {
/*
    struct super_block *sb;
    struct inode *inode;
    struct iofs_inode *iofs_inode;
    struct buffer_head *bh;
    char *buffer;
    int nbytes;

    inode = filp->f_path.dentry->d_inode;
    sb = inode->i_sb;
    iofs_inode = IOFS_INODE(inode);
    
    if (*ppos >= iofs_inode->file_size) {
        return 0;
    }

    bh = sb_bread(sb, iofs_inode->data_block_no);
    if (!bh) {
        printk(KERN_ERR "Failed to read data block %llu\n",
               iofs_inode->data_block_no);
        return 0;
    }

    buffer = (char *)bh->b_data + *ppos;
    nbytes = min((size_t)(iofs_inode->file_size - *ppos), len);

    if (copy_to_user(buf, buffer, nbytes)) {
        brelse(bh);
        printk(KERN_ERR
               "Error copying file content to userspace buffer\n");
        return -EFAULT;
    }

    brelse(bh);
    *ppos += nbytes;
    return nbytes;
*/
    return 0;
}

/* TODO We didn't use address_space/pagecache here.
   If we hook file_operations.write = do_sync_write,
   and file_operations.aio_write = generic_file_aio_write,
   we will use write to pagecache instead. */
ssize_t iofs_write(struct file *filp, const char __user *buf, size_t len,
                      loff_t *ppos) {

/*
    struct super_block *sb;
    struct inode *inode;
    struct iofs_inode *iofs_inode;
    struct buffer_head *bh;
    struct iofs_superblock *iofs_sb;
    char *buffer;
//    int ret;

    inode = filp->f_path.dentry->d_inode;
    sb = inode->i_sb;
    iofs_inode = IOFS_INODE(inode);
    iofs_sb = IOFS_SB(sb);

//    ret = generic_write_checks(filp, ppos, &len, 0);
//    if (ret) {
//        return ret;
//    }

    bh = sb_bread(sb, iofs_inode->data_block_no);
    if (!bh) {
        printk(KERN_ERR "Failed to read data block %llu\n",
               iofs_inode->data_block_no);
        return 0;
    }

    buffer = (char *)bh->b_data + *ppos;
    if (copy_from_user(buffer, buf, len)) {
        brelse(bh);
        printk(KERN_ERR
               "Error copying file content from userspace buffer "
               "to kernel space\n");
        return -EFAULT;
    }
    *ppos += len;

    mark_buffer_dirty(bh);
    sync_dirty_buffer(bh);
    brelse(bh);

    iofs_inode->file_size = max((size_t)(iofs_inode->file_size),
                                   (size_t)(*ppos));
    iofs_save_iofs_inode(sb, iofs_inode);

    // TODO We didn't update file size here. To be frank I don't know how. 

    return len;
 */
    return 0;
}
