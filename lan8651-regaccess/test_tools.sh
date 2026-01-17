#!/bin/bash
# Test script for LAN8651 register access tools
# Run this on the target ARM system

set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

echo -e "${GREEN}=== LAN8651 Register Access Test ===${NC}"

# Check if we're on the target system
if [ "$(uname -m)" != "armv7l" ] && [ "$(uname -m)" != "aarch64" ]; then
    echo -e "${YELLOW}Warning: Not running on ARM target system${NC}"
fi

# Test 1: Check for LAN8651 network interface
echo -e "${GREEN}Test 1: Searching for LAN8651 network interface...${NC}"
if python3 lan8651_kernelfs.py list > /dev/null 2>&1; then
    echo -e "${GREEN}✓ lan8651_kernelfs.py is working${NC}"
else
    echo -e "${YELLOW}! lan8651_kernelfs.py - might need kernel debugging enabled${NC}"
fi

# Test 2: Check ethtool approach
echo -e "${GREEN}Test 2: Testing ethtool-based access...${NC}"
if [ -f "./lan8651_ethtool_arm" ]; then
    echo -e "${GREEN}✓ lan8651_ethtool_arm binary found${NC}"
    # This will fail without driver extension, but shows the approach
    ./lan8651_ethtool_arm read 0x10000 2>/dev/null || echo -e "${YELLOW}! Needs driver extension for ethtool support${NC}"
else
    echo -e "${RED}✗ lan8651_ethtool_arm not found${NC}"
fi

# Test 3: List available network interfaces
echo -e "${GREEN}Test 3: Available network interfaces:${NC}"
ip link show | grep -E "^\d+:" | while read line; do
    iface=$(echo "$line" | cut -d: -f2 | tr -d ' ')
    driver=""
    if [ -d "/sys/class/net/$iface/device/driver" ]; then
        driver=$(readlink "/sys/class/net/$iface/device/driver" 2>/dev/null | xargs basename)
    fi
    echo "  $iface - Driver: ${driver:-unknown}"
done

# Test 4: Check for debugfs
echo -e "${GREEN}Test 4: Checking kernel debug interfaces...${NC}"
if mount | grep debugfs > /dev/null; then
    echo -e "${GREEN}✓ debugfs is mounted${NC}"
    if [ -d "/sys/kernel/debug" ]; then
        echo -e "${GREEN}✓ /sys/kernel/debug is accessible${NC}"
        # Look for relevant debug entries
        find /sys/kernel/debug -name "*tc6*" -o -name "*lan*" -o -name "*spi*" 2>/dev/null | head -5
    fi
else
    echo -e "${YELLOW}! debugfs not mounted - run: mount -t debugfs none /sys/kernel/debug${NC}"
fi

# Test 5: Show recommendations
echo -e "${GREEN}=== Recommendations ===${NC}"
echo -e "${YELLOW}To enable full register access:${NC}"
echo "1. Apply the kernel driver patch:"
echo "   cd linux-source && patch -p1 < lan865x_debug_patch.patch"
echo "2. Rebuild and install the kernel"
echo "3. After reboot, register access will be available via:"
echo "   echo 'read 0x10000' > /sys/kernel/debug/lan865x/reg_access"
echo "   dmesg | tail  # See register read result"
echo ""
echo "4. Alternative: Use existing interfaces if available:"
echo "   python3 lan8651_kernelfs.py read 0x10000"

echo -e "${GREEN}=== Test Complete ===${NC}"