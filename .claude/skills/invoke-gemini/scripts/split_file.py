#!/usr/bin/env python3
"""Delegate structural file splitting to Gemini CLI."""

import argparse
import subprocess
import sys
from datetime import datetime
from pathlib import Path


def main():
    parser = argparse.ArgumentParser(description="Split file via Gemini CLI")
    parser.add_argument("file_path", type=Path, help="Path to file to split")
    parser.add_argument("--intent", required=True, help="High-level split intent")
    args = parser.parse_args()

    if not args.file_path.exists():
        print(f"Error: File not found: {args.file_path}", file=sys.stderr)
        sys.exit(1)

    print("Delegating file split to Gemini...")
    print(f"Intent: {args.intent}")

    timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
    output_file = f"gemini_split_{timestamp}.txt"

    prompt = f"""Split file into logical components WITHOUT changing behavior.

Requirements:
- Preserve all imports, exports, dependencies
- Maintain execution order and invariants
- Do NOT refactor logic
- Do NOT introduce new abstractions
- Provide clear split plan with filenames and responsibilities
- Assume Saturn conventions per CLAUDE.md

Intent: {args.intent}

End with: TASK COMPLETED

File: {args.file_path.resolve()}"""

    try:
        with open(output_file, "w", encoding="utf-8") as f:
            subprocess.run(
                ["gemini", "-y", "-p", prompt],
                stdout=f,
                stderr=subprocess.STDOUT,
                check=True,
            )
        print(f"Split plan complete: {output_file}")
        print(f"OUTPUT_FILE={output_file}")
    except subprocess.CalledProcessError as e:
        print(f"Error: Gemini invocation failed: {e}", file=sys.stderr)
        sys.exit(1)
    except FileNotFoundError:
        print("Error: gemini CLI not found in PATH", file=sys.stderr)
        sys.exit(1)


if __name__ == "__main__":
    main()
