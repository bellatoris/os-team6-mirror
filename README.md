제출할 코드는 proj4 branch에 모두 merge해 두었습니다.  
System call의 추가와 관련해서   
arch/arm/configs/tizen_tm1_defconfig  
/arch/arm/include/uapi/asm/unistd.h  
/arch/arm/include/uapi/asm/unistd.h  
/arch/arm/kernel/calls.S  
include/linux/syscalls.h  
fs/ext2/ext2.h  
fs/ext2/file.c  
fs/ext2/namei.c  
include/linux/fs.h  
include/linux/gps.h  
kernel/gps.c  
을 변경, 생성하였고 file_loc 함수는 test폴더에 Makefile과 함께 들어있습니다.  

**1.high-level design**  
* gps 구조체  
ext2 file system에 gps 정보를 이용한 새로운 권한을 추가하고자 한다.  
file/ directory가 생성되거나 변경될 때 미다 구조체에서 gps정보를 가져와서 inode에 저장한다.  
이 때 변경되는 경우는 rename이 되는 경우를 의미한다.  

* 새로운 시스템 콜  
먼저 커널 내부와 gps정보를 저장하는 구조체를 만들고 구조체에 gps 정보를 넣는 시스템콜을 만든다.  
file / direcctory의 path를 인자로 받아 해당 path에 저장된 gps 정보를 받아오는 시스템콜을 만든다.   

시스템콜이 inode의 gps구조체에 접근할 때는 inode_operation에 등록된 함수를 이용한다.  
access control은 시간부족으로 제대로 구현하지 못하였습니다. 
혹시라도 제대로 동작하지 않으 실 경우 
 ```c
 *
 * This does the basic permission checking
 */
static int acl_permission_check(struct inode *inode, int mask)
{
	unsigned int mode = inode->i_mode;

	if (likely(uid_eq(current_fsuid(), inode->i_uid)))
		mode >>= 6;
	else {
		if (IS_POSIXACL(inode) && (mode & S_IRWXG)) {
			int error = check_acl(inode, mask);
			if (error != -EAGAIN)
				return error;
		}

		if (in_group_p(inode->i_gid))
			mode >>= 3;
	}
	/*
	 * If inode is ext2, check gps location.
	 */
	if (strcmp(inode->i_sb->s_type->name, "ext2")==0){
		printk("sm It's EXT2! we need gps check!\n");
		struct ext2_inode_info *ei = EXT2_I(inode);
		__u64 latitude = 0;
		__u64 longitude = 0;
		__u32 accuracy = 0;

		latitude = le64_to_cpu(ei->disk_gps.latitude);
		longitude = le64_to_cpu(ei->disk_gps.longitude);
		accuracy = le32_to_cpu(ei->disk_gps.accuracy);

		//여기도 lock

		if (*(unsigned long long *)&kernel_location.latitude != latitude){
			printk("lat miss matching. ker: %llu, file: %llu\n",*(unsigned long long *)&kernel_location.latitude,latitude);
			return -EACCES;
		}
		if (*(unsigned long long *)&kernel_location.longitude != longitude){
			printk("long miss matching. ker: %llu, file: %llu\n", *(unsigned long long *)&kernel_location.longitude, longitude);
			return -EACCES;
		}
		printk("sm GPS check pass!\n");
	}
	/*
	 * If the DACs are ok we don't need any capability check.
	 */
	if ((mask & ~mode & (MAY_READ | MAY_WRITE | MAY_EXEC)) == 0)
		return 0;
	return -EACCES;
}
```
fs/namei.c의 위의 함수에서 check gps location하는 부분을 전체 주석 처리해 주시길 바랍니다.

**2.implementation**  
* gps 구조체  
```c
struct gps_location {
	double latitude;
	double longitude;
	float accuracy;
};

struct disk_location{
	__le64 latitude;	
	__le64 longitude;	
	__le32 accuracy;
} disk_gps;
```
gps_location 구조체는 커널 내부에서 gps 정보를 저장하는데 쓰인다. disk_location 구조체는 파일 내부에서 gps_정보를 저장하는데 쓰인다. 커널 내부에서 float연산을 할 수 없고, endianness때문에 __le64 type을 사용하는데 이 때문에
```c
latitude = le64_to_cpu(ei->disk_gps.latitude);
longitude = le64_to_cpu(ei->disk_gps.longitude);
accuracy = le32_to_cpu(ei->disk_gps.accuracy);

gps->latitude = *(double *)&latitude;
gps->longitude = *(double *)&longitude;
gps->accuracy = *(float *)&accuracy;	
```
이렇게 형변환을 통해서 gps정보를 저장하고 불러온다.  

* inode 접근  
```c
const struct inode_operations ext2_file_inode_operations = {
        ...
        .set_location = ext2_set_gps_location,
        .get_location = ext2_get_gps_location,
};
```
inode에 저장되어 있는 gps 구조체에 접근하는 operation ext2_set_gps_location, ext2_get_gps_location을 다음과 같이 연결 해서 사용했다. set_gps_location은 file에 gps정보를 넣고, get_gps_location은 받아온다.

* 기존의 inode operation들 수정
ext2_create, ext2_mkdir(생성), ext2_rename(수정)이 불릴때 gps 정보를 inode에 set 하도록 했다.
```c
  new_inode->i_op = &ext2_file_inode_operations;
  new_inode->i_op->set_location(new_inode);
```
**3. user space testing **  
mke2fs를 이용해서 만든 proj4.fs을 mount해서 ext2 system을 이용했다.

pathname을 문자열로 받아 inode의 gps 정보를 가져오는 utility file_loc를 만들었다.
```
sh-4.1# ./setgps 1 2 3
sh-4.1# ./file_loc /home/developer/proj4/dir1/
get_gps_location: Success
latitude : 1.000000
longitude : 2.000000
accuracy : 3.000000
Google map url : https://www.google.com/maps/@1.000000,2.000000,4z
sh-4.1# ./setgps 4 5 6
sh-4.1# ./file_loc /home/developer/proj4/file1
get_gps_location: Success
latitude : 4.000000
longitude : 5.000000
accuracy : 6.000000
Google map url : https://www.google.com/maps/@4.000000,5.000000,4z
sh-4.1# ./setgps 7 8 9                        
sh-4.1# ./file_loc /home/developer/proj4/file2
get_gps_location: Success
latitude : 7.000000
longitude : 8.000000
accuracy : 9.000000
Google map url : https://www.google.com/maps/@7.000000,8.000000,4z
```
```
sh-4.1# ./setgps 1 2 3
sh-4.1# ./file_loc /home/developer/proj4/dir1/
get_gps_location: Success
latitude : 1.000000
longitude : 2.000000
accuracy : 3.000000
Google map url : https://www.google.com/maps/@1.000000,2.000000,4z
sh-4.1# ./setgps 4 5 6
sh-4.1# ./file_loc /home/developer/proj4/file1
get_gps_location: Success
latitude : 4.000000
longitude : 5.000000
accuracy : 6.000000
Google map url : https://www.google.com/maps/@4.000000,5.000000,4z
sh-4.1# ./setgps 7 8 9                        
sh-4.1# ./file_loc /home/developer/proj4/file2
get_gps_location: Success
latitude : 7.000000
longitude : 8.000000
accuracy : 9.000000
Google map url : https://www.google.com/maps/@7.000000,8.000000,4z
```
setgps로 latitude,longitude,accuracy를 (1,2,3),(4,5,6),(7,8,9)로 바꾸면서
dir1, file1, file2를 만들고 
file_loc으로 gps값을 불러왔다.

**4.lesson learned**  
1.inode에 접근하고,inode에 새로운 header를 추가하는 법을 알게 되었다.
2.file system maintain utility를 사용하는 법을 알게 되었다.
