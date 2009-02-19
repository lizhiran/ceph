#include "ioctl.h"
#include "super.h"
#include "ceph_debug.h"

int ceph_debug_ioctl = -1;
#define DOUT_MASK DOUT_MASK_IOCTL
#define DOUT_VAR ceph_debug_ioctl


/*
 * ioctls
 */

static long ceph_ioctl_get_layout(struct file *file, void __user *arg)
{
	struct ceph_inode_info *ci = ceph_inode(file->f_dentry->d_inode);
	int err;

	err = ceph_do_getattr(file->f_dentry, CEPH_STAT_CAP_LAYOUT);
	if (!err) {
		if (copy_to_user(arg, &ci->i_layout, sizeof(ci->i_layout)))
			return -EFAULT;
	}

	return err;
}

static long ceph_ioctl_set_layout(struct file *file, void __user *arg)
{
	struct inode *inode = file->f_dentry->d_inode;
	struct inode *parent_inode = file->f_dentry->d_parent->d_inode;
	struct ceph_mds_client *mdsc = &ceph_sb_to_client(inode->i_sb)->mdsc;
	struct ceph_mds_request *req;
	struct ceph_file_layout layout;
	int err;

	/* copy and validate */
	if (copy_from_user(&layout, arg, sizeof(layout)))
		return -EFAULT;

	req = ceph_mdsc_create_request(mdsc, CEPH_MDS_OP_LSETLAYOUT,
				       file->f_dentry, NULL, NULL, NULL,
				       USE_AUTH_MDS);
	req->r_args.setlayout.layout = layout;
	ceph_release_caps(inode, CEPH_CAP_FILE_RDCACHE);
	err = ceph_mdsc_do_request(mdsc, parent_inode, req);
	ceph_mdsc_put_request(req);
	return err;
}

long ceph_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	dout(10, "ioctl file %p cmd %u arg %lu\n", file, cmd, arg);
	switch (cmd) {
	case CEPH_IOC_GET_LAYOUT:
		return ceph_ioctl_get_layout(file, (void __user *)arg);

	case CEPH_IOC_SET_LAYOUT:
		return ceph_ioctl_set_layout(file, (void __user *)arg);
	}
	return -ENOTTY;
}
