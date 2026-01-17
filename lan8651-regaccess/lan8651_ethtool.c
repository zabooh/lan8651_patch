// SPDX-License-Identifier: GPL-2.0+
/*
 * LAN8651 Register Access via existing network interface
 * 
 * Uses ethtool private ioctls for register access
 * No need for separate kernel module
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <linux/if.h>
#include <linux/sockios.h>
#include <linux/ethtool.h>
#include <errno.h>
#include <time.h>

/* Debug output control */
#ifndef DEBUG_ENABLED
#define DEBUG_ENABLED 0  /* Set to 1 to enable debug output */
#endif

#if DEBUG_ENABLED
#define DEBUG_PRINT(fmt, ...) do { \
    struct timespec ts; \
    clock_gettime(CLOCK_MONOTONIC, &ts); \
    fprintf(stderr, "[DEBUG %ld.%03ld] %s:%d: " fmt "\n", \
            ts.tv_sec, ts.tv_nsec/1000000, __func__, __LINE__, ##__VA_ARGS__); \
} while(0)
#define DEBUG_ENTER() DEBUG_PRINT("ENTER")
#define DEBUG_EXIT(ret) DEBUG_PRINT("EXIT with %d", ret)
#define DEBUG_HEX_DUMP(data, len) do { \
    DEBUG_PRINT("Hex dump (%zu bytes):", (size_t)len); \
    for(int i = 0; i < len; i++) { \
        fprintf(stderr, "%02X ", ((unsigned char*)data)[i]); \
        if((i+1) % 16 == 0) fprintf(stderr, "\n"); \
    } \
    if(len % 16 != 0) fprintf(stderr, "\n"); \
} while(0)
#else
#define DEBUG_PRINT(fmt, ...) do { } while(0)
#define DEBUG_ENTER() do { } while(0)
#define DEBUG_EXIT(ret) do { } while(0)
#define DEBUG_HEX_DUMP(data, len) do { } while(0)
#endif

#define MAX_INTERFACES 10

struct lan8651_reg_access {
    __u32 cmd;
    __u32 address;
    __u32 value;
};

#define ETHTOOL_GLANREG     0x00001000  /* Get LAN register */
#define ETHTOOL_SLANREG     0x00001001  /* Set LAN register */

int find_lan8651_interface(char *ifname, size_t ifname_size) {
    FILE *fp;
    char line[256];
    
    DEBUG_ENTER();
    DEBUG_PRINT("Looking for LAN8651 interface in /proc/net/dev");
    
    fp = fopen("/proc/net/dev", "r");
    if (!fp) {
        DEBUG_PRINT("Failed to open /proc/net/dev: %s", strerror(errno));
        perror("Cannot open /proc/net/dev");
        DEBUG_EXIT(-1);
        return -1;
    }
    
    DEBUG_PRINT("Successfully opened /proc/net/dev");
    
    // Skip header lines
    fgets(line, sizeof(line), fp);
    fgets(line, sizeof(line), fp);
    DEBUG_PRINT("Skipped header lines");
    
    while (fgets(line, sizeof(line), fp)) {
        char *iface = strtok(line, ":");
        DEBUG_PRINT("Processing interface: %s", iface ? iface : "NULL");
        
        if (iface && strstr(iface, "eth")) {
            // Check if this interface uses lan865x driver
            char driver_path[512];
            char driver_name[64] = {0};
            FILE *driver_fp;
            
            snprintf(driver_path, sizeof(driver_path), 
                    "/sys/class/net/%s/device/driver/module", 
                    iface + (iface[0] == ' ' ? 1 : 0));
            
            DEBUG_PRINT("Checking driver path: %s", driver_path);
            
            ssize_t len = readlink(driver_path, driver_name, sizeof(driver_name)-1);
            if (len > 0) {
                driver_name[len] = '\0';
                DEBUG_PRINT("Driver link target: %s", driver_name);
                
                if (strstr(driver_name, "lan865x")) {
                    DEBUG_PRINT("Found LAN865x driver for interface: %s", iface);
                    strncpy(ifname, iface + (iface[0] == ' ' ? 1 : 0), ifname_size);
                    fclose(fp);
                    DEBUG_EXIT(0);
                    return 0;
                }
            } else {
                DEBUG_PRINT("readlink failed for %s: %s", driver_path, strerror(errno));
            }
        }
    }
    
    fclose(fp);
    return -1;
}

int lan8651_read_register(const char *ifname, u_int32_t address, u_int32_t *value) {
    int sock;
    struct ifreq ifr;
    struct ethtool_drvinfo drvinfo;
    struct lan8651_reg_access reg_access;
    
    DEBUG_ENTER();
    DEBUG_PRINT("Interface: %s, Address: 0x%08X", ifname, address);
    
    sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) {
        DEBUG_PRINT("Socket creation failed: %s", strerror(errno));
        perror("socket");
        DEBUG_EXIT(-1);
        return -1;
    }
    DEBUG_PRINT("Socket created successfully: fd=%d", sock);
    
    memset(&ifr, 0, sizeof(ifr));
    strncpy(ifr.ifr_name, ifname, IFNAMSIZ - 1);
    DEBUG_PRINT("Prepared ifreq for interface: %s", ifr.ifr_name);
    
    // First check if this is really a lan865x interface
    memset(&drvinfo, 0, sizeof(drvinfo));
    drvinfo.cmd = ETHTOOL_GDRVINFO;
    ifr.ifr_data = (char *)&drvinfo;
    
    DEBUG_PRINT("Calling ETHTOOL_GDRVINFO ioctl");
    if (ioctl(sock, SIOCETHTOOL, &ifr) < 0) {
        DEBUG_PRINT("ETHTOOL_GDRVINFO ioctl failed: %s", strerror(errno));
        perror("ETHTOOL_GDRVINFO ioctl");
        close(sock);
        DEBUG_EXIT(-1);
        return -1;
    }
    
    DEBUG_PRINT("Driver info: driver='%s', version='%s', fw_version='%s'", 
                drvinfo.driver, drvinfo.version, drvinfo.fw_version);
    
    if (strcmp(drvinfo.driver, "lan865x") != 0) {
        DEBUG_PRINT("Driver mismatch: expected 'lan865x', got '%s'", drvinfo.driver);
        fprintf(stderr, "Interface %s is not using lan865x driver\n", ifname);
        close(sock);
        DEBUG_EXIT(-1);
        return -1;
    }
    DEBUG_PRINT("Driver verification successful");
    
    // Now try to read register (this would need driver support)
    memset(&reg_access, 0, sizeof(reg_access));
    reg_access.cmd = ETHTOOL_GLANREG;
    reg_access.address = address;
    reg_access.value = 0;
    
    DEBUG_PRINT("Preparing register access: cmd=0x%08X, address=0x%08X", 
                reg_access.cmd, reg_access.address);
    
    ifr.ifr_data = (char *)&reg_access;
    DEBUG_HEX_DUMP(&reg_access, sizeof(reg_access));
    
    DEBUG_PRINT("Calling ETHTOOL_GLANREG ioctl");
    if (ioctl(sock, SIOCETHTOOL, &ifr) < 0) {
        DEBUG_PRINT("Register read ioctl failed: %s (errno=%d)", strerror(errno), errno);
        DEBUG_PRINT("This is expected - driver extension needed for register access");
        perror("Register read ioctl - driver extension needed");
        close(sock);
        DEBUG_EXIT(-1);
        return -1;
    }
    
    DEBUG_PRINT("Register read successful: value=0x%08X", reg_access.value);
    *value = reg_access.value;
    close(sock);
    DEBUG_EXIT(0);
    return 0;
}

int lan8651_write_register(const char *ifname, u_int32_t address, u_int32_t value) {
    int sock;
    struct ifreq ifr;
    struct lan8651_reg_access reg_access;
    
    DEBUG_ENTER();
    DEBUG_PRINT("Interface: %s, Address: 0x%08X, Value: 0x%08X", ifname, address, value);
    
    sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) {
        DEBUG_PRINT("Socket creation failed: %s", strerror(errno));
        perror("socket");
        DEBUG_EXIT(-1);
        return -1;
    }
    DEBUG_PRINT("Socket created successfully: fd=%d", sock);
    
    memset(&ifr, 0, sizeof(ifr));
    strncpy(ifr.ifr_name, ifname, IFNAMSIZ - 1);
    DEBUG_PRINT("Prepared ifreq for interface: %s", ifr.ifr_name);
    
    memset(&reg_access, 0, sizeof(reg_access));
    reg_access.cmd = ETHTOOL_SLANREG;
    reg_access.address = address;
    reg_access.value = value;
    
    DEBUG_PRINT("Preparing register write: cmd=0x%08X, address=0x%08X, value=0x%08X", 
                reg_access.cmd, reg_access.address, reg_access.value);
    
    ifr.ifr_data = (char *)&reg_access;
    DEBUG_HEX_DUMP(&reg_access, sizeof(reg_access));
    
    DEBUG_PRINT("Calling ETHTOOL_SLANREG ioctl");
    if (ioctl(sock, SIOCETHTOOL, &ifr) < 0) {
        DEBUG_PRINT("Register write ioctl failed: %s (errno=%d)", strerror(errno), errno);
        DEBUG_PRINT("This is expected - driver extension needed for register access");
        perror("Register write ioctl - driver extension needed");
        close(sock);
        DEBUG_EXIT(-1);
        return -1;
    }
    
    DEBUG_PRINT("Register write successful");
    close(sock);
    DEBUG_EXIT(0);
    return 0;
}

int main(int argc, char *argv[]) {
    char ifname[IFNAMSIZ];
    u_int32_t address, value;
    int ret;
    
    DEBUG_PRINT("=== LAN8651 ETHTOOL REGISTER ACCESS TOOL ===");
    DEBUG_PRINT("Debug output is %s", DEBUG_ENABLED ? "ENABLED" : "DISABLED");
    DEBUG_PRINT("Arguments: argc=%d", argc);
    for (int i = 0; i < argc; i++) {
        DEBUG_PRINT("  argv[%d] = '%s'", i, argv[i]);
    }
    
    if (argc < 2) {
        printf("Usage: %s <read|write> [address] [value]\n", argv[0]);
        printf("Example: %s read 0x10000\n", argv[0]);
        printf("Example: %s write 0x10000 0x0C\n", argv[0]);
        printf("\nNote: Compile with -DDEBUG_ENABLED=1 to enable debug output\n");
        return 1;
    }
    
    // Find LAN8651 interface
    if (find_lan8651_interface(ifname, sizeof(ifname)) < 0) {
        fprintf(stderr, "No LAN8651 interface found\n");
        return 1;
    }
    
    printf("Using interface: %s\n", ifname);
    
    if (strcmp(argv[1], "read") == 0) {
        if (argc != 3) {
            printf("Usage: %s read <address>\n", argv[0]);
            return 1;
        }
        
        address = strtoul(argv[2], NULL, 0);
        ret = lan8651_read_register(ifname, address, &value);
        if (ret == 0) {
            printf("READ 0x%08X = 0x%08X (%u)\n", address, value, value);
        } else {
            printf("ERROR: Read failed\n");
        }
    }
    else if (strcmp(argv[1], "write") == 0) {
        if (argc != 4) {
            printf("Usage: %s write <address> <value>\n", argv[0]);
            return 1;
        }
        
        address = strtoul(argv[2], NULL, 0);
        value = strtoul(argv[3], NULL, 0);
        ret = lan8651_write_register(ifname, address, value);
        if (ret == 0) {
            printf("WRITE 0x%08X = 0x%08X - OK\n", address, value);
        } else {
            printf("ERROR: Write failed\n");
        }
    }
    else {
        printf("Unknown command: %s\n", argv[1]);
        return 1;
    }
    
    return ret;
}