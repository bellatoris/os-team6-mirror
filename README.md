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
access control은 시간부족으로 구현하지 못했습니다.ㅜ  

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


