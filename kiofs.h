#ifndef __KIOFS_H__
#define __KIOFS_H__

/* kiofs.h defines symbols to work in kernel space */

#include <linux/blkdev.h>
#include <linux/buffer_head.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/namei.h>
#include <linux/module.h>
#include <linux/parser.h>
#include <linux/random.h>
#include <linux/slab.h>
#include <linux/time.h>
#include <linux/version.h>

#include "iofs.h"

/* Declare operations to be hooked to VFS */

extern struct file_system_type iofs_fs_type;
extern const struct super_operations iofs_sb_ops;
extern const struct inode_operations iofs_inode_ops;
extern const struct file_operations iofs_dir_operations;
extern const struct file_operations iofs_file_operations;

struct dentry *iofs_mount(struct file_system_type *fs_type,
                              int flags, const char *dev_name,
                              void *data);
void iofs_kill_superblock(struct super_block *sb);

void iofs_destroy_inode(struct inode *inode);
void iofs_put_super(struct super_block *sb);

int iofs_create(struct inode *dir, struct dentry *dentry,
                    umode_t mode, bool excl);
struct dentry *iofs_lookup(struct inode *parent_inode,
                               struct dentry *child_dentry,
                               unsigned int flags);
int iofs_mkdir(struct inode *dir, struct dentry *dentry,
                   umode_t mode);

int iofs_iterate_shared(struct file *filp, struct dir_context *dc);  

ssize_t iofs_read(struct file * filp, char __user * buf, size_t len,
                      loff_t * ppos);
ssize_t iofs_write(struct file * filp, const char __user * buf, size_t len,
                       loff_t * ppos);

extern struct kmem_cache *iofs_inode_cache;

/* Helper functions */

// To translate VFS superblock to iofs superblock
static inline struct iofs_superblock *IOFS_SB(struct super_block *sb) {
    return sb->s_fs_info;
}
static inline struct iofs_inode *IOFS_INODE(struct inode *inode) {
    return inode->i_private;
}

static inline uint64_t IOFS_INODES_PER_BLOCK(struct super_block *sb) {
    struct iofs_superblock *iofs_sb;
    iofs_sb = IOFS_SB(sb);
    return IOFS_INODES_PER_BLOCK_HSB(iofs_sb);
}

// Given the inode_no, calcuate which block in inode table contains the corresponding inode
static inline uint64_t IOFS_INODE_BLOCK_OFFSET(struct super_block *sb, uint64_t inode_no) {
    struct iofs_superblock *iofs_sb;
    iofs_sb = IOFS_SB(sb);
    return inode_no / IOFS_INODES_PER_BLOCK_HSB(iofs_sb);
}
static inline uint64_t IOFS_INODE_BYTE_OFFSET(struct super_block *sb, uint64_t inode_no) {
    struct iofs_superblock *iofs_sb;
    iofs_sb = IOFS_SB(sb);
    return (inode_no % IOFS_INODES_PER_BLOCK_HSB(iofs_sb)) * sizeof(struct iofs_inode);
}

static inline uint64_t IOFS_DIR_MAX_RECORD(struct super_block *sb) {
    struct iofs_superblock *iofs_sb;
    iofs_sb = IOFS_SB(sb);
    return iofs_sb->blocksize / sizeof(struct iofs_dir_record);
}

// From which block does data blocks start
static inline uint64_t IOFS_DATA_BLOCK_TABLE_START_BLOCK_NO(struct super_block *sb) {
    struct iofs_superblock *iofs_sb;
    iofs_sb = IOFS_SB(sb);
    return IOFS_DATA_BLOCK_TABLE_START_BLOCK_NO_HSB(iofs_sb);
}

void iofs_save_sb(struct super_block *sb);

// functions to operate inode
void iofs_fill_inode(struct super_block *sb, struct inode *inode,
                        struct iofs_inode *iofs_inode, int ino);
int iofs_alloc_iofs_inode(struct super_block *sb, uint64_t *out_inode_no);
struct iofs_inode *iofs_get_iofs_inode(struct super_block *sb,
                                                uint64_t inode_no);
void iofs_save_iofs_inode(struct super_block *sb,
                                struct iofs_inode *inode, int ino);
int iofs_add_dir_record(struct super_block *sb, struct inode *dir,
                           struct dentry *dentry, struct inode *inode);
int iofs_alloc_data_block(struct super_block *sb, uint64_t *out_data_block_no);
int iofs_create_inode(struct inode *dir, struct dentry *dentry,
                         umode_t mode);

#endif /*__KIOFS_H__*/
