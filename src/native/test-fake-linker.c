#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
int main(int argc,char **argv){
    assert(argc==3&&!strcmp(argv[1],"--list")&&!strcmp(argv[2],"/system/bin/init"));
    if(access("/proof/linker-fail",F_OK)==0){fprintf(stderr,"CONTROLLED_LINKER_FAILURE\n");return 42;}
    puts("CONTROLLED_LINKER_SUCCESS without running Android code");return 0;
}
