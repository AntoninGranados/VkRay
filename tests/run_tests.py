#!/usr/bin/env python3
import argparse
import re
import subprocess
import sys

GREEN = "\033[32m"
RED = "\033[31m"
RESET = "\033[0m"


def list_tests(test_dir):
    result = subprocess.run(["ctest", "--test-dir", test_dir, "-N"], capture_output=True, text=True, check=True)
    names = []
    for line in result.stdout.splitlines():
        line = line.strip()
        if line.startswith("Test #"):
            names.append(line.split(":", 1)[1].strip())
    return names


def run_test(test_dir, name):
    result = subprocess.run(
        ["ctest", "--test-dir", test_dir, "-R", f"^{re.escape(name)}$", "--output-on-failure"],
        capture_output=True, text=True,
    )
    return result.returncode == 0, result.stdout + result.stderr


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--test-dir", default="build")
    args = parser.parse_args()

    for name in list_tests(args.test_dir):
        print(f"[RUN] {name} ...", end="", flush=True)
        ok, log = run_test(args.test_dir, name)
        if ok:
            print(f"\r\033[K{GREEN}[OK]{RESET} {name}")
        else:
            print(f"\r\033[K{RED}[FAIL]{RESET} {name}")
            print(log)
            return 1

    return 0


if __name__ == "__main__":
    sys.exit(main())
