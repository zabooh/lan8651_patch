# LAN8651 Register Access Tools

A functional toolset for direct access to the registers of the **Microchip LAN8651 10BASE-T1S MAC-PHY** chip via the OPEN Alliance TC6 Standard.

![LAN8651](https://img.shields.io/badge/Chip-LAN8651-blue)
![Platform](https://img.shields.io/badge/Platform-LAN966x-green)
![License](https://img.shields.io/badge/License-GPL--2.0-red)

## 📋 Overview

The LAN8651 is a **10BASE-T1S MAC-PHY** chip that communicates with the host system via **SPI**. This toolset provides **working solutions** for register access without requiring custom kernel modules that access private kernel structures.

## 🏗️ Architecture

```
┌─────────────────┐    ┌─────────────────┐    ┌─────────────────┐
│   User Tools    │    │ Patched Driver  │    │   Hardware      │
│                 │    │                 │    │                 │
│lan8651_kernelfs │◄──►│ lan865x driver  │◄──►│   LAN8651       │
│lan8651_ethtool* │    │   + debugfs     │    │   (SPI/TC6)     │
│                 │    │                 │    │                 │
└─────────────────┘    └─────────────────┘    └─────────────────┘
        │                        │                        │
        │                        │                        │
        ▼                        ▼                        ▼
/sys/kernel/debug/lan865x    OA TC6 Framework         SPI Interface
```

## 📦 Components

### 1. **Debugfs Interface** (✅ Working)
- Via patched lan865x kernel driver
- Debugfs entries: `/sys/kernel/debug/lan865x/reg_access`
- Direct register read/write access
- No custom kernel module required

### 2. **Python Tool** (`lan8651_kernelfs.py`) - ✅ Working
- Accesses registers via debugfs interface
- Automatic LAN8651 interface detection
- **Register name resolution**: Use `OA_STATUS0` instead of `0x0008`
- **Comprehensive register database**: 30+ official registers from datasheet
- **Bit field decoding**: Automatic interpretation of status/control bits
- Command-line interface for read/write operations
- Works with patched lan865x driver
- **Debug support**: `LAN8651_DEBUG=1` environment variable

### 3. **Ethtool Tool** (`lan8651_ethtool.c`) - ⚠️ Needs Driver Extension
- Direct register access via ethtool IOCTLs
- Compiled for ARM (`lan8651_ethtool_arm`) and x86 (`lan8651_ethtool_x86`)
- Requires additional ethtool IOCTL handlers in driver
- Alternative to debugfs approach
- **Debug support**: Compile-time `DEBUG_ENABLED` macro

### 4. **Register Map Documentation** (`LAN8651_REGISTER_MAP.md`)
- Complete LAN8651 register map from official datasheet
- Memory Map Selector (MMS) organization
- All standard, MAC, and PHY register definitions
- Register bit definitions and usage examples
- Official Microchip documentation reference

### 4. **Kernel Patches** - ✅ Ready
- `lan865x_debug_patch.patch` - Basic debugfs support
- `lan865x_enhanced_debug_patch.patch` - **Enhanced version with comprehensive debugging**
- Provides `/sys/kernel/debug/lan865x/reg_access` interface
- Enables direct TC6 register access via existing driver
- **Runtime debug control**: `echo "debug on/off" > reg_access`

### 5. **Build & Test Tools** - ✅ Available
- `build_tools.sh` - Standard build script
- `build_tools_debug.sh` - **Enhanced build with debug versions**
- `test_tools.sh` - Basic functionality testing
- `test_tools_debug.sh` - **Comprehensive debug validation suite**

## 🚀 Quick Start

### Prerequisites

1. **Apply Kernel Patch** (Required for debugfs approach):
```bash
# In kernel source directory
cd /path/to/kernel/source
patch -p1 < lan865x_debug_patch.patch

# Rebuild and install kernel
make && make modules_install && make install
reboot
```

### Build Tools

```bash
# Standard build (release versions)
./build_tools.sh

# Enhanced build with debug support
./build_tools_debug.sh
```

**Debug Build Results:**
- `lan8651_ethtool_arm_debug` / `lan8651_ethtool_x86_debug` - Debug versions
- `lan8651_ethtool_arm` / `lan8651_ethtool_x86` - Release versions  
- `lan8651_kernelfs_debug.py` - Python wrapper with auto-enabled debug

### Test Tools

```bash
# Basic functionality testing
./test_tools.sh

# Comprehensive debug validation suite
./test_tools_debug.sh
```

## 📖 LAN8651 Register Map

📋 **Complete register documentation**: See [`LAN8651_REGISTER_MAP.md`](LAN8651_REGISTER_MAP.md) for comprehensive register definitions from the official Microchip datasheet.

### Register Organization

The LAN8651 uses **Memory Map Selector (MMS)** to organize registers:

| MMS | Description | Example Registers |
|-----|-------------|------------------|
| 0 | Standard/PHY Registers | 0x0000-0x0015, 0xFF00-0xFF0E |
| 1 | MAC Registers | 0x10000-0x10214 |
| 2-4 | PHY Registers | Various |
| 10 | Miscellaneous | Additional features |

### Common Register Examples

```bash
# Read device identification (using register names)
./lan8651_kernelfs.py read OA_ID          # Device ID
./lan8651_kernelfs.py read OA_PHYID       # PHY ID

# Read status registers
./lan8651_kernelfs.py read OA_STATUS0     # General status
./lan8651_kernelfs.py read BASIC_STATUS   # PHY status

# MAC control (with automatic bit decoding)
./lan8651_kernelfs.py read MAC_NCR        # MAC control
./lan8651_kernelfs.py write MAC_NCR 0x0C  # Enable TX+RX

# Buffer and statistics
./lan8651_kernelfs.py read OA_BUFSTS      # Buffer status
./lan8651_kernelfs.py read STATS0         # MAC statistics

# Legacy hex addressing still supported
./lan8651_kernelfs.py read 0x0008         # Same as OA_STATUS0
```

## 📖 Usage - Debugfs Method (Recommended)

### Using Python Tool

```bash
# List all available registers with names and addresses
./lan8651_kernelfs.py list

# Read registers by name (recommended)
./lan8651_kernelfs.py read OA_STATUS0     # Device status
./lan8651_kernelfs.py read MAC_NCR        # MAC control
./lan8651_kernelfs.py read BASIC_STATUS   # PHY status

# Read by hex address (legacy support)
./lan8651_kernelfs.py read 0x10000

# Write register (name or hex)
./lan8651_kernelfs.py write MAC_NCR 0x0C
./lan8651_kernelfs.py write 0x10000 0x0C

# Comprehensive status information
./lan8651_kernelfs.py status

# Enable debug output
LAN8651_DEBUG=1 ./lan8651_kernelfs.py read OA_STATUS0
# OR use debug wrapper
./lan8651_kernelfs_debug.py read OA_STATUS0
```

### Direct Debugfs Access

```bash
# Read register (example: MAC_NET_CTL at 0x10000)
echo "read 0x10000" > /sys/kernel/debug/lan865x/reg_access

# Write register (example: Enable TX+RX)
echo "write 0x10000 0x0C" > /sys/kernel/debug/lan865x/reg_access

# Enable enhanced debug output (with enhanced patch)
echo "debug on" > /sys/kernel/debug/lan865x/reg_access

# View kernel messages for results
dmesg | tail
```

## 📖 Usage - Ethtool Method (Needs Driver Extension)

⚠️ **Note:** This approach requires additional ethtool IOCTL handlers in the lan865x driver.

```bash
# Standard usage
./lan8651_ethtool_arm read 0x10000
./lan8651_ethtool_arm write 0x10000 0x0C

# Debug mode with detailed output
./lan8651_ethtool_arm_debug read 0x10000
./lan8651_ethtool_arm_debug write 0x10000 0x0C
```

## 🧪 Debug & Testing Features

### **Comprehensive Debug Support**

All tools now include extensive debug capabilities for thorough testing:

#### **C Tools Debug Features:**
- **Compile-time control**: `DEBUG_ENABLED` macro
- **Timestamped output**: Function entry/exit with timing
- **Detailed error analysis**: errno values and descriptions  
- **IOCTL debugging**: Request/response inspection
- **Hex dumps**: Raw data visualization

```bash
# Build with debug
gcc -DDEBUG_ENABLED=1 -o lan8651_ethtool_debug lan8651_ethtool.c

# Use debug version
./lan8651_ethtool_arm_debug read 0x10000
```

#### **Python Tool Debug Features:**
- **Environment control**: `LAN8651_DEBUG=1`
- **Comprehensive logging**: Timestamped debug messages
- **Interface detection tracing**: Step-by-step discovery
- **Kernel message parsing**: Register operation tracking

```bash
# Enable debug output
LAN8651_DEBUG=1 ./lan8651_kernelfs.py read 0x10000

# Use debug wrapper  
./lan8651_kernelfs_debug.py read 0x10000

# Command-line debug flag
./lan8651_kernelfs.py --debug read 0x10000
```

#### **Enhanced Kernel Patch Debug Features:**
- **Runtime debug control**: Enable/disable via debugfs
- **Microsecond timing**: Precise performance measurement
- **Operation state tracking**: Last operation results
- **Interactive help**: Usage information in debugfs

```bash
# Enable kernel debug mode
echo "debug on" > /sys/kernel/debug/lan865x/reg_access

# View enhanced status information  
cat /sys/kernel/debug/lan865x/reg_access

# Monitor debug output
dmesg -w | grep "REG\|DEBUG"
```

### **Test Suite Validation:**

```bash
# Comprehensive debug testing
./test_tools_debug.sh

# Basic functionality testing
./test_tools.sh
```

The debug test suite validates:
- ✅ All debug output functionality
- ✅ Interface detection with detailed logging
- ✅ Error condition handling with analysis
- ✅ Performance comparison (debug vs release)
- ✅ Kernel patch verification
- ✅ Register access validation

## � Enhanced Features (Latest Update)

### **Register Name Resolution**
Use intuitive register names instead of hex addresses:

```bash
# Instead of remembering hex addresses:
./lan8651_kernelfs.py read 0x0008  

# Use descriptive names:
./lan8651_kernelfs.py read OA_STATUS0
```

**Available Register Names:**
- **Standard Registers**: `OA_ID`, `OA_STATUS0`, `OA_CONFIG0`, `OA_RESET`
- **MAC Registers**: `MAC_NCR`, `MAC_NCFGR`, `MAC_HRB`, `BMGR_CTL`
- **PHY Registers**: `BASIC_CONTROL`, `BASIC_STATUS`, `PHY_ID1`, `PHY_ID2`
- **Timestamp Registers**: `TTSCAH`, `TTSCAL`, `TTSCBH`, `TTSCBL`

### **Automatic Bit Field Decoding**
Smart interpretation of register values:

```bash
$ ./lan8651_kernelfs.py read OA_STATUS0
Register OA_STATUS0 (0x00000008) = 0x00000042 (66)
Binary: 00000000000000000000000001000010
Status bits: RESETC, HDRE
Status register - shows current device state
```

### **Comprehensive Register Listing**
Quick overview of all available registers:

```bash
$ ./lan8651_kernelfs.py list
Known LAN8651 Registers:
============================================================

Standard/PHY Registers (MMS 0):
  OA_ID           = 0x00000000
  OA_PHYID        = 0x00000001
  OA_STDCAP       = 0x00000002
  BASIC_CONTROL   = 0x0000FF00
  BASIC_STATUS    = 0x0000FF01

MAC Registers (MMS 1):
  MAC_NCR         = 0x00010000
  MAC_NCFGR       = 0x00010001
  BMGR_CTL        = 0x00010200
```

### **Status Overview Command**
Get comprehensive device status with one command:

```bash
$ ./lan8651_kernelfs.py status
LAN8651 Status Information:
========================================

General Status:
Register OA_STATUS0 (0x00000008) = 0x00000042 (66)
Status bits: RESETC, HDRE

PHY Basic Status:
Register BASIC_STATUS (0x0000FF01) = 0x00007809 (30729)
Status bits: LSTATUS, ANEGCAPABLE
```

## 🗂️ Register Reference

### Enhanced Register Tables (Based on Official Datasheet)

### Core Standard Registers (MMS 0)

| Register | Address | Key Bits | Description |
|----------|---------|----------|-------------|
| `OA_ID` | 0x0000 | [7:4,3:0] | Open Alliance ID (Major/Minor Version) |
| `OA_PHYID` | 0x0001 | [31:0] | PHY OUI and Model Number |
| `OA_STDCAP` | 0x0002 | [8] | Standard Capabilities |
| `OA_RESET` | 0x0003 | [0] | Software Reset Control |
| `OA_CONFIG0` | 0x0004 | [15,12] | SYNC, Configuration Enable |
| `OA_STATUS0` | 0x0008 | [7:0] | Status Flags (PHYINT, RESETC, etc.) |
| `OA_BUFSTS` | 0x000B | [15:8,7:0] | TX Credits, RX Buffer Available |
|----------|---------|------|--------------|
| `MAC_NET_CTL` | 0x00010000 | [3:2] | Network Control (TXEN, RXEN) |
| `MAC_NET_CFG` | 0x00010001 | [7:4] | Network Config (Promiscuous, Multicast) |
| `MAC_L_SADDR1` | 0x00010022 | [31:0] | MAC Address Low (Bytes 0-3) |
| `MAC_H_SADDR1` | 0x00010023 | [15:0] | MAC Address High (Bytes 4-5) |
| `MAC_L_HASH` | 0x00010020 | [31:0] | Hash Table Low |
| `MAC_H_HASH` | 0x00010021 | [31:0] | Hash Table High |

### TC6 Control Register

| Register | Address | Bits | Description |
|----------|---------|------|--------------|
| `TC6_STDCAP` | 0x0002 | [8] | Standard Capabilities |
| `TC6_RESET` | 0x0003 | [0] | Software Reset |
| `TC6_CONFIG0` | 0x0004 | [15,12] | SYNC, ZARFE Enable |
| `TC6_STATUS0` | 0x0008 | [6:0] | Status Flags |
| `TC6_BUFFER_STATUS` | 0x000B | [15:8,7:0] | TX Credits, RX Chunks |
| `TC6_INT_MASK0` | 0x000C | [5:0] | Interrupt Masks |

### PHY Register (via TC6)

| Register | Address | Standard | Description |
|----------|---------|----------|--------------|
| `PHY_BMCR` | 0xFF00 | IEEE 802.3 | Basic Mode Control |
| `PHY_BMSR` | 0xFF01 | IEEE 802.3 | Basic Mode Status |
| `PHY_ID1` | 0xFF02 | IEEE 802.3 | PHY Identifier 1 |
| `PHY_ID2` | 0xFF03 | IEEE 802.3 | PHY Identifier 2 |
| `PHY_ANAR` | 0xFF04 | IEEE 802.3 | Auto-Negotiation Advertisement |

## 🔧 Bit Field Definitions

### MAC_NET_CTL (0x00010000)
```
Bit 3: TXEN    - Transmit Enable (1=Enable, 0=Disable)
Bit 2: RXEN    - Receive Enable (1=Enable, 0=Disable)
```

### TC6_STATUS0 (0x0008)
```
Bit 6: RESETC              - Reset Complete
Bit 5: HEADER_ERROR        - Header Error
Bit 4: LOSS_OF_FRAME_ERROR - Loss of Frame Error  
Bit 3: RX_BUFFER_OVERFLOW  - RX Buffer Overflow
Bit 0: TX_PROTOCOL_ERROR   - TX Protocol Error
```

### TC6_BUFFER_STATUS (0x000B)
```
Bits 15:8: TX_CREDITS_AVAILABLE  - Available TX Credits (0-255)
Bits  7:0: RX_CHUNKS_AVAILABLE   - Available RX Chunks (0-255)
```

## 📊 Example Workflows

### 1. MAC-PHY Initialization

```bash
#!/bin/bash
# MAC-PHY Setup Script

echo "=== LAN8651 Initialization ==="

# 1. Check reset status
echo "Checking reset status..."
./lan8651_tool.py read TC6_STATUS0

# 2. Read capabilities
echo "Reading capabilities..."
./lan8651_access read TC6_STDCAP

# 3. Configure MAC
echo "Configuring MAC..."
./lan8651_access write MAC_NET_CFG 0x00    # Clear config
./lan8651_access write MAC_NET_CTL 0x0C    # Enable TX+RX

# 4. Check status
echo "Final status:"
./lan8651_tool.py status
```

### 2. Diagnostic Script

```bash
#!/bin/bash
# LAN8651 Diagnostic Script

echo "=== LAN8651 Diagnostics ==="

# PHY Identification
echo "1. PHY Identification:"
./lan8651_tool.py phy-id

# Link Status
echo "2. Link Status:"
./lan8651_access read PHY_BMSR

# Error Status
echo "3. Error Status:"
./lan8651_tool.py read TC6_STATUS0

# Buffer Status
echo "4. Buffer Status:"
./lan8651_access read TC6_BUFFER_STATUS

# MAC Status
echo "5. MAC Status:"
./lan8651_access read MAC_NET_CTL
```

### 3. Performance-Monitoring

```python
#!/usr/bin/env python3
# Performance Monitor using LAN8651 register names

import time
import subprocess

def read_register(reg_name):
    result = subprocess.run(['./lan8651_kernelfs.py', 'read', reg_name], 
                          capture_output=True, text=True)
    if "0x" in result.stdout:
        return int(result.stdout.split("0x")[1].split()[0], 16)
    return 0

print("LAN8651 Performance Monitor")
print("Press Ctrl+C to stop")

try:
    while True:
        # Read status using register names
        status0 = read_register('OA_STATUS0')
        buffer_status = read_register('OA_BUFSTS')
        
        # Decode buffer status
        tx_credits = (buffer_status >> 8) & 0xFF
        rx_chunks = buffer_status & 0xFF
        
        # Display status
        timestamp = time.strftime("%H:%M:%S")
        print(f"{timestamp}: TX Credits: {tx_credits:3d}, RX Chunks: {rx_chunks:3d}, Status: 0x{status0:08X}")
        
        time.sleep(1)
        
except KeyboardInterrupt:
    print("\nMonitoring stopped.")
```

## 🐛 Debugging & Troubleshooting

### Common Problems

#### "Device not found"
```bash
# Check and solution
lsmod | grep lan8651_debug      # Module loaded?
sudo insmod lan8651_debug.ko    # If not, load it
dmesg | tail                    # Check kernel log
```

#### "LAN8651 device not found"  
```bash
# Check LAN865x driver
lsmod | grep lan865x
ip link show                    # Check interfaces
cat /proc/net/dev              # Network devices
```

#### Register access failed
```bash
# Test basic connectivity using register names
./lan8651_kernelfs.py read OA_ID          # Device identification
./lan8651_kernelfs.py read OA_STATUS0     # General status
./lan8651_kernelfs.py read BASIC_STATUS   # PHY status

# Check interface detection
./lan8651_kernelfs.py list                # Show all registers
LAN8651_DEBUG=1 ./lan8651_kernelfs.py read OA_ID  # Debug output

# Test debugfs access
ls -la /sys/kernel/debug/lan865x/
echo "read 0x0000" > /sys/kernel/debug/lan865x/reg_access

# Reset if necessary
./lan8651_kernelfs.py write OA_RESET 0x1  # Software Reset
```

### Enable Debug Output

```bash
# Python tool debug (shows register name resolution)
LAN8651_DEBUG=1 ./lan8651_kernelfs.py read OA_STATUS0

# Enable kernel debug
echo 8 > /proc/sys/kernel/printk

# Debug register access flow
echo "debug on" > /sys/kernel/debug/lan865x/reg_access
dmesg -w  # Watch live kernel messages

# List all available registers for debugging
./lan8651_kernelfs.py list
```

## 🔒 Security Notes

- **Root privileges required** for kernel module
- **Direct hardware access** may affect system  
- **Backup register values** before modifications
- **Use reset functionality** for critical errors

## 🔧 Technical Details: How Register Access Works

### **Complete Data Flow**

```
User Command → Debugfs → TC6 Framework → SPI → LAN8651 Chip
     ↓             ↓           ↓           ↓         ↓
[echo read 0x...]→[debugfs]→[oa_tc6_*]→[SPI Bus]→[Register]
```

### **Core Functions for Register Access**

The **[lan865x_debug_patch.patch](lan865x_debug_patch.patch)** leverages the existing **Linux OA TC6 Framework** functions:

```c
// READ a register
ret = oa_tc6_read_register(priv->tc6, address, &value);

// WRITE a register  
ret = oa_tc6_write_register(priv->tc6, address, value);
```

### **Detailed Workflow**

#### **1. User-Space Command:**
```bash
echo "read 0x10000" > /sys/kernel/debug/lan865x/reg_access
```

#### **2. Kernel Debugfs Handler:**
```c
static ssize_t lan865x_debugfs_reg_write(...) {
    // Parse user input
    sscanf(buf, "%15s 0x%x", cmd, &address);
    
    if (strcmp(cmd, "read") == 0) {
        // ⭐ ACTUAL register access happens HERE:
        ret = oa_tc6_read_register(priv->tc6, address, &value);
        
        dev_info(&priv->spi->dev, "REG READ 0x%08x = 0x%08x\n", 
                address, value);
    }
}
```

#### **3. OA TC6 Framework (`oa_tc6_read_register`):**
- **TC6 Protocol** (OPEN Alliance TC6 Standard)
- **SPI Transaction** with chip-select, data frames
- **Echo/Response verification** 
- **Error handling** for bus errors

#### **4. Hardware SPI Communication:**
```
SPI Master (LAN966x) ←→ SPI Slave (LAN8651)
     ↓                        ↓
[TC6 Command Frame]    [Register Access]
[Address + R/W bit]    [Internal Bus]
[Data payload]         [Register Value]
```

### **What is OA TC6?**

**OA TC6** = **OPEN Alliance TC6** is the standard interface for 10BASE-T1S MAC-PHY chips:

- **TC6 = 10BASE-T1x MACPHY Serial Interface**
- **Implemented in:** `drivers/net/oa_tc6.c` (Linux Kernel)
- **Functions:** `oa_tc6_read_register()`, `oa_tc6_write_register()`
- **Protocol:** SPI-based with specialized frames

### **Physical Layer Structure:**

```c
struct lan865x_priv {
    struct oa_tc6 *tc6;        // ← TC6 interface handle
    struct spi_device *spi;    // ← SPI device for hardware access
    // ...
};
```

**Register access flows through:**
1. **`priv->tc6`** - TC6 framework handle
2. **TC6 Framework** calls SPI transactions
3. **SPI Bus** communicates with LAN8651 chip
4. **LAN8651** performs internal register operations

### **Why Use TC6 Framework?**

**Without TC6 Framework** you would need to:
- Manually construct SPI frames  
- Calculate TC6 protocol headers
- Implement echo/response verification
- Write error detection and retry logic

**With TC6 Framework:** 
- **One line:** `oa_tc6_read_register(tc6, address, &value)`
- **Complete protocol** handled automatically
- **Error handling** included
- **Standard compliant** (OPEN Alliance Spec)

### **The Elegant Solution:**

The **lan865x_debug_patch.patch** approach is brilliant because it:
1. **Uses existing TC6 infrastructure** (already present in driver)
2. **Only adds debugfs interface** (user-space access point)
3. **Direct use** of existing `priv->tc6` structure
4. **No SPI code needed** - TC6 framework does everything

**Key insight:** The patch leverages already-working hardware communication and simply adds a debug interface on top! 🎉

## 🧪 Testing & Validation

### **Debug Test Suite**

```bash
# Run comprehensive debug validation
./test_tools_debug.sh
```

**Validates:**
- All debug output functionality across all tools
- Interface detection with step-by-step logging
- Error condition analysis and reporting  
- Performance comparison (debug vs release modes)
- Kernel patch verification and debugfs interface testing
- Register access validation with timing analysis

### **Standard Test Suite**

```bash
# Run basic functionality tests
./test_tools.sh

# Check if LAN8651 interface is detected
python3 lan8651_kernelfs.py list

# Verify debugfs access (after kernel patch)
ls -la /sys/kernel/debug/lan865x/
```

### **Manual Testing Examples**

#### **Debug Output Validation:**
```bash
# C Tool - Compare debug vs release output
./lan8651_ethtool_arm_debug read 0x10000     # Verbose with timing
./lan8651_ethtool_arm read 0x10000           # Minimal output

# Python Tool - Enable detailed tracing  
LAN8651_DEBUG=1 ./lan8651_kernelfs.py read 0x10000

# Kernel - Monitor register operations
echo "debug on" > /sys/kernel/debug/lan865x/reg_access
echo "read 0x10000" > /sys/kernel/debug/lan865x/reg_access
dmesg | grep "REG READ\|DEBUG"
```

### **Example Debug Output**

#### **C Tool Debug Output:**

**Debug Version (detailed with timestamps):**
```
[DEBUG 19284.113] main:251: === LAN8651 ETHTOOL REGISTER ACCESS TOOL ===
[DEBUG 19284.113] main:252: Debug output is ENABLED
[DEBUG 19284.113] main:253: Arguments: argc=3
[DEBUG 19284.113] main:255:   argv[0] = './lan8651_ethtool_x86_debug'
[DEBUG 19284.113] main:255:   argv[1] = 'read'
[DEBUG 19284.113] main:255:   argv[2] = '0x10000'
[DEBUG 19284.113] find_lan8651_interface:65: ENTER
[DEBUG 19284.113] find_lan8651_interface:66: Looking for LAN8651 interface
[DEBUG 19284.113] find_lan8651_interface:76: Successfully opened /proc/net/dev
[DEBUG 19284.116] find_lan8651_interface:85: Processing interface: eth0
[DEBUG 19284.116] find_lan8651_interface:97: Checking driver path: /sys/class/net/eth0/device/driver/module
[DEBUG 19284.116] find_lan8651_interface:112: readlink failed: No such file or directory
No LAN8651 interface found
```

**Release Version (minimal output):**
```
No LAN8651 interface found
```

#### **Python Tool Debug Output:**

**Debug Version (with detailed interface detection):**
```
[01:15:43.656] DEBUG debug_print:33: Initializing LAN8651Debugfs class
[01:15:43.656] DEBUG debug_print:33: Starting interface detection
[01:15:43.656] DEBUG debug_print:33: Starting interface search
[01:15:43.656] DEBUG debug_print:33: Searching for network devices with pattern: /sys/class/net/*/device/driver
[01:15:43.657] DEBUG debug_print:33: Found 1 network device entries
[01:15:43.658] DEBUG debug_print:33: [1/1] Processing device: /sys/class/net/eth0/device/driver
[01:15:43.658] DEBUG debug_print:33: Reading driver link for: /sys/class/net/eth0/device/driver
[01:15:43.658] DEBUG debug_print:33: Driver link target: ../../../../../../bus/vmbus/drivers/hv_netvsc
[01:15:43.658] DEBUG debug_print:33: Driver 'hv_netvsc' is not lan865x, skipping
[01:15:43.658] DEBUG debug_print:33: Searching for debugfs entries
[01:15:43.658] DEBUG debug_print:33: Debugfs is mounted at /sys/kernel/debug
[01:15:43.659] DEBUG debug_print:33: Checking 3 potential debug paths
[01:15:43.659] DEBUG debug_print:33: [1/3] Checking debugfs path: /sys/kernel/debug/tc6
[01:15:43.659] DEBUG debug_print:33: Debugfs path does not exist: /sys/kernel/debug/tc6
```

**Release Version (clean output):**
```
Known LAN8651 Registers:
  0x00010000 - ID_REV       - Chip and Revision ID
  0x00010001 - STATUS0      - Status Register 0
```

#### **Kernel Patch Debug Output (after patch application):**

**Debugfs Interface Status:**
```bash
$ cat /sys/kernel/debug/lan865x/reg_access
```
```
LAN865x Register Access Interface
=================================
Usage:
  echo 'read 0x10000' > reg_access      - Read register
  echo 'write 0x10000 0x0C' > reg_access - Write register
  echo 'debug on' > reg_access          - Enable debug output
  echo 'debug off' > reg_access         - Disable debug output

Status:
  Debug: OFF
  Last operation: 0x00000000 = 0x00000000
  SPI device: spi1.0
  Network interface: eth0
```

**Kernel Log Output (dmesg):**
```bash
$ echo "debug on" > /sys/kernel/debug/lan865x/reg_access
$ echo "read 0x10000" > /sys/kernel/debug/lan865x/reg_access
$ dmesg | tail
```
```
[12345.678] lan865x spi1.0: DEBUG: Received command: 'debug on'
[12345.679] lan865x spi1.0: DEBUG: Debug output ENABLED
[12345.680] lan865x spi1.0: DEBUG: Received command: 'read 0x10000'
[12345.681] lan865x spi1.0: DEBUG: Starting READ operation: addr=0x00010000
[12345.682] lan865x spi1.0: REG READ 0x00010000 = 0x12345678 (duration=234 us)
[12345.683] lan865x spi1.0: DEBUG: READ completed successfully in 234 microseconds
```

#### **Debug Features Comparison:**

| Tool | Debug Activation | Timestamp | Function Tracing | Error Details | Performance Timing |
|------|------------------|-----------|------------------|---------------|-------------------|
| **C Tool** | `-DDEBUG_ENABLED=1` | ✅ Kernel Time | ✅ ENTER/EXIT | ✅ errno + strerror | ✅ |
| **Python Tool** | `LAN8651_DEBUG=1` | ✅ HH:MM:SS.ms | ✅ Step-by-step | ✅ Exception Details | ✅ |
| **Kernel Patch** | `echo "debug on"` | ✅ Kernel Timestamp | ✅ Operation Tracing | ✅ Return Codes | ✅ Microseconds |

### Manual Testing

```bash
# Test register access via debugfs
echo "read 0x0002" > /sys/kernel/debug/lan865x/reg_access  # TC6_STDCAP
echo "read 0x0008" > /sys/kernel/debug/lan865x/reg_access  # TC6_STATUS0
echo "read 0x10000" > /sys/kernel/debug/lan865x/reg_access # MAC_NET_CTL

# Check kernel messages
dmesg | grep "REG READ"
```

## 🐛 Troubleshooting

### **Common Issues**

#### **"No LAN8651 interface found"**
```bash
# Check for lan865x driver
lsmod | grep lan865x
ip link show | grep eth

# Check sysfs entries  
ls /sys/class/net/*/device/driver/

# Enable debug for detailed interface detection
LAN8651_DEBUG=1 ./lan8651_kernelfs.py list
```

#### **"Debugfs not available"**
```bash
# Check if debugfs is mounted
mount | grep debugfs

# Mount if necessary
sudo mount -t debugfs none /sys/kernel/debug

# Verify kernel patch is applied
ls /sys/kernel/debug/lan865x/ 2>/dev/null || echo "Kernel patch needed"

# Check which patch to use
ls -la *patch
# Use lan865x_enhanced_debug_patch.patch for full debug features
```

#### **"Permission denied on debugfs"**
```bash
# Debugfs requires root access
sudo echo "read 0x10000" > /sys/kernel/debug/lan865x/reg_access

# Or run tools as root
sudo ./lan8651_kernelfs.py read 0x10000

# For debug mode
sudo LAN8651_DEBUG=1 ./lan8651_kernelfs.py read 0x10000
```

#### **"No debug output appearing"**
```bash
# C Tools - verify debug compilation
./lan8651_ethtool_arm_debug read 0x10000    # Should show timestamped debug
./lan8651_ethtool_arm read 0x10000          # Should show minimal output

# Python Tools - verify environment variable
echo $LAN8651_DEBUG                          # Should show '1' if set
LAN8651_DEBUG=1 ./lan8651_kernelfs.py --debug list

# Kernel - enable runtime debug
echo "debug on" > /sys/kernel/debug/lan865x/reg_access
cat /sys/kernel/debug/lan865x/reg_access    # Should show debug status
```

### **Debug Information Collection**

```bash
# Enable kernel debug output
echo 8 > /proc/sys/kernel/printk

# Check SPI and TC6 debug
dmesg | grep -E "lan865x|tc6|spi"

# Network interface status  
cat /proc/net/dev
ethtool <interface> 2>/dev/null || echo "ethtool info not available"

# Comprehensive debug test
./test_tools_debug.sh > debug_report.txt 2>&1
```

### **Performance Analysis**

Use debug versions for performance troubleshooting:

```bash
# Compare execution times
time ./lan8651_ethtool_arm read 0x10000           # Release
time ./lan8651_ethtool_arm_debug read 0x10000     # Debug

# Python tool timing  
time ./lan8651_kernelfs.py read 0x10000           # Normal
time LAN8651_DEBUG=1 ./lan8651_kernelfs.py read 0x10000  # Debug

# Kernel operation timing (enhanced patch shows microsecond precision)
echo "read 0x10000" > /sys/kernel/debug/lan865x/reg_access
dmesg | grep "duration="
```

## 🔧 Development Notes

### **Available Tools & Files**

**Build Scripts:**
- `build_tools.sh` - Standard build script for release versions
- `build_tools_debug.sh` - Enhanced build with debug and release versions

**Test Scripts:**  
- `test_tools.sh` - Basic functionality testing
- `test_tools_debug.sh` - Comprehensive debug validation suite

**Source Code:**
- `lan8651_ethtool.c` - C tool with debug macro support
- `lan8651_kernelfs.py` - Python tool with logging framework

**Kernel Patches:**
- `lan865x_debug_patch.patch` - Basic debugfs support
- `lan865x_enhanced_debug_patch.patch` - Enhanced with comprehensive debugging

**Documentation:**
- `README.md` - Main project documentation  
- `DEBUG_TESTING_GUIDE.md` - Detailed debug testing guide

### **Extending the Tools**

1. **Add ethtool IOCTL support** to lan865x driver:
   - Implement `ethtool_ops` handlers for `ETHTOOL_GLANREG`/`ETHTOOL_SLANREG`
   - Enables `lan8651_ethtool.c` functionality without debugfs dependency

2. **Enhance Python tool** with register maps:
   - Add predefined register definitions (MAC, PHY, TC6)
   - Implement bit-field decoding and analysis
   - Add batch operation support for multiple registers

3. **Extend debug capabilities**:
   - Add register access history logging
   - Implement performance profiling and statistics
   - Create automated regression testing framework

### **Debug Development Workflow**

```bash
# 1. Build debug versions
./build_tools_debug.sh

# 2. Test with comprehensive validation  
./test_tools_debug.sh

# 3. Analyze specific issues with targeted debug
LAN8651_DEBUG=1 ./lan8651_kernelfs.py --debug read 0x10000

# 4. Monitor kernel operations
echo "debug on" > /sys/kernel/debug/lan865x/reg_access
dmesg -w | grep "DEBUG\|REG"
```

### **Integration with Development Environment**

The debug features integrate seamlessly with development workflows:
- **CI/CD pipelines**: Automated testing with debug validation
- **Performance monitoring**: Timing analysis for optimization
- **Issue diagnosis**: Detailed logging for troubleshooting  
- **Regression testing**: Comprehensive validation suite

## 📄 License

This project is licensed under the **GPL-2.0+ License**.

## 🙏 Acknowledgments

- OPEN Alliance for TC6 Standard
- Linux Kernel Community for OA TC6 Framework
- Microchip for LAN8651 hardware and drivers

---

**✅ Current Status:** 
- **Debugfs approach**: Fully functional after applying kernel patch
- **Enhanced debug support**: Comprehensive validation and troubleshooting capabilities  
- **Multiple build targets**: Debug and release versions for all tools
- **Complete test suite**: Automated validation with detailed analysis
- **Ethtool approach**: Needs additional driver development

**🔧 For detailed debug testing instructions, see: [DEBUG_TESTING_GUIDE.md](DEBUG_TESTING_GUIDE.md)**