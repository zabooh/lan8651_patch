# LAN8651 Debug Testing Guide

## 🧪 Comprehensive Debug Features Added

All LAN8651 register access tools now have extensive debug capabilities for thorough testing and validation.

## 📋 Debug Features Overview

### **C Tool Debug Features** (`lan8651_ethtool.c`)
- **Compile-time debug control**: `DEBUG_ENABLED` macro
- **Timestamped debug output**: Function entry/exit tracking
- **Detailed error reporting**: errno values and descriptions
- **IOCTL debugging**: Complete request/response analysis
- **Hex dump capability**: Raw data inspection
- **Socket operations tracking**: Connection and communication details

**Debug Macros:**
- `DEBUG_PRINT(fmt, ...)` - Timestamped debug messages
- `DEBUG_ENTER()` - Function entry logging
- `DEBUG_EXIT(ret)` - Function exit with return value
- `DEBUG_HEX_DUMP(data, len)` - Hex data dumping

### **Python Tool Debug Features** (`lan8651_kernelfs.py`)
- **Environment variable control**: `LAN8651_DEBUG=1`
- **Comprehensive logging**: Info, debug, error levels with timestamps
- **Interface detection debugging**: Step-by-step sysfs/debugfs discovery
- **Kernel message parsing**: Register operation result tracking
- **Error condition analysis**: Detailed exception handling
- **Performance measurement**: Operation timing

**Debug Functions:**
- `debug_print(msg, *args)` - Debug message output
- `info_print(msg, *args)` - Information logging
- `error_print(msg, *args)` - Error reporting

### **Enhanced Kernel Patch Debug Features**
- **Runtime debug control**: Enable/disable via debugfs commands
- **Operation timing**: Microsecond-precision performance measurement  
- **Enhanced error reporting**: Detailed failure analysis with return codes
- **Usage information**: Interactive help system in debugfs
- **State tracking**: Last operation results and debug status
- **Comprehensive logging**: All operations logged with timing

**Debugfs Commands:**
- `echo 'debug on' > reg_access` - Enable detailed kernel logging
- `echo 'debug off' > reg_access` - Disable debug output
- `echo 'read 0xADDR' > reg_access` - Read register with timing
- `echo 'write 0xADDR 0xVALUE' > reg_access` - Write register with timing

## 🛠️ Build Instructions

### **Build All Debug Versions:**
```bash
./build_tools_debug.sh
```

This creates:
- `lan8651_ethtool_arm_debug` - ARM version with debug enabled
- `lan8651_ethtool_arm` - ARM version release mode
- `lan8651_ethtool_x86_debug` - x86 version with debug enabled  
- `lan8651_ethtool_x86` - x86 version release mode
- `lan8651_kernelfs_debug.py` - Python wrapper with debug auto-enabled

### **Manual Compilation with Debug:**
```bash
# ARM with debug
arm-linux-gcc -DDEBUG_ENABLED=1 -o lan8651_ethtool_arm_debug lan8651_ethtool.c

# x86 with debug
gcc -DDEBUG_ENABLED=1 -o lan8651_ethtool_x86_debug lan8651_ethtool.c

# Release versions
arm-linux-gcc -DDEBUG_ENABLED=0 -o lan8651_ethtool_arm lan8651_ethtool.c
gcc -DDEBUG_ENABLED=0 -o lan8651_ethtool_x86 lan8651_ethtool.c
```

## 🧪 Testing & Validation

### **Comprehensive Test Suite:**
```bash
./test_tools_debug.sh
```

This test script validates:
- ✅ All debug output functionality
- ✅ Interface detection with detailed logging
- ✅ Error condition handling
- ✅ Performance comparison (debug vs release)
- ✅ Kernel patch verification
- ✅ Debugfs interface testing
- ✅ Register access validation

### **Manual Testing Examples:**

#### **C Tool Debug Testing:**
```bash
# Debug mode - verbose output with timestamps
./lan8651_ethtool_arm_debug read 0x10000

# Release mode - minimal output
./lan8651_ethtool_arm read 0x10000

# Expected debug output:
# [DEBUG 12345.123] find_lan8651_interface:45: Looking for LAN8651 interface
# [DEBUG 12345.124] find_lan8651_interface:52: Processing interface: eth0
# [DEBUG 12345.125] lan8651_read_register:78: Interface: eth0, Address: 0x00010000
```

#### **Python Tool Debug Testing:**
```bash
# Enable debug via environment variable
LAN8651_DEBUG=1 ./lan8651_kernelfs.py read 0x10000

# Or use debug wrapper
./lan8651_kernelfs_debug.py read 0x10000

# With command-line debug flag
./lan8651_kernelfs.py --debug read 0x10000

# Expected debug output:
# [12:34:56.789] DEBUG find_interfaces:34: Starting interface search
# [12:34:56.790] DEBUG find_interfaces:45: Found 3 network device entries
```

#### **Kernel Patch Debug Testing:**
```bash
# Apply enhanced debug patch first
cd /path/to/kernel/source
patch -p1 < lan865x_enhanced_debug_patch.patch
# Rebuild kernel, install, reboot

# Enable kernel debug mode
echo "debug on" > /sys/kernel/debug/lan865x/reg_access

# Test register operations
echo "read 0x10000" > /sys/kernel/debug/lan865x/reg_access

# Monitor kernel output
dmesg | grep "REG READ\|DEBUG"

# Expected kernel output:
# [12345.678] lan865x 1-0: DEBUG: Starting READ operation: addr=0x00010000  
# [12345.679] lan865x 1-0: REG READ 0x00010000 = 0x00000000 (duration=234 us)
```

## 📊 Debug Output Examples

### **Successful Interface Detection:**
```
[DEBUG 12345.123] find_lan8651_interface:45: ENTER
[DEBUG 12345.124] find_lan8651_interface:52: Processing interface: eth0
[DEBUG 12345.125] find_lan8651_interface:67: Checking driver path: /sys/class/net/eth0/device/driver/module
[DEBUG 12345.126] find_lan8651_interface:72: Driver link target: /lib/modules/5.15.0/kernel/drivers/net/ethernet/microchip/lan865x
[DEBUG 12345.127] find_lan8651_interface:75: Found LAN865x driver for interface: eth0
[DEBUG 12345.128] find_lan8651_interface:79: EXIT with 0
```

### **Register Access with Timing:**
```
[DEBUG 12345.200] lan8651_read_register:95: ENTER
[DEBUG 12345.201] lan8651_read_register:96: Interface: eth0, Address: 0x00010000
[DEBUG 12345.202] lan8651_read_register:103: Socket created successfully: fd=3
[DEBUG 12345.203] lan8651_read_register:125: Calling ETHTOOL_GDRVINFO ioctl
[DEBUG 12345.204] lan8651_read_register:135: Driver info: driver='lan865x', version='1.0', fw_version=''
[DEBUG 12345.205] lan8651_read_register:145: Preparing register access: cmd=0x00001000, address=0x00010000
```

### **Error Condition Analysis:**
```
[DEBUG 12345.300] lan8651_read_register:150: Register read ioctl failed: Operation not supported (errno=95)
[DEBUG 12345.301] lan8651_read_register:151: This is expected - driver extension needed for register access
```

## 🎯 Troubleshooting Guide

### **No Debug Output Appearing:**
1. **C Tools**: Verify compiled with `-DDEBUG_ENABLED=1`
2. **Python Tool**: Check `LAN8651_DEBUG=1` environment variable
3. **Kernel Patch**: Ensure `echo "debug on" > reg_access` was executed

### **Permission Denied Errors:**
```bash
# Run as root for debugfs access
sudo echo "read 0x10000" > /sys/kernel/debug/lan865x/reg_access

# Or for Python tool
sudo LAN8651_DEBUG=1 ./lan8651_kernelfs.py read 0x10000
```

### **Interface Not Found:**
- Check debug output for interface detection details
- Verify LAN865x driver is loaded: `lsmod | grep lan865x`
- Check available interfaces: `ip link show`

### **Kernel Patch Not Working:**
- Verify patch applied: `ls /sys/kernel/debug/lan865x/`
- Check kernel messages: `dmesg | grep lan865x`
- Ensure debugfs mounted: `mount | grep debugfs`

## 📈 Performance Analysis

The debug versions include timing information:
- **Function execution time** tracking
- **IOCTL operation duration** measurement  
- **Kernel register access timing** (microsecond precision)
- **Interface detection performance** analysis

Use debug output to identify bottlenecks and optimize performance.

## ✅ Validation Checklist

- [ ] All tools compile without warnings in debug mode
- [ ] Debug output appears when enabled
- [ ] Release versions have no debug overhead
- [ ] Interface detection works with detailed logging
- [ ] Error conditions produce informative debug output
- [ ] Kernel patch provides timing and state information
- [ ] Test suite passes all validation checks

**The debug features provide complete visibility into every aspect of the LAN8651 register access tools for thorough testing and validation!** 🧪🔧