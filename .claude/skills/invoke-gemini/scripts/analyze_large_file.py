#!/usr/bin/env python3
"""Delegate large file analysis to Gemini CLI."""

import argparse
import subprocess
import sys
from datetime import datetime
from pathlib import Path


LARGE_FILE_THRESHOLD = 1000
DEFAULT_MODEL = "gemini-2.5-flash"


def count_lines(file_path: Path) -> int:
    with open(file_path, "r", encoding="utf-8", errors="ignore") as f:
        return sum(1 for _ in f)


def main():
    parser = argparse.ArgumentParser(description="Analyze large file via Gemini CLI")
    parser.add_argument("file_path", type=Path, help="Path to file to analyze")
    parser.add_argument("--model", default=DEFAULT_MODEL, help="Gemini model to use")
    args = parser.parse_args()

    if not args.file_path.exists():
        print(f"Error: File not found: {args.file_path}", file=sys.stderr)
        sys.exit(1)

    line_count = count_lines(args.file_path)
    if line_count <= LARGE_FILE_THRESHOLD:
        print(f"Warning: File has {line_count} lines (<=1000). Gemini delegation not required.")
        sys.exit(0)

    print(f"File has {line_count} lines. Delegating to Gemini...")

    timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
    output_file = f"gemini_analysis_{timestamp}.txt"

    prompt = f"""Analyze file for Saturn Game Engine.

Rules:
- Identify major structures (classes, functions, modules)
- Highlight areas relevant to changes
- Call out invariants, assumptions, coupling points
- DO NOT rewrite code
- DO NOT suggest design/architectural changes
- Assume Saturn conventions per CLAUDE.md

Output Sections:
- Overview
- Key Structures
- Areas Relevant to Change
- Invariants / Assumptions
- Coupling / Risk Points

End with: TASK COMPLETED

File: {args.file_path.resolve()}"""

    try:
        with open(output_file, "w", encoding="utf-8") as f:
            result = subprocess.run(
                ["gemini", "--model", args.model, "-y", "-a", "-p", prompt],
                stdout=f,
                stderr=subprocess.STDOUT,
                check=True,
            )
        print(f"Analysis complete: {output_file}")
        print(f"OUTPUT_FILE={output_file}")
    except subprocess.CalledProcessError as e:
        print(f"Error: Gemini invocation failed: {e}", file=sys.stderr)
        sys.exit(1)
    except FileNotFoundError:
        print("Error: gemini CLI not found in PATH", file=sys.stderr)
        sys.exit(1)


if __name__ == "__main__":
    main()
