#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <inttypes.h>
#include <fcntl.h>
#include <string.h>
#include <sys/ioctl.h>
#include <errno.h>
#include "amd_hsmp.h"

enum METHODS {
    SET,
    GET
};

int main(int argc, char const *argv[]){
    if (argc < 2 || strcmp(argv[argc - 1], "--help") == 0) {
        printf("Usage: %s set/get <frequency>(optional) --debug(optional)\n", argv[0]);
        printf("  <frequency> : Boost frequency in MHz (required for set)\n");
        printf("  --debug     : Enable debug output, prints the actual response from the HSMP message\n");
        printf("Example:  - %s set 3500\n", argv[0]);
        printf("          - %s get\n", argv[0]);
        exit(1);
    }
    int file;
    // Open file exposed by kernel module
    file = open("/dev/hsmp", O_RDWR);
    if (file < 0) {
        printf("Could not open HSMP device. Are modules loaded ?\n");
        exit(1);
    }
    // Prepare HSMP message structure
    struct hsmp_message msg;
    msg.sock_ind = 0;
    for (int i = 0; i < 8; i++) {
        msg.args[i] = 0;
    }
    // Prepare for set or get
    enum METHODS method; // 0 for set, 1 for get
    if (strcmp(argv[1], "set") == 0) {
        method = SET;
        msg.msg_id = HSMP_SET_BOOST_LIMIT_SOCKET;
        msg.num_args = 1;
        msg.response_sz = 0; // No response expected for a SET operation
        if (argc < 3) {
            printf("Usage: %s set <frequency>\n", argv[0]);
            exit(1);
        }
        // Convert frequency argument to integer
        msg.args[0] = atoi(argv[2]);
    } else if (strcmp(argv[1], "get") == 0) {
        method = GET;
        msg.msg_id = HSMP_GET_BOOST_LIMIT;
        msg.num_args = 1;
        msg.response_sz = 1;
    } else {
        printf("Invalid command. Use 'set' or 'get'.\n");
        exit(1);
    }
    // Send ioctl command to the driver
    int ret = ioctl(file, HSMP_IOCTL_CMD, &msg);
    if (ret < 0) {
        printf("HSMP ioctl failed\n");
        // Do a pretty print of the error
        printf("Error: %s\n", strerror(errno));
        close(file);
        exit(1);
    }
    if (method == GET) {
        // Print the response
        printf("Current Boost Limit: %" PRIu32 " MHz\n", msg.args[0]);
    }
    // Debug option, only if the last argument is "debug"
    if (strcmp(argv[argc - 1], "--debug") == 0) {
        for (int i = 0; i < 8; i++) {
            printf("Response[%d]: %" PRIu32 "\n", i, msg.args[i]);
        }
    }
    close(file);
    return 0;
}
