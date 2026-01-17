#!/usr/bin/env python3
"""
LAN8651 Register Access via existing kernel debugfs/sysfs interfaces

This approach uses existing kernel infrastructure instead of 
creating a custom kernel module.
"""

import os
import glob
import struct
import subprocess
import sys
import time
import logging
import argparse
from pathlib import Path

# Debug output control
DEBUG_ENABLED = os.environ.get('LAN8651_DEBUG', '0') == '1'

# LAN8651 Register Definitions (from official datasheet)
# MMS 0: Open Alliance Standard Registers
LAN8651_REGISTERS = {
    # Standard Control/Status Registers
    'OA_ID': 0x0000,           # Open Alliance ID
    'OA_PHYID': 0x0001,        # PHY Identification
    'OA_STDCAP': 0x0002,       # Standard Capabilities
    'OA_RESET': 0x0003,        # Reset Control
    'OA_CONFIG0': 0x0004,      # Configuration 0
    'OA_STATUS0': 0x0008,      # Status 0
    'OA_STATUS1': 0x0009,      # Status 1
    'OA_BUFSTS': 0x000B,       # Buffer Status
    'OA_IMASK0': 0x000C,       # Interrupt Mask 0
    'OA_IMASK1': 0x000D,       # Interrupt Mask 1
    
    # Timestamp Capture Registers
    'TTSCAH': 0x0010,          # TX Timestamp Capture A High
    'TTSCAL': 0x0011,          # TX Timestamp Capture A Low
    'TTSCBH': 0x0012,          # TX Timestamp Capture B High
    'TTSCBL': 0x0013,          # TX Timestamp Capture B Low
    'TTSCCH': 0x0014,          # TX Timestamp Capture C High
    'TTSCCL': 0x0015,          # TX Timestamp Capture C Low
    
    # Clause 22 Basic Control/Status
    'BASIC_CONTROL': 0xFF00,   # Basic Control Register
    'BASIC_STATUS': 0xFF01,    # Basic Status Register
    'PHY_ID1': 0xFF02,         # PHY Identifier 1
    'PHY_ID2': 0xFF03,         # PHY Identifier 2
    'MMDCTRL': 0xFF0D,         # MMD Access Control
    'MMDAD': 0xFF0E,           # MMD Access Address/Data
    
    # MMS 1: MAC Registers
    'MAC_NCR': 0x10000,        # MAC Network Control
    'MAC_NCFGR': 0x10001,      # MAC Network Configuration
    'MAC_HRB': 0x10020,        # MAC Hash Register Bottom
    'MAC_HRT': 0x10021,        # MAC Hash Register Top
    'MAC_SAB1': 0x10022,       # MAC Specific Address 1 Bottom
    'MAC_SAT1': 0x10023,       # MAC Specific Address 1 Top
    'MAC_SAB2': 0x10024,       # MAC Specific Address 2 Bottom
    'MAC_SAT2': 0x10025,       # MAC Specific Address 2 Top
    'BMGR_CTL': 0x10200,       # Buffer Manager Control
    'STATS0': 0x10208,         # Statistics 0
    'STATS1': 0x10209,         # Statistics 1
    'STATS2': 0x1020A,         # Statistics 2
}

# Register bit definitions
LAN8651_STATUS0_BITS = {
    'PHYINT': (1 << 7),        # PHY Interrupt
    'RESETC': (1 << 6),        # Reset Complete
    'HDRE': (1 << 5),          # Header Error
    'LOFE': (1 << 4),          # Loss of Frame Error
    'RXBOE': (1 << 3),         # RX Buffer Overflow Error
    'TXBUE': (1 << 2),         # TX Buffer Underflow Error
    'TXBOE': (1 << 1),         # TX Buffer Overflow Error
    'TXPE': (1 << 0),          # TX Protocol Error
}

LAN8651_BASIC_CONTROL_BITS = {
    'RESET': (1 << 15),        # Software Reset
    'LOOPBACK': (1 << 14),     # Loopback Enable
    'SPEED_SEL': (1 << 13),    # Speed Selection
    'ANENABLE': (1 << 12),     # Auto-Negotiation Enable
    'PDOWN': (1 << 11),        # Power Down
    'ANRESTART': (1 << 9),     # Restart Auto-Negotiation
    'FULLDPLX': (1 << 8),      # Full Duplex
}

LAN8651_BASIC_STATUS_BITS = {
    'ANEGCOMPLETE': (1 << 5),  # Auto-Negotiation Complete
    'RFAULT': (1 << 4),        # Remote Fault
    'ANEGCAPABLE': (1 << 3),   # Auto-Negotiation Capable
    'LSTATUS': (1 << 2),       # Link Status
    'JCD': (1 << 1),           # Jabber Detect
    'ERCAP': (1 << 0),         # Extended Register Capable
}

# Setup logging
logging.basicConfig(
    level=logging.DEBUG if DEBUG_ENABLED else logging.INFO,
    format='[%(asctime)s.%(msecs)03d] %(levelname)s %(funcName)s:%(lineno)d: %(message)s',
    datefmt='%H:%M:%S'
)
logger = logging.getLogger(__name__)

def debug_print(msg, *args):
    """Print debug message if debug is enabled"""
    if DEBUG_ENABLED:
        logger.debug(msg, *args)
        
def info_print(msg, *args):
    """Print info message"""
    logger.info(msg, *args)

def error_print(msg, *args):
    """Print error message"""
    logger.error(msg, *args)

def get_register_name(addr):
    """Get register name from address"""
    for name, address in LAN8651_REGISTERS.items():
        if address == addr:
            return name
    return f"0x{addr:04X}"

def parse_register_address(addr_str):
    """Parse register address from string (name or hex)"""
    # Try to parse as register name first
    if addr_str.upper() in LAN8651_REGISTERS:
        return LAN8651_REGISTERS[addr_str.upper()]
    
    # Try to parse as hex address
    try:
        if addr_str.startswith('0x') or addr_str.startswith('0X'):
            return int(addr_str, 16)
        else:
            return int(addr_str, 16)
    except ValueError:
        raise ValueError(f"Invalid register address: {addr_str}")

def decode_register_bits(addr, value):
    """Decode register bits for known registers"""
    if addr == LAN8651_REGISTERS['OA_STATUS0']:
        bits = []
        for name, bit_val in LAN8651_STATUS0_BITS.items():
            if value & bit_val:
                bits.append(name)
        return f"Status bits: {', '.join(bits) if bits else 'none'}"
    
    elif addr == LAN8651_REGISTERS['BASIC_CONTROL']:
        bits = []
        for name, bit_val in LAN8651_BASIC_CONTROL_BITS.items():
            if value & bit_val:
                bits.append(name)
        return f"Control bits: {', '.join(bits) if bits else 'none'}"
    
    elif addr == LAN8651_REGISTERS['BASIC_STATUS']:
        bits = []
        for name, bit_val in LAN8651_BASIC_STATUS_BITS.items():
            if value & bit_val:
                bits.append(name)
        return f"Status bits: {', '.join(bits) if bits else 'none'}"
    
    return None

class LAN8651Debugfs:
    def __init__(self):
        debug_print("Initializing LAN8651Debugfs class")
        self.debugfs_path = None
        self.sysfs_path = None
        debug_print("Starting interface detection")
        self.find_interfaces()
        debug_print("Initialization complete: debugfs_path=%s, sysfs_path=%s", 
                   self.debugfs_path, self.sysfs_path)
    
    def find_interfaces(self):
        """Find LAN8651 network interfaces via sysfs"""
        
        debug_print("Starting interface search")
        
        # Look for network interfaces using lan865x driver
        net_device_pattern = "/sys/class/net/*/device/driver"
        debug_print("Searching for network devices with pattern: %s", net_device_pattern)
        net_devices = glob.glob(net_device_pattern)
        debug_print("Found %d network device entries", len(net_devices))
        
        for i, device_path in enumerate(net_devices):
            debug_print("[%d/%d] Processing device: %s", i+1, len(net_devices), device_path)
            try:
                # Read the driver name
                debug_print("Reading driver link for: %s", device_path)
                driver_link = os.readlink(device_path)
                debug_print("Driver link target: %s", driver_link)
                
                if "lan865x" in driver_link:
                    # Found a LAN8651 interface
                    iface_name = device_path.split('/')[-3]
                    debug_print("Found LAN865x driver! Interface: %s", iface_name)
                    self.sysfs_path = f"/sys/class/net/{iface_name}/device"
                    info_print("Found LAN8651 interface: %s", iface_name)
                    debug_print("Set sysfs_path to: %s", self.sysfs_path)
                    break
                else:
                    debug_print("Driver '%s' is not lan865x, skipping", driver_link)
            except OSError as e:
                debug_print("OSError reading %s: %s", device_path, e)
                continue
        else:
            debug_print("No LAN865x interfaces found in sysfs")
        
        # Look for debugfs entries
        debug_print("Searching for debugfs entries")
        if os.path.exists("/sys/kernel/debug"):
            debug_print("Debugfs is mounted at /sys/kernel/debug")
            # Check for TC6 or lan865x specific debug entries
            debug_paths = [
                "/sys/kernel/debug/tc6",
                "/sys/kernel/debug/lan865x",
                "/sys/kernel/debug/spi"
            ]
            
            debug_print("Checking %d potential debug paths", len(debug_paths))
            for i, path in enumerate(debug_paths):
                debug_print("[%d/%d] Checking debugfs path: %s", i+1, len(debug_paths), path)
                if os.path.exists(path):
                    debug_print("Found debugfs entry: %s", path)
                    self.debugfs_path = path
                    info_print("Found debugfs interface: %s", path)
                    
                    # List contents for debugging
                    try:
                        contents = os.listdir(path)
                        debug_print("Debugfs contents: %s", contents)
                    except PermissionError as e:
                        debug_print("Permission denied listing %s: %s", path, e)
                    break
                else:
                    debug_print("Debugfs path does not exist: %s", path)
        else:
            debug_print("Debugfs is not mounted at /sys/kernel/debug")
            error_print("Debugfs not available - kernel may need CONFIG_DEBUG_FS=y")
    
    def read_via_debugfs(self, address):
        """Try to read register via debugfs if available"""
        
        if not self.debugfs_path:
            return None
        
        # This would depend on what the kernel driver exposes
        reg_file = f"{self.debugfs_path}/registers"
        if os.path.exists(reg_file):
            try:
                with open(reg_file, 'w') as f:
                    f.write(f"0x{address:08x}")
                
                with open(reg_file, 'r') as f:
                    result = f.read().strip()
                    return int(result, 0)
            except Exception as e:
                print(f"Debugfs read error: {e}")
        
        return None
    
    def read_via_spi_debug(self, address):
        """Try to access SPI debug information"""
        
        if not self.sysfs_path:
            return None
        
        # Check if there are SPI device attributes we can use
        spi_attrs = glob.glob(f"{self.sysfs_path}/spi*/registers") 
        if spi_attrs:
            try:
                with open(spi_attrs[0], 'w') as f:
                    f.write(f"read 0x{address:08x}")
                
                with open(spi_attrs[0], 'r') as f:
                    result = f.read().strip()
                    # Parse result - format depends on kernel implementation
                    if "=" in result:
                        value_str = result.split("=")[1].strip()
                        return int(value_str, 0)
            except Exception as e:
                print(f"SPI debug read error: {e}")
        
        return None
    
    def read_via_ethtool(self, iface, address):
        """Try to read via ethtool register dump"""
        
        try:
            # Use ethtool to dump registers
            result = subprocess.run(['ethtool', '-d', iface], 
                                  capture_output=True, text=True)
            
            if result.returncode == 0:
                # Parse ethtool output for our register
                lines = result.stdout.split('\n')
                for line in lines:
                    if f"0x{address:08x}" in line.lower():
                        # Extract value from line
                        parts = line.split()
                        for i, part in enumerate(parts):
                            if f"0x{address:08x}" in part.lower() and i+1 < len(parts):
                                return int(parts[i+1], 0)
        except Exception as e:
            print(f"Ethtool error: {e}")
        
        return None
    
    def read_register(self, address):
        """Try multiple methods to read register"""
        
        reg_name = get_register_name(address)
        debug_print("Attempting to read register %s (0x%08x)", reg_name, address)
        
        # Method 1: debugfs
        value = self.read_via_debugfs(address)
        if value is not None:
            info_print("Read via debugfs: %s = 0x%08x", reg_name, value)
            return value
        
        # Method 2: SPI debug
        value = self.read_via_spi_debug(address)
        if value is not None:
            info_print("Read via SPI debug: %s = 0x%08x", reg_name, value)
            return value
        
        # Method 3: Find network interface and try ethtool
        if self.sysfs_path:
            iface = self.sysfs_path.split('/')[-2]
            value = self.read_via_ethtool(iface, address)
            if value is not None:
                info_print("Read via ethtool: %s = 0x%08x", reg_name, value)
                return value
        
        error_print("All read methods failed for %s - kernel driver extension needed", reg_name)
        return None
        
    def write_register(self, address, value):
        """Try multiple methods to write register"""
        
        reg_name = get_register_name(address)
        debug_print("Attempting to write register %s (0x%08x) = 0x%08x", reg_name, address, value)
        
        # Similar implementation for write operations
        # This would need to be implemented based on available kernel interfaces
        error_print("Write operations not yet implemented for %s", reg_name)
        return False

def show_register_info(address, value):
    """Show detailed register information"""
    
    reg_name = get_register_name(address)
    print(f"\nRegister {reg_name} (0x{address:08x}) = 0x{value:08x} ({value})")
    print(f"Binary: {value:032b}")
    
    # Show bit field interpretation if available
    bit_desc = decode_register_bits(address, value)
    if bit_desc:
        print(f"{bit_desc}")
    
    # Show register description if known
    if reg_name in ['OA_STATUS0', 'OA_STATUS1']:
        print("Status register - shows current device state")
    elif reg_name in ['MAC_NCR', 'MAC_NCFGR']:
        print("MAC control register - controls network operations")
    elif reg_name in ['BASIC_CONTROL', 'BASIC_STATUS']:
        print("PHY basic register - standard IEEE 802.3 functionality")
        print(f"Name: {reg_names[address]}")
        
        # Decode specific registers
        if address == 0x10000:  # ID_REV
            chip_id = (value >> 16) & 0xFFFF
            rev_id = value & 0xFFFF
            print(f"Chip ID: 0x{chip_id:04x}, Revision: 0x{rev_id:04x}")
        
        elif address == 0x10001:  # STATUS0
            print(f"TX_FRAME_CHECK_SEQUENCE_ERROR: {(value >> 0) & 1}")
            print(f"TX_FRAME_ERROR: {(value >> 1) & 1}")
            print(f"TX_BUFFER_OVERFLOW_ERROR: {(value >> 2) & 1}")
            print(f"TX_FIFO_UNDERFLOW: {(value >> 3) & 1}")
            print(f"RX_FIFO_OVERFLOW: {(value >> 4) & 1}")
            print(f"RX_HEADER_ERROR: {(value >> 5) & 1}")
        
        elif address == 0x10003:  # CONFIG0
            print(f"PROTECTED: {(value >> 2) & 1}")
            print(f"TX_CUT_THROUGH: {(value >> 4) & 1}")
            print(f"RX_CUT_THROUGH: {(value >> 5) & 1}")

def main():
    if len(sys.argv) < 2:
        print("Usage: python3 lan8651_kernelfs.py <command> [args...]")
        print("Commands:")
        print("  read <address>    - Read register (address can be hex or register name)")
        print("  write <addr> <val> - Write register") 
        print("  list              - List known registers")
        print("  status            - Show device status")
        print("\nExamples:")
        print("  python3 lan8651_kernelfs.py read 0x10000")
        print("  python3 lan8651_kernelfs.py read OA_STATUS0")
        print("  python3 lan8651_kernelfs.py write MAC_NCR 0x0C")
        print("  python3 lan8651_kernelfs.py list")
        return
    
    debugfs = LAN8651Debugfs()
    
    if sys.argv[1] == "list":
        print("\nKnown LAN8651 Registers:")
        print("=" * 60)
        
        # Group registers by type
        std_regs = [(name, addr) for name, addr in LAN8651_REGISTERS.items() 
                   if addr < 0x10000]
        mac_regs = [(name, addr) for name, addr in LAN8651_REGISTERS.items() 
                   if 0x10000 <= addr < 0x20000]
                   
        if std_regs:
            print("\nStandard/PHY Registers (MMS 0):")
            for name, addr in sorted(std_regs, key=lambda x: x[1]):
                print(f"  {name:<15} = 0x{addr:08X}")
                
        if mac_regs:
            print("\nMAC Registers (MMS 1):")  
            for name, addr in sorted(mac_regs, key=lambda x: x[1]):
                print(f"  {name:<15} = 0x{addr:08X}")
        
    elif sys.argv[1] == "read":
        if len(sys.argv) < 3:
            print("Error: Address required for read command")
            return
        
        try:
            address = parse_register_address(sys.argv[2])
            value = debugfs.read_register(address)
            if value is not None:
                show_register_info(address, value)
            else:
                print("Failed to read register")
        except ValueError as e:
            print(f"Error: {e}")
            
    elif sys.argv[1] == "write":
        if len(sys.argv) < 4:
            print("Error: Address and value required for write command")
            return
        
        try:
            address = parse_register_address(sys.argv[2])
            value = int(sys.argv[3], 0)  # Auto-detect hex/decimal
            
            if debugfs.write_register(address, value):
                reg_name = get_register_name(address)
                print(f"Successfully wrote {reg_name} (0x{address:08X}) = 0x{value:08X}")
            else:
                print("Failed to write register")
        except ValueError as e:
            print(f"Error: {e}")
            
    elif sys.argv[1] == "status":
        print("\nLAN8651 Status Information:")
        print("=" * 40)
        
        # Read key status registers
        status_regs = [
            ('OA_STATUS0', 'General Status'),
            ('OA_STATUS1', 'Extended Status'),
            ('BASIC_STATUS', 'PHY Basic Status'),
            ('OA_BUFSTS', 'Buffer Status')
        ]
        
        for reg_name, description in status_regs:
            try:
                address = LAN8651_REGISTERS[reg_name]
                value = debugfs.read_register(address)
                if value is not None:
                    print(f"\n{description}:")
                    show_register_info(address, value)
                else:
                    print(f"\n{description}: Failed to read")
            except KeyError:
                print(f"\n{description}: Register not defined")
    else:
        print(f"Error: Unknown command '{sys.argv[1]}'")
        print("Use 'python3 lan8651_kernelfs.py' for usage help")

if __name__ == "__main__":
    main()