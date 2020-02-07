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
#include "iofs_vh.h"
#include "iofs_fs_sb.h"

static int iofs_statfs(struct dentry *dentry, struct kstatfs *buf);
static int iofs_fill_super(struct super_block *s, void *d, int silent);

static struct dentry *iofs_mount(struct file_system_type *fs_type,
	int flags, const char *dev_name, void *data)
{
	return mount_bdev(fs_type, flags, dev_name, data, iofs_fill_super);
}

static void iofs_kill_sb(struct super_block *s)
{
	struct iofs_sb_info *sbi = SUPER_INFO(s);
	kill_block_super(s);
	kfree(sbi);
}

static struct file_system_type iofs_fs_type = {
	.owner		= THIS_MODULE,
	.name		= "iofs",
	.mount		= iofs_mount,
	.kill_sb	= iofs_kill_sb,
	.fs_flags	= FS_REQUIRES_DEV,
};
MODULE_ALIAS_FS("efs");

static struct pt_types sgi_pt_types[] = {
	{0x00,		"SGI vh"},
	{0x01,		"SGI trkrepl"},
	{0x02,		"SGI secrepl"},
	{0x03,		"SGI raw"},
	{0x04,		"SGI bsd"},
	{SGI_SYSV,	"SGI sysv"},
	{0x06,		"SGI vol"},
	{SGI_EFS,	"SGI efs"},
	{0x08,		"SGI lv"},
	{0x09,		"SGI rlv"},
	{0x0A,		"SGI xfs"},
	{0x0B,		"SGI xfslog"},
	{0x0C,		"SGI xlv"},
	{0x82,		"Linux swap"},
	{0x83,		"Linux native"},
	{0,		NULL}
};


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

static iofs_block_t iofs_validate_vh(struct volume_header *vh) {
	int		i;
	__be32		cs, *ui;
	int		csum;
	iofs_block_t	sblock = 0; /* shuts up gcc */
	struct pt_types	*pt_entry;
	int		pt_type, slice = -1;

	if (be32_to_cpu(vh->vh_magic) != VHMAGIC) {
		/*
		 * assume that we're dealing with a partition and allow
		 * read_super() to try and detect a valid superblock
		 * on the next block.
		 */
		return 0;
	}

	ui = ((__be32 *) (vh + 1)) - 1;
	for(csum = 0; ui >= ((__be32 *) vh);) {
		cs = *ui--;
		csum += be32_to_cpu(cs);
	}
	if (csum) {
		pr_warn("SGI disklabel: checksum bad, label corrupted\n");
		return 0;
	}

#ifdef DEBUG
	pr_debug("bf: \"%16s\"\n", vh->vh_bootfile);

	for(i = 0; i < NVDIR; i++) {
		int	j;
		char	name[VDNAMESIZE+1];

		for(j = 0; j < VDNAMESIZE; j++) {
			name[j] = vh->vh_vd[i].vd_name[j];
		}
		name[j] = (char) 0;

		if (name[0]) {
			pr_debug("vh: %8s block: 0x%08x size: 0x%08x\n",
				name, (int) be32_to_cpu(vh->vh_vd[i].vd_lbn),
				(int) be32_to_cpu(vh->vh_vd[i].vd_nbytes));
		}
	}
#endif

	for(i = 0; i < NPARTAB; i++) {
		pt_type = (int) be32_to_cpu(vh->vh_pt[i].pt_type);
		for(pt_entry = sgi_pt_types; pt_entry->pt_name; pt_entry++) {
			if (pt_type == pt_entry->pt_type) break;
		}
#ifdef DEBUG
		if (be32_to_cpu(vh->vh_pt[i].pt_nblks)) {
			pr_debug("pt %2d: start: %08d size: %08d type: 0x%02x (%s)\n",
				 i, (int)be32_to_cpu(vh->vh_pt[i].pt_firstlbn),
				 (int)be32_to_cpu(vh->vh_pt[i].pt_nblks),
				 pt_type, (pt_entry->pt_name) ?
				 pt_entry->pt_name : "unknown");
		}
#endif
		if (IS_IOFS(pt_type)) {
			sblock = be32_to_cpu(vh->vh_pt[i].pt_firstlbn);
			slice = i;
		}
	}

	if (slice == -1) {
		pr_notice("partition table contained no EFS partitions\n");
#ifdef DEBUG
	} else {
		pr_info("using slice %d (type %s, offset 0x%x)\n", slice,
			(pt_entry->pt_name) ? pt_entry->pt_name : "unknown",
			sblock);
#endif
	}
	return sblock;
}

static int iofs_validate_super(struct iofs_sb_info *sb, struct iofs_super *super) {


	if (!IS_IOFS_MAGIC(le32_to_cpu(super->fs_magic)))
		return -1;
	sb->fs_magic     = le32_to_cpu(super->fs_magic);
/*
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
	struct iofs_sb_info *sb;
	struct buffer_head *bh;
	struct inode *root;

 	sb = kzalloc(sizeof(struct iofs_sb_info), GFP_KERNEL);
	if (!sb)
		return -ENOMEM;
	s->s_fs_info = sb;
 
	s->s_magic		= IOFS_SUPER_MAGIC;
	if (!sb_set_blocksize(s, IOFS_BLOCKSIZE)) {
		pr_err("device does not support %d byte blocks\n",
			IOFS_BLOCKSIZE);
		return -EINVAL;
	}

        sb->fs_start = 1;

	bh = sb_bread(s, 0);
	if (!bh) {
		pr_err("cannot read superblock\n");
		return -EIO;
	}
		
	if (iofs_validate_super(sb, (struct iofs_super *) bh->b_data)) {
#ifdef DEBUG
		pr_warn("invalid superblock at block %u\n",
			sb->fs_start );
#endif
		brelse(bh);
		return -EINVAL;
	}
	brelse(bh);

	if (!sb_rdonly(s)) {
#ifdef DEBUG
		pr_info("forcing read-only mode\n");
#endif
		s->s_flags |= SB_RDONLY;
	}

	s->s_op   = &iofs_superblock_operations;
	s->s_export_op = &iofs_export_ops;
	root = iofs_iget(s, IOFS_ROOTINODE);
	if (IS_ERR(root)) {
		pr_err("get root inode failed\n");
		return PTR_ERR(root);
	}

	s->s_root = d_make_root(root);
	if (!(s->s_root)) {
		pr_err("get root dentry failed\n");
		return -ENOMEM;
	}

	return 0;
}

static int iofs_statfs(struct dentry *dentry, struct kstatfs *buf) {
	struct super_block *sb = dentry->d_sb;
//	struct iofs_sb_info *sbi = SUPER_INFO(sb);
	u64 id = huge_encode_dev(sb->s_bdev->bd_dev);

	buf->f_type    = IOFS_SUPER_MAGIC;	/* efs magic number */
	buf->f_bsize   = IOFS_BLOCKSIZE;		/* blocksize */
//	buf->f_blocks  = sbi->total_groups *	/* total data blocks */
//			(sbi->group_size - sbi->inode_blocks);
//	buf->f_bfree   = sbi->data_free;	/* free data blocks */
//	buf->f_bavail  = sbi->data_free;	/* free blocks for non-root */
//	buf->f_files   = sbi->total_groups *	/* total inodes */
//			sbi->inode_blocks *
//			(IOFS_BLOCKSIZE / sizeof(struct iofs_dinode));
//	buf->f_ffree   = sbi->inode_free;	/* free inodes */
	buf->f_fsid.val[0] = (u32)id;
	buf->f_fsid.val[1] = (u32)(id >> 32);
	buf->f_namelen = IOFS_MAXNAMELEN;	/* max filename length */

	return 0;
}

