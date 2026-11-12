#define _GNU_SOURCE
#define _FILE_OFFSET_BITS 64
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/statfs.h>
#include <sys/statvfs.h>
#include <sys/sysmacros.h>
#include <sys/ioctl.h>
#include <sys/wait.h>
#include <linux/loop.h>
#define BASE_DATA UINT64_C(25769803776)
#define BASE_POOL UINT64_C(60671655936)
#define FIXED (UINT64_C(4294967296)+UINT64_C(2684354560)+UINT64_C(536870912))
_Static_assert(sizeof(off_t)==8,"large file ABI");
_Static_assert(sizeof(struct loop_info64)==232,"loop ABI");
struct role { const char *dir,*file; unsigned loop,ro; uint64_t bytes; unsigned char uuid[16]; };
static const struct role roles[]={
 {"ga36-android7","data.img",3,0,BASE_DATA,{0x14,0x77,0x6a,0xae,0x12,0xa2,0x40,0x0d,0x94,0xe4,0x88,0xf0,0xb0,0xfb,0xa8,0x8a}},
 {"ga36-android11","system.img",4,1,UINT64_C(2147483648),{0x3f,0x3a,0xc6,0x26,0xb1,0x16,0x43,0x26,0x92,0x62,0x64,0xf1,0x1d,0xdc,0xed,0xec}},
 {"ga36-android11","vendor.img",5,1,UINT64_C(536870912),{0x42,0x84,0xba,0x16,0x22,0x13,0x4c,0x35,0xa3,0x0c,0x11,0xa4,0x5d,0xf5,0xc6,0x8f}},
 {"ga36-android11","data.img",6,0,BASE_DATA,{0xff,0x0a,0xdc,0x71,0xe7,0xdb,0x4e,0x20,0x9d,0x3c,0xc3,0xff,0xf9,0x2f,0x59,0x2d}}
};
static uint32_t u32(const unsigned char *p){return (uint32_t)p[0]|(uint32_t)p[1]<<8|(uint32_t)p[2]<<16|(uint32_t)p[3]<<24;}
static uint64_t budget(uint64_t usable,uint64_t poolbytes){
 if(poolbytes<=BASE_POOL)return BASE_DATA;
 if(usable<FIXED+2*BASE_DATA)return 0;
 return ((usable-FIXED)/2)&~UINT64_C(4095);
}
static int command(const char *path,const char *flag,const char *dev,int max){
 pid_t p=fork();int s;if(p<0)return -1;
 if(!p){if(flag)execl(path,path,flag,dev,(char*)0);else execl(path,path,dev,(char*)0);_exit(127);}
 while(waitpid(p,&s,0)<0)if(errno!=EINTR)return -1;
 return WIFEXITED(s)&&WEXITSTATUS(s)<=max?0:-1;
}
static int valid(int fd,const struct role *r,struct stat *st,uint64_t max){
 unsigned char sb[1024];
 if(fd<0||fstat(fd,st)||!S_ISREG(st->st_mode)||st->st_nlink!=1||(st->st_mode&0777)!=0600||st->st_size<0)return -1;
#ifdef GA36_IMAGE_TEST
 if(st->st_uid!=geteuid())return -1;
#else
 if(st->st_uid)return -1;
#endif
 uint64_t size=(uint64_t)st->st_size;
 if((r->ro&&size!=r->bytes)||(!r->ro&&(size<r->bytes||size>max||size%4096))||pread(fd,sb,1024,1024)!=1024||sb[56]!=0x53||sb[57]!=0xef||u32(sb+24)!=2||memcmp(sb+104,r->uuid,16))return -1;
 uint64_t fsbytes=(uint64_t)u32(sb+4)*4096;
 if((r->ro&&fsbytes!=r->bytes)||fsbytes>size||fsbytes<r->bytes)return -1;
 uint32_t compat=u32(sb+92),incompat=u32(sb+96),ro=u32(sb+100);
 if((compat&~0x3fu)||(incompat&~(0x2u|0x40u|0x200u|(r->ro?0u:4u)))||(ro&~0x7bu)||(!r->ro&&!(compat&4u))||(r->ro&&(sb[58]!=1||sb[59])))return -1;
 if(!(compat&4u)&&(u32(sb+224)||u32(sb+228)||u32(sb+232)||memcmp(sb+208,(unsigned char[16]){0},16)))return -1;
 return 0;
}
static int attach(int seven){
 int result=1,base=-1,files[3]={-1,-1,-1},loops[3]={-1,-1,-1};unsigned attached=0;
 unsigned first=seven?0:1,count=seven?1:3;struct stat parent,st[3];struct statvfs fs;struct statfs sf;
 if(geteuid())return 64;
 base=open("/ga36-container",O_RDONLY|O_DIRECTORY|O_CLOEXEC|O_NOFOLLOW);
 if(base<0||fstat(base,&parent)||parent.st_uid||(parent.st_mode&0777)!=0700||parent.st_dev!=makedev(7,2)||fstatvfs(base,&fs)||fstatfs(base,&sf)||sf.f_type!=0xef53)goto out;
 unsigned char sb[1024];int pool=open("/dev/loop2",O_RDONLY|O_CLOEXEC|O_NOFOLLOW);if(pool<0)goto out;
 ssize_t got=pread(pool,sb,1024,1024);close(pool);if(got!=1024||u32(sb+24)!=2)goto out;
 uint64_t wanted=budget((uint64_t)fs.f_blocks*fs.f_frsize,(uint64_t)u32(sb+4)*4096);if(wanted<BASE_DATA)goto out;
 for(unsigned i=0;i<count;i++){
  const struct role *r=&roles[first+i];int dir=openat(base,r->dir,O_RDONLY|O_DIRECTORY|O_CLOEXEC|O_NOFOLLOW);struct stat ds;
  if(dir<0)goto out;
  int ok=!fstat(dir,&ds)&&ds.st_uid==0&&(ds.st_mode&0777)==0700&&ds.st_dev==parent.st_dev;
  if(ok)files[i]=openat(dir,r->file,(r->ro?O_RDONLY:O_RDWR)|O_CLOEXEC|O_NOFOLLOW);close(dir);
  if(!ok||valid(files[i],r,&st[i],wanted)||st[i].st_dev!=parent.st_dev)goto out;
  char dev[64];snprintf(dev,sizeof(dev),"/dev/loop%u",r->loop);loops[i]=open(dev,O_RDWR|O_CLOEXEC|O_NOFOLLOW);struct stat ls;struct loop_info64 check={0};
  if(loops[i]<0||fstat(loops[i],&ls)||!S_ISBLK(ls.st_mode)||ls.st_rdev!=makedev(7,r->loop))goto out;
  errno=0;if(ioctl(loops[i],LOOP_GET_STATUS64,&check)==0||errno!=ENXIO)goto out;
 }
 for(unsigned i=0;i<count;i++){
  const struct role *r=&roles[first+i];struct loop_info64 info={0},check={0};
  if(!r->ro&&((uint64_t)st[i].st_size>wanted||ftruncate(files[i],(off_t)wanted)||fsync(files[i])))goto out;
  if(ioctl(loops[i],LOOP_SET_FD,files[i]))goto out;attached++;
  info.lo_flags=r->ro?LO_FLAGS_READ_ONLY:0;info.lo_sizelimit=r->ro?r->bytes:wanted;
  if(ioctl(loops[i],LOOP_SET_STATUS64,&info)||ioctl(loops[i],LOOP_GET_STATUS64,&check)||check.lo_offset||check.lo_sizelimit!=info.lo_sizelimit||check.lo_inode!=(uint64_t)st[i].st_ino||check.lo_device!=(uint64_t)st[i].st_dev||!!(check.lo_flags&LO_FLAGS_READ_ONLY)!=r->ro)goto out;
  if(!r->ro){
   if(pread(files[i],sb,1024,1024)!=1024)goto out;
   if((uint64_t)u32(sb+4)*4096<wanted){
    char dev[64];snprintf(dev,sizeof(dev),"/dev/loop%u",r->loop);
    if(command("/sbin/e2fsck","-p",dev,1)||command("/sbin/resize2fs",NULL,dev,0))goto out;
    if(pread(files[i],sb,1024,1024)!=1024||(uint64_t)u32(sb+4)*4096!=wanted)goto out;
   }
  }
 }
 result=0;
out:
 if(result)while(attached){attached--;ioctl(loops[attached],LOOP_CLR_FD,0);}
 for(unsigned i=0;i<3;i++){if(loops[i]>=0)close(loops[i]);if(files[i]>=0)close(files[i]);}
 if(base>=0)close(base);return result;
}
int main(int argc,char **argv){
 if(argc==2&&!strcmp(argv[1],"--selftest")){
  if(budget(BASE_POOL,BASE_POOL)!=BASE_DATA||budget(UINT64_C(120000000000),UINT64_C(124000000000))!=UINT64_C(56241901568)||budget(10,BASE_POOL+4096)!=0)return 1;
  puts("PASS dual image ABI, equal budgets, baseline 24GiB per OS; hardware not exercised");return 0;
 }
#ifdef GA36_IMAGE_TEST
 if(argc==4&&!strcmp(argv[1],"--inspect")){
  unsigned i=(unsigned)strtoul(argv[2],NULL,10);if(i>3)return 2;
  int fd=open(argv[3],O_RDONLY|O_CLOEXEC|O_NOFOLLOW);struct stat st;int rc=valid(fd,&roles[i],&st,UINT64_C(2199023255552));if(fd>=0)close(fd);return rc?3:0;
 }
#endif
 if(argc==2&&!strcmp(argv[1],"--attach-android7"))return attach(1);
 if(argc==2&&!strcmp(argv[1],"--attach"))return attach(0);
 return 64;
}
