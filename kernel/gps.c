#include <linux/gps.h>
#include <linux/kernel.h>
#include <linux/uaccess.h>
#include <linux/slab.h>
#include <linux/syscalls.h>
#include <linux/namei.h>
#include <linux/limits.h>

struct gps_location kernel_location;
EXPORT_SYMBOL(kernel_location);

int inode_to_gps(struct inode *inode, struct gps_location *loc)
{
        int ret = 0;
        if (inode == NULL || loc == NULL)
                return -EINVAL;

        if (inode->i_op->get_location != NULL)
                ret = inode->i_op->get_location(inode, loc);
        else
                ret = -ENOENT; /* No such GPS-capable file */

        return ret;
}

static int get_file_gps_location(const char *kfile, struct gps_location *loc)
{
	int ret;
	struct inode *d_inode;
	struct path kpath = { .mnt = NULL, .dentry = NULL} ;

	if (kfile == NULL || loc == NULL){
		return -EINVAL;
	}

	if (kern_path(kfile, LOOKUP_FOLLOW | LOOKUP_AUTOMOUNT, &kpath) != 0){
		return -EAGAIN;
	}

	d_inode = kpath.dentry->d_inode;

	if (d_inode == NULL){
		return -EINVAL;
	}

	ret =  inode_to_gps(d_inode, loc);

	path_put(&kpath);
	return ret;

}


SYSCALL_DEFINE1(set_gps_location, struct gps_location __user *, loc)
{

	copy_from_user(&kernel_location, loc,sizeof(struct gps_location));
	return 0;
}

SYSCALL_DEFINE2(get_gps_location, const char *, pathname, struct gps_location __user *, loc)
{
	struct gps_location kloc;
	char *kpathname;
	int ret;

	int path_size = PATH_MAX + 1;
	if (pathname == NULL || loc == NULL)
		return -EINVAL;

	kpathname = kcalloc(path_size, sizeof(char), GFP_KERNEL);
	if (kpathname == NULL)
		return -ENOMEM;

	ret = strncpy_from_user(kpathname, pathname, path_size);

	if (ret < 0) {
		kfree(kpathname);
		return -EFAULT;
	} else if (ret == path_size) {
		kfree(kpathname);
		return -ENAMETOOLONG;
	}

	memset(&kloc, 0, sizeof(kloc));

	ret = get_file_gps_location(kpathname, &kloc);

	if (ret < 0) {
		kfree(kpathname);
		return -EAGAIN;
	}

	if (copy_to_user(loc, &kloc, sizeof(struct gps_location)) != 0) {
		kfree(kpathname);
		return -EFAULT;
	}

	kfree(kpathname);
	return ret;
}
