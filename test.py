#!/usr/bin/env python3
import argparse
import re
import subprocess
import sys
import time
from pathlib import Path


SLA_PATTERNS = {
    "SLA0": re.compile(r"SLA0:\s*([0-9]+(?:\.[0-9]+)?)%"),
    "SLA1": re.compile(r"SLA1:\s*([0-9]+(?:\.[0-9]+)?)%"),
    "SLA2": re.compile(r"SLA2:\s*([0-9]+(?:\.[0-9]+)?)%"),
}
KW_PATTERNS = {
    "KW-Hour": re.compile(r"Total\s*Energy\s*([0-9]+(?:\.[0-9]+)?)")
}

def find_md_files(input_dir: Path):
    return sorted(p for p in input_dir.iterdir() if p.is_file() and p.suffix.lower() == ".md")


def extract_sla_values(output_text: str):
    values = {}
    for key, pattern in SLA_PATTERNS.items():
        match = pattern.search(output_text)
        values[key] = match.group(1) + "%" if match else "N/A"
    return values
def extract_kw_hr(output_text: str):
    values = {}
    for key, pattern in KW_PATTERNS.items():
        match = pattern.search(output_text)
        values[key] = match.group(1) + "KWh" if match else "N/A"
    return values

def format_table(rows):
    headers = ["File", "Exit", "SLA0", "SLA1", "SLA2", "KW-Hours"]
    widths = [len(h) for h in headers]

    for row in rows:
        widths[0] = max(widths[0], len(row["file"]))
        widths[1] = max(widths[1], len(str(row["exit_code"])))
        widths[2] = max(widths[2], len(row["SLA0"]))
        widths[3] = max(widths[3], len(row["SLA1"]))
        widths[4] = max(widths[4], len(row["SLA2"]))
        widths[5] = max(widths[5], len(row["KW-Hour"]))

    def fmt_line(values):
        return " | ".join(str(v).ljust(w) for v, w in zip(values, widths))

    separator = "-+-".join("-" * w for w in widths)

    lines = [
        fmt_line(headers),
        separator,
    ]

    for row in rows:
        lines.append(
            fmt_line([
                row["file"],
                row["exit_code"],
                row["SLA0"],
                row["SLA1"],
                row["SLA2"],
                row["KW-Hour"]
            ])
        )

    return "\n".join(lines)


def run_one(simulator: Path, input_file: Path, extra_args):
    cmd = [str(simulator), str(input_file), *extra_args]
    print("=" * 80)
    print("Running:", " ".join(cmd))
    print("=" * 80)

    start = time.time()
    result = subprocess.run(cmd, capture_output=True, text=True)
    elapsed = time.time() - start

    if result.stdout:
        print(result.stdout, end="" if result.stdout.endswith("\n") else "\n")
    if result.stderr:
        print(result.stderr, file=sys.stderr, end="" if result.stderr.endswith("\n") else "\n")

    print(f"\nExit code: {result.returncode}")
    print(f"Wall time: {elapsed:.2f} s\n")

    combined = (result.stdout or "") + "\n" + (result.stderr or "")
    sla_values = extract_sla_values(combined)
    kw_values = extract_kw_hr(combined)
    return {
        "exit_code": result.returncode,
        "elapsed": elapsed,
        "SLA0": sla_values["SLA0"],
        "SLA1": sla_values["SLA1"],
        "SLA2": sla_values["SLA2"],
        "KW-Hour": kw_values["KW-Hour"]
    }


def main():
    parser = argparse.ArgumentParser(
        description="Run the simulator on every .md input file in the input/ folder."
    )
    parser.add_argument(
        "--simulator",
        default="./simulator",
        help="Path to the simulator executable (default: ./simulator)",
    )
    parser.add_argument(
        "--input-dir",
        default="./input",
        help="Directory containing .md input files (default: ./input)",
    )
    parser.add_argument(
        "--stop-on-fail",
        action="store_true",
        help="Stop immediately if any run returns a nonzero exit code.",
    )
    parser.add_argument(
        "sim_args",
        nargs=argparse.REMAINDER,
        help="Extra arguments passed to the simulator. Example: -- -v 1",
    )
    args = parser.parse_args()

    simulator = Path(args.simulator).resolve()
    input_dir = Path(args.input_dir).resolve()

    if not simulator.exists():
        print(f"Error: simulator not found at {simulator}", file=sys.stderr)
        sys.exit(1)

    if not input_dir.exists() or not input_dir.is_dir():
        print(f"Error: input directory not found at {input_dir}", file=sys.stderr)
        sys.exit(1)

    md_files = find_md_files(input_dir)
    if not md_files:
        print(f"No .md files found in {input_dir}", file=sys.stderr)
        sys.exit(1)

    failures = []
    rows = []

    for md_file in md_files:
        run_info = run_one(simulator, md_file, args.sim_args)
        row = {
            "file": md_file.name,
            "exit_code": run_info["exit_code"],
            "SLA0": run_info["SLA0"],
            "SLA1": run_info["SLA1"],
            "SLA2": run_info["SLA2"],
            "KW-Hour": run_info["KW-Hour"]
        }
        rows.append(row)

        if run_info["exit_code"] != 0:
            failures.append((md_file.name, run_info["exit_code"]))
            if args.stop_on_fail:
                break

    print("=" * 80)
    print("Summary")
    print("=" * 80)
    print(format_table(rows))
    print()

    if failures:
        print("Some runs failed:")
        for name, rc in failures:
            print(f"  {name}: exit code {rc}")
        sys.exit(1)
    else:
        print(f"All {len(rows)} runs completed successfully.")
        sys.exit(0)


if __name__ == "__main__":
    main()
