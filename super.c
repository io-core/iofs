// SPDX-License-Identifier: GPL-2.0
/*
 * super.c
 *
 * Copyright (c) 2020 Charles Perkins
 *
 * Portions derived from work (c) 1999 Al Smith
 * Portions derived from work (c) 1995,1996 Christian Vogelgsang
 */

#include <linux/init.h>
#include <linux/module.h>
#include <linux/exportfs.h>
#include <linux/slab.h>
#include <linux/buffer_head.h>
#include <linux/vfs.h>

#include "iofs.h"

static int iofs_statfs(struct dentry *dentry, struct kstatfs *buf);
static int iofs_fill_super(struct super_block *s, void *d, int silent);

static struct dentry *iofs_mount(struct file_system_type *fs_type,
	int flags, const char *dev_name, void *data)
{
	return mount_bdev(fs_type, flags, dev_name, data, iofs_fill_super);
}

static void iofs_kill_sb(struct super_block *s)
{
	struct iofs_bm *sbm = SUPER_INFO(s);
	kill_block_super(s);
	kfree(sbm);
}

static struct file_system_type iofs_fs_type = {
	.owner		= THIS_MODULE,
	.name		= "iofs",
	.mount		= iofs_mount,
	.kill_sb	= iofs_kill_sb,
	.fs_flags	= FS_REQUIRES_DEV,
};

MODULE_ALIAS_FS("iofs");

static struct kmem_cache * iofs_inode_cachep;

static struct inode *iofs_alloc_inode(struct super_block *sb)
{
	struct iofs_inode_info *ei;
	ei = kmem_cache_alloc(iofs_inode_cachep, GFP_KERNEL);
	if (!ei)
		return NULL;
	return &ei->vfs_inode;
}

static void iofs_i_callback(struct rcu_head *head)
{
        struct inode *inode = container_of(head, struct inode, i_rcu);
        kmem_cache_free(iofs_inode_cachep, INODE_INFO(inode));
}

void iofs_destroy_inode(struct inode *inode)
{
	call_rcu(&inode->i_rcu, iofs_i_callback);
}

static void init_once(void *foo)
{
	struct iofs_inode_info *ei = (struct iofs_inode_info *) foo;

	inode_init_once(&ei->vfs_inode);
}

static int __init init_inodecache(void)
{
	iofs_inode_cachep = kmem_cache_create("iofs_inode_cache",
				sizeof(struct iofs_inode_info), 0,
				SLAB_RECLAIM_ACCOUNT|SLAB_MEM_SPREAD|
				SLAB_ACCOUNT, init_once);
	if (iofs_inode_cachep == NULL)
		return -ENOMEM;
	return 0;
}

static void destroy_inodecache(void)
{
	/*
	 * Make sure all delayed rcu free inodes are flushed before we
	 * destroy cache.
	 */
	rcu_barrier();
	kmem_cache_destroy(iofs_inode_cachep);
}

static int iofs_remount(struct super_block *sb, int *flags, char *data)
{
	sync_filesystem(sb);
	*flags |= SB_RDONLY;
	return 0;
}

static const struct super_operations iofs_superblock_operations = {
	.alloc_inode	= iofs_alloc_inode,
	.destroy_inode  = iofs_destroy_inode,
	.statfs		= iofs_statfs,
	.remount_fs	= iofs_remount,
};

static const struct export_operations iofs_export_ops = {
	.fh_to_dentry	= iofs_fh_to_dentry,
	.fh_to_parent	= iofs_fh_to_parent,
	.get_parent	= iofs_get_parent,
};

static int __init init_iofs_fs(void) {
	int err;
	pr_info(IOFS_VERSION);
	err = init_inodecache();
	if (err)
		goto out1;
	err = register_filesystem(&iofs_fs_type);
	if (err)
		goto out;
	return 0;
out:
	destroy_inodecache();
out1:
	return err;
}

static void __exit exit_iofs_fs(void) {
	unregister_filesystem(&iofs_fs_type);
	destroy_inodecache();
}

module_init(init_iofs_fs)
module_exit(exit_iofs_fs)


static int iofs_validate_super(struct iofs_bm *sbm, struct iofs_dinode *super) {


	if (le32_to_cpu(super->origin) != IOFS_DIRMARK )
		return -1;
/*
	sb->origin     = le32_to_cpu(super->origin);

	sb->total_blocks = be32_to_cpu(super->fs_size);
	sb->first_block  = be32_to_cpu(super->fs_firstcg);
	sb->group_size   = be32_to_cpu(super->fs_cgfsize);
	sb->data_free    = be32_to_cpu(super->fs_tfree);
	sb->inode_free   = be32_to_cpu(super->fs_tinode);
	sb->inode_blocks = be16_to_cpu(super->fs_cgisize);
	sb->total_groups = be16_to_cpu(super->fs_ncg);
*/    
	return 0;    
}

static int iofs_fill_super(struct super_block *s, void *d, int silent)
{
	struct iofs_bm *sbm;
//	struct buffer_head *bh;
	struct page *page;
	struct inode *root;
	void *data;
        int ret;

 	sbm = kzalloc(sizeof(struct iofs_bm), GFP_KERNEL);
	if (!sbm)
		return -ENOMEM;
	sbm->s[0]=~(uint32_t)0;
        sbm->s[1]=~(uint32_t)0;

	s->s_fs_info = sbm;
 
	s->s_magic		= IOFS_DIRMARK;
	if (!sb_set_blocksize(s, IOFS_BLOCKSIZE)) {
		pr_err("device does not support %d byte blocks\n",
			IOFS_BLOCKSIZE);
		return -EINVAL;
	}

        //sb->fs_start = 1;

//	bh = sb_bread(s, 0);
//	if (!bh) {
	page = read_mapping_page(s->s_bdev->bd_inode->i_mapping, 0, NULL);
	if (!page) {		pr_err("cannot read superblock\n");
		return -EIO;
	}
		
	data = kmap_atomic(page);
//	dsb = (struct erofs_super_block *)(data + EROFS_SUPER_OFFSET);

	if (iofs_validate_super(sbm, (struct iofs_dinode *) data )) {  //bh->b_data)) {
#ifdef DEBUG
		pr_warn("invalid superblock at block %u\n",
			0 );
#endif
//		brelse(bh);
		kunmap_atomic(data);
		put_page(page);
		return -EINVAL;
	}
//	brelse(bh);
	kunmap_atomic(data);
	put_page(page);

	s->s_op   = &iofs_superblock_operations;
	s->s_export_op = &iofs_export_ops;
	root = iofs_iget(s, IOFS_ROOTINODE);
	if (IS_ERR(root)) {
		pr_err("get root inode (%d) failed\n",IOFS_ROOTINODE);
		return PTR_ERR(root);
	}

	s->s_root = d_make_root(root);
	if (!(s->s_root)) {
		pr_err("get root dentry failed\n");
		return -ENOMEM;
	}

        ret = do_iofs_readdir( root, 29, 0, 0, true );


	return 0;
}

static int iofs_statfs(struct dentry *dentry, struct kstatfs *buf) {
	struct super_block *sb = dentry->d_sb;
	struct iofs_bm *bm = SUPER_INFO(sb);
	u64 id = huge_encode_dev(sb->s_bdev->bd_dev);
	int i,a = 0;

	buf->f_type    = IOFS_DIRMARK;	/* iofs magic number */
	buf->f_bsize   = IOFS_BLOCKSIZE;		/* blocksize */
	buf->f_blocks  = 65536;
	for(i=0;i<2048;i++){ a += hweight_long(bm->s[i]);}

	buf->f_bfree   = 65536-a;	/* free data blocks */
	buf->f_bavail  = 65536-a;	/* free blocks for non-root */
//	buf->f_files   = sbi->total_groups *	/* total inodes */
//			sbi->inode_blocks *
//			(IOFS_BLOCKSIZE / sizeof(struct iofs_dinode));
//	buf->f_ffree   = sbi->inode_free;	/* free inodes */
	buf->f_fsid.val[0] = (u32)id;
	buf->f_fsid.val[1] = (u32)(id >> 32);
	buf->f_namelen = IOFS_MAXNAMELEN;	/* max filename length */

	return 0;
}

