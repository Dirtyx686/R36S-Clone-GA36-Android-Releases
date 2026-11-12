#define _GNU_SOURCE
#define _FILE_OFFSET_BITS 64
#include <errno.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/swap.h>
#include <sys/sysmacros.h>
#include <time.h>
#include <unistd.h>

#define ZRAM_BYTES (256ULL * 1024 * 1024)
#define SD_BYTES (4ULL * 1024 * 1024 * 1024)
#define PAGE_BYTES 4096
#define SD_BASE "/mnt/vendor/ga36-swap"
#define SD_FILE SD_BASE "/swapfile"
_Static_assert(sizeof(off_t) >= 8, "4GiB files require large-file support");

static const char *path(const char *logical, char out[1024]) {
#ifdef GA36_MEMORY_TEST
    const char *root=getenv("GA36_TEST_ROOT");
    if (!root || snprintf(out,1024,"%s%s",root,logical)>=1024) abort();
#else
    if (snprintf(out,1024,"%s",logical)>=1024) abort();
#endif
    return out;
}
static void note(const char *message) {
    struct timespec t; clock_gettime(CLOCK_MONOTONIC,&t);
    printf("uptime=%lld.%03ld %s\n",(long long)t.tv_sec,t.tv_nsec/1000000,message);
    fflush(stdout);
}
static int failure(const char *operation) {
    printf("FAILED %s errno=%d %s\n",operation,errno,strerror(errno));
    return -1;
}
static int read_text(const char *logical,char *out,size_t n) {
    char p[1024]; int fd=open(path(logical,p),O_RDONLY|O_CLOEXEC|O_NOFOLLOW);
    if(fd<0)return -1;
    ssize_t got=read(fd,out,n-1); close(fd);
    if(got<0)return -1;
    out[got]=0; return 0;
}
static int write_text(const char *logical,const char *value) {
    int flags=O_WRONLY|O_CLOEXEC|O_NOFOLLOW;
#ifdef GA36_MEMORY_TEST
    flags |= O_TRUNC;
#endif
    char p[1024];int fd=open(path(logical,p),flags);
    if(fd<0)return failure(logical);
    size_t n=strlen(value);ssize_t got=write(fd,value,n);int saved=errno;close(fd);errno=saved;
    return got==(ssize_t)n?0:failure(logical);
}
static void setup_io(void) {
    const char *name="/sys/block/mmcblk0/queue/scheduler";
    char text[256];
    if(read_text(name,text,sizeof(text))){note("SD_IO_PROFILE_UNAVAILABLE_PRESERVED");return;}
    if(strstr(text,"[deadline]")){note("SD_IO_DEADLINE_ALREADY_ACTIVE");return;}
    if(!strstr(text,"deadline")){note("SD_IO_DEADLINE_NOT_SUPPORTED_PRESERVED");return;}
    if(write_text(name,"deadline\n")){note("SD_IO_PROFILE_FAILED_MEMORY_CONTINUES");return;}
    if(!read_text(name,text,sizeof(text))&&(strstr(text,"[deadline]")||!strcmp(text,"deadline\n")))note("SD_IO_DEADLINE_ACTIVE");
    else note("SD_IO_PROFILE_READBACK_UNCONFIRMED");
}
static void put32(unsigned char *b,size_t pos,uint32_t v) {
    for(unsigned i=0;i<4;i++)b[pos+i]=(unsigned char)(v>>(8*i));
}
static uint32_t get32(const unsigned char *b,size_t pos) {
    return (uint32_t)b[pos]|(uint32_t)b[pos+1]<<8|(uint32_t)b[pos+2]<<16|(uint32_t)b[pos+3]<<24;
}
static void header(unsigned char b[PAGE_BYTES],uint64_t size,const char *label) {
    memset(b,0,PAGE_BYTES);put32(b,1024,1);put32(b,1028,(uint32_t)(size/PAGE_BYTES-1));
    memcpy(b+1052,label,strlen(label));memcpy(b+PAGE_BYTES-10,"SWAPSPACE2",10);
}
static int active(const char *logical,int zram) {
    char p[1024],line[1024],name[512],type[32];unsigned long long size,used;int prio;
    FILE *f=fopen(path("/proc/swaps",p),"r");if(!f)return -1;
    int found=0;
    while(fgets(line,sizeof(line),f)) {
        if(sscanf(line,"%511s %31s %llu %llu %d",name,type,&size,&used,&prio)!=5)continue;
        if(!strcmp(name,logical)||(zram&&(!strcmp(name,"/dev/zram0")||!strcmp(name,"/dev/block/zram0")))) {
            printf("ALREADY_ACTIVE %s size_kib=%llu used_kib=%llu priority=%d\n",name,size,used,prio);
            unsigned long long expected=(zram?ZRAM_BYTES:SD_BYTES)/1024-4;
            if(size!=expected || used>size || prio!=(zram?100:10)) {
                note("ACTIVE_SWAP_PROFILE_MISMATCH_PRESERVED");
                fclose(f);errno=EINVAL;return -1;
            }
            found=1;
        }
    }
    fclose(f);return found;
}
static int enable_swap(const char *logical,int priority) {
    char p[1024];
#ifdef GA36_MEMORY_TEST
    char events[1024];FILE *f=fopen(path("/events",events),"a");
    if(!f)return -1;
    fprintf(f,"swapon %s %d\n",logical,priority);fclose(f);
    if(getenv("GA36_TEST_SWAPON_FAIL")){errno=EPERM;return failure("swapon");}
    f=fopen(path("/proc/swaps",p),"a");if(!f)return -1;
    fprintf(f,"%s %s %llu 0 %d\n",logical,priority==100?"partition":"file",(priority==100?ZRAM_BYTES:SD_BYTES)/1024-4,priority);
    fclose(f);
#else
    if(swapon(path(logical,p),SWAP_FLAG_PREFER|priority)<0)return failure("swapon");
#endif
    printf("SWAPON_OK %s priority=%d\n",logical,priority);
    return active(logical,priority==100)==1?0:-1;
}
static int setup_zram(void) {
    int a=active("/dev/block/zram0",1);if(a<0)return failure("read /proc/swaps");
    if(a==1){note("ZRAM_EXISTING_PRESERVED");return 0;}
    char text[256];if(read_text("/sys/block/zram0/disksize",text,sizeof(text)))return failure("zram unavailable");
    if(strtoull(text,0,10)!=0){note("ZRAM_CONFIGURED_BUT_INACTIVE_PRESERVED");return -1;}
    if(read_text("/sys/block/zram0/dev",text,sizeof(text)))return failure("zram dev");
    unsigned ma,mi;if(sscanf(text,"%u:%u",&ma,&mi)!=2){errno=EINVAL;return failure("zram dev format");}
    char node[1024];path("/dev/block/zram0",node);struct stat s;
#ifndef GA36_MEMORY_TEST
    if(lstat(node,&s)<0) {
        if(errno!=ENOENT||mknod(node,S_IFBLK|0600,makedev(ma,mi)))return failure("zram node");
    }
    if(lstat(node,&s)||!S_ISBLK(s.st_mode)||s.st_rdev!=makedev(ma,mi)){errno=EINVAL;return failure("zram node mismatch");}
#else
    if(lstat(node,&s)||!S_ISREG(s.st_mode)){errno=EINVAL;return failure("test zram node");}
#endif
    if(write_text("/sys/block/zram0/disksize","268435456\n"))return -1;
    int fd=open(node,O_RDWR|O_CLOEXEC|O_NOFOLLOW);if(fd<0)return failure("open zram");
    unsigned char page[PAGE_BYTES],check[PAGE_BYTES];header(page,ZRAM_BYTES,"GA36-ZRAM-V67");
    int ok=pwrite(fd,page,PAGE_BYTES,0)==PAGE_BYTES&&pread(fd,check,PAGE_BYTES,0)==PAGE_BYTES&&!memcmp(page,check,PAGE_BYTES);
    close(fd);if(!ok){errno=EIO;return failure("zram swap header");}
    if(enable_swap("/dev/block/zram0",100))return -1;
    // Keep the kernel's existing balanced reclaim policy; avoid clustered reads
    // when swapping through compression or SD. Do not alter Android LMK limits.
    if(write_text("/proc/sys/vm/swappiness","60\n"))return -1;
    if(write_text("/proc/sys/vm/page-cluster","0\n"))return -1;
    note("ZRAM_READY logical_bytes=268435456");return 0;
}
static int setup_sd(void) {
    int a=active(SD_FILE,0);if(a<0)return failure("read /proc/swaps");
    if(a==1){note("SD_SWAP_EXISTING_PRESERVED");return 0;}
    char p[1024];int dir=open(path(SD_BASE,p),O_RDONLY|O_CLOEXEC|O_DIRECTORY|O_NOFOLLOW);
    if(dir<0)return failure("open private swap directory");
    struct stat d,s;int fd=-1,ok=0;
    if(fstat(dir,&d)||d.st_uid!=geteuid()||(d.st_mode&0777)!=0700){errno=EPERM;failure("private directory ownership/mode");goto done;}
    fd=openat(dir,"swapfile",O_RDONLY|O_CLOEXEC|O_NOFOLLOW);
    if(fd<0){failure("open prepared SD swap");goto done;}
    if(fstat(fd,&s)||!S_ISREG(s.st_mode)||s.st_uid!=geteuid()||(s.st_mode&0777)!=0600||s.st_nlink!=1||s.st_dev!=d.st_dev||(uint64_t)s.st_size!=SD_BYTES){errno=EINVAL;failure("SD swap identity/mode/size");goto done;}
#ifndef GA36_MEMORY_TEST
    if((uint64_t)s.st_blocks*512<SD_BYTES){errno=EINVAL;failure("SD swap has holes");goto done;}
#endif
    unsigned char page[PAGE_BYTES];
    if(pread(fd,page,PAGE_BYTES,0)!=PAGE_BYTES||memcmp(page+PAGE_BYTES-10,"SWAPSPACE2",10)||get32(page,1024)!=1||get32(page,1028)!=SD_BYTES/PAGE_BYTES-1||get32(page,1032)!=0||memcmp(page+1052,"GA36-SDSWAP-V67",15)){errno=EINVAL;failure("SD swap header/owner label");goto done;}
    // Kernel swapon performs the final extent/hole checks. Never format, resize,
    // zero, create or swapoff a data file during Android boot.
    ok=enable_swap(SD_FILE,10)==0;
done:
    if(fd>=0)close(fd);
    close(dir);
    if(ok)note("SD_SWAP_READY logical_bytes=4294967296");
    return ok?0:-1;
}
static void status(void) {
    char text[16384];const char *files[]={"/proc/meminfo","/proc/swaps","/proc/pressure/memory","/sys/block/zram0/disksize","/sys/block/zram0/orig_data_size","/sys/block/zram0/compr_data_size","/sys/block/zram0/mem_used_total","/proc/sys/vm/swappiness","/proc/sys/vm/page-cluster","/sys/block/mmcblk0/queue/scheduler","/sys/block/mmcblk0/queue/read_ahead_kb","/sys/devices/system/cpu/cpu0/cpufreq/scaling_governor"};
    note("MEMORY_SNAPSHOT");
    for(unsigned i=0;i<sizeof(files)/sizeof(*files);i++)if(!read_text(files[i],text,sizeof(text)))printf("%s\n%s\n",files[i],text);
    if(!read_text("/proc/vmstat",text,sizeof(text))) {
        char *save=0;
        for(char *s=strtok_r(text,"\n",&save);s;s=strtok_r(0,"\n",&save))if(!strncmp(s,"pswp",4)||!strncmp(s,"pgmajfault",10)||!strncmp(s,"allocstall",10))puts(s);
    }
    fflush(stdout);
}
int main(int argc,char **argv) {
    if(argc!=2){fprintf(stderr,"usage: ga36-memory --zram|--sd|--status|--self-test\n");return 2;}
    if(!strcmp(argv[1],"--self-test")) {
        unsigned char b[PAGE_BYTES];header(b,SD_BYTES,"GA36-SDSWAP-V67");
        if(sizeof(off_t)<8||get32(b,1024)!=1||get32(b,1028)!=1048575||memcmp(b+4086,"SWAPSPACE2",10))return 1;
        header(b,ZRAM_BYTES,"GA36-ZRAM-V67");if(get32(b,1028)!=65535)return 1;
        puts("MEMORY_ARM32_LARGEFILE_AND_SWAP_HEADERS_OK");return 0;
    }
    if(!strcmp(argv[1],"--status")){status();return 0;}
#ifndef GA36_MEMORY_TEST
    if(geteuid()!=0){errno=EPERM;return failure("root required")?1:1;}
#endif
    if(sysconf(_SC_PAGESIZE)!=PAGE_BYTES){errno=EINVAL;failure("page size");return 1;}
    const char *log=!strcmp(argv[1],"--zram")?"/ga36logs/memory-zram.txt":!strcmp(argv[1],"--sd")?"/ga36logs/memory-sd.txt":0;
    if(!log)return 2;
    char p[1024];int fd=open(path(log,p),O_WRONLY|O_CREAT|O_TRUNC|O_CLOEXEC|O_NOFOLLOW,0600);
    if(fd>=0){dup2(fd,1);dup2(fd,2);close(fd);}
    note("GA36_MEMORY_V67");
    if(!strcmp(argv[1],"--zram"))setup_io();
    int result=!strcmp(argv[1],"--zram")?setup_zram():setup_sd();status();
    return result?1:0;
}
