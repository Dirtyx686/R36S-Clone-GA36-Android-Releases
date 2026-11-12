#define _GNU_SOURCE
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <linux/fb.h>
#include <poll.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/klog.h>
#include <sys/mman.h>
#include <sys/mount.h>
#include <sys/prctl.h>
#include <sys/personality.h>
#include <sys/wait.h>
#include <sys/resource.h>
#include <signal.h>
#include <sys/reboot.h>
#include <sys/stat.h>
#include <sys/vfs.h>
#include <sys/statvfs.h>
#include <sys/syscall.h>
#include <sys/sysmacros.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <stddef.h>
#include <sys/utsname.h>
#include <time.h>
#include <unistd.h>
#include <sys/xattr.h>
#include <linux/loop.h>
#include "sha256.h"
#include "config.h"
#include "ga36-uart-config.h"
#define BOOT_BYTES (32u*1024u*1024u)
#define CAP_MIN UINT64_C(63909658624)
#define CAP_MAX UINT64_C(2199023255552)
#define BLKSIZE64 _IOR(0x12,114,size_t)
#define UUID_BYTES "\x58\x3a\xf6\xfd\x04\x39\x41\x17\x8b\x47\x1a\xb3\x06\x1d\x02\xbf"
static int logfd=-1;static const char *root="/ga36boot";
static void pause_ms(unsigned ms){struct timespec t={ms/1000,(ms%1000)*1000000L};while(nanosleep(&t,&t)&&errno==EINTR){}}
static void note(const char *fmt,...){(void)fmt;}
static int exact_read(int fd,void *data,size_t n,off_t off){unsigned char *p=data;while(n){ssize_t got=pread(fd,p,n,off);if(got<0&&errno==EINTR)continue;if(got<=0)return -1;p+=got;off+=got;n-=(size_t)got;}return 0;}
static int num(const char *path,uint64_t *out){char s[64];int fd=open(path,O_RDONLY|O_CLOEXEC);if(fd<0)return -1;ssize_t n=read(fd,s,sizeof(s)-1);close(fd);if(n<=0)return -1;s[n]=0;char *end;errno=0;*out=strtoull(s,&end,10);if(errno||end==s)return -1;while(*end=='\n'||*end==' ')end++;return *end?-1:0;}
static int identity(void){uint64_t a,b,c;unsigned char super[1024];struct stat st;int fd=-1;int ok=0;
 if(num("/sys/class/block/mmcblk0/size",&a)||(a<CAP_MIN/512||a>CAP_MAX/512)||num("/sys/class/block/mmcblk0p6/start",&b)||b!=172032||num("/sys/class/block/mmcblk0p6/size",&c)||c!=65536||num("/sys/class/block/mmcblk0p8/start",&b)||b!=1286144||num("/sys/class/block/mmcblk0p8/size",&c)||c!=2097192)return -1;
 fd=open("/dev/mmcblk0p8",O_RDONLY|O_CLOEXEC|O_NOFOLLOW);if(fd>=0){ok=!fstat(fd,&st)&&S_ISBLK(st.st_mode)&&major(st.st_rdev)==179&&minor(st.st_rdev)==8&&!ioctl(fd,BLKSIZE64,&a)&&a==1073762304&&!exact_read(fd,super,sizeof(super),1024)&&super[56]==0x53&&super[57]==0xef&&!memcmp(super+104,UUID_BYTES,16)&&!memcmp(super+120,"GA36START",9);close(fd);}return ok?0:-1;}
static void path(char out[256],const char *name){snprintf(out,256,"%s/.config/ga36-android11-p07/%s",root,name);}
static unsigned char *load(const char *name,size_t max,size_t *length){char p[256];path(p,name);int fd=open(p,O_RDONLY|O_CLOEXEC|O_NOFOLLOW);struct stat s;if(fd<0)return NULL;if(fstat(fd,&s)||!S_ISREG(s.st_mode)||s.st_uid!=0||s.st_size<=0||(uint64_t)s.st_size>max){close(fd);return NULL;}unsigned char *data=malloc((size_t)s.st_size);if(!data||exact_read(fd,data,(size_t)s.st_size,0)){free(data);close(fd);return NULL;}close(fd);*length=(size_t)s.st_size;return data;}
static void marker(void){char p[256];path(p,"return7.once");int fd=open(p,O_WRONLY|O_CREAT|O_TRUNC|O_CLOEXEC|O_NOFOLLOW,0600);if(fd>=0){const char token[]="V71_RETURN_ANDROID7\n";if(write(fd,token,sizeof(token)-1)==sizeof(token)-1)fsync(fd);close(fd);}}
static void clean_reboot(int returning7){note("CLEAN_REBOOT target=%s",returning7?"Android7_normal":"Android11_system_test");if(returning7)marker();if(logfd>=0){fsync(logfd);close(logfd);logfd=-1;}sync();int c=open("/dev/console",O_WRONLY);if(c>=0){dup2(c,1);dup2(c,2);if(c>2)close(c);}for(int fd=3;fd<1024;fd++)close(fd);
 (void)umount("/androidroot/system/lib/hw");(void)umount("/androidroot/system/usr/keylayout");(void)umount("/androidroot/system/usr/idc");(void)umount("/androidroot/system");(void)umount("/ga36-system-image");(void)umount("/androidroot/cache");(void)umount("/androidroot/data");(void)umount("/androidroot/ga36logs");(void)umount("/ga36boot");
 (void)umount("/androidroot/vendor");(void)umount("/androidroot/mnt/vendor/ga36-swap");(void)umount("/ga36-container");
 (void)mount(NULL,root,"ext4",MS_REMOUNT|MS_RDONLY,NULL);(void)umount(root);sync();reboot(RB_AUTOBOOT);for(;;)pause_ms(1000);}
static void fatal(void){int console=open("/dev/console",O_WRONLY|O_CLOEXEC);if(console>=0){dprintf(console,"GA36 boot failed; restore the saved boot package.\n");close(console);}if(logfd>=0)fsync(logfd);sync();for(;;)pause_ms(1000);}
static int module(const char *name){char file[100];snprintf(file,sizeof(file),"modules/%s.ko",name);size_t size;unsigned char *data=load(file,8*1024*1024,&size);if(!data){note("MODULE_UNAVAILABLE %s",name);return -1;}char expected_name[100];snprintf(expected_name,sizeof(expected_name),"modules/%s.sha256",name);size_t n;unsigned char *expected=load(expected_name,65,&n);char hash[65];hash_buffer(data,size,hash);if(!expected||n<64||memcmp(hash,expected,64)){free(data);free(expected);return -1;}free(expected);int ret=(int)syscall(SYS_init_module,data,size,"");int err=errno;free(data);note("MODULE %s result=%d errno=%d",name,ret,err);return ret;}
struct binder_version {int32_t protocol_version;};
struct binder_wr {uint64_t write_size,write_consumed,write_buffer,read_size,read_consumed,read_buffer;};
_Static_assert(sizeof(struct binder_wr)==48,"Binder8 ARM32 UAPI");
static int binder_probe(const char *name){char p[80];snprintf(p,sizeof(p),"/dev/%s",name);int fd=open(p,O_RDWR|O_CLOEXEC|O_NOFOLLOW);struct binder_version v={0};if(fd<0){note("BINDER_OPEN_FAILED %s errno=%d",name,errno);return -1;}int ok=!ioctl(fd,_IOWR('b',9,struct binder_version),&v)&&v.protocol_version==8;struct binder_wr w={0};if(ok)ok=!ioctl(fd,_IOWR('b',1,struct binder_wr),&w);void *m=MAP_FAILED;if(ok){m=mmap(NULL,65536,PROT_READ,MAP_PRIVATE|MAP_NORESERVE,fd,0);ok=m!=MAP_FAILED;}if(m!=MAP_FAILED)munmap(m,65536);note("BINDER %s protocol=%d struct_bytes=%u mmap_and_zero_BWR=%s",name,v.protocol_version,(unsigned)sizeof(w),ok?"PASS":"FAIL");close(fd);return ok?0:-1;}
static int paint(const char *name){int fb=open("/dev/fb0",O_RDWR|O_CLOEXEC|O_NOFOLLOW);if(fb<0)fb=open("/dev/graphics/fb0",O_RDWR|O_CLOEXEC|O_NOFOLLOW);if(fb<0)return -1;struct fb_var_screeninfo v;struct fb_fix_screeninfo f;
 if(ioctl(fb,FBIOGET_VSCREENINFO,&v)||ioctl(fb,FBIOGET_FSCREENINFO,&f)||v.xres!=640||v.yres!=480||v.bits_per_pixel!=32||v.xoffset||v.yres_virtual<480||v.yres_virtual>2048||v.yoffset>v.yres_virtual-480||f.line_length<2560||f.line_length%4||f.smem_len>16*1024*1024||(uint64_t)f.line_length*v.yres_virtual>f.smem_len||v.red.length!=8||v.green.length!=8||v.blue.length!=8||v.red.offset>24||v.green.offset>24||v.blue.offset>24){close(fb);return -1;}
 uint32_t bits=(255u<<v.red.offset)|(255u<<v.green.offset)|(255u<<v.blue.offset);if(__builtin_popcount(bits)!=24||(v.transp.length&& (v.transp.length!=8||v.transp.offset>24||(bits&(255u<<v.transp.offset))))){close(fb);return -1;}
 size_t n;unsigned char *rgb=load(name,640*480*3,&n);if(!rgb||n!=640*480*3){free(rgb);close(fb);return -1;}volatile unsigned char *mem=mmap(NULL,f.smem_len,PROT_READ|PROT_WRITE,MAP_SHARED,fb,0);if(mem==MAP_FAILED){free(rgb);close(fb);return -1;}
 for(unsigned y=0;y<480;y++){volatile uint32_t *row=(volatile uint32_t *)(mem+(y+v.yoffset)*(size_t)f.line_length);for(unsigned x=0;x<640;x++){unsigned char *p=rgb+(y*640+x)*3;row[x]=((uint32_t)p[0]<<v.red.offset)|((uint32_t)p[1]<<v.green.offset)|((uint32_t)p[2]<<v.blue.offset)|(v.transp.length?(255u<<v.transp.offset):0);}}
 __sync_synchronize();munmap((void*)mem,f.smem_len);free(rgb);close(fb);int bl=open("/sys/devices/virtual/disp/disp/attr/lcdbl",O_WRONLY|O_CLOEXEC);if(bl>=0){(void)write(bl,"128\n",4);close(bl);}return 0;}

static void reboot_to_normal(void){clean_reboot(1);}
static int profile_text(const char *release,const char *version,const char *proc,int diag){
 return !strcmp(release,"3.4.39")&&strstr(version,diag?"#80 ":"#72 ")&&
        strstr(proc,diag?"ga36@android11-k09":"ga36@android7-d04");
}
static int profile_valid(const struct utsname *u,int diag){
 char b[1024];int fd=open("/proc/version",O_RDONLY|O_CLOEXEC);if(fd<0)return 0;
 ssize_t n=read(fd,b,sizeof(b)-1);close(fd);if(n<=0)return 0;b[n]=0;
 return profile_text(u->release,u->version,b,diag);
}
static int hash_file(const char *p){int fd=open(p,O_RDONLY|O_CLOEXEC|O_NOFOLLOW);struct stat st;if(fd<0||fstat(fd,&st)||!S_ISREG(st.st_mode)||st.st_size>64*1024*1024)return 2;struct sha256 s;sha_init(&s);unsigned char b[4096];ssize_t n;while((n=read(fd,b,sizeof(b)))>0)sha_update(&s,b,(size_t)n);close(fd);if(n<0)return 2;char out[65];sha_hex(&s,out);puts(out);return 0;}
static int run_child(const char *program, char *const argv[]) {
    pid_t p=fork(); if(p<0)return -1;
    if(p==0){execv(program,argv);_exit(127);}
    int s; while(waitpid(p,&s,0)<0){if(errno!=EINTR)return -1;}
    return WIFEXITED(s)&&WEXITSTATUS(s)==0?0:-1;
}
static int make_dir(const char *p,mode_t mode) {
    if(mkdir(p,mode)&&errno!=EEXIST)return -1;
    struct stat st;
    return lstat(p,&st)||!S_ISDIR(st.st_mode)?-1:0;
}
static int make_block(unsigned minor_no) {
    char p[64];snprintf(p,sizeof(p),"/dev/loop%u",minor_no);
    if(mknod(p,S_IFBLK|0600,makedev(7,minor_no))&&errno!=EEXIST)return -1;
    struct stat st;
    if(lstat(p,&st)||!S_ISBLK(st.st_mode)||st.st_rdev!=makedev(7,minor_no))return -1;
    char canonical[64];snprintf(canonical,sizeof(canonical),"/dev/block/loop%u",minor_no);
    if(link(p,canonical)&&errno!=EEXIST)return -1;
    struct stat other;
    return lstat(canonical,&other)||!S_ISBLK(other.st_mode)||
           other.st_dev!=st.st_dev||other.st_ino!=st.st_ino?-1:0;
}
static int payload_ready(void) {
    size_t n;unsigned char *p=load("system-test.ready",128,&n);
    const char token[]="GA36_ANDROID11_P07_VERIFIED\n";
    int ok=p&&n==sizeof(token)-1&&!memcmp(p,token,n);free(p);
    return ok?0:-1;
}
static int archive_valid(void) {
    size_t n,hsize;unsigned char *p=load("android11-root.tar",16*1024*1024,&n);
    unsigned char *h=load("root.sha256",65,&hsize);char got[65];
    int ok=p&&h&&(hsize==64||hsize==65);
    if(ok){hash_buffer(p,n,got);ok=!memcmp(got,h,64);}
    free(h);free(p);return ok?0:-1;
}
static int attach_pool(void) {
    static const unsigned char uuid[16]={0xbe,0x7a,0xf2,0xcb,0x70,0x55,0x42,0x49,
        0xbd,0x58,0x4a,0x47,0xff,0x40,0x93,0x35};
    /* This exact inode is restored by real init's recursive /dev/block
     * restorecon. A separate /dev/mmcblk0 inode would retain the early label. */
    int f=open("/dev/block/mmcblk0",O_RDWR|O_CLOEXEC|O_NOFOLLOW),l=-1;
    struct stat st;uint64_t bytes=0;unsigned char sb[1024];int attached=0,ok=-1;
    const uint64_t offset=UINT64_C(3221225472);uint64_t length=0;
    if(f<0)return -1;
    if(fstat(f,&st)||!S_ISBLK(st.st_mode)||st.st_rdev!=makedev(179,0)||
       ioctl(f,BLKSIZE64,&bytes)||(bytes<CAP_MIN||bytes>CAP_MAX)||
       exact_read(f,sb,sizeof(sb),(off_t)(offset+1024))||
       sb[56]!=0x53||sb[57]!=0xef||memcmp(sb+104,uuid,16))goto out;
    l=open("/dev/loop2",O_RDWR|O_CLOEXEC|O_NOFOLLOW);if(l<0)goto out;
    struct loop_info64 info={0},check={0};errno=0;
    if(ioctl(l,LOOP_GET_STATUS64,&check)==0||errno!=ENXIO)goto out;
    if(ioctl(l,LOOP_SET_FD,f))goto out;attached=1;
    length=(bytes-offset-UINT64_C(16777216))&~UINT64_C(4095);
    info.lo_offset=offset;info.lo_sizelimit=length;
    if(ioctl(l,LOOP_SET_STATUS64,&info)||ioctl(l,LOOP_GET_STATUS64,&check)||
       check.lo_offset!=offset||check.lo_sizelimit!=length||
       (check.lo_flags&LO_FLAGS_READ_ONLY))goto out;
    if(sb[24]!=2||sb[25]||sb[26]||sb[27])goto out;
    uint64_t fsbytes=((uint64_t)sb[4]|(uint64_t)sb[5]<<8|(uint64_t)sb[6]<<16|(uint64_t)sb[7]<<24)*4096;
    if(fsbytes<UINT64_C(60671655936)||fsbytes>length)goto out;
    if(fsbytes<length){
        pid_t p=fork();int status;if(p<0)goto out;
        if(!p){execl("/sbin/e2fsck","e2fsck","-p","/dev/loop2",(char*)NULL);_exit(127);}
        while(waitpid(p,&status,0)<0)if(errno!=EINTR)goto out;
        if(!WIFEXITED(status)||WEXITSTATUS(status)>1)goto out;
        if(run_child("/sbin/resize2fs",(char*const[]){"resize2fs","/dev/loop2",NULL}))goto out;
        if(exact_read(l,sb,sizeof(sb),1024))goto out;
        uint64_t checkbytes=((uint64_t)sb[4]|(uint64_t)sb[5]<<8|(uint64_t)sb[6]<<16|(uint64_t)sb[7]<<24)*4096;
        if(checkbytes!=length)goto out;
    }
    ok=0;
out:
    if(ok&&attached)ioctl(l,LOOP_CLR_FD,0);
    if(l>=0)close(l);close(f);return ok;
}
static int bind_boot_views(const char *base) {
    /* First SELinux setup still runs in the kernel domain. Its normal
     * ramdisk symlink permissions do not cover this tmpfs root. Use the
     * actual immutable directories, retaining their original SELinux labels. */
    static const struct {const char *name,*source;int alias;} views[]={
        {"etc","/system/etc",1},{"product","/system/product",1},
        {"system_ext","/system/system_ext",1},{"odm","/vendor/odm",0}
    };
    for(unsigned i=0;i<sizeof(views)/sizeof(views[0]);i++){
        char target[256],source[256],link[128];struct stat st;
        if(snprintf(target,sizeof(target),"%s/%s",base,views[i].name)>=(int)sizeof(target)||
           snprintf(source,sizeof(source),"%s%s",base,views[i].source)>=(int)sizeof(source))return -1;
        if(lstat(source,&st)||!S_ISDIR(st.st_mode))return -1;
        if(views[i].alias){
            ssize_t n=readlink(target,link,sizeof(link)-1);
            if(n<0||n>=(ssize_t)sizeof(link)-1)return -1;link[n]=0;
            if(strcmp(link,views[i].source)||unlink(target)||make_dir(target,0755))return -1;
        }else if(lstat(target,&st)||!S_ISDIR(st.st_mode))return -1;
        if(mount(source,target,NULL,MS_BIND,NULL))return -1;
    }
    return 0;
}
static int clear_legacy_exec_personality(void) {
    int before=personality(0xffffffffUL);
    if(before<0)return -1;
    if((unsigned)before&READ_IMPLIES_EXEC){
        if(personality((unsigned long)((unsigned)before&~READ_IMPLIES_EXEC))<0)return -1;
    }
    int after=personality(0xffffffffUL);
    if(after<0||((unsigned)after&READ_IMPLIES_EXEC))return -1;
    note("NATIVE_EXEC_PERSONALITY before=0x%x after=0x%x no_read_implies_exec=1",before,after);
    return 0;
}
/* A single verified executable gets its own mount. The parent tmpfs keeps
 * NOSUID; this file must permit the standard init -> virtual_touchpad SELinux
 * domain transition. No setuid/setgid bits or untrusted writable file exists. */
static int bind_pointer_executable(const char *base) {
    char file[256];struct stat st;struct statvfs before,after;
    if(snprintf(file,sizeof(file),"%s/ga36-pointer",base)>=(int)sizeof(file))return -1;
    if(lstat(file,&st)||!S_ISREG(st.st_mode)||st.st_uid!=0||st.st_nlink!=1||
       (st.st_mode&07777)!=0755||statvfs(base,&before))return -1;
    if(mount(file,file,NULL,MS_BIND,NULL)||
       mount(NULL,file,NULL,MS_BIND|MS_REMOUNT|MS_NODEV,NULL)||statvfs(file,&after))return -1;
    if(after.f_flag&(ST_NOSUID|ST_NOEXEC))return -1;
    note("POINTER_EXEC_MOUNT_READY parent_nosuid=%d file_nosuid=0 root_owned_mode0755=1",!!(before.f_flag&ST_NOSUID));
    return 0;
}
static int move_android_root(void) {
    struct stat old,new;struct statfs fs;int saved;
    note("NATIVE_ROOT_CHECK_BEGIN pid=%ld statfs_struct_bytes=%zu",(long)getpid(),sizeof(fs));
    if(getpid()!=1){errno=EPERM;note("NATIVE_ROOT_REFUSED reason=not_PID1");return -1;}
    if(stat("/",&old)){saved=errno;note("NATIVE_ROOT_FAILED step=stat_old errno=%d",saved);errno=saved;return -1;}
    if(lstat("/androidroot",&new)){saved=errno;note("NATIVE_ROOT_FAILED step=stat_new errno=%d",saved);errno=saved;return -1;}
    note("NATIVE_ROOT_DEVICES old=%u:%u new=%u:%u new_mode=0%o",major(old.st_dev),minor(old.st_dev),major(new.st_dev),minor(new.st_dev),new.st_mode);
    if(!S_ISDIR(new.st_mode)||old.st_dev==new.st_dev){errno=EINVAL;note("NATIVE_ROOT_REFUSED reason=not_distinct_directory_mount");return -1;}
    if(statfs("/androidroot",&fs)){saved=errno;note("NATIVE_ROOT_FAILED step=statfs_new errno=%d",saved);errno=saved;return -1;}
    note("NATIVE_ROOT_FILESYSTEM magic=0x%lx wanted_tmpfs=0x1021994",(unsigned long)fs.f_type);
    if(fs.f_type!=0x01021994){errno=EINVAL;note("NATIVE_ROOT_REFUSED reason=not_tmpfs");return -1;}
    if(clear_legacy_exec_personality())return -1;
    /* Reclaim only fixed bootstrap helpers and menu assets from our own cpio.
     * No recursive deletion, mounted filesystems, or userdata are traversed. */
    const char *files[]={"/init","/sbin/busybox","/sbin/image-loops","/sbin/preboot-menu","/sbin/data-loop","/sbin/e2fsck","/sbin/resize2fs","/v1-menu/menu-11-locked.rgb","/v1-menu/menu-11-system.rgb","/v1-menu/menu-7-1.rgb","/v1-menu/menu-7-2.rgb","/v1-menu/menu-7-3.rgb","/v1-menu/menu-7-4.rgb","/v1-menu/menu-7-5.rgb","/v1-menu/menu-7-manual.rgb",NULL};
    for(unsigned i=0;files[i];i++){
        struct stat s;
        if(lstat(files[i],&s)){saved=errno;note("NATIVE_ROOT_FAILED step=stat_bootstrap file=%s errno=%d",files[i],saved);errno=saved;return -1;}
        note("NATIVE_ROOT_BOOTSTRAP file=%s dev=%u:%u mode=0%o uid=%u links=%lu",files[i],major(s.st_dev),minor(s.st_dev),s.st_mode,s.st_uid,(unsigned long)s.st_nlink);
        if(!S_ISREG(s.st_mode)||s.st_dev!=old.st_dev||s.st_uid!=0||s.st_nlink!=1){errno=EINVAL;note("NATIVE_ROOT_REFUSED reason=bootstrap_metadata file=%s",files[i]);return -1;}
    }
    for(unsigned i=0;files[i];i++)if(unlink(files[i])){saved=errno;note("NATIVE_ROOT_FAILED step=unlink_bootstrap file=%s errno=%d",files[i],saved);errno=saved;return -1;}
    if(chdir("/androidroot")){saved=errno;note("NATIVE_ROOT_FAILED step=chdir_new errno=%d",saved);errno=saved;return -1;}
    if(mount(".","/",NULL,MS_MOVE,NULL)){saved=errno;note("NATIVE_ROOT_FAILED step=move_mount errno=%d",saved);errno=saved;return -1;}
    if(chroot(".")){saved=errno;note("NATIVE_ROOT_FAILED step=chroot errno=%d",saved);errno=saved;return -1;}
    if(chdir("/")){saved=errno;note("NATIVE_ROOT_FAILED step=chdir_root errno=%d",saved);errno=saved;return -1;}
    note("NATIVE_ROOT_MOVE_COMPLETE");
    return 0;
}
/* Match AOSP first-stage /dev itself, not only selected device inodes.
 * devtmpfs uses SELinux filesystem SID device; properties and Unix sockets
 * require tmpfs. Rebuild BEFORE opening loop backing files so real init's
 * /dev/block restorecon labels the exact inodes held by the loop driver.
 * Leave hardware char nodes to ueventd coldboot: copying them with an early
 * tmpfs label would require extra relabel permissions that AOSP does not grant.
 */
static int prepare_standard_devices(const char *devbase) {
    static const struct {const char *name;unsigned maj,min;mode_t mode;} nodes[]={
        {"null",1,3,0666},{"kmsg",1,11,0600},
        {"random",1,8,0666},{"urandom",1,9,0666},{"ptmx",5,2,0666},
#if A11_WORLD_WRITABLE_KMSG
        {"kmsg_debug",1,11,0622}
#endif
    };
    char target[256];struct stat st;struct statfs fs;
    if(statfs(devbase,&fs)||fs.f_type!=0x01021994){errno=EINVAL;return -1;}
    for(unsigned i=0;i<sizeof(nodes)/sizeof(nodes[0]);i++){
        int n=snprintf(target,sizeof(target),"%s/%s",devbase,nodes[i].name);
        if(n<0||n>=(int)sizeof(target)){errno=ENAMETOOLONG;return -1;}
        if(lstat(target,&st)){
            if(errno!=ENOENT||mknod(target,S_IFCHR|nodes[i].mode,makedev(nodes[i].maj,nodes[i].min))||lstat(target,&st))return -1;
        }
        if(!S_ISCHR(st.st_mode)||st.st_rdev!=makedev(nodes[i].maj,nodes[i].min)){errno=EINVAL;return -1;}
        if(chmod(target,nodes[i].mode)||chown(target,0,0))return -1;
        note("EARLY_DEVICE_TMPFS_READY path=%s major=%u minor=%u mode=0%o",target,nodes[i].maj,nodes[i].min,nodes[i].mode);
    }
    return 0;
}
static int rebuild_early_dev(const char *devbase,const char *backup) {
    struct stat old,now;struct statfs fs;int saved;
    if(lstat(devbase,&old)||!S_ISDIR(old.st_mode)||lstat(backup,&now)==0||errno!=ENOENT){errno=EINVAL;return -1;}
    if(mkdir(backup,0700))return -1;
    if(mount(devbase,backup,NULL,MS_MOVE,NULL)){saved=errno;rmdir(backup);errno=saved;return -1;}
    if(mount("tmpfs",devbase,"tmpfs",MS_NOSUID,"mode=0755,uid=0,gid=0,size=32m,nr_inodes=8192"))goto rollback;
    if(stat(devbase,&now)||now.st_dev==old.st_dev||statfs(devbase,&fs)||fs.f_type!=0x01021994||
       prepare_standard_devices(devbase))goto rollback_new;
    if(umount2(backup,MNT_DETACH))goto rollback_new;
    if(rmdir(backup))return -1;
    note("EARLY_DEV_TMPFS_READY fresh_AOSP_device_namespace=1 hardware_nodes_by_ueventd_coldboot=1 before_loop_backing_open=1");
    return 0;
rollback_new:
    saved=errno;umount2(devbase,MNT_DETACH);errno=saved;
rollback:
    saved=errno;
    if(mount(backup,devbase,NULL,MS_MOVE,NULL)){
        note("EARLY_DEV_ROLLBACK_FAILED errno=%d",errno);return -1;
    }
    rmdir(backup);errno=saved;note("EARLY_DEV_PREPARATION_FAILED_OLD_MOUNT_RESTORED errno=%d",saved);return -1;
}
static int stdio_to_new_null(void) {
    int fd=open("/dev/null",O_RDWR|O_CLOEXEC|O_NOFOLLOW);
    if(fd<0)return -1;
    for(int i=0;i<3;i++)if(dup2(fd,i)<0){if(fd>2)close(fd);return -1;}
    if(fd>2)close(fd);
    note("EARLY_STDIO_NEW_TMPFS_NULL old_console_descriptors_released=1");return 0;
}
#include "ga36-codec-config.inc"
static int mount_runtime(void) {
    if(payload_ready()||archive_valid())return -1;
    if(make_dir("/androidroot",0755)||
       mount("tmpfs","/androidroot","tmpfs",MS_NOSUID,"mode=0755,size=24m"))return -1;
    char archive[256];path(archive,"android11-root.tar");
    if(run_child("/sbin/busybox",(char*const[]){"busybox","tar","-xf",archive,"-C","/androidroot",NULL}))return -1;
    if(prepare_codec_config_tmpfs("/androidroot/odm/etc")||
       make_dir("/ga36-codec-staging",0755)||
       move_codec_config_tmpfs("/androidroot/odm/etc","/ga36-codec-staging"))return -1;
    note("CODEC_CONFIG_TMPFS_STAGED before_immutable_ODM_bind=1");
    if(bind_pointer_executable("/androidroot"))return -1;
    if(rebuild_early_dev("/dev","/ga36-original-dev")||stdio_to_new_null())return -1;
    if(make_dir("/ga36-container",0700)||make_dir("/dev/block",0755))return -1;
    if(mknod("/dev/block/mmcblk0",S_IFBLK|0600,makedev(179,0))&&errno!=EEXIST)return -1;
    for(unsigned i=2;i<=6;i++)if(i!=3&&make_block(i))return -1;
    if(attach_pool()||mount("/dev/loop2","/ga36-container","ext4",
                           MS_NOATIME|MS_NOSUID|MS_NODEV|MS_NOEXEC,NULL))return -1;
    if(run_child("/sbin/image-loops",(char*const[]){"image-loops","--attach",NULL}))return -1;
    static const char *dirs[]={"system","vendor","data","proc","sys","dev","mnt",
        "ga36logs","apex","linkerconfig","debug_ramdisk",NULL};
    for(unsigned i=0;dirs[i];i++){
        char p[128];snprintf(p,sizeof(p),"/androidroot/%s",dirs[i]);if(make_dir(p,0755))return -1;
    }
    /* Android11's official build_image always wraps system under /system.
     * Keep the complete immutable image, expose exactly its system subtree
     * under the separate tmpfs root, and hide the temporary parent mount. */
    if(make_dir("/ga36-system-image",0700)||
       mount("/dev/loop4","/ga36-system-image","ext4",MS_RDONLY|MS_NODEV,NULL))return -1;
    struct stat system_dir;
    if(lstat("/ga36-system-image/system",&system_dir)||!S_ISDIR(system_dir.st_mode)||
       access("/ga36-system-image/system/bin/init",X_OK)||
       mount("/ga36-system-image/system","/androidroot/system",NULL,MS_BIND,NULL)||
       umount2("/ga36-system-image",MNT_DETACH))return -1;
    if(mount("/dev/loop5","/androidroot/vendor","ext4",MS_RDONLY|MS_NODEV,NULL)||
       mount("/dev/loop6","/androidroot/data","ext4",MS_NOATIME|MS_NOSUID|MS_NODEV,NULL))return -1;
    if(bind_boot_views("/androidroot"))return -1;
    if(move_codec_config_tmpfs("/ga36-codec-staging","/androidroot/odm/etc")||
       rmdir("/ga36-codec-staging"))return -1;
    note("CODEC_CONFIG_TMPFS_READY_AFTER_ODM_BIND standard_restorecon_before_readonly_seal=1");
    note("IMMUTABLE_BOOT_CONTEXT_VIEWS_READY count=4 no_tmpfs_symlink_policy_exception=1");
    if(access("/androidroot/system/bin/init",X_OK)||access("/androidroot/system/bin/bootstrap/linker",X_OK))return -1;
    /* Match Android11 first-stage mount environment before its real SELinux setup. */
    if(mount("tmpfs","/androidroot/mnt","tmpfs",MS_NOEXEC|MS_NOSUID|MS_NODEV,"mode=0755,uid=0,gid=1000")||
       make_dir("/androidroot/mnt/vendor",0755)||make_dir("/androidroot/mnt/product",0755)||
       make_dir("/androidroot/mnt/vendor/ga36-swap",0700)||
       mount("/ga36-container/ga36-shared-swap","/androidroot/mnt/vendor/ga36-swap",NULL,MS_BIND,NULL))return -1;
    if(umount2("/ga36-container",MNT_DETACH))return -1;
    if(mount("tmpfs","/androidroot/debug_ramdisk","tmpfs",MS_NOEXEC|MS_NOSUID|MS_NODEV,"mode=0755"))return -1;
    if(make_dir("/sys/fs/selinux",0755)||mount("selinuxfs","/sys/fs/selinux","selinuxfs",0,NULL))return -1;
    if(make_dir("/dev/pts",0755)||make_dir("/dev/socket",0755)||make_dir("/dev/block",0755))return -1;
    if(mount("devpts","/dev/pts","devpts",MS_NOSUID|MS_NOEXEC,NULL))return -1;
    chmod("/proc/cmdline",0440);
    return 0;
}
static int handoff(void) {
    char p[128];
    for(const char **s=(const char*[]){"proc","sys","dev",NULL};*s;s++){
        snprintf(p,sizeof(p),"/androidroot/%s",*s);
        char source[32];snprintf(source,sizeof(source),"/%s",*s);
        if(mount(source,p,NULL,MS_MOVE,NULL))return -1;
    }
    /* Run the already-tested standalone memory helper before loading SEPolicy.
     * Swap is optional: missing SD storage must not prevent Android from booting. */
    pid_t child=fork();if(child<0)return -1;
    if(child==0){
        if(chdir("/androidroot")||chroot(".")||chdir("/"))_exit(120);
        execl("/ga36-memory","ga36-memory","--zram",(char*)NULL);_exit(127);
    }
    int s;while(waitpid(child,&s,0)<0)if(errno!=EINTR)return -1;
    note("EARLY_ZRAM_CHILD status=%d",s);
    child=fork();if(child<0)return -1;
    if(child==0){
        if(chdir("/androidroot")||chroot(".")||chdir("/"))_exit(120);
        execl("/ga36-memory","ga36-memory","--sd",(char*)NULL);_exit(127);
    }
    while(waitpid(child,&s,0)<0)if(errno!=EINTR)return -1;
    note("EARLY_SD_SWAP_CHILD status=%d",s);
    if(unlink("/androidroot/ga36-memory"))return -1;
    if(clear_legacy_exec_personality())return -1;
    note("NATIVE_ANDROID11_ROOT_HANDOFF_BEGIN kernel=K11 handoff=native_P21 no_GUI_success_claimed=1");
    /* Complete filesystem changes before handing off to the real Android init. */
    if(logfd>=0)fsync(logfd);sync();
    if(umount2(root,MNT_DETACH)){int e=errno;note("NATIVE_ROOT_FAILED step=detach_startup errno=%d",e);errno=e;return -1;}
    note("NATIVE_STARTUP_MOUNT_DETACHED log_fd_preserved_until_exec=1");
    setenv("PATH","/system/bin:/system/xbin",1);
    if(move_android_root())return -1;
    note("REAL_ANDROID11_INIT_EXEC_BEGIN path=/system/bin/init argument=selinux_setup");
    execl("/system/bin/init","/system/bin/init","selinux_setup",(char*)NULL);
    {int e=errno;note("REAL_ANDROID11_INIT_EXEC_FAILED errno=%d",e);errno=e;}
    return -1;
}
static int selftest(void) {
    char h[65];hash_buffer("abc",3,h);
    assert(!strcmp(h,"ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad"));
    assert(profile_text("3.4.39","#72 SMP PREEMPT date","Linux version3.4.39 (ga36@android7-d04)",0));
    assert(profile_text("3.4.39","#80 SMP PREEMPT date","Linux version3.4.39 (ga36@android11-k09)",1));
    assert(!profile_text("3.4.39","#74 SMP PREEMPT date","Linux version3.4.39 (ga36@android11-k03)",1));
    assert(!profile_text("3.4.39","#80 SMP PREEMPT date","Linux version3.4.39 (ga36@android7-d04)",1));
    assert(sizeof(void*)==4&&BLKSIZE64==0x80041272&&sizeof(struct binder_wr)==48);
    puts("PASS Android11 runtime profile/SHA256/ARM32 ioctl guards; no device writes");return 0;
}
