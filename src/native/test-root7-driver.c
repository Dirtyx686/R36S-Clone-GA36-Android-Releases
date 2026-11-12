#define main production_main
#include "dual-init.c"
#undef main
int main(void){
 assert(getpid()==1);
 logfd=open("/androidroot/proof/android7-handoff.log",O_WRONLY|O_CREAT|O_EXCL|O_CLOEXEC|O_DSYNC,0600);assert(logfd>=0);
 int before=personality(0xffffffffUL);assert(before>=0);assert(personality((unsigned)before|READ_IMPLIES_EXEC)>=0);
 handoff_android7();perror("handoff_android7");return 90;
}
