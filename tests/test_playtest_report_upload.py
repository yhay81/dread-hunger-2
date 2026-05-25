from __future__ import annotations

import json
import subprocess
import sys
import tempfile
import threading
import unittest
from http.server import BaseHTTPRequestHandler, HTTPServer
from pathlib import Path

from Tools.log_summary import summarize_events
from Tools.playtest_report_upload import build_payload, load_summary


ROOT = Path(__file__).resolve().parents[1]
FIXTURE = ROOT / "tests" / "fixtures" / "p1_024_human_events.jsonl"


class CaptureHandler(BaseHTTPRequestHandler):
    captured_body: dict[str, object] | None = None

    def do_POST(self) -> None:  # noqa: N802 - stdlib handler API.
        length = int(self.headers.get("content-length", "0"))
        CaptureHandler.captured_body = json.loads(self.rfile.read(length).decode("utf-8"))
        response = json.dumps({"accepted": True, "id": "ptr_test"}).encode("utf-8")
        self.send_response(202)
        self.send_header("content-type", "application/json")
        self.send_header("content-length", str(len(response)))
        self.end_headers()
        self.wfile.write(response)

    def log_message(self, _format: str, *_args: object) -> None:
        return


class PlaytestReportUploadTests(unittest.TestCase):
    def test_builds_backend_payload_from_summary(self) -> None:
        payload = build_payload(summarize_events(FIXTURE), note="anonymized keep/cut/change recorded")

        self.assertEqual(payload["runId"], "P1-024-run-01")
        self.assertEqual(payload["buildId"], "AbyssLock-Win64-Development-local")
        self.assertEqual(payload["mapId"], "/Game/Maps/L_IcebreakerWhitebox")
        self.assertEqual(payload["playerCount"], 6)
        self.assertEqual(payload["winner"], "crew")
        self.assertIn("repairs=1", payload["summary"])
        self.assertIn("sabotages=1", payload["summary"])

    def test_dry_run_cli_prints_payload_without_upload(self) -> None:
        with tempfile.TemporaryDirectory() as temp_dir:
            summary_path = Path(temp_dir) / "summary.json"
            summary_path.write_text(json.dumps(summarize_events(FIXTURE)), encoding="utf-8")
            completed = subprocess.run(
                [sys.executable, "Tools/playtest_report_upload.py", str(summary_path)],
                cwd=ROOT,
                text=True,
                capture_output=True,
                check=False,
            )

        self.assertEqual(completed.returncode, 0, completed.stderr)
        self.assertIn('"runId": "P1-024-run-01"', completed.stdout)
        self.assertIn("dry-run", completed.stdout)

    def test_load_summary_accepts_raw_jsonl_events(self) -> None:
        summary = load_summary(FIXTURE)

        self.assertEqual(summary["run_id"], "P1-024-run-01")
        self.assertEqual(summary["players_connected"], 6)

    def test_send_posts_payload_to_endpoint(self) -> None:
        CaptureHandler.captured_body = None
        with tempfile.TemporaryDirectory() as temp_dir:
            summary_path = Path(temp_dir) / "summary.json"
            summary_path.write_text(json.dumps(summarize_events(FIXTURE)), encoding="utf-8")
            server = HTTPServer(("127.0.0.1", 0), CaptureHandler)
            thread = threading.Thread(target=server.handle_request, daemon=True)
            thread.start()
            try:
                completed = subprocess.run(
                    [
                        sys.executable,
                        "Tools/playtest_report_upload.py",
                        str(summary_path),
                        "--endpoint",
                        f"http://127.0.0.1:{server.server_port}/v1/playtest-reports",
                        "--send",
                    ],
                    cwd=ROOT,
                    text=True,
                    capture_output=True,
                    check=False,
                    timeout=10,
                )
            finally:
                thread.join(timeout=5)
                server.server_close()

        self.assertEqual(completed.returncode, 0, completed.stderr)
        self.assertEqual(CaptureHandler.captured_body["runId"], "P1-024-run-01")
        self.assertIn('"id": "ptr_test"', completed.stdout)


if __name__ == "__main__":
    unittest.main()
