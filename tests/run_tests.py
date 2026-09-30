#!/usr/bin/env python3
import argparse
import subprocess
import sys
import tempfile
import xml.etree.ElementTree as ET
from pathlib import Path

GREEN = "\033[32m"
RED = "\033[31m"
RESET = "\033[0m"


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--test-dir", default="build")
    args = parser.parse_args()

    with tempfile.TemporaryDirectory() as tmp:
        junit_path = Path(tmp) / "results.xml"
        result = subprocess.run(
            ["ctest", "--test-dir", args.test_dir, "-Q", "--output-junit", str(junit_path)]
        )

        root = ET.parse(junit_path).getroot()
        for testcase in root.iter("testcase"):
            name = testcase.get("name")
            time = float(testcase.get("time", "0"))
            failure = testcase.find("failure")
            if failure is not None:
                print(f"{RED}[FAILED]{RESET} {name} ({time:.2f}s)")
                message = (failure.get("message") or failure.text or "").strip()
                if message:
                    for line in message.splitlines():
                        print(f"    {line}")
            else:
                print(f"{GREEN}[PASSED]{RESET} {name} ({time:.2f}s)")

    return result.returncode


if __name__ == "__main__":
    sys.exit(main())
