#define _GNU_SOURCE
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <linux/fb.h>
#include <linux/input.h>
#include <poll.h>
#include <signal.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/sysmacros.h>
#include <time.h>
#include <unistd.h>
#define ASSETS "/v1-menu/"
#define LOGFILE "/ga36boot/GA36-ANDROID7-BOOT/pre-android-menu.txt"
enum { W=640,H=480,RGB_BYTES=W*H*3,MAX_INPUTS=16 };
/* The board's Linux 3.4 evdev ABI uses 32-bit timeval, including with a
 * time64 userspace libc. Never use libc's struct timeval for this ABI. */
struct board_event { int32_t sec,usec; uint16_t type,code; int32_t value; };
_Static_assert(sizeof(struct board_event)==16,"Linux 3.4 ARM evdev ABI");
struct state { int selection,manual,done; int64_t deadline,last_input; };
static volatile sig_atomic_t stopped;
static FILE *log_file;
static int64_t now_ms(void) {
    struct timespec t;if(clock_gettime(CLOCK_MONOTONIC,&t))return -1;
    return (int64_t)t.tv_sec*1000+t.tv_nsec/1000000;
}
static void record(const char *fmt,...) {
    if(!log_file)return;
    fprintf(log_file,"uptime_ms=%lld ",(long long)now_ms());
    va_list ap;va_start(ap,fmt);vfprintf(log_file,fmt,ap);va_end(ap);
    fputc('\n',log_file);fflush(log_file);
}
static void stop_handler(int sig){(void)sig;stopped=1;}
static void backlight(int level) {
    int fd=open("/sys/devices/virtual/disp/disp/attr/lcdbl",O_WRONLY|O_CLOEXEC|O_NOFOLLOW);
    if(fd<0)return;
    const char *s=level?"128\n":"0\n";ssize_t n=write(fd,s,strlen(s));
    if(n!=(ssize_t)strlen(s))record("BACKLIGHT_WRITE_FAILED errno=%d",errno);
    close(fd);
}
static int geometry(const struct fb_var_screeninfo *v,const struct fb_fix_screeninfo *f) {
    if(v->xres!=W||v->yres!=H||v->bits_per_pixel!=32||v->xoffset||v->grayscale||v->nonstd||
       v->xres_virtual<W||v->yres_virtual<H||v->yres_virtual>2048||v->yoffset>v->yres_virtual-H||
       f->line_length<W*4||f->line_length%4||!f->smem_len||f->smem_len>16*1024*1024||
       (uint64_t)f->line_length*v->yres_virtual>f->smem_len)return 0;
    struct fb_bitfield fields[4]={v->red,v->green,v->blue,v->transp};uint32_t used=0;
    for(unsigned i=0;i<4;i++) {
        if(i==3&&!fields[i].length)continue;
        if(fields[i].length!=8||fields[i].offset>24||fields[i].msb_right)return 0;
        uint32_t bits=255u<<fields[i].offset;
        if(used&bits)return 0;used|=bits;
    }
    return 1;
}
static uint32_t packed(const unsigned char *p,const struct fb_var_screeninfo *v) {
    return ((uint32_t)p[0]<<v->red.offset)|((uint32_t)p[1]<<v->green.offset)|
           ((uint32_t)p[2]<<v->blue.offset)|(v->transp.length?(255u<<v->transp.offset):0);
}
static int read_exact(int fd,void *dst,size_t size) {
    unsigned char *p=dst;
    while(size){ssize_t n=read(fd,p,size);if(n<0&&errno==EINTR&&!stopped)continue;
        if(n<=0)return -1;p+=n;size-=(size_t)n;}
    return 0;
}
static int paint(int fb,volatile unsigned char *mem,const struct fb_fix_screeninfo *f,
                 unsigned char *rgb,const char *asset) {
    char path[256];struct stat st;struct fb_var_screeninfo v;
    if(snprintf(path,sizeof(path),ASSETS"%s.rgb",asset)>=(int)sizeof(path))return -1;
    int fd=open(path,O_RDONLY|O_CLOEXEC|O_NOFOLLOW);
    if(fd<0)return -1;
    int ok=!fstat(fd,&st)&&S_ISREG(st.st_mode)&&st.st_size==RGB_BYTES&&!read_exact(fd,rgb,RGB_BYTES);
    close(fd);
    if(!ok||ioctl(fb,FBIOGET_VSCREENINFO,&v)||!geometry(&v,f))return -1;
    for(unsigned y=0;y<H;y++) {
        volatile uint32_t *row=(volatile uint32_t *)(mem+(size_t)(y+v.yoffset)*f->line_length);
        for(unsigned x=0;x<W;x++)row[x]=packed(rgb+(y*W+x)*3,&v);
    }
    __sync_synchronize();return 0;
}
static int system_ready(void) {
    const char *p="/ga36boot/.config/ga36-android11-p07/system-test.ready";
    const char token[]="GA36_ANDROID11_P07_VERIFIED\n";
    char data[sizeof(token)]={0};struct stat st;
    int fd=open(p,O_RDONLY|O_CLOEXEC|O_NOFOLLOW);if(fd<0)return 0;
    int ok=!fstat(fd,&st)&&S_ISREG(st.st_mode)&&st.st_uid==0&&st.st_nlink==1&&
        (st.st_mode&0777)==0600&&st.st_size==sizeof(token)-1&&
        !read_exact(fd,data,sizeof(token)-1)&&!memcmp(data,token,sizeof(token)-1);
    close(fd);return ok;
}
static int selected_exit(int done,int ready){return done==2&&ready?42:0;}
/* A single policy is used by both real input and the host ARM tests.
 * Android11 selection requests the guarded first-system-boot helper only when ready. */
static int event(struct state *s,unsigned code,int value,int64_t t) {
    if(s->done||value!=1)return 0;
    int dir=code==KEY_LEFT||code==KEY_RIGHT||code==KEY_UP||code==KEY_DOWN||
            code==544||code==545||code==546||code==547;
    int confirm=code==BTN_A||code==BTN_START||code==KEY_ENTER;
    int back=code==BTN_B||code==KEY_ESC;
    s->manual=1;s->last_input=t;s->deadline=0;
    if(!dir&&!confirm&&!back)return 1;
    if(dir){s->selection=1-s->selection;return 1;}
    if(back){s->selection=0;return 1;}
    if(confirm){s->done=s->selection?2:1;return 2;}
    return 0;
}
static int tick(struct state *s,int64_t t) {
    if(!s->done && !s->manual && t>=s->deadline) {
        s->done=s->selection?2:1;return 1;
    }return 0;
}
static void selftest(void) {
    struct state s={0,0,0,5000,0};
    assert(!tick(&s,4999));assert(tick(&s,5000)&&s.done&&s.selection==0);
    s=(struct state){0,0,0,5000,0};assert(event(&s,547,1,100)==1&&s.selection==1&&s.manual);
    assert(!tick(&s,5001));
    assert(!event(&s,BTN_A,2,201)&&!s.done);assert(!event(&s,BTN_A,0,202));
    assert(event(&s,BTN_B,1,300)==1&&s.selection==0);
    assert(event(&s,KEY_ENTER,1,301)==2&&s.done&&s.selection==0);
    s=(struct state){1,1,0,0,100};assert(!tick(&s,5000));assert(!tick(&s,3600000)&&!s.done);
    s=(struct state){0,0,0,5000,0};assert(event(&s,BTN_X,1,400)==1&&s.manual&&!s.done);assert(!tick(&s,3600000));
    const unsigned keys[]={BTN_A,BTN_START,KEY_ENTER};
    for(unsigned i=0;i<sizeof(keys)/sizeof(*keys);i++) {
        s=(struct state){1,1,0,0,0};assert(event(&s,keys[i],1,1)==2&&s.done==2);
    }
    struct fb_var_screeninfo v={0};struct fb_fix_screeninfo f={0};
    v.xres=v.xres_virtual=W;v.yres=H;v.yres_virtual=H*2;v.yoffset=H;v.bits_per_pixel=32;
    v.red=(struct fb_bitfield){16,8,0};v.green=(struct fb_bitfield){8,8,0};
    v.blue=(struct fb_bitfield){0,8,0};v.transp=(struct fb_bitfield){24,8,0};
    f.line_length=W*4;f.smem_len=W*H*8;assert(geometry(&v,&f));
    unsigned char rgb[]={0x12,0x34,0x56};assert(packed(rgb,&v)==0xff123456);
    v.yoffset++;assert(!geometry(&v,&f));v.yoffset--;
    v.green.offset=v.red.offset;assert(!geometry(&v,&f));v.green.offset=8;
    v.transp.offset=16;assert(!geometry(&v,&f));v.transp.offset=24;
    f.smem_len--;assert(!geometry(&v,&f));f.smem_len++;
    v.xoffset=1;assert(!geometry(&v,&f));v.xoffset=0;
    assert(selected_exit(2,1)==42);
    assert(selected_exit(2,0)==0);
    assert(selected_exit(1,1)==0);
    assert(selected_exit(0,1)==0);
    puts("PASS: ARM32 evdev ABI, gated Android11 system exit42, safe default7, navigation, confirmation and framebuffer guards");
}
static int return_once(void){
 const char *path="/ga36boot/.config/ga36-android11-p07/return7.once";
 const char token[]="V71_RETURN_ANDROID7\n";char data[sizeof(token)]={0};struct stat st;
 int fd=open(path,O_RDONLY|O_CLOEXEC|O_NOFOLLOW);if(fd<0)return 0;
 int ok=!fstat(fd,&st)&&S_ISREG(st.st_mode)&&st.st_uid==0&&st.st_size==sizeof(token)-1&&
        !read_exact(fd,data,sizeof(token)-1)&&!memcmp(data,token,sizeof(token)-1);
 close(fd);return ok&&unlink(path)==0;
}
int main(int argc,char **argv) {
    if(argc==2&&!strcmp(argv[1],"--selftest")){selftest();return 0;}
    if(argc==2&&!strcmp(argv[1],"--check-ready"))return system_ready()?0:3;
    if(argc!=1)return 2;
    int lfd=open(LOGFILE,O_WRONLY|O_CREAT|O_TRUNC|O_CLOEXEC|O_NOFOLLOW,0600);
    if(lfd>=0)log_file=fdopen(lfd,"w");
    record("V1_PRE_ANDROID_ENTER android11=direct_same_kernel ready=%d",system_ready());
    if(return_once()){
        record("RETURN_FROM_TEST_CONTINUE_ANDROID7_WITHOUT_MENU");
        if(log_file){fflush(log_file);fsync(fileno(log_file));fclose(log_file);}
        return 0;
    }
    signal(SIGALRM,stop_handler);signal(SIGTERM,stop_handler);signal(SIGINT,stop_handler);/* No hidden auto-start after manual timer cancellation. */
    int ready=system_ready();
    int result=0;int fb=-1;volatile unsigned char *mem=MAP_FAILED;unsigned char *rgb=NULL;
    struct pollfd inputs[MAX_INPUTS];int count=0,shown=0;
    struct fb_var_screeninfo v;struct fb_fix_screeninfo f;struct stat st;
    const char *paths[]={"/dev/fb0","/dev/graphics/fb0"};
    for(unsigned i=0;i<2;i++) {
        fb=open(paths[i],O_RDWR|O_CLOEXEC|O_NOFOLLOW);
        if(fb>=0&&!fstat(fb,&st)&&S_ISCHR(st.st_mode)&&major(st.st_rdev)==29&&minor(st.st_rdev)==0&&
           !ioctl(fb,FBIOGET_VSCREENINFO,&v)&&!ioctl(fb,FBIOGET_FSCREENINFO,&f)&&geometry(&v,&f))break;
        if(fb>=0)close(fb);fb=-1;
    }
    if(fb<0){record("FALLBACK_ANDROID7 framebuffer_unavailable");goto done;}
    mem=mmap(NULL,f.smem_len,PROT_READ|PROT_WRITE,MAP_SHARED,fb,0);
    if(mem==MAP_FAILED){record("FALLBACK_ANDROID7 framebuffer_map_failed errno=%d",errno);goto done;}
    for(int i=0;i<32&&count<MAX_INPUTS;i++) {
        char path[64],name[128]={0};snprintf(path,sizeof(path),"/dev/input/event%d",i);
        int fd=open(path,O_RDONLY|O_NONBLOCK|O_CLOEXEC|O_NOFOLLOW);
        if(fd<0)continue;
        if(fstat(fd,&st)||!S_ISCHR(st.st_mode)||major(st.st_rdev)!=13||minor(st.st_rdev)!=(unsigned)(64+i)||
           ioctl(fd,EVIOCGNAME(sizeof(name)),name)<0||
           !(strstr(name,"gamepad")||strstr(name,"joystick")||strstr(name,"keyboard"))) {close(fd);continue;}
        inputs[count++]=(struct pollfd){fd,POLLIN,0};record("INPUT %s %s",path,name);
    }
    if(!count){record("FALLBACK_ANDROID7 no_input_device");goto done;}
    rgb=malloc(RGB_BYTES);if(!rgb){record("FALLBACK_ANDROID7 no_memory");goto done;}
    struct state s={0,0,0,now_ms()+5000,now_ms()};char previous[32]={0};
    if(s.deadline<5000){record("FALLBACK_ANDROID7 monotonic_clock_failed");goto done;}
    while(!stopped&&!s.done) {
        int64_t t=now_ms();if(t<0){record("FALLBACK_ANDROID7 clock_failed");break;}
        if(tick(&s,t)){record("MENU_TIMEOUT selection=%s after5seconds=1",s.selection?"android11":"android7");break;}
        char asset[32];
        if(s.selection)snprintf(asset,sizeof(asset),ready?"menu-11-system":"menu-11-locked");
        else if(s.manual)snprintf(asset,sizeof(asset),"menu-7-manual");
        else snprintf(asset,sizeof(asset),"menu-7-%lld",(long long)((s.deadline-t+999)/1000));
        if(strcmp(asset,previous)) {
            if(paint(fb,mem,&f,rgb,asset)){record("FALLBACK_ANDROID7 asset_or_frame_failed %s",asset);break;}
            strcpy(previous,asset);
            if(!shown){backlight(128);shown=1;record("MENU_SHOWN before_android_init=1 framebuffer=%ux%u",v.xres,v.yres);}
        }
        int p=poll(inputs,(nfds_t)count,50);
        if(p<0&&errno!=EINTR){record("FALLBACK_ANDROID7 poll_failed");break;}
        for(int i=0;i<count;i++)if(inputs[i].revents&POLLIN) {
            for(int n=0;n<64;n++) {
                struct board_event e;ssize_t got=read(inputs[i].fd,&e,sizeof(e));
                if(got<0&&(errno==EAGAIN||errno==EINTR))break;
                if(got!=(ssize_t)sizeof(e)){record("INPUT_READ_FAILED fd=%d size=%zd",inputs[i].fd,got);break;}
                if(e.type!=EV_KEY)continue;
                int action=event(&s,e.code,e.value,now_ms());
                if(action)record("KEY code=%u action=%d selection=%s",e.code,action,s.selection?"android11_system_test":"android7");
                if(action==2&&s.done==2)record("ANDROID11_SYSTEM_TEST_REQUESTED ready=%d",ready);
            }
        }
    }
    if(stopped)record("ANDROID7_DEFAULT reason=bounded_menu_timeout");
    result=selected_exit(s.done,system_ready());
    if(s.done==2&&!result)record("ANDROID11_NOT_READY_CONTINUE_ANDROID7");
    /* Do not preview the Android logo here. Android's own boot animation
     * starts after switch_root; a preview caused logo -> black -> logo. */
 done:
    if(shown)backlight(0);
    for(int i=0;i<count;i++)close(inputs[i].fd);
    free(rgb);if(mem!=MAP_FAILED)munmap((void*)mem,f.smem_len);if(fb>=0)close(fb);
    alarm(0);record(result==42?"ANDROID11_SELECTED direct_common_kernel=1 exit42 GUI_not_verified=1":"ANDROID7_SELECTED direct_common_kernel=1");
    if(log_file){fflush(log_file);fsync(fileno(log_file));fclose(log_file);}
    return result;
}
