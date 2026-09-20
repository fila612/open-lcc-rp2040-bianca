#!/usr/bin/env python3
"""Compile production control logic with host hardware stubs and sanitizers."""
import os
from pathlib import Path
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]
SOURCES = [
    "tests/host/regressions.cpp",
    "src/Controller/Core0/SystemController.cpp",
    "src/Controller/Core0/Util/HybridController.cpp",
    "src/Controller/Core0/Util/PIDController.cpp",
    "src/Controller/Core0/Util/HysteresisController.cpp",
    "src/Controller/Core0/Util/TimedLatch.cpp",
    "src/Controller/Core0/Protocol/lcc_protocol.cpp",
    "src/Controller/Core0/Protocol/control_board_protocol.cpp",
    "src/Controller/Core1/SettingsFlash.cpp",
    "src/Controller/Core1/SettingsManager.cpp",
    "src/Controller/Core1/Automations.cpp",
    "src/utils/triplet.cpp", "src/utils/checksum.cpp", "src/utils/polymath.cpp",
    "src/utils/crc32.cpp",
]

with tempfile.TemporaryDirectory(prefix="bianca-host-tests-") as temp:
    binary = str(Path(temp) / "regressions")
    subprocess.run([
        os.environ.get("CXX", "clang++"), "-std=c++17", "-g", "-O1",
        "-fsanitize=address,undefined", "-fno-omit-frame-pointer",
        # Allow the harness to set clock/state boundaries without a production test API.
        "-fno-access-control", "-Wno-c99-designator",
        "-Itests/host/include", "-Isrc", "-Ilib/optional-bare",
        *SOURCES, "-o", binary,
    ], cwd=ROOT, check=True)
    subprocess.run([binary], check=True)
