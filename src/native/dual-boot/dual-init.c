#include "runtime-init.c"
#include "v1-config.h"
struct binder_wr7 { uint32_t write_size,write_consumed,write_buffer,read_size,read_consumed,read_buffer; };
_Static_assert(sizeof(struct binder_wr7)==24,"Binder7 wire ABI");
static int select_protocol(int wanted){
 const char *p="/sys/module/ga36_binder_mux/parameters/protocol";
 note("BINDER_ABI_SELECT_BEGIN wanted=%d",wanted);
 int fd=open(p,O_WRONLY|O_CLOEXEC|O_NOFOLLOW);char text[4];int n=snprintf(text,sizeof(text),"%d\n",wanted);
 if(fd<0){note("BINDER_ABI_SELECT_OPEN_FAILED errno=%d",errno);return -1;}
 ssize_t wrote=write(fd,text,(size_t)n);int saved=errno;close(fd);
 if(wrote!=n){note("BINDER_ABI_SELECT_WRITE_FAILED bytes=%ld errno=%d",(long)wrote,saved);return -1;}
 uint64_t got=0;int rc=num(p,&got);note("BINDER_ABI_SELECT_READBACK rc=%d value=%llu",rc,(unsigned long long)got);
 return !rc&&got==(uint64_t)wanted?0:-1;
}
static int binder_probe7(void){
 int fd=open("/dev/binder",O_RDWR|O_CLOEXEC|O_NOFOLLOW);if(fd<0)return -1;
 struct binder_version v={0};struct binder_wr7 w={0};
 int ok=!ioctl(fd,_IOWR('b',9,struct binder_version),&v)&&v.protocol_version==7&&!ioctl(fd,_IOWR('b',1,struct binder_wr7),&w);
 void *m=MAP_FAILED;if(ok){m=mmap(NULL,65536,PROT_READ,MAP_PRIVATE|MAP_NORESERVE,fd,0);ok=m!=MAP_FAILED;}
 if(m!=MAP_FAILED)munmap(m,65536);close(fd);note("BINDER_CANONICAL protocol=7 zero_BWR24_and_mmap=%s",ok?"PASS":"FAIL");return ok?0:-1;
}
static int range7(void){
 int f=open("/dev/block/mmcblk0",O_RDONLY|O_CLOEXEC|O_NOFOLLOW),l=-1;struct stat s;uint64_t size=0;int attached=0,rc=-1;unsigned char magic[4];
 if(f<0)return -1;
 if(fstat(f,&s)||!S_ISBLK(s.st_mode)||s.st_rdev!=makedev(179,0)||ioctl(f,BLKSIZE64,&size)||(size<CAP_MIN||size>CAP_MAX)||exact_read(f,magic,4,2147483648)||memcmp(magic,"hsqs",4))goto out;
 l=open("/dev/loop1",O_RDWR|O_CLOEXEC|O_NOFOLLOW);if(l<0)goto out;struct loop_info64 info={0},check={0};errno=0;
 if(ioctl(l,LOOP_GET_STATUS64,&check)==0||errno!=ENXIO||ioctl(l,LOOP_SET_FD,f))goto out;attached=1;
 info.lo_offset=UINT64_C(2147483648);info.lo_sizelimit=UINT64_C(536870912);info.lo_flags=LO_FLAGS_READ_ONLY;
 if(ioctl(l,LOOP_SET_STATUS64,&info)||ioctl(l,LOOP_GET_STATUS64,&check)||check.lo_offset!=info.lo_offset||check.lo_sizelimit!=info.lo_sizelimit||!(check.lo_flags&LO_FLAGS_READ_ONLY))goto out;rc=0;
 out:if(rc&&attached)ioctl(l,LOOP_CLR_FD,0);if(l>=0)close(l);close(f);return rc;
}
static int root7_valid(char archive[256]){
 snprintf(archive,256,"%s/.config/ga36-android7-boot/android7-root.tar",root);int fd=open(archive,O_RDONLY|O_CLOEXEC|O_NOFOLLOW);struct stat st;
 if(fd<0)return -1;int ok=!fstat(fd,&st)&&S_ISREG(st.st_mode)&&st.st_uid==0&&st.st_nlink==1&&st.st_size>0&&st.st_size<16*1024*1024;
 struct sha256 h;sha_init(&h);char b[8192],digest[65];ssize_t n;
 while(ok&&(n=read(fd,b,sizeof(b)))>0)sha_update(&h,b,(size_t)n);if(ok&&n<0)ok=0;close(fd);sha_hex(&h,digest);return ok&&!strcmp(digest,A7_ROOT_SHA)?0:-1;
}
static int mount_android7(void){
 char archive[256],gate[256];snprintf(gate,sizeof(gate),"%s/.config/ga36-kernel-d03-test/storage-P05.ready",root);int g=open(gate,O_RDONLY|O_CLOEXEC|O_NOFOLLOW);char token[32]={0};if(g<0)return -1;
 ssize_t n=read(g,token,sizeof(token)-1);close(g);if(n!=(ssize_t)(sizeof("GA36_P05_STORAGE_VERIFIED\n")-1)||memcmp(token,"GA36_P05_STORAGE_VERIFIED\n",sizeof("GA36_P05_STORAGE_VERIFIED\n")-1)||root7_valid(archive))return -1;
 if(make_dir("/androidroot",0755)||mount("tmpfs","/androidroot","tmpfs",0,"mode=755,size=24m"))return -1;
 if(run_child("/sbin/busybox",(char*const[]){"busybox","tar","-xf",archive,"-C","/androidroot",NULL}))return -1;
 if(make_dir("/dev/block",0755))return -1;
 if(mknod("/dev/block/mmcblk0",S_IFBLK|0600,makedev(179,0))&&errno!=EEXIST)return -1;
 for(unsigned i=1;i<=3;i++)if(make_block(i))return -1;
 if(range7()||mount("/dev/loop1","/androidroot/system","squashfs",MS_RDONLY,NULL))return -1;
 const char *pairs[][2]={{"ga36-hw","system/lib/hw"},{"ga36-keylayout","system/usr/keylayout"},{"ga36-idc","system/usr/idc"}};
 for(unsigned i=0;i<3;i++){char a[128],b[128];snprintf(a,sizeof(a),"/androidroot/%s",pairs[i][0]);snprintf(b,sizeof(b),"/androidroot/%s",pairs[i][1]);if(mount(a,b,NULL,MS_BIND,NULL))return -1;}
 if(mount("tmpfs","/androidroot/cache","tmpfs",0,"mode=770,size=16m")||make_dir("/ga36-container",0700)||attach_pool()||mount("/dev/loop2","/ga36-container","ext4",MS_NOATIME|MS_NOSUID|MS_NODEV|MS_NOEXEC,NULL))return -1;
 if(run_child("/sbin/data-loop",(char*const[]){"data-loop","--attach-android7",NULL})||mount("/dev/loop3","/androidroot/data","ext4",MS_NOATIME|MS_NOSUID|MS_NODEV,NULL))return -1;
 struct stat st;if(stat("/androidroot/data/ga36-memory",&st)||!S_ISDIR(st.st_mode)||mount("/ga36-container/ga36-shared-swap","/androidroot/data/ga36-memory",NULL,MS_BIND,NULL)||umount2("/ga36-container",MNT_DETACH))return -1;
 return 0;
}
static int handoff_android7(void){
 for(const char **p=(const char*[]){"proc","sys","dev",NULL};*p;p++){char a[32],b[64];snprintf(a,sizeof(a),"/%s",*p);snprintf(b,sizeof(b),"/androidroot/%s",*p);if(mount(a,b,NULL,MS_MOVE,NULL))return -1;}
 note("DIRECT_ANDROID7_HANDOFF protocol7 no_boot_write=1 no_selection_reboot=1");
 if(logfd>=0)fsync(logfd);sync();if(umount2(root,MNT_DETACH)||move_android_root())return -1;
 setenv("PATH","/system/bin:/system/xbin",1);execl("/init","/init",(char*)NULL);return -1;
}
static int run_menu(void){
 pid_t p=fork();if(p<0)return 0;if(!p){execl("/sbin/preboot-menu","preboot-menu",(char*)NULL);_exit(127);}
 int status;while(waitpid(p,&status,0)<0)if(errno!=EINTR)return 0;
 return WIFEXITED(status)&&WEXITSTATUS(status)==42?42:0;
}
static int valid_common_kernel(const struct utsname *u){
 if(strcmp(u->release,"3.4.39")||!strstr(u->version,"#82 "))return 0;
 char b[1024];int fd=open("/proc/version",O_RDONLY|O_CLOEXEC);if(fd<0)return 0;ssize_t n=read(fd,b,sizeof(b)-1);close(fd);if(n<=0)return 0;b[n]=0;return strstr(b,"ga36@ga36-v1-k11")!=NULL;
}
int main(int argc,char **argv){
 if(argc==3&&!strcmp(argv[1],"--hash-file"))return hash_file(argv[2]);
 if(argc==2&&!strcmp(argv[1],"--selftest")){assert(sizeof(struct binder_wr7)==24&&sizeof(struct binder_wr)==48);puts("PASS V1 Binder7/8 wire sizes and native helper linked; hardware not exercised");return 0;}
 if(argc!=1||getpid()!=1||geteuid()!=0){fputs("V1 direct bootstrap requires initial PID1/root\n",stderr);return 64;}
 struct utsname u;if(uname(&u)||strcmp(u.release,"3.4.39")||!strstr(u.version,"#82 "))fatal();
 make_dir("/proc",0555);make_dir("/sys",0555);make_dir("/dev",0755);make_dir(root,0700);
 mount("proc","/proc","proc",MS_NOSUID|MS_NODEV|MS_NOEXEC,NULL);mount("sysfs","/sys","sysfs",MS_NOSUID|MS_NODEV|MS_NOEXEC,NULL);mount("devtmpfs","/dev","devtmpfs",MS_NOSUID,"mode=0755");
 /* proc must be mounted before checking the exact common-kernel provenance. */
 if(!valid_common_kernel(&u))fatal();unsigned t;for(t=0;t<30;t++){if(!identity())break;pause_ms(1000);}if(t==30)fatal();
 if(mount("/dev/mmcblk0p8",root,"ext4",MS_NOSUID|MS_NODEV|MS_NOEXEC|MS_NOATIME,NULL))fatal();
 note("V1_COMMON_KERNEL_MENU_ENTERED no_BOOT_rewrite=1 kernel=%s",u.version);
 for(const char **n=(const char*[]){"disp","lcd","mali","gpio-sunxi","udt_joystick",NULL};*n;n++){if(module(*n))fatal();if(!strcmp(*n,"lcd"))pause_ms(1000);}
 int status=run_menu();int android11=status==42;
 note("V1_MENU_SELECTION os=%s menu_status=%d same_kernel=1",android11?"11":"7",status);
 if(select_protocol(android11?8:7))fatal();
 /* The menu leaves the backlight off. Restore the selected Android11 image
  * once before userspace starts, as the former standalone bootstrap did. */
 if(android11){int shown=paint("android11-boot.rgb");note("ANDROID11_SELECTED_BOOT_IMAGE result=%d no_Android7_display_change=1",shown);}
 if(!android11){if(binder_probe7()){note("V1_STAGE_FAILED binder7 errno=%d",errno);fatal();}note("ANDROID7_MOUNT_BEGIN");if(mount_android7()){note("V1_STAGE_FAILED mount_android7 errno=%d",errno);fatal();}note("ANDROID7_MOUNT_READY");if(handoff_android7()){note("V1_STAGE_FAILED handoff_android7 errno=%d",errno);fatal();}}
 else {
  if(clear_legacy_exec_personality())fatal();
  for(const char **n=(const char*[]){"binder","hwbinder","vndbinder",NULL};*n;n++)if(binder_probe(*n))fatal();
  note("DIRECT_ANDROID11_HANDOFF protocol8 no_boot_write=1 no_selection_reboot=1");
  if(mount_runtime()||handoff())fatal();
 }
 return 1;
}
