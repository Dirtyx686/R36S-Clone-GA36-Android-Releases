/* Real K08 device probe: unlinked tmpfs file and dual RW/RX mappings, never RWX. */
static int device_k08_memfd_probe(void) {
    note("K08_MEMFD_PROBE_BEGIN unsealable_flags0_and1=1");
    errno=0;if(syscall(385,"ga36-k08",2)!=-1||errno!=EINVAL)return -1;
    errno=0;if(syscall(385,"ga36-k08",4)!=-1||errno!=EINVAL)return -1;
    int plain=(int)syscall(385,"ga36-k08-plain",0);if(plain<0)return -1;
    int flags=fcntl(plain,F_GETFD);close(plain);if(flags<0||(flags&FD_CLOEXEC))return -1;
    int fd=(int)syscall(385,"ga36-k08-jit",1);if(fd<0)return -1;
    unsigned char *rw=MAP_FAILED,*rx=MAP_FAILED;int result=-1;struct stat st;
    if(!(fcntl(fd,F_GETFD)&FD_CLOEXEC)||fstat(fd,&st)||!S_ISREG(st.st_mode)||st.st_nlink||st.st_size)goto out;
    if(fcntl(fd,1034,0)!=1)goto out;
    errno=0;if(fcntl(fd,1033,0)!=-1||errno!=EPERM)goto out;
    char path[64],name[256];snprintf(path,sizeof(path),"/proc/self/fd/%d",fd);ssize_t n=readlink(path,name,sizeof(name)-1);
    if(n<=0)goto out;name[n]=0;if(!strstr(name,"memfd:ga36-k08-jit"))goto out;
    note("K08_MEMFD_FILE_IDENTITY_AND_FLAGS_VERIFIED");
    if(ftruncate(fd,8192)||fstat(fd,&st)||st.st_size!=8192)goto out;
    rw=mmap(NULL,8192,PROT_READ|PROT_WRITE,MAP_SHARED,fd,0);
    rx=mmap(NULL,8192,PROT_READ|PROT_EXEC,MAP_SHARED,fd,0);
    if(rw==MAP_FAILED||rx==MAP_FAILED||rw==rx)goto out;
    static const unsigned char thumb_return42[]={0x2a,0x20,0x70,0x47};
    memcpy(rw,thumb_return42,sizeof(thumb_return42));memcpy(rw+4096,"GA36_REAL_SHARED",16);
    if(memcmp(rx,thumb_return42,4)||memcmp(rx+4096,"GA36_REAL_SHARED",16))goto out;
    note("K08_MEMFD_DISTINCT_SHARED_RW_RX_ALIASES_VERIFIED no_RWX=1");
    if(madvise(rw,8192,MADV_DONTFORK))goto out;
    if(syscall(0xf0002,rw,rw+4,0)||syscall(0xf0002,rx,rx+4,0))goto out;
    if(close(fd))goto out;fd=-1;
    note("K08_MEMFD_JIT_EXEC_BEGIN fd_closed_mapping_alive=1");
    int (*code)(void)=(int(*)(void))((uintptr_t)rx|1u);
    if(code()!=42||memcmp(rx+4096,"GA36_REAL_SHARED",16))goto out;
    note("K08_REAL_MEMFD_UNLINKED_SHARED_RW_RX_JIT_EXEC_AND_CLOEXEC_PASSED sealing_unsupported=1");result=0;
out:
    if(result)note("K08_MEMFD_PROBE_FAILED errno=%d",errno);
    if(rw!=MAP_FAILED)munmap(rw,8192);if(rx!=MAP_FAILED)munmap(rx,8192);if(fd>=0)close(fd);return result;
}
