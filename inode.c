#include "kiofs.h"

void iofs_destroy_inode(struct inode *inode) {
    struct iofs_inode *iofs_inode = IOFS_INODE(inode);

    printk(KERN_INFO "Freeing private data of inode %p (%lu)\n",
           iofs_inode, inode->i_ino);
    kmem_cache_free(iofs_inode_cache, iofs_inode);
}

void iofs_fill_inode(struct super_block *sb, struct inode *inode,
                        struct iofs_inode *iofs_inode, int ino) {

    if (iofs_inode->origin == IOFS_DIRMARK) {
      inode->i_mode = 0040777; //octal
    }else{
      inode->i_mode = 0100777; //octal
    }
    inode->i_sb = sb;
    inode->i_ino = ino;
    inode->i_op = &iofs_inode_ops;
    inode->i_atime = inode->i_mtime 
                   = inode->i_ctime
                   = current_time(inode);
    inode->i_private = iofs_inode;    
    
    if (S_ISDIR(inode->i_mode)) {
        inode->i_fop = &iofs_dir_operations;
    } else if (S_ISREG(inode->i_mode)) {
        inode->i_fop = &iofs_file_operations;
    } else {
        printk(KERN_WARNING
               "Inode %lu is neither a directory nor a regular file",
               inode->i_ino);
        inode->i_fop = NULL;
    }
}

/* TODO I didn't implement any function to dealloc iofs_inode */
int iofs_alloc_iofs_inode(struct super_block *sb, uint64_t *out_inode_no) {
    struct iofs_superblock *iofs_sb;
    struct buffer_head *bh;
    uint64_t i;
    int ret;
    char *bitmap;
    char *slot;
    char needle;

    iofs_sb = IOFS_SB(sb);

    mutex_lock(&iofs_sb_lock);

    bh = sb_bread(sb, IOFS_INODE_BITMAP_BLOCK_NO);
    BUG_ON(!bh);

    bitmap = bh->b_data;
    ret = -ENOSPC;
    for (i = 0; i < iofs_sb->inode_table_size; i++) {
        slot = bitmap + i / BITS_IN_BYTE;
        needle = 1 << (i % BITS_IN_BYTE);
        if (0 == (*slot & needle)) {
            *out_inode_no = i;
            *slot |= needle;
            iofs_sb->inode_count += 1;
            ret = 0;
            break;
        }
    }

    mark_buffer_dirty(bh);
    sync_dirty_buffer(bh);
    brelse(bh);
    iofs_save_sb(sb);

    mutex_unlock(&iofs_sb_lock);
    return ret;
}

struct iofs_inode *iofs_get_iofs_inode(struct super_block *sb,
                                                uint64_t inode_no) {
    struct buffer_head *bh;
    struct iofs_inode *inode;
    struct iofs_inode *inode_buf;

    bh = sb_bread(sb, IOFS_INODE_TABLE_START_BLOCK_NO + IOFS_INODE_BLOCK_OFFSET(sb, inode_no));
    BUG_ON(!bh);
    
    inode = (struct iofs_inode *)(bh->b_data + IOFS_INODE_BYTE_OFFSET(sb, inode_no));
    inode_buf = kmem_cache_alloc(iofs_inode_cache, GFP_KERNEL);
    memcpy(inode_buf, inode, sizeof(*inode_buf));

    brelse(bh);
    return inode_buf;
}

void iofs_save_iofs_inode(struct super_block *sb,
                                struct iofs_inode *inode_buf, int ino) {
    struct buffer_head *bh;
    struct iofs_inode *inode;
    uint64_t inode_no;

    inode_no = ino;
    bh = sb_bread(sb, IOFS_INODE_TABLE_START_BLOCK_NO + IOFS_INODE_BLOCK_OFFSET(sb, inode_no));
    BUG_ON(!bh);

    inode = (struct iofs_inode *)(bh->b_data + IOFS_INODE_BYTE_OFFSET(sb, inode_no));
    memcpy(inode, inode_buf, sizeof(*inode));

    mark_buffer_dirty(bh);
    sync_dirty_buffer(bh);
    brelse(bh);
}

int iofs_add_dir_record(struct super_block *sb, struct inode *dir,
                           struct dentry *dentry, struct inode *inode) {
/*
    struct buffer_head *bh;
    struct iofs_inode *parent_iofs_inode;
    struct iofs_dir_record *dir_record;

    parent_iofs_inode = IOFS_INODE(dir);
    if (unlikely(parent_iofs_inode->dirb.m
            >= IOFS_DIR_MAX_RECORD(sb))) {
        return -ENOSPC;
    }

    bh = sb_bread(sb, parent_iofs_inode->data_block_no);
    BUG_ON(!bh);

    dir_record = (struct iofs_dir_record *)bh->b_data;
    dir_record += parent_iofs_inode->dirb.m;
    dir_record->inode_no = inode->i_ino;
    strcpy(dir_record->filename, dentry->d_name.name);

    mark_buffer_dirty(bh);
    sync_dirty_buffer(bh);
    brelse(bh);

    parent_iofs_inode->dir_children_count += 1;
    iofs_save_iofs_inode(sb, parent_iofs_inode, inode->i_ino);
*/
    return 0;
}

int iofs_alloc_data_block(struct super_block *sb, uint64_t *out_data_block_no) {
    struct iofs_superblock *iofs_sb;
    struct buffer_head *bh;
    uint64_t i;
    int ret;
    char *bitmap;
    char *slot;
    char needle;

    iofs_sb = IOFS_SB(sb);

    mutex_lock(&iofs_sb_lock);

    bh = sb_bread(sb, IOFS_DATA_BLOCK_BITMAP_BLOCK_NO);
    BUG_ON(!bh);

    bitmap = bh->b_data;
    ret = -ENOSPC;
    for (i = 0; i < iofs_sb->data_block_table_size; i++) {
        slot = bitmap + i / BITS_IN_BYTE;
        needle = 1 << (i % BITS_IN_BYTE);
        if (0 == (*slot & needle)) {
            *out_data_block_no
                = IOFS_DATA_BLOCK_TABLE_START_BLOCK_NO(sb) + i;
            *slot |= needle;
            iofs_sb->data_block_count += 1;
            ret = 0;
            break;
        }
    }

    mark_buffer_dirty(bh);
    sync_dirty_buffer(bh);
    brelse(bh);
    iofs_save_sb(sb);

    mutex_unlock(&iofs_sb_lock);
    return ret;
}

int iofs_create_inode(struct inode *dir, struct dentry *dentry,
                         umode_t mode) {
/*

    struct super_block *sb;
    struct iofs_superblock *iofs_sb;
    uint64_t inode_no;
    struct iofs_inode *iofs_inode;
    struct inode *inode;
    int ret;

    sb = dir->i_sb;
    iofs_sb = IOFS_SB(sb);

    // Create iofs_inode 
    ret = iofs_alloc_iofs_inode(sb, &inode_no);
    if (0 != ret) {
        printk(KERN_ERR "Unable to allocate on-disk inode. "
                        "Is inode table full? "
                        "Inode count: %llu\n",
                        iofs_sb->inode_count);
        return -ENOSPC;
    }
    iofs_inode = kmem_cache_alloc(iofs_inode_cache, GFP_KERNEL);
    iofs_inode->inode_no = inode_no;
    iofs_inode->mode = mode;
    if (S_ISDIR(mode)) {
        iofs_inode->dir_children_count = 0;
    } else if (S_ISREG(mode)) {
        iofs_inode->file_size = 0;
    } else {
        printk(KERN_WARNING
               "Inode %llu is neither a directory nor a regular file",
               inode_no);
    }

    // Allocate data block for the new iofs_inode 
    ret = iofs_alloc_data_block(sb, &iofs_inode->data_block_no);
    if (0 != ret) {
        printk(KERN_ERR "Unable to allocate on-disk data block. "
                        "Is data block table full? "
                        "Data block count: %llu\n",
                        iofs_sb->data_block_count);
        return -ENOSPC;
    }

    // Create VFS inode 
    inode = new_inode(sb);
    if (!inode) {
        return -ENOMEM;
    }
    iofs_fill_inode(sb, inode, iofs_inode);

    // Add new inode to parent dir 
    ret = iofs_add_dir_record(sb, dir, dentry, inode);
    if (0 != ret) {
        printk(KERN_ERR "Failed to add inode %lu to parent dir %lu\n",
               inode->i_ino, dir->i_ino);
        return -ENOSPC;
    }

    inode_init_owner(inode, dir, mode);
    d_add(dentry, inode);

    // TODO we should free newly allocated inodes when error occurs 
*/
    return 0;
}

int iofs_create(struct inode *dir, struct dentry *dentry,
                   umode_t mode, bool excl) {
    return iofs_create_inode(dir, dentry, mode);
}

int iofs_mkdir(struct inode *dir, struct dentry *dentry,
                  umode_t mode) {
    /* @Sankar: The mkdir callback does not have S_IFDIR set.
       Even ext2 sets it explicitly. Perhaps this is a bug */
    mode |= S_IFDIR;
    return iofs_create_inode(dir, dentry, mode);
}

struct dentry *iofs_lookup(struct inode *dir,
                              struct dentry *child_dentry,
                              unsigned int flags) {
    struct iofs_inode *parent_iofs_inode = IOFS_INODE(dir);
    struct super_block *sb = dir->i_sb;
    struct buffer_head *bh;
    struct iofs_dir_record *dir_record;
    struct iofs_inode *iofs_child_inode;
    struct inode *child_inode;
    uint64_t i;

    bh = sb_bread(sb, parent_iofs_inode->data_block_no);
    BUG_ON(!bh);

    dir_record = (struct iofs_dir_record *)bh->b_data;

    for (i = 0; i < parent_iofs_inode->dir_children_count; i++) {
        printk(KERN_INFO "iofs_lookup: i=%llu, dir_record->filename=%s, child_dentry->d_name.name=%s", i, dir_record->filename, child_dentry->d_name.name);    // TODO
        if (0 == strcmp(dir_record->filename, child_dentry->d_name.name)) {
            iofs_child_inode = iofs_get_iofs_inode(sb, dir_record->inode_no);
            child_inode = new_inode(sb);
            if (!child_inode) {
                printk(KERN_ERR "Cannot create new inode. No memory.\n");
                return NULL; 
            }
            iofs_fill_inode(sb, child_inode, iofs_child_inode);
            inode_init_owner(child_inode, dir, iofs_child_inode->mode);
            d_add(child_dentry, child_inode);
            return NULL;    
        }
        dir_record++;
    }

    printk(KERN_ERR
           "No inode found for the filename: %s\n",
           child_dentry->d_name.name);
    return NULL;
}
