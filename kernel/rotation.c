#include <linux/rotation.h>

extern struct dev_rotation rotation;
extern struct thread_cond_t rotation_read;
extern struct thread_cond_t rotation_write;
extern struct area_descriptor rot_area;
extern spinlock_t rot_lock;


/*
 * signal의 경우 waiting_list가 비어있지 않다면,
 * wake_up시킨다. wake_up함수 자체에 reschedule함수가 존재한다.
 */
static void thread_cond_signal(struct thread_cond_t *x)
{
	complete(&x->cv);
}

static void thread_cond_broadcast(struct thread_cond_t *x)
{
	complete_all(&x->cv);
}

static void thread_cond_wait(struct thread_cond_t *x)
{
	unsigned long flags;
	
	//wait을 할 때 lock을 풀고 wait을 한다.
	spin_unlock_irqrestore(&rot_lock, flags);
	//wait으로 들어간다.
	wait_for_completion(&x->cv);
	//wake_up해서 돌아오면 lock을 다시 잡는다.
	spin_lock_irqsave(&rot_lock, flags);
}

static int read_should_wait(struct rotation_range *rot)
{
	//현재 각도에 write wait or runner가 있으면
	//내각도가 아니야
	int cur = rotation.degree;
	int max = rot->rot.degree + rot->degree_range + 360;
	int min = rot->rot.degree - rot->degree_range + 360;

	return (rot_area.active_writers[cur / 30] > 0 ||
		    rot_area.waiting_writers[cur / 30] > 0 ||
			    cur + 360 > max || cur + 360 < min);
}

static int write_should_wait(struct rotation_range *rot)
{
	//현재 각도에 write runner가 있으면
	//현재 각도에 read runner가 있으면
	//내각도가 아니야
	int cur = rotation.degree;
	int max = rot->rot.degree + rot->degree_range + 360;
	int min = rot->rot.degree - rot->degree_range + 360;

	return (rot_area.active_writers[cur / 30] > 0 ||
		    rot_area.active_readers[cur / 30] > 0 ||
			    cur + 360 > max || cur + 360 < min);
}


static inline void remove_read_waiter(struct rotation_range *rot)
{
	int max = (rot->rot.degree + rot->degree_range + 360) / 30;
	int min = (rot->rot.degree - rot->degree_range + 360);
	int i;

	min = min % 30 ? min / 30 + 1 : min / 30;

	for (i = min; i <= max; i++) {
		rot_area.waiting_readers[i % 12] -= 1;
	}
}

static inline void add_read_waiter(struct rotation_range *rot)
{
	int max = (rot->rot.degree + rot->degree_range + 360) / 30;
	int min = (rot->rot.degree - rot->degree_range + 360);
	int i;

	min = min % 30 ? min / 30 + 1 : min / 30;

	for (i = min; i <= max; i++) {
		rot_area.waiting_readers[i % 12] += 1;
	}
}

static inline void remove_write_waiter(struct rotation_range *rot)
{	
	int max = (rot->rot.degree + rot->degree_range + 360) / 30;
	int min = (rot->rot.degree - rot->degree_range + 360);
	int i;

	min = min % 30 ? min / 30 + 1 : min / 30;

	for (i = min; i <= max; i++) {
		rot_area.waiting_writers[i % 12] -= 1;
	}
}

static inline void add_write_waiter(struct rotation_range *rot)
{	
	int max = (rot->rot.degree + rot->degree_range + 360) / 30;
	int min = (rot->rot.degree - rot->degree_range + 360);
	int i;

	min = min % 30 ? min / 30 + 1 : min / 30;

	for (i = min; i <= max; i++) {
		rot_area.waiting_writers[i % 12] += 1;
	}
}

static inline void remove_read_runner(struct rotation_range *rot)
{	
	int max = (rot->rot.degree + rot->degree_range + 360) / 30;
	int min = (rot->rot.degree - rot->degree_range + 360);
	int i;

	min = min % 30 ? min / 30 + 1 : min / 30;

	for (i = min; i <= max; i++) {
		rot_area.active_readers[i % 12] -= 1;
	}
}

static inline void add_read_runner(struct rotation_range *rot)
{	
	int max = (rot->rot.degree + rot->degree_range + 360) / 30;
	int min = (rot->rot.degree - rot->degree_range + 360);
	int i;

	min = min % 30 ? min / 30 + 1 : min / 30;

	for (i = min; i <= max; i++) {
		rot_area.active_readers[i % 12] += 1;
	}
}

static inline void remove_write_runner(struct rotation_range *rot)
{	
	int max = (rot->rot.degree + rot->degree_range + 360) / 30;
	int min = (rot->rot.degree - rot->degree_range + 360);
	int i;

	min = min % 30 ? min / 30 + 1 : min / 30;

	for (i = min; i <= max; i++) {
		rot_area.active_writers[i % 12] -= 1;
	}
}

static inline void add_write_runner(struct rotation_range *rot)
{	
	int max = (rot->rot.degree + rot->degree_range + 360) / 30;
	int min = (rot->rot.degree - rot->degree_range + 360);
	int i;

	min = min % 30 ? min / 30 + 1 : min / 30;

	for (i = min; i <= max; i++) {
		rot_area.active_writers[i % 12] += 1;
	}
}

asmlinkage int sys_set_rotation(struct dev_rotation __user *rot) 
{
	get_user(rotation.degree, &rot->degree);
	spin_lock(&rot_lock);
	if (rot_area.waiting_writers[rotation.degree / 30] > 0) {
		thread_cond_broadcast(&rotation_write);
	} else {
		thread_cond_broadcast(&rotation_read);
	}
	spin_unlock(&rot_lock);
	return 0;
}

asmlinkage int sys_rotlock_read(struct rotation_range __user *rot)
{
	struct rotation_range krot;
	get_user(krot, rot);

	spin_lock(&rot_lock);
	add_read_waiter(&krot);
	while (read_should_wait(&krot)) {
		thread_cond_wait(&rotation_read);
	}
	remove_read_waiter(&krot);
	add_read_runner(&krot);
	spin_unlock(&rot_lock);
	return 0;
}

asmlinkage int sys_rotlock_write(struct rotation_range __user *rot)
{
	struct rotation_range krot;
	get_user(krot, rot);

	spin_lock(&rot_lock);
	add_write_waiter(&krot);
	while (write_should_wait(&krot)) {
		thread_cond_wait(&rotation_write);
	}
	remove_write_waiter(&krot);
	add_write_runner(&krot);
	spin_unlock(&rot_lock);
	return 0;
}

asmlinkage int sys_rotunlock_read(struct rotation_range __user *rot)
{
	struct rotation_range krot;
	get_user(krot, rot);

	spin_lock(&rot_lock);
	remove_read_runner(&krot);
	if (rot_area.active_readers[rotation.degree / 30] == 0 &&
			rot_area.waiting_writers[rotation.degree / 30] > 0) {
		thread_cond_broadcast(&rotation_write);
	}
	spin_unlock(&rot_lock);
	return 0;
}

asmlinkage int sys_rotunlock_write(struct rotation_range __user *rot)
{
	struct rotation_range krot;
	get_user(krot, rot);

	spin_lock(&rot_lock);
	remove_write_runner(&krot);
	if (rot_area.waiting_writers[rotation.degree / 30] > 0) {
		thread_cond_broadcast(&rotation_write);
	} else {
		thread_cond_broadcast(&rotation_read);
	}
	spin_unlock(&rot_lock);
	return 0;
}
