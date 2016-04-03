/*
 * sets current device rotation in the kernel.
 * syscall number 384 (you may want to check this number!)
 */

struct dev_rotation {
        int degree;     /* 0 <= degree < 360 */
}; 

int set_rotation(struct dev_rotation *rot);

