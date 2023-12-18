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
 * @param argc number of arguments
 * @param *argv[] array of arguments
 * @return int returns 0 if successfully terminated
 * Example of script call: ./gettftp localhost 1069 [filename]
 */
int main(int argc, char *argv[]) {
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
    int sfd, s;
    ssize_t nread;
    char buf[BUF_SIZE];

    // Obtain address(es) matching host/port
    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_INET;    // Allow IPv4
    hints.ai_socktype = SOCK_DGRAM; // Datagram socket
    hints.ai_flags = 0;
    hints.ai_protocol = IPPROTO_UDP; // UDP protocol

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

    // Send the file request to the server through an RRQ request
    char *rrq_request = buildRRQRequest(fileName);
    if (DEBUG) print("RRQ request written\n");

    ssize_t sent_bytes = sendto(sfd, rrq_request, strlen(rrq_request) + 1, 0, rp->ai_addr, rp->ai_addrlen);
    if (sent_bytes == -1) {
        // Error management
        perror("sendto");
        exit(EXIT_FAILURE);
    }
    if (DEBUG) print("RRQ request sent to server\n");

    // Receive the file content from the server
    FILE *file = fopen(fileName, "wb");
    if (file == NULL) {
        // Error management
        perror("fopen");
        exit(EXIT_FAILURE);
    }
    if (DEBUG) print("File to fill with requested content created at client\n");

    // Receive the initial response from the server
    char response[BUF_SIZE];
    struct sockaddr_storage client_addr;
    socklen_t client_addr_len = sizeof(client_addr);

    if (DEBUG) print("Receiving initial response from server\n");
    ssize_t nrecv = recvfrom(sfd, response, BUF_SIZE, 0, (struct sockaddr*)&client_addr, &client_addr_len);
    if (DEBUG) {
        print("Initial response received from server\n");
        printf("Bytes received: %zd\n", nrecv);
    }

    if (nrecv < 0) {
        // Error management
        perror("read");
        exit(EXIT_FAILURE);
    }
    if (nrecv == 0) {
        if (DEBUG) print("Server closed the connection\n");
    }

    if (DEBUG) print("Checking received response\n");
    // Check if the server acknowledged the request with options
    if (strncmp(response, "tftp2", 5) == 0) {
        if (DEBUG) print("Server acknowledged TFTP Protocol (Revision 2) request\n");

        // Handle additional negotiation or proceed with data transfer
        // ... (implement negotiation logic as needed)

        // Continue with reading file content
        while ((nrecv = read(sfd, response, BUF_SIZE)) > 0) {
            if (DEBUG) {
                print("Receiving file content from the server...\n");
                printf("Received %zd bytes\n", nrecv);
            }

            if (fwrite(response, sizeof(char), nrecv, file) != nrecv) {
                // Error management
                perror("fwrite");
                exit(EXIT_FAILURE);
            }
        }
    } else {
        // The server does not support TFTP Protocol (Revision 2)
        if (DEBUG) print("Server does not support TFTP Protocol (Revision 2)\n");

        // Handle the response as if it were a standard TFTP response
        // ... (implement standard TFTP response handling)
    }


    if (DEBUG) print("File successfully downloaded to the current directory\n");
    fclose(file); // Close the file descriptor
    close(sfd);     // Close the socket

    exit(EXIT_SUCCESS);
}

/**
 * @brief Prints the String passed using the write function.
 * Has error management.
 * @param string string to print.
 */
void print(const char *message) {
    if (write(STDOUT_FILENO, message, strlen(message)) == -1) { // Error management
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
char *buildRRQRequest(const char *filename) {
    // The size of the request is determined by the length of the opcode, filename, mode, option, and null terminators
    size_t request_size = strlen(TFTP_OPCODE_RRQ) + strlen(filename) + strlen(TFTP_OCTET_MODE) + strlen("tftp2") + 4;

    char *request = (char *)malloc(request_size);
    if (request == NULL) {
        perror("malloc");
        exit(EXIT_FAILURE);
    }

    sprintf(request, "%s%s0%s0%s0", TFTP_OPCODE_RRQ, filename, TFTP_OCTET_MODE, "tftp2");

    // Set opcode values
    request[0] = '0';
    request[1] = '1';

    // Null terminate the filename, mode, and option
    request[strlen(TFTP_OPCODE_RRQ) + strlen(filename)] = 0;
    request[strlen(TFTP_OPCODE_RRQ) + strlen(filename) + strlen(TFTP_OCTET_MODE)] = 0;
    request[request_size - 1] = 0;

    return request;
}
