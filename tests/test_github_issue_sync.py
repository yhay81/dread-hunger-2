from __future__ import annotations

import tempfile
import unittest
from pathlib import Path

from Tools.github_issue_sync import (
    IssueSpec,
    build_gh_issue_create_command,
    filter_new_specs,
    load_issue_specs,
    shell_command,
)


class GithubIssueSyncTests(unittest.TestCase):
    def test_loads_issue_specs_from_csv(self) -> None:
        with tempfile.TemporaryDirectory() as temp_dir:
            path = Path(temp_dir) / "issues.csv"
            path.write_text(
                "title,body,labels,milestone\n"
                "\"P1-999: Test\",\"Body text\",\"phase-1,prototype\",\"Phase 1\"\n",
                encoding="utf-8",
            )

            specs = load_issue_specs(path)

        self.assertEqual(
            specs,
            [
                IssueSpec(
                    title="P1-999: Test",
                    body="Body text",
                    labels=("phase-1", "prototype"),
                    milestone="Phase 1",
                )
            ],
        )

    def test_builds_gh_issue_create_command(self) -> None:
        command = build_gh_issue_create_command(
            IssueSpec(
                title="P1-999: Test",
                body="Body text",
                labels=("phase-1", "prototype"),
                milestone="Phase 1",
            )
        )

        self.assertEqual(command[:6], ["gh", "issue", "create", "--title", "P1-999: Test", "--body"])
        self.assertIn("--label", command)
        self.assertIn("phase-1", command)
        self.assertIn("--milestone", command)
        self.assertIn("Phase 1", command)

    def test_shell_command_quotes_arguments(self) -> None:
        command = shell_command(["gh", "issue", "create", "--title", "P1-999: spaced title"])

        self.assertEqual(command, "gh issue create --title 'P1-999: spaced title'")

    def test_filters_existing_titles(self) -> None:
        specs = [
            IssueSpec("P1-001: Existing", "body", (), "Phase 1"),
            IssueSpec("P1-002: New", "body", (), "Phase 1"),
        ]

        self.assertEqual(filter_new_specs(specs, {"P1-001: Existing"}), [specs[1]])


if __name__ == "__main__":
    unittest.main()
