#!/usr/bin/env python3
"""Pre-flight check for Gemini delegation."""

import argparse
import sys
from pathlib import Path


LARGE_FILE_THRESHOLD = 1000


def count_lines(file_path: Path) -> int:
    with open(file_path, "r", encoding="utf-8", errors="ignore") as f:
        return sum(1 for _ in f)


def main():
    parser = argparse.ArgumentParser(description="Check if file requires Gemini delegation")
    parser.add_argument("file_path", type=Path, help="Path to file to check")
    args = parser.parse_args()

    if not args.file_path.exists():
        print(f"Error: File not found: {args.file_path}", file=sys.stderr)
        sys.exit(1)

    line_count = count_lines(args.file_path)

    if line_count > LARGE_FILE_THRESHOLD:
        print("DELEGATE")
        print(f"LINE_COUNT={line_count}")
        print("File exceeds 1000 lines. Claude MUST NOT read directly.")
    else:
        print("PROCEED")
        print(f"LINE_COUNT={line_count}")
        print("File within threshold. Claude may proceed normally.")


if __name__ == "__main__":
    main()
