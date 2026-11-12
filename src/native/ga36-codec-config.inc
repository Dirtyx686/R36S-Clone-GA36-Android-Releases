/* Preserve verified root-archive XML in its own small mount. Android remounts
 * its rootfs read-only before early-init; this mount must remain writable for
 * the standard SELinux restorecon, then init seals only this mount read-only.
 * No policy or permission exception and no persistent userdata are needed. */
static int prepare_codec_config_tmpfs(const char *directory) {
    static const char *names[] = {"media_codecs.xml", "media_codecs_google_audio.xml",
                                  "media_codecs_google_video.xml"};
    int input[3] = {-1,-1,-1}, output=-1, mounted=0, result=-1;
    struct stat original, st; struct statfs fs; off_t sizes[3];
    char pathbuf[512], buffer[4096]; size_t total=0;
    if(lstat(directory,&original)||!S_ISDIR(original.st_mode)||original.st_uid!=0||
       (original.st_mode&0777)!=0755){errno=EINVAL;return -1;}
    for(unsigned i=0;i<3;i++) {
        if(snprintf(pathbuf,sizeof(pathbuf),"%s/%s",directory,names[i])>=(int)sizeof(pathbuf)){
            errno=ENAMETOOLONG;goto done;
        }
        input[i]=open(pathbuf,O_RDONLY|O_CLOEXEC|O_NOFOLLOW);
        if(input[i]<0)goto done;
        if(fstat(input[i],&st)||!S_ISREG(st.st_mode)||st.st_uid!=0||st.st_gid!=0||
           st.st_nlink!=1||(st.st_mode&07777)!=0644||st.st_size<=0||st.st_size>65536){
            errno=EINVAL;goto done;
        }
        sizes[i]=st.st_size;total+=(size_t)st.st_size;
    }
    if(total>131072){errno=EFBIG;goto done;}
    if(mount("tmpfs",directory,"tmpfs",MS_NOSUID|MS_NODEV|MS_NOEXEC,
             "mode=0755,uid=0,gid=0,size=256k,nr_inodes=16"))goto done;
    mounted=1;
    if(stat(directory,&st)||st.st_dev==original.st_dev||statfs(directory,&fs)||
       fs.f_type!=0x01021994){errno=EINVAL;goto done;}
    for(unsigned i=0;i<3;i++) {
        snprintf(pathbuf,sizeof(pathbuf),"%s/%s",directory,names[i]);
        output=open(pathbuf,O_WRONLY|O_CREAT|O_EXCL|O_CLOEXEC|O_NOFOLLOW,0600);
        if(output<0)goto done;
        for(off_t at=0;at<sizes[i];) {
            size_t count=(size_t)(sizes[i]-at);if(count>sizeof(buffer))count=sizeof(buffer);
            ssize_t n=pread(input[i],buffer,count,at);
            if(n!=(ssize_t)count){if(n>=0)errno=EIO;goto done;}
            size_t written=0;
            while(written<count) {
                n=write(output,buffer+written,count-written);
                if(n<0&&errno==EINTR)continue;
                if(n<=0){if(n==0)errno=EIO;goto done;}
                written+=(size_t)n;
            }
            at+=(off_t)count;
        }
        if(fchmod(output,0644)||fsync(output)||fstat(output,&st)||st.st_size!=sizes[i])goto done;
        close(output);output=-1;
    }
    result=0;
done:
    {int saved=errno;
     if(output>=0)close(output);
     for(unsigned i=0;i<3;i++)if(input[i]>=0)close(input[i]);
     if(result&&mounted)umount(directory);
     errno=saved;}
    return result;
}

/* Keep this child mount visible after the immutable /vendor/odm view replaces
 * its original parent. Check the real mount and file identity after every move. */
static int move_codec_config_tmpfs(const char *source,const char *target) {
    struct stat before,dest,after;struct statfs fs;
    static const char *names[]={"media_codecs.xml","media_codecs_google_audio.xml",
                                "media_codecs_google_video.xml"};
    char p[512];
    if(lstat(source,&before)||!S_ISDIR(before.st_mode)||before.st_uid!=0||
       lstat(target,&dest)||!S_ISDIR(dest.st_mode)||dest.st_uid!=0||
       (dest.st_mode&0022)||statfs(source,&fs)||fs.f_type!=0x01021994){errno=EINVAL;return -1;}
    for(unsigned i=0;i<3;i++){
        if(snprintf(p,sizeof(p),"%s/%s",source,names[i])>=(int)sizeof(p)||
           lstat(p,&after)||!S_ISREG(after.st_mode)||after.st_uid!=0||
           after.st_nlink!=1||(after.st_mode&07777)!=0644||after.st_dev!=before.st_dev){errno=EINVAL;return -1;}
    }
    if(mount(source,target,NULL,MS_MOVE,NULL))return -1;
    if(stat(target,&after)||after.st_dev!=before.st_dev||statfs(target,&fs)||
       fs.f_type!=0x01021994){errno=EINVAL;return -1;}
    return 0;
}
