#!/bin/bash
# Comprehensive test script for LAN8651 register access tools with debug validation

set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
PURPLE='\033[0;35m'
NC='\033[0m' # No Color

# Test configuration
TEST_REGISTERS=(
    "0x0002"  # TC6_STDCAP
    "0x0008"  # TC6_STATUS0
    "0x10000" # MAC_NET_CTL
    "0x10001" # MAC_NET_CFG
)

echo -e "${GREEN}=== LAN8651 Register Access Tools - Debug Test Suite ===${NC}"
echo -e "${BLUE}Testing all tools with comprehensive debug validation${NC}"

# Function to run command and show both output and debug info
run_with_debug() {
    local cmd="$1"
    local desc="$2"
    
    echo -e "\n${PURPLE}=== $desc ===${NC}"
    echo -e "${YELLOW}Command: $cmd${NC}"
    
    echo -e "${BLUE}--- Standard Output ---${NC}"
    if eval "$cmd" 2>debug.tmp; then
        echo -e "${GREEN}✓ Command succeeded${NC}"
    else
        echo -e "${RED}✗ Command failed${NC}"
    fi
    
    if [ -s debug.tmp ]; then
        echo -e "${BLUE}--- Debug Output ---${NC}"
        cat debug.tmp
    fi
    
    rm -f debug.tmp
}

# Test 1: Check if we're on the target system
echo -e "\n${GREEN}Test 1: System Information${NC}"
echo -e "${YELLOW}Architecture: $(uname -m)${NC}"
echo -e "${YELLOW}Kernel version: $(uname -r)${NC}"
echo -e "${YELLOW}Date: $(date)${NC}"

if [ "$(uname -m)" != "armv7l" ] && [ "$(uname -m)" != "aarch64" ]; then
    echo -e "${YELLOW}Warning: Not running on ARM target system - ethtool tests may fail${NC}"
fi

# Test 2: Build verification
echo -e "\n${GREEN}Test 2: Build Verification${NC}"
echo -e "${YELLOW}Available binaries and scripts:${NC}"
ls -la lan8651_* 2>/dev/null || echo "No LAN8651 tools found"

# Test 3: Python tool debug functionality
echo -e "\n${GREEN}Test 3: Python Tool Debug Functionality${NC}"

if [ -f "./lan8651_kernelfs.py" ]; then
    echo -e "${BLUE}Testing Python tool with debug enabled...${NC}"
    
    # Test list command with debug
    run_with_debug "LAN8651_DEBUG=1 ./lan8651_kernelfs.py --debug list" \
                   "Python tool list interfaces (debug mode)"
    
    # Test status command with debug
    run_with_debug "LAN8651_DEBUG=1 ./lan8651_kernelfs.py status" \
                   "Python tool status (debug mode)"
    
    # Test with debug wrapper if available
    if [ -f "./lan8651_kernelfs_debug.py" ]; then
        run_with_debug "./lan8651_kernelfs_debug.py list" \
                       "Python debug wrapper - list interfaces"
    fi
    
    # Test register read with debug (will fail without kernel patch but shows debug info)
    for reg in "${TEST_REGISTERS[@]}"; do
        run_with_debug "LAN8651_DEBUG=1 ./lan8651_kernelfs.py read $reg" \
                       "Python tool read register $reg (debug mode)"
    done
    
else
    echo -e "${RED}✗ Python tool not found${NC}"
fi

# Test 4: C tool debug functionality
echo -e "\n${GREEN}Test 4: C Tool Debug Functionality${NC}"

# Test ARM debug version
if [ -f "./lan8651_ethtool_arm_debug" ]; then
    echo -e "${BLUE}Testing ARM debug binary...${NC}"
    
    for reg in "${TEST_REGISTERS[@]}"; do
        run_with_debug "./lan8651_ethtool_arm_debug read $reg" \
                       "ARM debug binary read register $reg"
    done
    
    # Test write operation
    run_with_debug "./lan8651_ethtool_arm_debug write 0x10000 0x0C" \
                   "ARM debug binary write register (enable TX+RX)"
else
    echo -e "${RED}✗ ARM debug binary not found${NC}"
fi

# Test x86 debug version
if [ -f "./lan8651_ethtool_x86_debug" ]; then
    echo -e "${BLUE}Testing x86 debug binary...${NC}"
    
    for reg in "${TEST_REGISTERS[@]}"; do
        run_with_debug "./lan8651_ethtool_x86_debug read $reg" \
                       "x86 debug binary read register $reg"
    done
else
    echo -e "${RED}✗ x86 debug binary not found${NC}"
fi

# Test 5: Kernel interface validation
echo -e "\n${GREEN}Test 5: Kernel Interface Validation${NC}"

# Check for LAN865x driver
echo -e "${YELLOW}Checking for LAN865x driver...${NC}"
if lsmod | grep -q lan865x; then
    echo -e "${GREEN}✓ LAN865x driver is loaded${NC}"
    lsmod | grep lan865x
else
    echo -e "${YELLOW}! LAN865x driver not loaded or built-in${NC}"
fi

# Check for debugfs
echo -e "${YELLOW}Checking debugfs mount...${NC}"
if mount | grep -q debugfs; then
    echo -e "${GREEN}✓ Debugfs is mounted${NC}"
    mount | grep debugfs
    
    # Check for lan865x debugfs entry
    if [ -d "/sys/kernel/debug/lan865x" ]; then
        echo -e "${GREEN}✓ LAN865x debugfs interface found${NC}"
        ls -la /sys/kernel/debug/lan865x/
        
        # Test debugfs interface if available
        if [ -f "/sys/kernel/debug/lan865x/reg_access" ]; then
            echo -e "${BLUE}Testing debugfs interface...${NC}"
            
            # Test read from debugfs interface (show usage)
            echo -e "${YELLOW}Reading usage information:${NC}"
            cat /sys/kernel/debug/lan865x/reg_access 2>/dev/null || echo "Access denied - try as root"
            
            # Test debug enable/disable
            if [ "$(id -u)" -eq 0 ]; then
                echo -e "${YELLOW}Testing debug control (running as root):${NC}"
                echo "debug on" > /sys/kernel/debug/lan865x/reg_access 2>/dev/null || echo "Failed to enable debug"
                echo "debug off" > /sys/kernel/debug/lan865x/reg_access 2>/dev/null || echo "Failed to disable debug"
                
                # Test register operations
                for reg in "${TEST_REGISTERS[@]}"; do
                    echo -e "${YELLOW}Testing debugfs read $reg:${NC}"
                    echo "read $reg" > /sys/kernel/debug/lan865x/reg_access 2>/dev/null || echo "Read failed"
                    dmesg | tail -n 2
                done
            else
                echo -e "${YELLOW}! Not running as root - debugfs write tests skipped${NC}"
            fi
        else
            echo -e "${YELLOW}! reg_access file not found - kernel patch not applied${NC}"
        fi
    else
        echo -e "${YELLOW}! LAN865x debugfs directory not found - kernel patch not applied${NC}"
    fi
else
    echo -e "${RED}✗ Debugfs not mounted${NC}"
fi

# Test 6: Network interface detection
echo -e "\n${GREEN}Test 6: Network Interface Detection${NC}"

echo -e "${YELLOW}Available network interfaces:${NC}"
if command -v ip >/dev/null; then
    ip link show | grep -E "^\d+:" | while IFS= read -r line; do
        iface=$(echo "$line" | cut -d: -f2 | tr -d ' ')
        
        # Check driver
        driver="unknown"
        driver_path="/sys/class/net/$iface/device/driver"
        if [ -L "$driver_path" ]; then
            driver=$(readlink "$driver_path" 2>/dev/null | xargs basename 2>/dev/null || echo "unknown")
        fi
        
        echo "  $iface - Driver: $driver"
        
        # Check if this is a lan865x interface
        if [[ "$driver" == *"lan865x"* ]]; then
            echo -e "    ${GREEN}✓ LAN865x interface detected!${NC}"
        fi
    done
else
    cat /proc/net/dev
fi

# Test 7: Debug output comparison
echo -e "\n${GREEN}Test 7: Debug Output Comparison${NC}"

if [ -f "./lan8651_ethtool_arm" ] && [ -f "./lan8651_ethtool_arm_debug" ]; then
    echo -e "${YELLOW}Comparing release vs debug output for ARM binary:${NC}"
    
    echo -e "${BLUE}--- Release version ---${NC}"
    ./lan8651_ethtool_arm read 0x10000 2>&1 || echo "Command failed"
    
    echo -e "${BLUE}--- Debug version ---${NC}"
    ./lan8651_ethtool_arm_debug read 0x10000 2>&1 || echo "Command failed"
fi

# Test 8: Performance comparison
echo -e "\n${GREEN}Test 8: Performance Analysis${NC}"

if command -v time >/dev/null; then
    echo -e "${YELLOW}Performance comparison (if tools work):${NC}"
    
    if [ -f "./lan8651_kernelfs.py" ]; then
        echo -e "${BLUE}Python tool (normal):${NC}"
        time -p ./lan8651_kernelfs.py list >/dev/null 2>&1 || echo "Failed"
        
        echo -e "${BLUE}Python tool (debug):${NC}"
        time -p env LAN8651_DEBUG=1 ./lan8651_kernelfs.py list >/dev/null 2>&1 || echo "Failed"
    fi
fi

# Test 9: Error condition testing
echo -e "\n${GREEN}Test 9: Error Condition Testing${NC}"

echo -e "${YELLOW}Testing error conditions with debug output:${NC}"

# Test invalid commands
run_with_debug "LAN8651_DEBUG=1 ./lan8651_kernelfs.py invalid_command 2>&1 || true" \
               "Python tool - invalid command"

if [ -f "./lan8651_ethtool_arm_debug" ]; then
    run_with_debug "./lan8651_ethtool_arm_debug invalid_command 2>&1 || true" \
                   "C tool - invalid command"
    
    run_with_debug "./lan8651_ethtool_arm_debug read 2>&1 || true" \
                   "C tool - missing address"
fi

# Test 10: Summary and recommendations
echo -e "\n${GREEN}=== Test Summary and Recommendations ===${NC}"

echo -e "${BLUE}Debug Features Available:${NC}"
echo "✓ C tools: Compile-time debug macros with timestamps"
echo "✓ Python tool: Environment variable LAN8651_DEBUG=1"
echo "✓ Kernel patch: Runtime debug enable/disable via debugfs"

echo -e "\n${BLUE}Usage for Testing:${NC}"
echo "1. Enable Python debug: export LAN8651_DEBUG=1"
echo "2. Use debug binaries: lan8651_ethtool_*_debug"
echo "3. Monitor kernel messages: dmesg -w"
echo "4. Check debugfs: cat /sys/kernel/debug/lan865x/reg_access"

echo -e "\n${BLUE}Next Steps:${NC}"
if [ ! -d "/sys/kernel/debug/lan865x" ]; then
    echo "1. Apply kernel patch: patch -p1 < lan865x_enhanced_debug_patch.patch"
    echo "2. Rebuild and install kernel"
    echo "3. Reboot system"
    echo "4. Re-run this test script"
else
    echo "1. Kernel patch appears to be applied"
    echo "2. Test register access with: echo 'read 0x10000' > /sys/kernel/debug/lan865x/reg_access"
    echo "3. Monitor with: dmesg | grep REG"
fi

echo -e "\n${GREEN}✅ Debug validation test suite completed!${NC}"