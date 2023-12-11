//
// Created by oliver on 11/12/23.
//

#ifndef CLIENTTFTP_GETTFTP_H
#define CLIENTTFTP_GETTFTP_H

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#define BUF_SIZE 1024
#define DEBUG 1

#define RRQ_MODE "octet"
#define RRQ_OPCODE_1 "01"
#define RRQ_OPCODE_2 "10"

void print(char *);
char * getrrq(char *);

#endif //CLIENTTFTP_GETTFTP_H
