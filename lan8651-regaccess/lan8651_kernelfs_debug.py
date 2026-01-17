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
