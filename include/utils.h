
#ifndef _UTILS_H
#define _UTILS_H

#include <unistd.h>
#include <fcntl.h>
#include <termios.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define FLAG       0x7E
#define ESC        0x7D
#define STUFF(x)   x ^ 0x20
#define BCC(x, y)  x ^ y
#define A_RCVR     0x01
#define A_SND      0x03
#define UAKN       0x07
#define DISC       0x0B


enum state {START, FLAG_RCV, A_RCV, C_RCV, BCC_OK, STOP};

void stateMachine(char byteReceived, enum state * currentState, unsigned char addressField, unsigned char controlField);

int sendInfoFrame(int fd, unsigned char adressField, unsigned char controlField);

int receiveInfoFrame(int fd, unsigned char adressField, unsigned char controlField);


#endif /*_UTILS_H*/