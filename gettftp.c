//
// Created by oliver on 11/12/23.
//

#include "gettftp.h"

//
// CLIENT: GET FILE THROUGH TFTP
//

/**
 * @brief Downloads a requested file from a specified host through arguments
 * User guide for getaddrinfo in official manual at >>> man getaddrinfo
 * @param argc
 * @param argv
 * @return int returns 0 if successfully terminated
 */
int main(int argc, char *argv[])
{
    if (argc < 4) {
        // Error management
        fprintf(stderr, "Usage: %s host port filename\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    char *host = argv[1];
    char *port = argv[2];
    char *fileName = argv[3];

    struct addrinfo hints;
    struct addrinfo *result, *rp;
    int sfd, s, j;
    size_t len;
    ssize_t nread;
    char buf[BUF_SIZE];

    // Obtain address(es) matching host/port
    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_INET;    // Allow IPv4
    hints.ai_socktype = SOCK_DGRAM; // Datagram socket
    hints.ai_flags = 0;
    hints.ai_protocol = IPPROTO_UDP;          // Any protocol

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

        if (connect(sfd, rp->ai_addr, rp->ai_addrlen) != -1) {
            if (DEBUG) printf("Connection using this socket established:\nsai_family: %d\t| ai_socktype: %d\t| ai_protocol: %d\n",
                              rp->ai_family, rp->ai_socktype, rp->ai_protocol);
            break; // Success: no need to keep trying
        }

        close(sfd);
    }

    if (rp == NULL) {
        // Error management
        // No address succeeded
        fprintf(stderr, "Could not connect\n");
        exit(EXIT_FAILURE);
    }

    freeaddrinfo(result); // No longer needed

    if (DEBUG) print("Attempt to send filename to server\n");
    // Send the file to the server through an RRQ request
    if (write(sfd, getrrq(fileName), strlen(getrrq(fileName)) + 1) != strlen(fileName) + 1) {
        // Error management
        fprintf(stderr, "partial/failed write\n");
        exit(EXIT_FAILURE);
    }

    // Receive the file content from the server
    FILE *file = fopen(fileName, "w");
    if (file == NULL) {
        // Error management
        perror("fopen");
        exit(EXIT_FAILURE);
    }

    if (DEBUG) print("Attempt to write of requested file from server in client directory\n");
    while ((nread = read(sfd, buf, BUF_SIZE)) > 0) {
        if (DEBUG) print("Receiving file content from server...\n");
        if (fwrite(buf, sizeof(char), nread, file) != nread) {
            // Error management
            perror("fwrite");
            exit(EXIT_FAILURE);
        }
    }

    if (nread == -1) {
        // Error management
        perror("read");
        exit(EXIT_FAILURE);
    }

    if (DEBUG) print("File successfully downloaded to current directory\n");
    fclose(file);

    exit(EXIT_SUCCESS);
}

/**
 * @brief Prints the String passed using the write function.
 * Has error management.
 * @param string string to print.
 */
void print(char *string) {
    if (write(STDOUT_FILENO, string, strlen(string)) == -1) { // Error management
        perror("write");
        exit(EXIT_FAILURE);
    }
}

/**
 * @brief Builds an RRQ request to request a specified file
 * RRQ request packet composition:
 * 2 bytes     string    1 byte     string   1 byte
 *  ------------------------------------------------
 * | Opcode |  Filename  |   0  |    Mode    |   0  |
 *  ------------------------------------------------
 * @param filename file to request
 * @return RRQ request
 */
char * getrrq(char * filename) {
    char request[sizeof (RRQ_OPCODE_1) + sizeof (filename) + sizeof (RRQ_MODE) + 2];

    sprintf(request, "%s%s\0%s\0", RRQ_OPCODE_1, filename, RRQ_MODE);
    print(request);

    return request;
}