#define _GNU_SOURCE
#include <assert.h>
#include <fcntl.h>
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/personality.h>
#include <sys/mman.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/stat.h>
#include <sys/sysmacros.h>
#include <sys/vfs.h>
#include <unistd.h>
#include "config.h"
int main(int argc,char **argv){
    assert(getpid()==1);
    assert(argc==2);errno=0;assert(fcntl(atoi(argv[1]),F_GETFD)==-1&&errno==EBADF);
    struct stat err;assert(fstat(2,&err)==0&&S_ISREG(err.st_mode));
    assert(fcntl(2,F_GETFL)&O_DSYNC);dprintf(2,"EARLY_STDERR_MARKER_PERSISTED\n");
    int p=personality(0xffffffffUL);assert(p>=0&&!(p&READ_IMPLIES_EXEC));
    struct stat root;assert(stat("/",&root)==0);
    const char *devs[]={"/dev/null","/dev/kmsg","/dev/random","/dev/urandom","/dev/ptmx",
#if A11_WORLD_WRITABLE_KMSG
        "/dev/kmsg_debug"
#endif
    };
    unsigned majors[]={1,1,1,1,5,1},minors[]={3,11,8,9,2,11};
    unsigned modes[]={0666,0600,0666,0666,0666,0622};
    for(unsigned i=0;i<sizeof(devs)/sizeof(devs[0]);i++){
        struct stat s;struct statfs fs;
        assert(lstat(devs[i],&s)==0&&S_ISCHR(s.st_mode));
        assert(s.st_rdev==makedev(majors[i],minors[i])&&s.st_dev!=root.st_dev);
        assert((s.st_mode&0777)==modes[i]);
        assert(statfs(devs[i],&fs)==0&&fs.f_type==0x01021994);
    }
    /* ueventd must create hardware nodes with final contexts after policy load,
     * not inherit pre-policy tmpfs-labelled hardware char inodes. */
    const char *absent[]={"/dev/console","/dev/fb0","/dev/mali","/dev/input/event0","/dev/snd/pcmC0D0p"};
    for(unsigned i=0;i<sizeof(absent)/sizeof(absent[0]);i++){
        struct stat s;errno=0;assert(lstat(absent[i],&s)==-1&&errno==ENOENT);
    }
    struct stat a,b;assert(lstat("/dev/loop7",&a)==0&&S_ISBLK(a.st_mode));
    assert(lstat("/dev/block/loop7",&b)==0&&a.st_dev==b.st_dev&&a.st_ino==b.st_ino&&a.st_nlink==2);
    assert(a.st_rdev==makedev(7,7));
    assert(mkdir("/dev/__properties__",0711)==0);
    int prop=open("/dev/__properties__/test_area",O_RDWR|O_CREAT|O_EXCL,0444);assert(prop>=0);
    assert(ftruncate(prop,131072)==0);
    void *area=mmap(NULL,131072,PROT_READ|PROT_WRITE,MAP_SHARED,prop,0);assert(area!=MAP_FAILED);
    memcpy(area,"GA36_PROPERTY_TMPFS_TEST",24);assert(msync(area,4096,MS_SYNC)==0);
    assert(munmap(area,131072)==0);close(prop);
    const char *sockets[]={"/dev/socket/property_service","/dev/socket/logd"};
    for(unsigned i=0;i<2;i++){
        int fd=socket(AF_UNIX,SOCK_STREAM|SOCK_CLOEXEC,0);assert(fd>=0);
        struct sockaddr_un addr={.sun_family=AF_UNIX};strcpy(addr.sun_path,sockets[i]);
        assert(bind(fd,(struct sockaddr*)&addr,sizeof(addr))==0);close(fd);
        struct statfs fs;assert(statfs(sockets[i],&fs)==0&&fs.f_type==0x01021994);
    }
    const char *names[]={"/etc","/product","/system_ext","/odm"};
    const char *sources[]={"/system/etc","/system/product","/system/system_ext","/vendor/odm"};
    for(unsigned i=0;i<4;i++){
        struct stat a,b;assert(lstat(names[i],&a)==0&&S_ISDIR(a.st_mode));
        assert(stat(sources[i],&b)==0&&a.st_dev==b.st_dev&&a.st_ino==b.st_ino);
    }
    int contexts=open("/etc/selinux/plat_file_contexts",O_RDONLY);assert(contexts>=0);close(contexts);
    FILE *f=fopen("/proof/result.json","w");assert(f);
    fprintf(f,"{\"result\":\"passed\",\"pid\":1,\"READ_IMPLIES_EXEC\":false,\"native_log_fd_closed_by_exec\":true,\"actual_A11_readonly_directory_views\":4,\"executed\":\"static_test_marker_only\"}\n");
    assert(fflush(f)==0&&fsync(fileno(f))==0&&fclose(f)==0);
    puts("PASS native PID1 root handoff, inherited exec flag cleared, four real A11 directory views; no Android init executed");
    return 0;
}
