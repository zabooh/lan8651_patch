#!/bin/bash
#
# LAN865x Kernel Configuration Management Script
# 
# This script manages the kernel .config file for LAN865x module development
#

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
KERNEL_CONFIG="$SCRIPT_DIR/kernel.config"
TARGET_CONFIG="../../../../../.config"
BUILDROOT_BASE="/home/martin/AIoT/lan9662/mchp-brsdk-source-2025.12"

echo "🔧 LAN865x Kernel Configuration Manager"
echo "======================================="

# Verify we're in the correct directory
if [[ ! -f "$SCRIPT_DIR/lan865x.c" ]]; then
    echo "❌ Error: Script must be run from lan865x source directory"
    echo "   Expected: .../drivers/net/ethernet/microchip/lan865x/"
    exit 1
fi

case "$1" in
    apply)
        echo "📋 Applying stored kernel configuration..."
        
        if [[ ! -f "$KERNEL_CONFIG" ]]; then
            echo "❌ Error: kernel.config not found in $SCRIPT_DIR"
            echo "   Run: $0 backup first"
            exit 1
        fi
        
        # Copy to linux build directory
        cp "$KERNEL_CONFIG" "$TARGET_CONFIG"
        echo "✓ Configuration applied to: $TARGET_CONFIG"
        
        # Show relevant LAN865x settings
        echo ""
        echo "📊 LAN865x Module Configuration:"
        grep -E "CONFIG_(LAN865X|OA_TC6|MODULES)" "$KERNEL_CONFIG" || echo "  (No LAN865x config found)"
        
        echo ""
        echo "⚡ Next steps:"
        echo "   cd $BUILDROOT_BASE"
        echo "   make linux-rebuild O=output/mybuild_regacces"
        ;;
        
    backup)
        echo "💾 Backing up current kernel configuration..."
        
        if [[ ! -f "$TARGET_CONFIG" ]]; then
            echo "❌ Error: No kernel .config found at $TARGET_CONFIG"
            echo "   Build kernel first or check path"
            exit 1
        fi
        
        cp "$TARGET_CONFIG" "$KERNEL_CONFIG"
        echo "✓ Configuration backed up to: $KERNEL_CONFIG"
        
        # Show file info
        echo ""
        echo "📋 Backup Details:"
        echo "  Size: $(du -h "$KERNEL_CONFIG" | cut -f1)"
        echo "  Date: $(date)"
        echo "  LAN865x settings:"
        grep -E "CONFIG_(LAN865X|OA_TC6)" "$KERNEL_CONFIG" || echo "    (Not found - may need configuration)"
        ;;
        
    diff)
        echo "🔍 Comparing stored vs. current kernel configuration..."
        
        if [[ ! -f "$KERNEL_CONFIG" ]]; then
            echo "❌ Error: No backup found. Run: $0 backup first"
            exit 1
        fi
        
        if [[ ! -f "$TARGET_CONFIG" ]]; then
            echo "❌ Error: No current .config found"
            exit 1
        fi
        
        echo "📊 Configuration differences:"
        diff "$KERNEL_CONFIG" "$TARGET_CONFIG" || true
        echo ""
        echo "📋 LAN865x specific changes:"
        diff <(grep -E "CONFIG_(LAN865X|OA_TC6|MODULES)" "$KERNEL_CONFIG" || true) \
             <(grep -E "CONFIG_(LAN865X|OA_TC6|MODULES)" "$TARGET_CONFIG" || true) || true
        ;;
        
    status)
        echo "📊 Kernel Configuration Status:"
        echo ""
        
        if [[ -f "$KERNEL_CONFIG" ]]; then
            echo "✓ Backup config: $(du -h "$KERNEL_CONFIG" | cut -f1) ($(date -r "$KERNEL_CONFIG"))"
            echo "  LAN865x settings in backup:"
            grep -E "CONFIG_(LAN865X|OA_TC6)=" "$KERNEL_CONFIG" | sed 's/^/    /'
        else
            echo "❌ No backup config found"
        fi
        
        echo ""
        if [[ -f "$TARGET_CONFIG" ]]; then
            echo "✓ Current config: $(du -h "$TARGET_CONFIG" | cut -f1) ($(date -r "$TARGET_CONFIG"))"
            echo "  LAN865x settings in current:"
            grep -E "CONFIG_(LAN865X|OA_TC6)=" "$TARGET_CONFIG" | sed 's/^/    /'
        else
            echo "❌ No current config found"
        fi
        ;;
        
    clean)
        echo "🗑️  Cleaning backup configuration..."
        
        if [[ -f "$KERNEL_CONFIG" ]]; then
            rm "$KERNEL_CONFIG"
            echo "✓ Backup configuration removed"
        else
            echo "ℹ️  No backup to remove"
        fi
        ;;
        
    *)
        echo "Usage: $0 {apply|backup|diff|status|clean}"
        echo ""
        echo "Commands:"
        echo "  backup  - Save current kernel .config as backup"
        echo "  apply   - Apply backed up .config to kernel build"
        echo "  diff    - Show differences between backup and current"
        echo "  status  - Show backup and current config status"
        echo "  clean   - Remove backup configuration"
        echo ""
        echo "Typical workflow:"
        echo "  1. $0 backup    # Save working configuration"
        echo "  2. [make changes to kernel config]"
        echo "  3. $0 apply     # Restore known-good config"
        echo "  4. [rebuild kernel with restored config]"
        echo ""
        echo "Files:"
        echo "  Backup:  $KERNEL_CONFIG"
        echo "  Target:  $TARGET_CONFIG"
        exit 1
        ;;
esac

echo ""
echo "✅ Operation completed successfully"