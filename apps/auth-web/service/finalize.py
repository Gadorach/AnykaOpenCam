#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-only
"""Collect public canonical service build/test evidence into a fresh local handoff."""
from pathlib import Path
import sys
sys.path.insert(0,str(Path(__file__).resolve().parent.parent))
from qualification import collect
if __name__=='__main__':collect('service')
