#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <sys/personality.h>
#include <sys/stat.h>
#include <sys/vfs.h>
#include <unistd.h>
int main(void){
 assert(getpid()==1&&!(personality(0xffffffffUL)&READ_IMPLIES_EXEC));
 struct statfs fs;assert(!statfs("/",&fs)&&fs.f_type==0x01021994);assert(!statfs("/system",&fs)&&fs.f_type==0x73717368);assert(!statfs("/data",&fs)&&fs.f_type==0x01021994);
 const char*names[]={"dev","proc","sys","cache"};for(unsigned i=0;i<4;i++){char p[64];snprintf(p,sizeof(p),"/%s",names[i]);assert(!statfs(p,&fs)&&fs.f_type==0x01021994);}
 const char*src[]={"/ga36-hw","/ga36-keylayout","/ga36-idc"};const char*dst[]={"/system/lib/hw","/system/usr/keylayout","/system/usr/idc"};for(unsigned i=0;i<3;i++){struct stat a,b;assert(!stat(src[i],&a)&&!stat(dst[i],&b)&&a.st_ino==b.st_ino&&a.st_dev==b.st_dev);}
 assert(access("/data/ga36-memory/swap-bind-marker",R_OK)==0);
 assert(access("/ga36boot",F_OK)==-1); /* startup/pool views not exposed to Android7 */
 FILE*f=fopen("/proof/android7-result.json","w");assert(f);fputs("{\"result\":\"passed\",\"actual_Android7_init_executed\":false,\"actual_native_handoff_executed\":true,\"real_A7_system_readonly\":true,\"userdata_test_tmpfs_only\":true}\n",f);assert(!fclose(f));puts("PASS actual native Android7 PID1 root handoff; real read-only system, three HAL/input binds, private data/swap bind; marker init only");return 0;
}
