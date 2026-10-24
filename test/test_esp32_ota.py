"""Exercise firmware update failure paths without writing hardware flash."""
import shutil
import subprocess
import sys
import tempfile
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


class Esp32OtaTests(unittest.TestCase):
    @unittest.skipUnless(sys.platform == "win32" and shutil.which("clang++"),
                         "Requires Windows with Clang and LLD")
    def test_upload_completion_failure_and_cancellation(self):
        with tempfile.TemporaryDirectory() as directory:
            executable = Path(directory) / "esp32_ota_test.exe"
            command = [
                "clang++", "-std=c++17", "-ffreestanding", "-fno-exceptions",
                "-fno-rtti", "-fno-stack-protector", "-fuse-ld=lld", "-nostdlib",
                "-DARDUINO_ARCH_ESP32", "-Itest/ota_fakes", "-Iinclude",
                "test/test_esp32_ota.cpp", "src/esp32/firmware_updater.cpp",
                "-Wl,/entry:main,/subsystem:console", "-o", str(executable),
            ]
            subprocess.run(command, cwd=ROOT, check=True, capture_output=True, text=True)
            result = subprocess.run([str(executable)], timeout=10)
            self.assertEqual(result.returncode, 0,
                             f"OTA check failed at C++ line {result.returncode}")
