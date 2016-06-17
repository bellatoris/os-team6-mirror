#include <linux/gps.h>
#include <linux/kernel.h>
#include <linux/uaccess.h>
#include <linux/slab.h>
#include <linux/syscalls.h>
#include <linux/namei.h>
#include <linux/limits.h>

#ifndef R_OK
#define R_OK 4
#endif

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

static int is_directory(const char *path)
{
	int ret;
	if (path == NULL)
		return 0;
	ret = sys_open(path, O_DIRECTORY, O_RDONLY);

	if (ret < 0) /* Failed to open DIR, */
		return 0;
	else { /* Definitely is a directory */
		sys_close(ret);
		return 1;
	}
}


static int can_access_file(const char *file)
{
	/* TO be implemented */
	int ret;

	if (file == NULL)
		return 0;

	if (is_directory(file)) {
		/* returns -1 on error */
		ret = sys_open(file, O_DIRECTORY, O_RDONLY);
		if (ret < 0)
			/* leave it that way */;
		else {
			sys_close(ret);
			ret = 0; /* set success value. */
		}

	} else {
		/* Access system call returns 0 on success. R_OK flags checks
		 * both that the file exists and that the current process has
		 * permission to read the file */
		ret = sys_access(file, R_OK);
	}

	if (ret == 0)
		return 1;
	else
		return 0;
}


static int get_file_gps_location(const char *kfile, struct gps_location *loc)
{
	int ret;
	struct inode *d_inode;
	struct path kpath = { .mnt = NULL, .dentry = NULL} ;

	/* Still to be implemented */
	if (kfile == NULL || loc == NULL){
		return -EINVAL;
	}

	/*
	 * After looking at namei.c file in /fs, I determined
	 * path_lookup is the function we want to use.
	 * This function is inturn called from do_path_lookup,
	 * with tries different variations, which is inturn called by
	 * kern_path(). So, we should use kern_path(). It returns 0 on
	 * success and something else on failure.
	 * Note: that the complimentary macro user_path() won't work in
	 * this case because the character string we're checking is
	 * in kernel-address space
	 */
	if (kern_path(kfile, LOOKUP_FOLLOW | LOOKUP_AUTOMOUNT, &kpath) != 0){
		return -EAGAIN;
	}

	/* d_inode represents the inode of the looked up path */
	d_inode = kpath.dentry->d_inode;

	if (d_inode == NULL){
		return -EINVAL;
	}

	/* Make the System GPS Read Call.*/
	ret =  inode_to_gps(d_inode, loc);
	/* release the path found */
	path_put(&kpath);
	return ret;

}


SYSCALL_DEFINE1(set_gps_location, struct gps_location __user *, loc)
{
	/*
	__u64 longitude = * (unsigned long long *)&loc->longitude;
	__u64 latitude = * (unsigned long long *)&loc->latitude;
	__u32 accuracy = * (unsigned int *)&loc->accuracy;
	printk("lat :%llu long : %llu accuracy : %x",latitude,longitude,accuracy);
	*/
	copy_from_user(&kernel_location, loc,sizeof(struct gps_location));
	return 0;
}

SYSCALL_DEFINE2(get_gps_location, const char *, pathname, struct gps_location __user *, loc)
{
		/* still to be implemented */
	struct gps_location kloc;
	char *kpathname;
	int ret;
	/* PATH_MAX is the maximum valid length of a filepath in UNIX */
	int path_size = PATH_MAX + 1;
	if (pathname == NULL || loc == NULL)
		return -EINVAL;

	kpathname = kcalloc(path_size, sizeof(char), GFP_KERNEL);
	if (kpathname == NULL)
		return -ENOMEM;

	ret = strncpy_from_user(kpathname, pathname, path_size);

	if (ret < 0) { /* Error occured */
		kfree(kpathname);
		return -EFAULT;
	} else if (ret == path_size) { /* Path is too long */
		kfree(kpathname);
		return -ENAMETOOLONG;
	}

	memset(&kloc, 0, sizeof(kloc));

	/* TODO: Enable these checks when we implement the functions.
	 * Don't forget to free the kpathname memory too. */
	if (!can_access_file(pathname)) {
		kfree(kpathname);
		return -EACCES;
	}

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
	/* TODO:
	 * On success, the system call should return the i_coord_age value
	 * of the inode associated with the path.*/
	return ret;
}
/*
int set_gps_location(struct gps_location __user * loc)
{
	copy_from_user(&kernel_location, loc,sizeof(struct gps_location));
        return 0;
}
int get_gps_location(const char * pathname, struct gps_location __user * loc)
{
	return 0;
}
*/
