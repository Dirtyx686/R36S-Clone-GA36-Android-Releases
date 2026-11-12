/* GA36 backlight port: D04 serialized writes and cancellation retained.
 * Android11 official lights/hardware headers validate the legacy HAL ABI. */
#include <hardware/hardware.h>
#include <hardware/lights.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
_Static_assert(sizeof(struct hw_module_t)==128,"ARM32 hardware module ABI");
_Static_assert(sizeof(struct hw_device_t)==64,"ARM32 hardware device ABI");
_Static_assert(sizeof(struct light_device_t)==68,"ARM32 legacy light ABI");
static unsigned generation,write_lock;
static int brightness_now(void){
 char buf[16];int fd=open("/sys/devices/virtual/disp/disp/attr/lcdbl",0|0x80000);
 if(fd<0)return -1;int n=read(fd,buf,sizeof(buf));close(fd);
 if(n<=0||buf[0]<'0'||buf[0]>'9')return -1;
 unsigned v=0;for(int i=0;i<n&&buf[i]>='0'&&buf[i]<='9';i++){v=v*10+(unsigned)(buf[i]-'0');if(v>255)return -1;}
 return (int)v;
}
static int kernel_serialized(void){
 char value[4]={0};int fd=open("/sys/devices/virtual/disp/disp/attr/lcdbl_sync",0|0x80000);
 if(fd<0)return 0;int n=read(fd,value,sizeof(value)-1);close(fd);return n>0&&value[0]=='1';
}
static void trace(unsigned requested,int before,int delayed,int result){
 int fd=open("/data/vendor/ga36/backlight.txt",1|0x400|0x80000);if(fd<0)return;
 if(lseek(fd,0,2)>65536)ftruncate(fd,0);
 struct timespec t={0};clock_gettime(1,&t);
 char buf[128];int n=snprintf(buf,sizeof(buf),"uptime=%ld.%03ld request=%u before=%d resume_delay_ms=%d after=%d result=%d sync=%d\n",t.tv_sec,t.tv_nsec/1000000,requested,before,delayed,brightness_now(),result,kernel_serialized());
 if(n>0&&n<(int)sizeof(buf))write(fd,buf,(unsigned)n);if(requested==0||before==0)fsync(fd);close(fd);
}
static int close_light(struct hw_device_t *d){free(d);return 0;}
static int set_backlight(struct light_device_t *dev,const struct light_state_t *s){
 (void)dev;if(!s)return -22;
 unsigned c=s->color;unsigned value=(77*((c>>16)&255)+150*((c>>8)&255)+29*(c&255))>>8;
 unsigned ticket=__sync_add_and_fetch(&generation,1);
 int before=brightness_now(),delayed=0;
 /* Stock LCD resume finishes asynchronously around430ms after wake. A
  * brightness write before that can be lost when the panel flow finishes.
  * Delay only a transition from physically dark to a requested nonzero level.
  * Newer requests, especially screen-off, invalidate this pending write. */
 if(value>0&&before==0&&!kernel_serialized()){usleep(650000);delayed=650;}
 while(__sync_lock_test_and_set(&write_lock,1))usleep(1000);
 if(__sync_add_and_fetch(&generation,0)!=ticket){__sync_lock_release(&write_lock);trace(value,before,delayed,1);return 0;}
 char buf[16];int len=snprintf(buf,sizeof(buf),"%u\n",value);
 int fd=open("/sys/devices/virtual/disp/disp/attr/lcdbl",1|0x80000);
 if(fd<0){int result=-errno;__sync_lock_release(&write_lock);return result;}
 int n=write(fd,buf,(unsigned)len);int result=n==len?0:n<0?-errno:-5;close(fd);trace(value,before,delayed,result);__sync_lock_release(&write_lock);return result;
}
static int open_light(const struct hw_module_t *m,const char *id,struct hw_device_t **out){
 if(!out)return -22;*out=0;if(!id||strcmp(id,"backlight"))return -38;
 struct light_device_t *l=calloc(1,sizeof(*l));if(!l)return -12;
 l->common.tag=0x48574454;l->common.version=LIGHTS_DEVICE_API_VERSION_1_0;l->common.module=(struct hw_module_t*)m;l->common.close=close_light;l->set_light=set_backlight;*out=&l->common;return 0;
}
static struct hw_module_methods_t methods={open_light};
__attribute__((visibility("default"))) struct hw_module_t HMI={.tag=0x48574d54,.module_api_version=HARDWARE_MODULE_API_VERSION(1,0),.hal_api_version=HARDWARE_HAL_API_VERSION,.id="lights",.name="GA36 D04 synchronized backlight, legacy fallback",.author="GA36 bring-up",.methods=&methods};
