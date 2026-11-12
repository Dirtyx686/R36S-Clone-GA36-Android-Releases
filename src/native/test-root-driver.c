#define main production_main
#include "runtime-init.c"
#undef main
int main(int argc,char **argv){
    if(argc==2&&!strcmp(argv[1],"--refuse-non-pid1")){
        assert(getpid()!=1);assert(move_android_root()==-1);
        puts("PASS non-PID1 root movement refused");return 0;
    }
    assert(getpid()==1);
    if(argc==2&&!strcmp(argv[1],"--reject-wrong-device")){
        logfd=open("/androidroot/proof/wrong-device.log",O_WRONLY|O_CREAT|O_EXCL|O_CLOEXEC|O_DSYNC,0600);assert(logfd>=0);
        assert(unlink("/androidroot/dev/urandom")==0);
        int bad=open("/androidroot/dev/urandom",O_WRONLY|O_CREAT|O_EXCL,0600);assert(bad>=0);close(bad);
        assert(prepare_standard_devices("/androidroot/dev")==-1);
        assert(access("/init",F_OK)==0);
        note("TEST_WRONG_DEVICE_TYPE_REFUSED_BEFORE_EXEC");fsync(logfd);close(logfd);return 0;
    }
    if(argc==2&&!strcmp(argv[1],"--reject-samefs")){
        logfd=open("/androidroot/proof/failure.log",O_WRONLY|O_CREAT|O_EXCL|O_CLOEXEC,0600);assert(logfd>=0);
        assert(mount("/","/androidroot",NULL,MS_BIND,NULL)==0);
        assert(move_android_root()==-1);
        assert(access("/init",F_OK)==0);
        note("TEST_CRITICAL_GUARD_REFUSED_WITH_LOG_PRESERVED");
        assert(fsync(logfd)==0);close(logfd);return 0;
    }
    int rejected=argc==2&&!strcmp(argv[1],"--probe-reject");
    logfd=open(rejected?"/androidroot/proof/probe-rejected.log":"/androidroot/proof/native.log",O_WRONLY|O_CREAT|O_EXCL|O_CLOEXEC|O_DSYNC,0600);assert(logfd>=0);
    int old=personality(0xffffffffUL);assert(old>=0);
    assert(personality((unsigned)old|READ_IMPLIES_EXEC)>=0);
    assert((unsigned)personality(0xffffffffUL)&READ_IMPLIES_EXEC);
    assert(rebuild_early_dev("/androidroot/dev","/fake-original-dev")==0);
    /* Canonical aliases are created only after the new /dev exists. */
    assert(make_dir("/androidroot/dev/block",0755)==0);
    assert(mknod("/androidroot/dev/loop7",S_IFBLK|0600,makedev(7,7))==0);
    assert(link("/androidroot/dev/loop7","/androidroot/dev/block/loop7")==0);
    assert(make_dir("/androidroot/dev/socket",0755)==0);
    assert(bind_boot_views("/androidroot")==0);
    assert(bind_pointer_executable("/androidroot")==0);
    assert(move_android_root()==0);
    assert(capture_early_init_stderr()==0);
    int probe=probe_real_linker();
    if(rejected){assert(probe==-1);note("TEST_FAILED_LINKER_RETURNED_WITHOUT_EXECUTING_INIT");fsync(logfd);return 0;}
    assert(probe==0);
    char inherited[32];snprintf(inherited,sizeof(inherited),"%d",logfd);
    execl("/system/bin/init","ga36-test-marker",inherited,(char*)NULL);
    perror("exec marker");return 10;
}
