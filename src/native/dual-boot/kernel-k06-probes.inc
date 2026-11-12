/* Actual device probes; early PID1, before SELinux and real Android init. */
static int device_k06_new_api_probe(void) {
    unsigned char random[32];errno=0;
    if(syscall(384,random,sizeof(random),4)!=-1||errno!=EINVAL)return -1;
    long n=syscall(384,random,sizeof(random),1);
    if(n==(long)sizeof(random))note("K08_GETRANDOM_REAL_POOL_READY bytes=%ld",n);
    else if(n==-1&&errno==EAGAIN)note("K08_GETRANDOM_API_VALID_POOL_NOT_YET_READY legitimate_EAGAIN=1");
    else {note("K08_GETRANDOM_BAD_RESULT result=%ld errno=%d",n,errno);return -1;}
    int server=-1,client=-1,syncfd[2]={-1,-1},accepted=-1,status;pid_t child=-1;int result=-1;
    struct sockaddr_un a={0};a.sun_family=AF_UNIX;
    int length=snprintf(a.sun_path+1,sizeof(a.sun_path)-1,"ga36-k06-creds-%ld",(long)getpid());
    if(length<0||length>=(int)sizeof(a.sun_path)-1)return -1;
    socklen_t address_bytes=offsetof(struct sockaddr_un,sun_path)+1+length;
    server=socket(AF_UNIX,SOCK_SEQPACKET|SOCK_CLOEXEC,0);int on=1;
    if(server<0||setsockopt(server,SOL_SOCKET,SO_PASSCRED,&on,sizeof(on))||
       bind(server,(struct sockaddr*)&a,address_bytes)||listen(server,1)||pipe2(syncfd,O_CLOEXEC))goto out;
    child=fork();if(child<0)goto out;
    if(child==0){
        close(server);close(syncfd[1]);client=socket(AF_UNIX,SOCK_SEQPACKET|SOCK_CLOEXEC,0);
        if(client<0||connect(client,(struct sockaddr*)&a,address_bytes))_exit(111);
        char go;if(read(syncfd[0],&go,1)!=1)_exit(112);
        unsigned value=0x364b0616;if(send(client,&value,sizeof(value),0)!=(ssize_t)sizeof(value))_exit(113);
        close(client);_exit(0);
    }
    close(syncfd[0]);syncfd[0]=-1;struct pollfd ready={server,POLLIN,0};
    if(poll(&ready,1,5000)<=0)goto out;
    accepted=accept4(server,NULL,NULL,SOCK_CLOEXEC);on=0;socklen_t optlen=sizeof(on);
    if(accepted<0||getsockopt(accepted,SOL_SOCKET,SO_PASSCRED,&on,&optlen)||on!=1){note("K08_PASSCRED_NOT_INHERITED");goto out;}
    if(write(syncfd[1],"G",1)!=1)goto out;
    struct pollfd data={accepted,POLLIN,0};if(poll(&data,1,5000)<=0)goto out;
    unsigned value=0;union {struct cmsghdr align;char data[CMSG_SPACE(sizeof(struct ucred))];} control;
    memset(&control,0,sizeof(control));struct iovec iov={&value,sizeof(value)};
    struct msghdr msg={0};msg.msg_iov=&iov;msg.msg_iovlen=1;msg.msg_control=control.data;msg.msg_controllen=sizeof(control.data);
    if(recvmsg(accepted,&msg,0)!=(ssize_t)sizeof(value)||value!=0x364b0616||(msg.msg_flags&MSG_CTRUNC))goto out;
    int valid=0;
    for(struct cmsghdr *c=CMSG_FIRSTHDR(&msg);c;c=CMSG_NXTHDR(&msg,c))
        if(c->cmsg_level==SOL_SOCKET&&c->cmsg_type==SCM_CREDENTIALS&&c->cmsg_len>=CMSG_LEN(sizeof(struct ucred))){
            struct ucred *u=(struct ucred*)CMSG_DATA(c);valid=u->pid==child&&u->uid==getuid()&&u->gid==getgid();
        }
    if(!valid)goto out;
    if(wait_compat_child(child,&status,5)||!WIFEXITED(status)||WEXITSTATUS(status))goto out;
    child=-1;note("K08_AF_UNIX_PASSCRED_INHERITED_AND_REAL_SENDER_VERIFIED");result=0;
out:
    if(child>0){kill(child,SIGKILL);(void)waitpid(child,NULL,WNOHANG);}
    if(server>=0)close(server);if(accepted>=0)close(accepted);
    for(unsigned i=0;i<2;i++)if(syncfd[i]>=0)close(syncfd[i]);return result;
}
