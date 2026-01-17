#!/bin/bash
# Script to remove non-working files from LAN8651 register access project
# These files use the problematic kernel module approach that doesn't work with ARM

set -e

echo "🗑️  Removing non-functional files..."

# Remove kernel module files (don't work with ARM - access private structures)
echo "Removing kernel module files..."
rm -f lan8651_debug.c
rm -f lan8651_debug.ko  
rm -f lan8651_debug.mod
rm -f lan8651_debug.mod.c
rm -f lan8651_debug.mod.o
rm -f lan8651_debug.o

# Remove tools that depend on the broken kernel module
echo "Removing tools that depend on broken kernel module..."
rm -f lan8651_access.c
rm -f lan8651_access_arm
rm -f lan8651_access_x86
rm -f lan8651_tool.py

# Remove build system for kernel module
echo "Removing kernel module build system..."
rm -f Makefile
rm -f Module.symvers
rm -f modules.order
rm -f module-common.o

# Remove all .cmd files (build temporary files)
echo "Removing build temporary files..."
rm -f .*.cmd
rm -f ..module-common.o.cmd

# Remove old build script
echo "Removing old build script..."
rm -f build.sh

echo "✅ Cleanup complete!"
echo ""
echo "Remaining functional files:"
ls -la lan8651_ethtool* lan8651_kernelfs.py build_tools.sh test_tools.sh *.patch README.md 2>/dev/null || true