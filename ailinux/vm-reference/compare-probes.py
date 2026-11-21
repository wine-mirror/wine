#!/usr/bin/env python3
"""Compare results from genuine Windows and Wine; never fabricate Windows evidence."""
import argparse
import json
from pathlib import Path

REQUIRED = {"schema", "arch_bits", "api"}


def load(path):
    obj = json.loads(Path(path).read_text(encoding="utf-8-sig"))
    if not isinstance(obj, dict) or not REQUIRED.issubset(obj):
        raise ValueError("Missing schema, architecture or API fields")
    if obj["schema"] != 1 or obj["arch_bits"] not in (32, 64):
        raise ValueError("Unsupported probe schema or bitness")
    apis = obj["api"]
    if not isinstance(apis, dict) or not apis:
        raise ValueError("Empty or non-object API table")
    if not all(isinstance(k, str) and isinstance(v, bool) for k, v in apis.items()):
        raise ValueError("API export fields must be string-to-boolean")
    return obj


def compare(win, wine):
    if win["arch_bits"] != wine["arch_bits"]:
        raise ValueError("Architecture mismatch; do not mix x86/x64 evidence")
    keys = sorted(set(win["api"]) | set(wine["api"]))
    results = []
    missing = []
    for key in keys:
        expected = win["api"].get(key)
        actual = wine["api"].get(key)
        match = expected is not None and actual is not None and expected == actual
        results.append({"export": key, "windows": expected, "wine": actual, "matches": match})
        if expected is True and actual is not True:
            missing.append(key)
    return {
        "schema": 1, "arch_bits": win["arch_bits"],
        "windows_version": win.get("windows_version"),
        "wine_compat_version": wine.get("windows_version"),
        "export_matches": sum(item["matches"] for item in results),
        "total": len(results), "wine_missing_windows_exports": missing,
        "note": "Export presence only; behavioral equivalence remains unverified.",
        "results": results,
    }


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--windows", required=True, help="Genuine Windows guest output")
    parser.add_argument("--wine", required=True, help="Wine output of same architecture")
    parser.add_argument("--output", help="Write JSON report to path; stdout by default")
    args = parser.parse_args()
    report = compare(load(args.windows), load(args.wine))
    encoded = json.dumps(report, indent=2, sort_keys=True) + "\n"
    if args.output:
        Path(args.output).write_text(encoded, encoding="utf-8")
        print(f"COMPARISON_WRITTEN={args.output}")
    else:
        print(encoded, end="")


if __name__ == "__main__":
    main()
