/* Linux3.4 ARM ABI uses signed32-bit seconds/nanoseconds, even with time64 musl. */
struct timerfd_time32 {int32_t sec,nsec;};
struct timerfd_spec32 {struct timerfd_time32 interval,value;};
_Static_assert(sizeof(struct timerfd_spec32)==16,"ARM timerfd itimerspec ABI");
static int device_k07_timerfd_probe(void) {
    errno=0;if(syscall(350,-123,0)!=-1||errno!=EINVAL)return -1;
    errno=0;if(syscall(350,7,4)!=-1||errno!=EINVAL)return -1;
    const int clocks[]={0,1,7,8,9};
    for(unsigned i=0;i<sizeof(clocks)/sizeof(clocks[0]);i++){
        int test=(int)syscall(350,clocks[i],O_CLOEXEC|O_NONBLOCK);
        if(test<0){note("K08_TIMERFD_CLOCK_REJECTED clock=%d errno=%d",clocks[i],errno);return -1;}
        close(test);
    }
    int fd=(int)syscall(350,7,O_CLOEXEC|O_NONBLOCK);if(fd<0)return -1;int result=-1;
    if(!(fcntl(fd,F_GETFL)&O_NONBLOCK)||!(fcntl(fd,F_GETFD)&FD_CLOEXEC))goto out;
    struct timerfd_spec32 spec={{0,0},{0,100000000}},remaining;
    if(syscall(353,fd,0,&spec,NULL)||syscall(354,fd,&remaining))goto out;
    if(remaining.interval.sec||remaining.interval.nsec||remaining.value.sec||remaining.value.nsec<0||remaining.value.nsec>100000000)goto out;
    struct pollfd p={fd,POLLIN,0};uint64_t ticks=0;
    if(poll(&p,1,2000)!=1||read(fd,&ticks,sizeof(ticks))!=sizeof(ticks)||ticks!=1)goto out;
    struct timerfd_time32 now;if(syscall(263,7,&now))goto out;
    spec.value=now;spec.value.nsec+=100000000;if(spec.value.nsec>=1000000000){spec.value.nsec-=1000000000;spec.value.sec++;}
    if(syscall(353,fd,1,&spec,NULL))goto out;p.revents=0;ticks=0;
    if(poll(&p,1,2000)!=1||read(fd,&ticks,sizeof(ticks))!=sizeof(ticks)||ticks!=1)goto out;
    memset(&spec,0,sizeof(spec));if(syscall(353,fd,0,&spec,NULL)||syscall(354,fd,&remaining))goto out;
    if(remaining.value.sec||remaining.value.nsec||remaining.interval.sec||remaining.interval.nsec)goto out;
    note("K08_TIMERFD_ALL_ALARM_CLOCKS_AND_REAL_BOOTTIME_RELATIVE_ABSOLUTE_EXPIRY_PASSED");result=0;
out:
    if(result)note("K08_TIMERFD_PROBE_FAILED errno=%d",errno);close(fd);return result;
}
