#!/bin/bash
# Enhanced build script for LAN8651 register access tools with debug support

set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

echo -e "${GREEN}=== Building LAN8651 Register Access Tools (Debug Enabled) ===${NC}"

# Find ARM cross-compiler
BUILDROOT_DIR="/home/martin/AIoT/lan9662/mchp-brsdk-source-2025.12/output"
ARM_GCC=$(find $BUILDROOT_DIR -name "arm-linux-gcc" -path "*/bin/*" | head -1)

if [ -z "$ARM_GCC" ]; then
    echo -e "${RED}ERROR: ARM cross-compiler not found in Buildroot output${NC}"
    exit 1
fi

echo -e "${YELLOW}Using ARM compiler: $ARM_GCC${NC}"

# Build ARM versions with debug enabled
echo -e "${GREEN}Building lan8651_ethtool for ARM (DEBUG ENABLED)...${NC}"
$ARM_GCC -DDEBUG_ENABLED=1 -o lan8651_ethtool_arm_debug lan8651_ethtool.c
if [ $? -eq 0 ]; then
    echo -e "${GREEN}✓ lan8651_ethtool_arm_debug built successfully${NC}"
else
    echo -e "${RED}✗ Failed to build lan8651_ethtool_arm_debug${NC}"
    exit 1
fi

# Build ARM version without debug
echo -e "${GREEN}Building lan8651_ethtool for ARM (RELEASE)...${NC}"
$ARM_GCC -DDEBUG_ENABLED=0 -o lan8651_ethtool_arm lan8651_ethtool.c
if [ $? -eq 0 ]; then
    echo -e "${GREEN}✓ lan8651_ethtool_arm built successfully${NC}"
else
    echo -e "${RED}✗ Failed to build lan8651_ethtool_arm${NC}"
    exit 1
fi

# Build x86 versions with debug enabled
echo -e "${GREEN}Building lan8651_ethtool for x86 (DEBUG ENABLED)...${NC}"
gcc -DDEBUG_ENABLED=1 -o lan8651_ethtool_x86_debug lan8651_ethtool.c
if [ $? -eq 0 ]; then
    echo -e "${GREEN}✓ lan8651_ethtool_x86_debug built successfully${NC}"
else
    echo -e "${RED}✗ Failed to build lan8651_ethtool_x86_debug${NC}"
    exit 1
fi

# Build x86 version without debug
echo -e "${GREEN}Building lan8651_ethtool for x86 (RELEASE)...${NC}"
gcc -DDEBUG_ENABLED=0 -o lan8651_ethtool_x86 lan8651_ethtool.c
if [ $? -eq 0 ]; then
    echo -e "${GREEN}✓ lan8651_ethtool_x86 built successfully${NC}"
else
    echo -e "${RED}✗ Failed to build lan8651_ethtool_x86${NC}"
    exit 1
fi

# Make Python scripts executable and create debug wrapper
echo -e "${GREEN}Making Python scripts executable...${NC}"
chmod +x lan8651_kernelfs.py

# Create debug-enabled wrapper script
cat > lan8651_kernelfs_debug.py << 'EOF'
#!/usr/bin/env python3
"""
Debug-enabled wrapper for LAN8651 kernelfs tool
Automatically enables debug output
"""
import os
import sys

# Enable debug output
os.environ['LAN8651_DEBUG'] = '1'

# Import and run the main module
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from lan8651_kernelfs import main

if __name__ == "__main__":
    exit(main())
EOF

chmod +x lan8651_kernelfs_debug.py

# Check file sizes and permissions
echo -e "${BLUE}=== Build Results ===${NC}"
echo -e "${YELLOW}ARM Binaries:${NC}"
ls -lh lan8651_ethtool_arm*
echo -e "${YELLOW}x86 Binaries:${NC}"
ls -lh lan8651_ethtool_x86*
echo -e "${YELLOW}Python Tools:${NC}"
ls -lh lan8651_kernelfs*.py

echo ""
echo -e "${GREEN}=== Usage Instructions ===${NC}"
echo -e "${YELLOW}C Tools (Debug mode):${NC}"
echo "  ./lan8651_ethtool_arm_debug read 0x10000"
echo "  ./lan8651_ethtool_x86_debug read 0x10000"
echo ""
echo -e "${YELLOW}C Tools (Release mode):${NC}"
echo "  ./lan8651_ethtool_arm read 0x10000"
echo "  ./lan8651_ethtool_x86 read 0x10000"
echo ""
echo -e "${YELLOW}Python Tool (Debug mode):${NC}"
echo "  ./lan8651_kernelfs_debug.py read 0x10000"
echo "  # OR:"
echo "  LAN8651_DEBUG=1 ./lan8651_kernelfs.py read 0x10000"
echo ""
echo -e "${YELLOW}Python Tool (Release mode):${NC}"
echo "  ./lan8651_kernelfs.py read 0x10000"
echo ""
echo -e "${YELLOW}Kernel Patch:${NC}"
echo "  Enhanced debug version: lan865x_enhanced_debug_patch.patch"
echo "  Original version: lan865x_debug_patch.patch"
echo ""
echo -e "${GREEN}✅ All tools built successfully with debug support!${NC}"