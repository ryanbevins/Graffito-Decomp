#!/usr/bin/env python3
"""Render the README progress graphic from an objdiff report."""

from __future__ import annotations

import argparse
import json
from pathlib import Path
from typing import Any


ROOT = Path(__file__).resolve().parents[1]
DEFAULT_REPORT = ROOT / "build" / "GMSJ01" / "report.json"
DEFAULT_OUTPUT = ROOT / "assets" / "progress.svg"


def _number(value: Any) -> int:
    return int(value or 0)


def _percent(value: Any) -> float:
    return max(0.0, min(100.0, float(value or 0.0)))


def _bar_width(percent: float, width: int) -> str:
    return f"{width * percent / 100:.2f}"


def render(measures: dict[str, Any]) -> str:
    fuzzy = _percent(measures.get("fuzzy_match_percent"))
    matched_code = _number(measures.get("matched_code"))
    total_code = _number(measures.get("total_code"))
    code_percent = _percent(
        measures.get(
            "matched_code_percent",
            matched_code * 100 / total_code if total_code else 0,
        )
    )
    matched_functions = _number(measures.get("matched_functions"))
    total_functions = _number(measures.get("total_functions"))
    function_percent = _percent(
        measures.get(
            "matched_functions_percent",
            matched_functions * 100 / total_functions if total_functions else 0,
        )
    )

    return f"""<svg xmlns="http://www.w3.org/2000/svg" width="820" height="108" viewBox="0 0 820 108" role="img" aria-labelledby="title desc">
  <title id="title">Graffito Decomp matching progress</title>
  <desc id="desc">{fuzzy:.2f}% fuzzy match, {matched_code:,} of {total_code:,} exact code bytes, and {matched_functions:,} of {total_functions:,} matched functions.</desc>
  <style>
    .primary {{ fill: #1f2328; }}
    .secondary {{ fill: #656d76; }}
    .track {{ fill: #d0d7de; }}
    @media (prefers-color-scheme: dark) {{
      .primary {{ fill: #f0f6fc; }}
      .secondary {{ fill: #8c959f; }}
      .track {{ fill: #30363d; }}
    }}
  </style>

  <g font-family="-apple-system,BlinkMacSystemFont,Segoe UI,Helvetica,Arial,sans-serif">
    <text x="0" y="22" class="primary" font-size="16" font-weight="600">Fuzzy match</text>
    <text x="820" y="22" class="primary" font-size="20" font-weight="600" text-anchor="end">{fuzzy:.2f}%</text>

    <rect y="35" width="820" height="12" rx="6" class="track"/>
    <rect y="35" width="{_bar_width(fuzzy, 820)}" height="12" rx="6" fill="#1f883d"/>

    <text x="0" y="75" class="primary" font-size="14" font-weight="600">Exact code</text>
    <text x="82" y="75" class="secondary" font-size="14">{code_percent:.2f}%</text>
    <text x="0" y="99" class="secondary" font-size="13">{matched_code:,} / {total_code:,} bytes</text>

    <line x1="403" y1="65" x2="403" y2="101" stroke="#d0d7de"/>

    <text x="430" y="75" class="primary" font-size="14" font-weight="600">Functions</text>
    <text x="504" y="75" class="secondary" font-size="14">{function_percent:.2f}%</text>
    <text x="430" y="99" class="secondary" font-size="13">{matched_functions:,} / {total_functions:,}</text>
  </g>
</svg>
"""


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--report", type=Path, default=DEFAULT_REPORT)
    parser.add_argument("--output", type=Path, default=DEFAULT_OUTPUT)
    args = parser.parse_args()

    report = json.loads(args.report.read_text(encoding="utf-8"))
    measures = report.get("measures")
    if not isinstance(measures, dict):
        raise SystemExit(f"{args.report} has no top-level measures object")

    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(render(measures), encoding="utf-8", newline="\n")
    print(f"Updated {args.output}")


if __name__ == "__main__":
    main()
