# LAN865x Debugfs Interface

This document describes the usage of the debugfs interface in the LAN865x 10BASE-T1S MAC-PHY driver.

## 📋 Table of Contents

- [Debug Mechanism Functionality](#debug-mechanism-functionality)
- [Overview](#overview)  
- [Debugfs Structure](#debugfs-structure)
- [System Requirements](#system-requirements)
- [Module Management](#module-management)
- [Kernel Configuration for Modules](#kernel-configuration-for-modules)
- [Basic Usage](#basic-usage)
- [Important Register Addresses](#important-register-addresses)
- [Register Bit Definitions](#register-bit-definitions)
- [Practical Examples](#practical-examples)
- [Logging and Monitoring](#logging-and-monitoring)
- [Repository Files](#repository-files)
- [LAN8651 Register-Access Tools](#lan8651-register-access-tools)
- [Security Features](#security-features)
- [Troubleshooting](#troubleshooting)
- [Developer Notes](#developer-notes)
- [Warning](#warning)

---

## Debug Mechanism Functionality

⚡ **Parallel and Independent Operation:**

The debug mechanism runs completely parallel to normal driver functionality:

- The **normal Ethernet driver** functions completely independently - network traffic, MAC configuration, multicast handling etc. continue running
- The **debugfs interface** is an additional, separate channel only for debugging purposes  
- **No interference**: Debug accesses do not interfere with normal network operation

🔒 **Access Protection via debug_enabled Flag:**

Register access is protected by the `debug_enabled` switch:

```bash
# Enable debug → Register accesses allowed
echo 1 > /sys/kernel/debug/lan865x/debug_enable

# Disable debug → All register accesses blocked  
echo 0 > /sys/kernel/debug/lan865x/debug_enable
```

**Security aspect:**
- **Production environment**: Debug disabled → No unauthorized register manipulation
- **Development/Testing**: Debug enabled → Full access for diagnostics
- **Runtime switchable**: Flexible according to needs

## Overview

The debugfs interface provides a comprehensive interface for debugging the LAN865x Ethernet driver at runtime. It enables direct access to hardware registers and provides detailed status information.

## Debugfs Structure

The interface creates a debugfs directory under `/sys/kernel/debug/lan865x/` with the following files:

- `regs` - Register read/write access
- `debug_enable` - Debug status enable/disable (boolean)

## System Requirements

- Linux kernel with CONFIG_DEBUG_FS enabled
- Root privileges for debugfs access
- **Kernel module support** enabled
- LAN865x driver compiled as **loadable module**

## Module Management

The LAN865x driver is implemented as a kernel module for flexible development and testing.

### Automatic Loading at Boot

```bash
# Modules are automatically loaded at system startup
/etc/init.d/lan865x-modules start
```

### Manual Module Management

```bash
# Load modules
modprobe oa_tc6     # OA-TC6 library (dependency)
modprobe lan865x    # LAN865x driver

# Check module status
lsmod | grep -E "(oa_tc6|lan865x)"
/etc/init.d/lan865x-modules status

# Unload modules (for development)
rmmod lan865x
rmmod oa_tc6

# Quick reload for testing
/etc/init.d/lan865x-modules reload
```

### Developer Workflow

```bash
# 1. Copy new module version
cp lan865x.ko /lib/modules/$(uname -r)/kernel/drivers/net/ethernet/microchip/

# 2. Update module cache  
depmod -a

# 3. Unload old module and load new one
/etc/init.d/lan865x-modules reload

# 4. Driver is immediately active - no reboot required!
```

**Advantages of module implementation:**
- ✅ Fast development cycles (no complete kernel rebuild)
- ✅ Runtime loading/unloading for testing
- ✅ Easy updates without restart
- ✅ Debug-friendly

## Kernel Configuration for Modules

For module development, a specific kernel configuration is required. This is managed via the `config_manager.sh` script:

### Initial Setup (after Git Clone)

```bash
# Apply saved kernel configuration
./config_manager.sh apply

# Check status
./config_manager.sh status

# Recompile kernel with module support
cd /home/martin/AIoT/lan9662/mchp-brsdk-source-2025.12
make linux-rebuild O=output/mybuild_regacces
```

### Configuration Management

```bash
# Save current kernel config (after successful builds)
./config_manager.sh backup

# Show differences between saved and current
./config_manager.sh diff

# Restore saved config (in case of problems)
./config_manager.sh apply

# Show configuration status
./config_manager.sh status

# Delete backup
./config_manager.sh clean
```

### Important Kernel Settings

The saved `kernel.config` contains:
- `CONFIG_LAN865X=m` - LAN865x as loadable module
- `CONFIG_OA_TC6=m` - OA-TC6 library as loadable module  
- `CONFIG_MODULES=y` - Module support enabled
- `CONFIG_MODULE_UNLOAD=y` - Modules can be unloaded
- All other dependencies required for LAN865x

## Basic Usage

### 1. Enable/disable debug

```bash
# Enable debug
echo 1 > /sys/kernel/debug/lan865x/debug_enable

# Disable debug  
echo 0 > /sys/kernel/debug/lan865x/debug_enable
```

### 2. Show register status

```bash
cat /sys/kernel/debug/lan865x/regs
```

**Example Output:**
```
=== LAN865x Register Debug Info ===
MAC_NET_CTL (0x00010000): 0x0000000c
  TX_EN: ON
  RX_EN: ON
Last accessed: addr=0x00010000, val=0x0000000c
Debug enabled: YES

Usage: echo 'addr value' > regs  # Write register
       echo 'addr' > regs        # Read register
```

### 3. Read registers

```bash
# Read MAC Network Control Register
echo "00010000" > /sys/kernel/debug/lan865x/regs

# Read MAC Network Configuration Register
echo "00010001" > /sys/kernel/debug/lan865x/regs

# Read MAC Address Low Bytes
echo "00010022" > /sys/kernel/debug/lan865x/regs
```

### 4. Write registers

```bash
# Enable TX and RX (set bits 2 and 3)
echo "00010000 0000000c" > /sys/kernel/debug/lan865x/regs

# Enable promiscuous mode (set bit 4 in NET_CFG)
echo "00010001 00000010" > /sys/kernel/debug/lan865x/regs

# Enable multicast mode (set bit 6 in NET_CFG)
echo "00010001 00000040" > /sys/kernel/debug/lan865x/regs
```

## Important Register Addresses

| Register | Address | Description |
|----------|---------|-------------|
| MAC_NET_CTL | 0x00010000 | Network Control (TX/RX Enable) |
| MAC_NET_CFG | 0x00010001 | Network Configuration (Promiscuous/Multicast) |
| MAC_L_HASH | 0x00010020 | MAC Hash Register Bottom |
| MAC_H_HASH | 0x00010021 | MAC Hash Register Top |
| MAC_L_SADDR1 | 0x00010022 | MAC Specific Address 1 Bottom |
| MAC_H_SADDR1 | 0x00010023 | MAC Specific Address 1 Top |
| MAC_TSU_TIMER_INCR | 0x00010077 | MAC TSU Timer Increment |

## Register Bit Definitions

### MAC_NET_CTL (0x00010000)
- Bit 3: `MAC_NET_CTL_TXEN` - Transmit Enable
- Bit 2: `MAC_NET_CTL_RXEN` - Receive Enable

### MAC_NET_CFG (0x00010001)
- Bit 4: `MAC_NET_CFG_PROMISCUOUS_MODE` - Promiscuous Mode
- Bit 6: `MAC_NET_CFG_MULTICAST_MODE` - Multicast Mode
- Bit 7: `MAC_NET_CFG_UNICAST_MODE` - Unicast Mode

## Practical Examples

### Enable/disable hardware

```bash
# Enable hardware completely (TX + RX)
echo "00010000 0000000c" > /sys/kernel/debug/lan865x/regs

# Enable only TX
echo "00010000 00000008" > /sys/kernel/debug/lan865x/regs

# Enable only RX
echo "00010000 00000004" > /sys/kernel/debug/lan865x/regs

# Disable hardware
echo "00010000 00000000" > /sys/kernel/debug/lan865x/regs
```

### Configure network modes

```bash
# Promiscuous mode
echo "00010001 00000010" > /sys/kernel/debug/lan865x/regs

# Multicast mode
echo "00010001 00000040" > /sys/kernel/debug/lan865x/regs

# Normal mode (only local MAC address)
echo "00010001 00000000" > /sys/kernel/debug/lan865x/regs
```

### Read MAC address

```bash
# Read MAC low bytes
echo "00010022" > /sys/kernel/debug/lan865x/regs

# Read MAC high bytes
echo "00010023" > /sys/kernel/debug/lan865x/regs
```

## Logging and Monitoring

All register accesses are documented in the kernel log:

```bash
# Follow kernel log in real-time
dmesg -w | grep lan865x

# Show last entries
dmesg | tail -20 | grep "REG_READ\|REG_WRITE"
```

**Example Log Output:**
```
[  123.456] lan865x: REG_READ: 0x00010000 = 0x0000000c
[  124.567] lan865x: REG_WRITE: 0x00010001 = 0x00000010
```

### Verbose Debug Logging

For detailed register access logging, conditional compilation is available:

**Activation:**
```c
/* Enable verbose debug logging for register access (comment out for production) */
#define CONFIG_LAN865X_DEBUG_VERBOSE
```

**Deactivation (for production):**
```c
/* Enable verbose debug logging for register access (comment out for production) */
// #define CONFIG_LAN865X_DEBUG_VERBOSE
```

**Behavior:**
- **Enabled**: Each debugfs register access is additionally written to kernel log
- **Disabled**: Optimized performance, no verbose logging (recommended for production)
- **Debug info**: Always available via `cat /sys/kernel/debug/lan865x/regs`

**Performance Note:** 
⚠️ Verbose logging can slow down the system with many register accesses. Enable only for testing/debugging!

## Repository Files

This repository contains the following important files for LAN865x module development:

### **Source Code and Documentation**
- `lan865x.c` - Main driver source code with debugfs interface
- `README.md` - This documentation

### **Configuration Management**
- `kernel.config` - Saved kernel configuration with module support
- `config_manager.sh` - Script for managing kernel configuration

### **Using Repository Files**

```bash
# After cloning the repository:
git clone https://github.com/zabooh/lan8651_patch.git
cd lan8651_patch

# 1. Restore kernel configuration
./config_manager.sh apply

# 2. Compile kernel with correct settings
cd /home/martin/AIoT/lan9662/mchp-brsdk-source-2025.12
make linux-rebuild O=output/mybuild_regacces

# 3. Modules are ready for development and testing
```

**Self-documenting workflow:**
- All necessary configurations are saved in the repository
- Reproducible builds on different systems
- No manual configuration steps required

## LAN8651 Register-Access Tools

In addition to the debugfs interface, comprehensive tools for direct access to LAN8651 registers are available:

### 📦 Available Tools (Directory: `lan8651-regaccess/`)

#### **1. Python Tool - `lan8651_kernelfs.py` (✅ Functional)**
Complete register access tool via the debugfs interface:

```bash
# Read register by name
python3 lan8651_kernelfs.py read OA_STATUS0

# Read register by address  
python3 lan8651_kernelfs.py read 0x0008

# Write register
python3 lan8651_kernelfs.py write OA_CONFIG0 0x12345678

# List all available registers
python3 lan8651_kernelfs.py list

# Device status overview
python3 lan8651_kernelfs.py status

# Enable debug mode
LAN8651_DEBUG=1 python3 lan8651_kernelfs.py status
```

**Features:**
- **Register name resolution**: Use names instead of hexadecimal addresses
- **30+ official registers** from the Microchip datasheet
- **Bit field decoding**: Automatic interpretation of status/control bits  
- **Automatic interface detection**: Finds LAN8651 devices automatically
- **Comprehensive debugging**: Detailed debug outputs

#### **2. C Tool - `lan8651_ethtool.c` (⚠️ Requires Driver Extension)**
Ethtool-based register access tool:

```bash
# Compiled binaries for different architectures
./lan8651_ethtool_arm_debug read 0x0008
./lan8651_ethtool_x86_debug write 0x0004 0x12345678
```

**Features:**
- **Cross-platform**: ARM and x86 binaries available
- **Ethtool integration**: Uses standard Linux ethtool interface
- **Debug support**: Compile-time debug options
- **Direct kernel communication**: Via IOCTL without filesystem accesses

### 📚 Detailed Documentation

The `lan8651-regaccess/` directory contains comprehensive documentation:

- **[LAN8651 Tools README](lan8651-regaccess/README.md)** - Complete tool documentation
- **[Register Map](lan8651-regaccess/LAN8651_REGISTER_MAP.md)** - Complete register reference from the Microchip datasheet
- **[Debug Testing Guide](lan8651-regaccess/DEBUG_TESTING_GUIDE.md)** - Comprehensive debug testing
- **[Register Update Summary](lan8651-regaccess/REGISTER_UPDATE_SUMMARY.md)** - Change log

### 🔧 Build & Test Tools

```bash
# Compile tools
lan8651-regaccess/build_tools.sh

# Test tools
lan8651-regaccess/test_tools.sh

# Debug versions 
lan8651-regaccess/build_tools_debug.sh
lan8651-regaccess/test_tools_debug.sh
```

### 🏗️ Architecture

```
┌─────────────────┐    ┌─────────────────┐    ┌─────────────────┐
│   User Tools    │    │ Patched Driver  │    │   Hardware      │
│                 │    │                 │    │                 │
│lan8651_kernelfs │◄──►│ lan865x driver  │◄──►│   LAN8651       │
│lan8651_ethtool* │    │   + debugfs     │    │   (SPI/TC6)     │
│                 │    │                 │    │                 │
└─────────────────┘    └─────────────────┘    └─────────────────┘
        │                        │                        │
        ▼                        ▼                        ▼
/sys/kernel/debug/lan865x    OA TC6 Framework         SPI Interface
```

**Integration:**
- **Python Tool**: Uses the already implemented debugfs interface  
- **C Tool**: Can be extended with additional ethtool handlers if needed
- **Both tools**: Use official register definitions from the Microchip datasheet

The tools provide a **complete abstraction layer** for LAN8651 register accesses and perfectly complement the debugfs interface for comprehensive hardware diagnostics and development.

## Security Features

- **Access protection**: Debug access only when `debug_enabled = true`
- **Input validation**: Automatic verification of input formats
- **Error handling**: Comprehensive error output for failed operations
- **Permissions**: Root access required (file permissions: 0600)

## Troubleshooting

### Debug interface not available
```bash
# Check if debugfs is mounted
mount | grep debugfs

# Mount debugfs manually if needed
mount -t debugfs none /sys/kernel/debug
```

### Driver not loaded
```bash
# Check driver status
lsmod | grep lan865x

# Show SPI devices
cat /sys/bus/spi/devices/*/modalias
```

### Access errors
```bash
# Check permissions
ls -la /sys/kernel/debug/lan865x/

# Run as root
sudo bash
```

## Developer Notes

The debugfs interface is implemented in the following functions:

- `lan865x_debugfs_init()` - Interface initialization
- `lan865x_debugfs_reg_read()` - Register read access
- `lan865x_debugfs_reg_write()` - Register write access  
- `lan865x_debugfs_remove()` - Interface cleanup

The debug functionality is enabled by default (`debug_enabled = true`) and can be controlled at runtime via the `debug_enable` file.

## Warning

⚠️ **Caution when writing registers!** Improper register values can damage the hardware or lead to unstable behavior. Use this interface only if you understand the hardware specification.

