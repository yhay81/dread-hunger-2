import tempfile
import unittest
from pathlib import Path

from Tools.backlog_to_issues import ROOT, parse_backlog, slug_label, write_csv, write_markdown


class BacklogToIssuesTests(unittest.TestCase):
    def test_slug_label_normalizes_section_names(self):
        self.assertEqual(slug_label("Server Discovery And Tool App"), "server-discovery-and-tool-app")
        self.assertEqual(slug_label("  Phase 2: Steam Lobby  "), "phase-2-steam-lobby")

    def test_parse_backlog_filters_by_phase_prefix_and_labels_rows(self):
        with tempfile.TemporaryDirectory(dir=ROOT) as temp_dir:
            backlog = Path(temp_dir) / "phase2-backlog.md"
            backlog.write_text(
                "\n".join(
                    [
                        "# Phase 2 Backlog",
                        "",
                        "## Steam Lobby",
                        "",
                        "| ID | Task | Done When |",
                        "| --- | --- | --- |",
                        "| P2-001 | Enable Steam dev config safely | Local dev config is documented |",
                        "| P1-999 | Ignore older phase row | Should not export |",
                    ]
                ),
                encoding="utf-8",
            )

            rows = parse_backlog(
                backlog,
                phase=2,
                id_prefix="P2-",
                milestone="Phase 2",
                base_label="platform",
            )

        self.assertEqual(len(rows), 1)
        self.assertEqual(rows[0].title, "P2-001: Enable Steam dev config safely")
        self.assertEqual(rows[0].section, "Steam Lobby")
        self.assertEqual(rows[0].labels, "phase-2,steam-lobby,platform")
        self.assertEqual(rows[0].milestone, "Phase 2")
        self.assertIn("phase2-backlog.md", rows[0].body)

    def test_write_outputs_use_phase_metadata(self):
        with tempfile.TemporaryDirectory(dir=ROOT) as temp_dir:
            temp_path = Path(temp_dir)
            backlog = temp_path / "phase2-backlog.md"
            backlog.write_text(
                "\n".join(
                    [
                        "# Phase 2 Backlog",
                        "",
                        "## Voice",
                        "| ID | Task | Done When |",
                        "| --- | --- | --- |",
                        "| P2-014 | Voice Chat Interface provider spike | Provider decision is recorded |",
                    ]
                ),
                encoding="utf-8",
            )
            rows = parse_backlog(
                backlog,
                phase=2,
                id_prefix="P2-",
                milestone="Phase 2",
                base_label="platform",
            )
            csv_path = temp_path / "issues.csv"
            markdown_path = temp_path / "issues.md"

            write_csv(rows, csv_path)
            write_markdown(rows, markdown_path, phase=2, source_path=backlog)

            csv_text = csv_path.read_text(encoding="utf-8")
            markdown_text = markdown_path.read_text(encoding="utf-8")

        self.assertIn("P2-014: Voice Chat Interface provider spike", csv_text)
        self.assertIn("phase-2,voice,platform", csv_text)
        self.assertIn("Phase 2", csv_text)
        self.assertIn("# Phase 2 Issue Import", markdown_text)
        self.assertIn("| P2-014 | Voice | Voice Chat Interface provider spike |", markdown_text)


if __name__ == "__main__":
    unittest.main()
