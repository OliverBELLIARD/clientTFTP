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

#define BUF_SIZE 516

// Enable/disable debugging statements
#define DEBUG 1

#define TFTP_OCTET_MODE "octet"
#define TFTP_OPCODE_RRQ "01"
#define TFTP_OPCODE_WRQ "10"

void print(const char *message);
char *buildRRQRequest(const char *filename);

#endif //CLIENTTFTP_GETTFTP_H
