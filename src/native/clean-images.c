
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
#include <sys/statvfs.h>
#include <sys/sysmacros.h>
#include <sys/ioctl.h>
#include <sys/wait.h>
#include <linux/loop.h>
_Static_assert(sizeof(off_t)==8,"large file ABI");
static uint32_t u32(unsigned char*p){return p[0]|p[1]<<8|p[2]<<16|(uint32_t)p[3]<<24;}
static int cmd(const char *path,const char *flag,const char *dev,int max){pid_t p=fork();int s;if(p<0)return -1;if(!p){if(flag)execl(path,path,flag,dev,(char*)0);else execl(path,path,dev,(char*)0);_exit(127);}while(waitpid(p,&s,0)<0)if(errno!=EINTR)return -1;return WIFEXITED(s)&&WEXITSTATUS(s)<=max?0:-1;}
struct role{const char*name;unsigned loop,ro;uint64_t bytes;unsigned char uuid[16];};
static struct role data7={"ga36-android7/data.img",3,0,12884901888ULL,{0x14,0x77,0x6a,0xae,0x12,0xa2,0x40,0x0d,0x94,0xe4,0x88,0xf0,0xb0,0xfb,0xa8,0x8a}};
static struct role a11[]={
{"ga36-android11/system.img",4,1,2147483648ULL,{0x3f,0x3a,0xc6,0x26,0xb1,0x16,0x43,0x26,0x92,0x62,0x64,0xf1,0x1d,0xdc,0xed,0xec}},
{"ga36-android11/vendor.img",5,1,536870912ULL,{0x42,0x84,0xba,0x16,0x22,0x13,0x4c,0x35,0xa3,0x0c,0x11,0xa4,0x5d,0xf5,0xc6,0x8f}},
{"ga36-android11/data.img",6,0,12884901888ULL,{0xff,0x0a,0xdc,0x71,0xe7,0xdb,0x4e,0x20,0x9d,0x3c,0xc3,0xff,0xf9,0x2f,0x59,0x2d}}};
static int valid(int fd,struct role*r,struct stat*st){unsigned char sb[1024];if(fd<0||fstat(fd,st)||!S_ISREG(st->st_mode)||st->st_uid||st->st_nlink!=1||(st->st_mode&0777)!=0600||st->st_size<0)return -1;
if((r->ro&&(uint64_t)st->st_size!=r->bytes)||(!r->ro&&(uint64_t)st->st_size<r->bytes)||pread(fd,sb,1024,1024)!=1024||sb[56]!=0x53||sb[57]!=0xef||u32(sb+24)!=2||memcmp(sb+104,r->uuid,16))return -1;
uint64_t fsbytes=(uint64_t)u32(sb+4)*4096;if((r->ro&&fsbytes!=r->bytes)||fsbytes>(uint64_t)st->st_size||fsbytes<r->bytes)return -1;
if((u32(sb+92)&~0x3fu)||(u32(sb+96)&~(0x2u|0x40u|0x200u|(r->ro?0u:4u)))||(u32(sb+100)&~0x7bu)||(!r->ro&&!(u32(sb+92)&4u))||(r->ro&&sb[58]!=1))return -1;return 0;}
int main(int argc,char**argv){if(argc==2&&!strcmp(argv[1],"--selftest")){puts("PASS clean image helper 64-bit file ABI");return 0;}if(argc!=2||geteuid())return 64;
int seven=!strcmp(argv[1],"--attach-android7");if(!seven&&strcmp(argv[1],"--attach"))return 64;
struct stat base;struct statvfs fs;if(stat("/ga36-container",&base)||base.st_uid||major(base.st_dev)!=7||minor(base.st_dev)!=2||statvfs("/ga36-container",&fs))return 1;
struct role *roles=seven?&data7:a11;unsigned count=seven?1:3;int files[3]={-1,-1,-1};struct stat st[3];
for(unsigned i=0;i<count;i++){char path[256];snprintf(path,sizeof(path),"/ga36-container/%s",roles[i].name);files[i]=open(path,(roles[i].ro?O_RDONLY:O_RDWR)|O_CLOEXEC|O_NOFOLLOW);if(valid(files[i],&roles[i],&st[i])||st[i].st_dev!=base.st_dev)return 2;}
uint64_t reserve=UINT64_C(5)*1024*1024*1024+(seven?0:UINT64_C(2684354560));uint64_t available=(uint64_t)fs.f_blocks*fs.f_frsize;if(available<reserve+UINT64_C(12884901888))return 3;uint64_t wanted=(available-reserve)&~UINT64_C(4095);
for(unsigned i=0;i<count;i++){struct role*r=&roles[i];char dev[64];snprintf(dev,sizeof(dev),"/dev/loop%u",r->loop);int l=open(dev,O_RDWR|O_CLOEXEC|O_NOFOLLOW);struct stat ls;struct loop_info64 check={0},info={0};if(l<0||fstat(l,&ls)||!S_ISBLK(ls.st_mode)||ls.st_rdev!=makedev(7,r->loop))return 4;errno=0;if(ioctl(l,LOOP_GET_STATUS64,&check)==0||errno!=ENXIO)return 5;
if(!r->ro){if((uint64_t)st[i].st_size>wanted||ftruncate(files[i],(off_t)wanted))return 6;}
if(ioctl(l,LOOP_SET_FD,files[i]))return 7;info.lo_flags=r->ro?LO_FLAGS_READ_ONLY:0;info.lo_sizelimit=r->ro?r->bytes:wanted;if(ioctl(l,LOOP_SET_STATUS64,&info)||ioctl(l,LOOP_GET_STATUS64,&check)||check.lo_sizelimit!=info.lo_sizelimit||check.lo_flags!=info.lo_flags)return 8;
if(!r->ro){unsigned char sb[1024];if(pread(files[i],sb,1024,1024)!=1024)return 9;uint64_t bytes=(uint64_t)u32(sb+4)*4096;if(bytes<wanted&&(cmd("/sbin/e2fsck","-p",dev,1)||cmd("/sbin/resize2fs",0,dev,0)))return 10;}
close(l);close(files[i]);}return 0;}
