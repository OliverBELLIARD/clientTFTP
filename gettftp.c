//
// Created by oliver on 11/12/23.
//

#include "gettftp.h"

//
// CLIENT: GET FILE THROUGH TFTP
//

/**
 * @brief Downloads a requested file from a specified host through arguments
 * @param argc
 * @param argv
 * @return int returns 0 if successfully terminated
 */
int main(int argc, char *argv[])
{
    char* host = argv[1];
    char* port = argv[2];
    char* fileName = argv[3];

    struct addrinfo hints;
    struct addrinfo *result, *rp;
    int sfd, s, j;
    size_t len;
    ssize_t nread;
    char buf[BUF_SIZE];

    if (argc < 3) {
        // Error management
        fprintf(stderr, "Usage: %s host port filename\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    // Obtain address(es) matching host/port

    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_UNSPEC;    // Allow IPv4 or IPv6
    hints.ai_socktype = SOCK_DGRAM; // Datagram socket
    hints.ai_flags = 0;
    hints.ai_protocol = 0;          // Any protocol

    s = getaddrinfo(host, port, &hints, &result);
    if (s != 0) {
        // Error management
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(s));
        exit(EXIT_FAILURE);
    }

    // getaddrinfo() returns a list of address structures.
    // Try each address until we successfully connect(2).
    // If socket(2) (or connect(2)) fails, we (close the socket and)
    // try the next address.

    for (rp = result; rp != NULL; rp = rp->ai_next) {
        sfd = socket(rp->ai_family, rp->ai_socktype,
                     rp->ai_protocol);
        if (sfd == -1)
            continue;

        printf("ai_family: %d\t| ai_socktype: %d\t| ai_protocol: %d\n",
               rp->ai_family, rp->ai_socktype, rp->ai_protocol);
        if (connect(sfd, rp->ai_addr, rp->ai_addrlen) != -1)
            break; // Success: no need to keep trying

        close(sfd);
    }

    if (rp == NULL) {
        // Error management
        // No address succeeded
        fprintf(stderr, "Could not connect\n");
        exit(EXIT_FAILURE);
    }

    freeaddrinfo(result);           // No longer needed

    // Send remaining command-line arguments as separate datagrams,
    // and read responses from server

    for (j = 3; j < argc; j++) {
        len = strlen(argv[j]) + 1;
        // +1 for terminating null byte

        if (len + 1 > BUF_SIZE) {
            // Error management
            fprintf(stderr,
                    "Ignoring long message in argument %d\n", j);
            continue;
        }

        if (write(sfd, argv[j], len) != len) {
            // Error management
            fprintf(stderr, "partial/failed write\n");
            exit(EXIT_FAILURE);
        }

        nread = read(sfd, buf, BUF_SIZE);
        if (nread == -1) {
            perror("read");
            exit(EXIT_FAILURE);
        }

        printf("Received %ld bytes: %s\n", (long) nread, buf);
    }

    exit(EXIT_SUCCESS);
}