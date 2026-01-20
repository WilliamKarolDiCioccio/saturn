#!/usr/bin/env python3
"""Delegate bulk code generation to Gemini CLI."""

import argparse
import subprocess
import sys
from datetime import datetime


def main():
    parser = argparse.ArgumentParser(description="Generate bulk code via Gemini CLI")
    parser.add_argument("--spec", required=True, help="Code generation specification")
    args = parser.parse_args()

    print("Delegating bulk generation to Gemini...")

    timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
    output_file = f"gemini_gen_{timestamp}.txt"

    prompt = f"""Generate code per specification.

Constraints:
- Follow Saturn conventions per CLAUDE.md
- Do NOT introduce new patterns
- Do NOT make architectural decisions
- Match existing style exactly

Specification:
{args.spec}

End with: TASK COMPLETED"""

    try:
        with open(output_file, "w", encoding="utf-8") as f:
            subprocess.run(
                ["gemini", "-y", "-p", prompt],
                stdout=f,
                stderr=subprocess.STDOUT,
                check=True,
            )
        print(f"Generation complete: {output_file}")
        print(f"OUTPUT_FILE={output_file}")
    except subprocess.CalledProcessError as e:
        print(f"Error: Gemini invocation failed: {e}", file=sys.stderr)
        sys.exit(1)
    except FileNotFoundError:
        print("Error: gemini CLI not found in PATH", file=sys.stderr)
        sys.exit(1)


if __name__ == "__main__":
    main()
