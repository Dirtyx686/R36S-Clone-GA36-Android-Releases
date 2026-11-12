/* Device-only probes: child process credentials/namespaces; parent untouched.
 * These execute on K08 before real Android init. No success inferred on PC. */
struct ga36_cap_header {uint32_t version;int pid;};
struct ga36_cap_data {uint32_t effective,permitted,inheritable;};
static int compat_exec_check(void) {
    struct ga36_cap_header h={0x20080522,0};struct ga36_cap_data d[2]={{0}};
    return getuid()==65534&&prctl(47,1,0,0,0)==1&&
       syscall(SYS_capget,&h,d)==0&&d[0].effective==1&&d[0].permitted==1&&
       d[0].inheritable==1&&d[1].effective==0&&d[1].permitted==0&&d[1].inheritable==0?0:91;
}
static int compat_mmap_sample(void) {
    void *p=mmap(NULL,4096,PROT_READ|PROT_WRITE,MAP_PRIVATE|MAP_ANONYMOUS,-1,0);
    if(p==MAP_FAILED)return 92;
    uint32_t address=(uint32_t)(uintptr_t)p;
    int ok=write(1,&address,sizeof(address))==sizeof(address);munmap(p,4096);return ok?0:93;
}
static int wait_compat_child(pid_t child,int *status,unsigned seconds) {
    struct timespec start,now;clock_gettime(CLOCK_MONOTONIC,&start);
    for(;;){
        pid_t r=waitpid(child,status,WNOHANG);
        if(r==child)return 0;
        if(r<0&&errno!=EINTR)return -1;
        clock_gettime(CLOCK_MONOTONIC,&now);
        if(now.tv_sec-start.tv_sec>=seconds){kill(child,SIGKILL);(void)waitpid(child,status,WNOHANG);note("K08_PROBE_CHILD_TIMEOUT child=%ld",(long)child);errno=ETIMEDOUT;return -1;}
        pause_ms(10);
    }
}
static int device_kernel_compat_probe(void) {
    uint64_t bits;if(num("/proc/sys/vm/mmap_rnd_bits",&bits)||bits!=16)return -1;
    /* Exercise real min/max handler; invalid writes must preserve16. */
    for(const char **v=(const char*[]){"7\n","17\n",NULL};*v;v++){
        int f=open("/proc/sys/vm/mmap_rnd_bits",O_WRONLY|O_CLOEXEC);
        if(f<0)return -1;ssize_t n=write(f,*v,strlen(*v));close(f);
        if(n>=0||num("/proc/sys/vm/mmap_rnd_bits",&bits)||bits!=16)return -1;
    }
    note("K08_ASLR_SYSCTL_BITS16_AND_BOUNDS_OK");
    uint32_t smallest=UINT32_MAX,largest=0;
    for(unsigned i=0;i<8;i++){
        int pipefd[2];if(pipe2(pipefd,O_CLOEXEC))return -1;
        pid_t child=fork();if(child<0){close(pipefd[0]);close(pipefd[1]);return -1;}
        if(child==0){close(pipefd[0]);if(dup2(pipefd[1],1)<0)_exit(94);close(pipefd[1]);execl("/init","init","--compat-mmap-sample",(char*)NULL);_exit(95);}
        close(pipefd[1]);struct pollfd event={pipefd[0],POLLIN,0};
        if(poll(&event,1,5000)<=0){close(pipefd[0]);kill(child,SIGKILL);(void)waitpid(child,NULL,WNOHANG);return -1;}
        uint32_t address=0;ssize_t got=read(pipefd[0],&address,4);close(pipefd[0]);int status;
        if(wait_compat_child(child,&status,5))return -1;
        if(got!=4||!WIFEXITED(status)||WEXITSTATUS(status))return -1;
        if(address<smallest)smallest=address;if(address>largest)largest=address;
    }
    if(largest-smallest<=1048576)return -1;
    note("K08_REAL_EXEC_MMAP_SPREAD_OK samples=8 spread_bytes=%u",largest-smallest);
    pid_t child=fork();if(child<0)return -1;
    if(child==0){
        char before[64]={0},after[64]={0};int ns=open("/proc/self/ns/mnt",O_RDONLY|O_CLOEXEC);
        ssize_t a=readlink("/proc/self/ns/mnt",before,sizeof(before)-1);
        if(ns<0||a<=0||strncmp(before,"mnt:[",5))_exit(96);
        if(syscall(SYS_unshare,0x00020000))_exit(97);
        if(readlink("/proc/self/ns/mnt",after,sizeof(after)-1)<=0||!strcmp(before,after))_exit(98);
        if(syscall(SYS_setns,ns,0x00020000))_exit(99);close(ns);
        memset(after,0,sizeof(after));if(readlink("/proc/self/ns/mnt",after,sizeof(after)-1)<=0||strcmp(before,after))_exit(100);
        if(prctl(47,1,0,0,0)!=0||prctl(PR_CAPBSET_READ,37,0,0,0)!=1)_exit(101);
        errno=0;if(prctl(47,2,0,0,0)!=-1||errno!=EPERM)_exit(102);
        struct ga36_cap_header h={0x20080522,0};struct ga36_cap_data d[2]={{0}};
        if(syscall(SYS_capget,&h,d))_exit(103);d[0].inheritable|=1;
        if(syscall(SYS_capset,&h,d)||prctl(47,2,0,0,0)||prctl(47,1,0,0,0)!=1)_exit(104);
        d[0].inheritable=0;
        if(syscall(SYS_capset,&h,d)||prctl(47,1,0,0,0)!=0)_exit(105);
        if(prctl(PR_SET_KEEPCAPS,1,0,0,0)||setresuid(65534,65534,65534))_exit(106);
        if(prctl(47,1,0,0,0)!=0)_exit(107);
        memset(d,0,sizeof(d));d[0].permitted=1;d[0].inheritable=1;
        if(syscall(SYS_capset,&h,d)||prctl(47,2,0,0,0)||prctl(47,1,0,0,0)!=1)_exit(108);
        execl("/init","init","--compat-exec-check",(char*)NULL);_exit(109);
    }
    int status;if(wait_compat_child(child,&status,10))return -1;
    note("K08_NAMESPACE_AND_AMBIENT_EXEC_PROBE status=%d exit=%d signal=%d",status,WIFEXITED(status)?WEXITSTATUS(status):-1,WIFSIGNALED(status)?WTERMSIG(status):0);
    return WIFEXITED(status)&&WEXITSTATUS(status)==0?0:-1;
}
