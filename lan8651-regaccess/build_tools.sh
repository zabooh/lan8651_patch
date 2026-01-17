#!/bin/bash
# Build script for LAN8651 register access tools
# For ARM target (LAN966x system)

set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

echo -e "${GREEN}=== Building LAN8651 Register Access Tools ===${NC}"

# Find ARM cross-compiler
BUILDROOT_DIR="/home/martin/AIoT/lan9662/mchp-brsdk-source-2025.12/output"
ARM_GCC=$(find $BUILDROOT_DIR -name "arm-linux-gcc" -path "*/bin/*" | head -1)

if [ -z "$ARM_GCC" ]; then
    echo -e "${RED}ERROR: ARM cross-compiler not found in Buildroot output${NC}"
    exit 1
fi

echo -e "${YELLOW}Using ARM compiler: $ARM_GCC${NC}"

# Build ARM version of ethtool-based tool
echo -e "${GREEN}Building lan8651_ethtool for ARM...${NC}"
$ARM_GCC -o lan8651_ethtool_arm lan8651_ethtool.c
if [ $? -eq 0 ]; then
    echo -e "${GREEN}✓ lan8651_ethtool_arm built successfully${NC}"
else
    echo -e "${RED}✗ Failed to build lan8651_ethtool_arm${NC}"
    exit 1
fi

# Build x86 version for testing on host
echo -e "${GREEN}Building lan8651_ethtool for x86...${NC}"
gcc -o lan8651_ethtool_x86 lan8651_ethtool.c
if [ $? -eq 0 ]; then
    echo -e "${GREEN}✓ lan8651_ethtool_x86 built successfully${NC}"
else
    echo -e "${RED}✗ Failed to build lan8651_ethtool_x86${NC}"
    exit 1
fi

# Make Python scripts executable
echo -e "${GREEN}Making Python scripts executable...${NC}"
chmod +x lan8651_kernelfs.py

# Check file sizes and permissions
echo -e "${GREEN}Build summary:${NC}"
ls -lh lan8651_ethtool_* lan8651_kernelfs.py 2>/dev/null | while read line; do
    echo "  $line"
done

echo -e "${GREEN}=== Build Complete ===${NC}"
echo ""
echo -e "${YELLOW}Next Steps:${NC}"
echo "1. Copy tools to target system:"
echo "   scp lan8651_ethtool_arm lan8651_kernelfs.py root@<target-ip>:~/"
echo ""
echo "2. For kernel driver extension approach:"
echo "   cd $BUILDROOT_DIR/../linux"
echo "   patch -p1 < $(pwd)/lan865x_debug_patch.patch"
echo "   # Rebuild kernel"
echo ""
echo "3. Test on target:"
echo "   ./lan8651_ethtool_arm read 0x10000"
echo "   python3 lan8651_kernelfs.py read 0x10000"