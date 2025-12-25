#!/usr/bin/env python3
"""
Helper script for tonl CLI operations.

Handles input translation (raw content → temp files) when file paths are not provided.
Used by Claude Code's tonl-tool skill.
"""

import sys
import tempfile
import os
import subprocess
from pathlib import Path


def create_temp_file(content: str, suffix: str = ".json") -> Path:
    """Create a temporary file with the given content."""
    with tempfile.NamedTemporaryFile(mode='w', suffix=suffix, delete=False) as f:
        f.write(content)
        return Path(f.name)


def run_tonl_command(args: list[str]) -> int:
    """Execute tonl CLI command and return exit code."""
    try:
        result = subprocess.run(
            ["tonl"] + args,
            capture_output=False,
            text=True
        )
        return result.returncode
    except FileNotFoundError:
        print("Error: tonl CLI not found. Install with: npm install -g tonl", file=sys.stderr)
        return 1


def main():
    """
    Usage:
        tonl-helper.py encode <content> [--smart]
        tonl-helper.py decode <content>
        tonl-helper.py query <content> <path-expression>
        tonl-helper.py get <content> <field-path>
        tonl-helper.py validate <schema-content> <data-content>
        tonl-helper.py stats <content> [--tokenizer <model>]
    """
    if len(sys.argv) < 3:
        print("Usage: tonl-helper.py <operation> <content> [args...]", file=sys.stderr)
        return 1

    operation = sys.argv[1]
    content = sys.argv[2]
    extra_args = sys.argv[3:]

    # Determine file extension based on operation
    if operation == "encode":
        suffix = ".json"
    elif operation == "decode":
        suffix = ".tonl"
    else:
        # Query/get/stats can handle both formats
        suffix = ".json" if content.strip().startswith("{") else ".tonl"

    temp_file = create_temp_file(content, suffix)

    try:
        # Special handling for validate (needs two files)
        if operation == "validate":
            if len(extra_args) < 1:
                print("Error: validate requires schema content", file=sys.stderr)
                return 1

            schema_content = content
            data_content = extra_args[0]
            remaining_args = extra_args[1:]

            schema_file = create_temp_file(schema_content, ".tonl")
            data_file = create_temp_file(data_content, ".tonl")

            try:
                tonl_args = [operation, "--schema", str(schema_file), str(data_file)] + remaining_args
                return run_tonl_command(tonl_args)
            finally:
                os.unlink(schema_file)
                os.unlink(data_file)

        # Standard operations
        tonl_args = [operation, str(temp_file)] + extra_args
        return run_tonl_command(tonl_args)

    finally:
        os.unlink(temp_file)


if __name__ == "__main__":
    sys.exit(main())
