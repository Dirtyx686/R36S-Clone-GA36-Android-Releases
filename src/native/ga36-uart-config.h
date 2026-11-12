/* GA36 analog MCU on UART1: raw9600 8N1. No data read or written here. */
#ifndef GA36_UART_CONFIG_H
#define GA36_UART_CONFIG_H
#include <termios.h>
static const char *ga36_uart_stage="not_started";
static int ga36_configure_mcu_fd(int fd) {
    struct termios t;
    ga36_uart_stage="tcgetattr";
    if(tcgetattr(fd,&t))return -1;
    cfmakeraw(&t);
    t.c_cflag&=~(CSIZE|PARENB|CSTOPB|CRTSCTS|HUPCL);
    t.c_cflag|=CS8|CLOCAL|CREAD;
    t.c_cc[VMIN]=1;t.c_cc[VTIME]=0;
    ga36_uart_stage="baud";
    if(cfsetispeed(&t,B9600)||cfsetospeed(&t,B9600))return -1;
    ga36_uart_stage="tcsetattr";
    return tcsetattr(fd,TCSANOW,&t);
}
#endif
