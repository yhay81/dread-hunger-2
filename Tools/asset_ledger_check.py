#!/usr/bin/env python3
from __future__ import annotations

import argparse
import csv
import re
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
DEFAULT_LEDGER = ROOT / "docs" / "asset-ledger-candidates.csv"

REQUIRED_COLUMNS = [
    "asset_id",
    "asset_name",
    "asset_type",
    "created_at",
    "creator",
    "tool_or_model",
    "tool_version",
    "prompt_summary",
    "reference_used",
    "reference_source",
    "output_path",
    "modification_history",
    "license",
    "fab_listing_url",
    "publisher_or_seller",
    "license_snapshot_date",
    "package_version",
    "permitted_engine_scope",
    "proof_of_purchase_path",
    "reviewer_approval",
    "commercial_use_allowed",
    "credit_required",
    "redistribution_allowed",
    "training_data_restriction",
    "rights_risk",
    "adoption_state",
    "final_reviewer",
    "notes",
]

VALID_ADOPTION_STATES = {"prototype", "candidate", "approved", "rejected", "replaced"}
VALID_RIGHTS_RISK = {"low", "medium", "high"}
YES_NO_UNKNOWN = {"yes", "no", "unknown"}
DATE_OR_PENDING = re.compile(r"^\d{4}-\d{2}-\d{2}$|^pending$")
ASSET_ID = re.compile(r"^[a-z0-9][a-z0-9_.-]*$")


def empty_or_pending(value: str) -> bool:
    return value.strip().lower() in {"", "pending", "unknown", "none", "n/a"}


def validate_row(row: dict[str, str], row_number: int, seen_ids: set[str]) -> list[str]:
    errors: list[str] = []
    prefix = f"row {row_number}"

    asset_id = row["asset_id"].strip()
    if not ASSET_ID.match(asset_id):
        errors.append(f"{prefix}: asset_id must be lowercase slug-like text")
    if asset_id in seen_ids:
        errors.append(f"{prefix}: duplicate asset_id {asset_id}")
    seen_ids.add(asset_id)

    adoption_state = row["adoption_state"].strip().lower()
    if adoption_state not in VALID_ADOPTION_STATES:
        errors.append(f"{prefix}: invalid adoption_state {adoption_state!r}")

    rights_risk = row["rights_risk"].strip().lower()
    if rights_risk not in VALID_RIGHTS_RISK:
        errors.append(f"{prefix}: invalid rights_risk {rights_risk!r}")

    for field in ("commercial_use_allowed", "credit_required", "redistribution_allowed"):
        value = row[field].strip().lower()
        if value not in YES_NO_UNKNOWN:
            errors.append(f"{prefix}: {field} must be yes/no/unknown")

    if not DATE_OR_PENDING.match(row["license_snapshot_date"].strip().lower()):
        errors.append(f"{prefix}: license_snapshot_date must be YYYY-MM-DD or pending")

    if empty_or_pending(row["asset_name"]):
        errors.append(f"{prefix}: asset_name is required")
    if empty_or_pending(row["asset_type"]):
        errors.append(f"{prefix}: asset_type is required")

    fab_url = row["fab_listing_url"].strip()
    if "fab" in row["tool_or_model"].lower() or fab_url:
        if not fab_url.startswith("https://www.fab.com/listings/"):
            errors.append(f"{prefix}: Fab assets require a fab_listing_url under https://www.fab.com/listings/")
        if empty_or_pending(row["publisher_or_seller"]):
            errors.append(f"{prefix}: Fab assets require publisher_or_seller")

    output_path = row["output_path"].strip()
    if adoption_state != "approved" and output_path.startswith("Content/Frostwake/"):
        errors.append(f"{prefix}: non-approved assets may not point at Content/Frostwake/")

    if adoption_state == "approved":
        if row["commercial_use_allowed"].strip().lower() != "yes":
            errors.append(f"{prefix}: approved assets require commercial_use_allowed=yes")
        for field in (
            "license",
            "license_snapshot_date",
            "proof_of_purchase_path",
            "reviewer_approval",
            "final_reviewer",
            "output_path",
        ):
            if empty_or_pending(row[field]):
                errors.append(f"{prefix}: approved assets require {field}")

    return errors


def validate_ledger(path: Path) -> list[str]:
    if not path.exists():
        return [f"{path.relative_to(ROOT)}: file missing"]

    with path.open("r", encoding="utf-8", newline="") as handle:
        reader = csv.DictReader(handle)
        if reader.fieldnames is None:
            return [f"{path.relative_to(ROOT)}: missing header"]

        missing = [column for column in REQUIRED_COLUMNS if column not in reader.fieldnames]
        extra = [column for column in reader.fieldnames if column not in REQUIRED_COLUMNS]
        errors: list[str] = []
        if missing:
            errors.append(f"missing columns: {', '.join(missing)}")
        if extra:
            errors.append(f"unexpected columns: {', '.join(extra)}")
        if errors:
            return errors

        seen_ids: set[str] = set()
        row_count = 0
        for row_count, row in enumerate(reader, start=2):
            if not any((value or "").strip() for value in row.values()):
                continue
            errors.extend(validate_row(row, row_count, seen_ids))

    if row_count == 0:
        return [f"{path.relative_to(ROOT)}: no asset rows"]
    return errors


def main() -> int:
    parser = argparse.ArgumentParser(description="Validate Frostwake asset ledger candidate rows.")
    parser.add_argument("--ledger", default=str(DEFAULT_LEDGER), help="CSV ledger path.")
    args = parser.parse_args()

    path = Path(args.ledger)
    if not path.is_absolute():
        path = ROOT / path

    errors = validate_ledger(path)
    if errors:
        print("[FAIL] asset_ledger_check")
        for error in errors:
            print(f"- {error}")
        return 1

    print(f"[PASS] asset_ledger_check: {path.relative_to(ROOT)}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
