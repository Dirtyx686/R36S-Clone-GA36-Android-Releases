#define _FILE_OFFSET_BITS 64
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include "sha256.h"
int main(int argc,char**argv){if(argc!=2)return 2;int fd=open(argv[1],O_RDONLY|O_NOFOLLOW);if(fd<0)return 2;struct sha256 h;sha_init(&h);unsigned char b[65536];ssize_t n;while((n=read(fd,b,sizeof(b)))!=0){if(n<0){if(errno==EINTR)continue;close(fd);return 2;}sha_update(&h,b,(size_t)n);}close(fd);char out[65];sha_hex(&h,out);puts(out);return 0;}
