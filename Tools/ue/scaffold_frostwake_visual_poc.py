#!/usr/bin/env python3
from __future__ import annotations

import argparse
import json
from dataclasses import asdict, dataclass
from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
DEFAULT_SOURCE_MAP = "/Game/Maps/L_IcebreakerWhitebox"
DEFAULT_TARGET_MAP = "/Game/Maps/L_FrostwakeVisualPOC"
DEFAULT_MANIFEST = ROOT / "Saved" / "VisualPOC" / "frostwake_visual_poc_plan.json"
QUARANTINE_ROOT = "Content/ThirdParty/Quarantine"
PROJECT_ART_ROOT = "Content/Frostwake"


@dataclass(frozen=True)
class PocZone:
    zone_id: str
    purpose: str
    source_whitebox_area: str
    art_direction: str
    acceptance: str


POC_ZONES = [
    PocZone(
        zone_id="central-corridor",
        purpose="Social choke point and proximity voice suspicion",
        source_whitebox_area="Mess / Workshop / central passage",
        art_direction="near-future bulkheads, emergency LEDs, frost on windows, readable sight breaks",
        acceptance="players can read it as the main meeting and accusation space within 5 seconds",
    ),
    PocZone(
        zone_id="engine-fuel-battery-bay",
        purpose="Core systems pressure and sabotage readability",
        source_whitebox_area="Engine Room / Fuel Store / Boiler",
        art_direction="industrial ship machinery, battery racks, fuel lines, pumps, repair panels",
        acceptance="players can identify at least two interactable system targets without UI explanation",
    ),
    PocZone(
        zone_id="exterior-ice-access",
        purpose="High-risk exterior route and isolation pressure",
        source_whitebox_area="Stern Work Deck / Ice Field Access",
        art_direction="snowy metal deck, floodlights, containers, safety rails, whiteout boundary",
        acceptance="players immediately understand that leaving the vessel is useful but dangerous",
    ),
]


REQUIRED_LEDGER_FIELDS = [
    "asset_id",
    "asset_name",
    "asset_type",
    "fab_listing_url",
    "publisher_or_seller",
    "license_snapshot_date",
    "proof_of_purchase_path",
    "commercial_use_allowed",
    "rights_risk",
    "adoption_state",
    "reviewer_approval",
]


def fail(message: str) -> None:
    raise SystemExit(f"[FAIL] {message}")


def repo_relative(path: Path) -> str:
    return str(path.relative_to(ROOT))


def build_manifest(args: argparse.Namespace) -> dict:
    return {
        "mode": "dry-run" if args.dry_run else "plan",
        "source_map": args.source_map,
        "target_map": args.target_map,
        "will_modify_unreal_assets": False,
        "will_change_default_maps": False,
        "quarantine_root": QUARANTINE_ROOT,
        "project_art_root": PROJECT_ART_ROOT,
        "poc_zones": [asdict(zone) for zone in POC_ZONES],
        "required_ledger_fields": REQUIRED_LEDGER_FIELDS,
        "blocked_actions": [
            "Do not rename or overwrite /Game/Maps/L_IcebreakerWhitebox.",
            "Do not change GameDefaultMap or EditorStartupMap for the visual POC.",
            "Do not import third-party assets into Content/Frostwake until approved.",
            "Do not purchase or import paid assets without a fresh listing/license check.",
        ],
        "next_execute_step": (
            "Create the target map in Unreal Editor only after selecting assets and recording "
            "candidate ledger entries. This script intentionally does not create .umap files."
        ),
    }


def validate_args(args: argparse.Namespace) -> None:
    if args.source_map != DEFAULT_SOURCE_MAP:
        fail(f"source_map must remain {DEFAULT_SOURCE_MAP} for this scaffold")
    if args.target_map == args.source_map:
        fail("target_map must not equal source_map")
    if not args.target_map.startswith("/Game/Maps/"):
        fail("target_map must be under /Game/Maps/")
    if "IcebreakerWhitebox" in args.target_map:
        fail("target_map must not look like the automation whitebox map")


def write_manifest(manifest: dict, output: Path, force: bool) -> None:
    if output.exists() and not force:
        fail(f"{repo_relative(output)} already exists; use --force to overwrite")
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_text(json.dumps(manifest, indent=2, sort_keys=True) + "\n", encoding="utf-8")


def main() -> int:
    parser = argparse.ArgumentParser(
        description="Create a safe dry-run manifest for the Frostwake near-future visual POC map."
    )
    parser.add_argument("--source-map", default=DEFAULT_SOURCE_MAP)
    parser.add_argument("--target-map", default=DEFAULT_TARGET_MAP)
    parser.add_argument("--out", default=str(DEFAULT_MANIFEST), help="Manifest output path.")
    parser.add_argument("--write", action="store_true", help="Write the manifest under Saved/VisualPOC.")
    parser.add_argument("--force", action="store_true", help="Overwrite an existing manifest when --write is used.")
    parser.add_argument("--dry-run", action="store_true", help="Print the planned manifest without writing files.")
    args = parser.parse_args()

    validate_args(args)
    manifest = build_manifest(args)

    output = Path(args.out)
    if not output.is_absolute():
        output = ROOT / output

    if args.write:
        write_manifest(manifest, output, args.force)
        print(f"[PASS] wrote {repo_relative(output)}")
    else:
        print(json.dumps(manifest, indent=2, sort_keys=True))

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
