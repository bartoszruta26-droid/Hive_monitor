#!/usr/bin/env python3
"""Require every third-party GitHub Action to be pinned to a full commit SHA."""

import re
from pathlib import Path

root = Path(__file__).resolve().parents[3]
workflows = sorted((root / ".github" / "workflows").glob("*.yml"))
uses_pattern = re.compile(r"^\s*uses:\s*([^\s#]+)", re.MULTILINE)
sha_pattern = re.compile(r"^[^@]+@[0-9a-f]{40}$")

assert workflows, "No GitHub Actions workflows found"
for workflow in workflows:
    content = workflow.read_text(encoding="utf-8")
    for action in uses_pattern.findall(content):
        if action.startswith("./"):
            continue
        assert sha_pattern.fullmatch(action), (
            f"{workflow.relative_to(root)} uses an unpinned action: {action}"
        )

print("All external GitHub Actions are pinned to full commit SHAs")
