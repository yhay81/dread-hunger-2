use std::collections::{BTreeMap, HashSet};
use std::fs;
use std::io::{Read, Write};
use std::net::{TcpStream, ToSocketAddrs};
use std::path::{Path, PathBuf};
use std::process::{Child, Command as ProcessCommand, ExitCode, Stdio};
use std::time::{Duration, UNIX_EPOCH};

use chrono::{DateTime, Local, NaiveDateTime, Utc};
use clap::{Parser, Subcommand, ValueEnum};
use regex::Regex;
use serde_json::{Map, Value, json};
use sha2::{Digest, Sha256};

const FIXED_MATCH_PLAYERS: i64 = 8;
const STANDARD_MINIMUM_COMPLETED_MATCHES: i64 = 50;
const REQUIRED_ASSET_LEDGER_COLUMNS: &[&str] = &[
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
];
const DEFAULT_REDACT_FIELDS: &[&str] = &[
    "token",
    "secret",
    "password",
    "authorization",
    "steam_id",
    "external_chat_id",
    "chat_account_id",
    "ip",
];

#[derive(Debug, Parser)]
#[command(author, version, about)]
struct Cli {
    #[command(subcommand)]
    command: Command,
}

#[derive(Debug, Subcommand)]
enum Command {
    #[command(about = "Validate a Frostwake dedicated-server config JSON file.")]
    ServerConfigCheck {
        config: PathBuf,
        #[arg(long, default_value = "Tools/ops/server_config.schema.json")]
        schema: PathBuf,
        #[arg(long)]
        json: bool,
    },
    #[command(about = "Validate Frostwake Steam Lobby metadata without calling Steam APIs.")]
    LobbyMetadataCheck {
        metadata: PathBuf,
        #[arg(long, default_value = "Tools/ops/lobby_metadata.schema.json")]
        schema: PathBuf,
        #[arg(long)]
        expected_build_id: Option<String>,
        #[arg(long)]
        expected_map_id: Option<String>,
        #[arg(long)]
        json: bool,
    },
    #[command(about = "Decide whether a Steam Lobby row is safe to join before client travel.")]
    LobbyJoinDecision {
        metadata: PathBuf,
        #[arg(long, default_value = "Tools/ops/lobby_metadata.schema.json")]
        schema: PathBuf,
        #[arg(long)]
        expected_build_id: Option<String>,
        #[arg(long)]
        expected_map_id: Option<String>,
        #[arg(long)]
        require_accepted: bool,
        #[arg(long)]
        expected_reject_reason: Option<String>,
        #[arg(long)]
        json: bool,
    },
    #[command(about = "Validate Frostwake asset ledger candidate CSV rows.")]
    AssetLedgerCheck {
        #[arg(long, default_value = "docs/asset-ledger-candidates.csv")]
        ledger: PathBuf,
    },
    #[command(
        about = "Create a local-only reference inventory from references/external-paths.json."
    )]
    ReferenceInventory {
        #[arg(long, default_value = "references/external-paths.json")]
        manifest: PathBuf,
        #[arg(long, default_value = "references/inventory")]
        out_dir: PathBuf,
        #[arg(long)]
        max_depth: Option<usize>,
        #[arg(long)]
        max_files_per_source: Option<usize>,
        #[arg(long)]
        hash_files: bool,
    },
    #[command(about = "Redact obvious secret and identifier fields from a JSON document.")]
    RedactJson {
        input: PathBuf,
        #[arg(long)]
        out: Option<PathBuf>,
        #[arg(long = "field")]
        fields: Vec<String>,
    },
    #[command(about = "Print ban entries from a local banlist JSON file.")]
    BanlistList { path: PathBuf },
    #[command(about = "Append one entry to a local banlist JSON file.")]
    BanlistAdd {
        path: PathBuf,
        #[arg(long)]
        platform_user_id: String,
        #[arg(long)]
        scope: String,
        #[arg(long)]
        reason: String,
        #[arg(long)]
        created_at: String,
    },
    #[command(about = "List Phase 1 Steam registry placeholder server listings.")]
    SteamRegistryList,
    #[command(about = "Scan tracked-safe repository paths for obvious secret patterns.")]
    SecretScan {
        #[arg(long)]
        root: Option<PathBuf>,
    },
    #[command(about = "Summarize Frostwake JSONL match logs.")]
    LogSummary {
        events: PathBuf,
        #[arg(long)]
        out: Option<PathBuf>,
    },
    #[command(about = "Prepare or upload an anonymized playtest report to the local backend.")]
    PlaytestReportUpload {
        summary: PathBuf,
        #[arg(long, default_value = "http://localhost:8787/v1/playtest-reports")]
        endpoint: String,
        #[arg(long, default_value = "")]
        note: String,
        #[arg(long, default_value_t = 5.0)]
        timeout: f64,
        #[arg(long)]
        send: bool,
    },
    #[command(about = "Generate an anonymized P1 playtest summary skeleton from log-summary JSON.")]
    PlaytestSummary {
        summary: PathBuf,
        #[arg(long)]
        out: Option<PathBuf>,
        #[arg(long, default_value = "P1-024 Anonymized Summary")]
        title: String,
        #[arg(long, default_value_t = 8)]
        min_players: i64,
        #[arg(long)]
        target_players: Option<i64>,
        #[arg(long)]
        raw_evidence_label: Option<String>,
        #[arg(long)]
        snapshot: Option<String>,
        #[arg(long)]
        author: Option<String>,
        #[arg(long)]
        date: Option<String>,
        #[arg(long)]
        dry_run_example: bool,
    },
    #[command(
        about = "Preflight-check a P1-024 playtest summary before committing anonymized evidence."
    )]
    PlaytestPreflight {
        summary: PathBuf,
        #[arg(long)]
        markdown: Option<PathBuf>,
        #[arg(long, value_enum, default_value_t = PlaytestPreflightMode::Human)]
        mode: PlaytestPreflightMode,
        #[arg(long, default_value_t = 8)]
        min_players: i64,
    },
    #[command(about = "Create a local ignored P1-024 playtest run scaffold under Saved/Playtests.")]
    PlaytestRunScaffold {
        #[arg(long, default_value_t = 1)]
        run_number: i64,
        #[arg(long, default_value_t = 8)]
        target_players: i64,
        #[arg(long, default_value_t = 7777)]
        port: i64,
        #[arg(long, default_value_t = 1280)]
        res_x: i64,
        #[arg(long, default_value_t = 720)]
        res_y: i64,
        #[arg(long, default_value = "Frostwake-Win64-Development-local")]
        build_id: String,
        #[arg(long, default_value = "/Game/Maps/L_IcebreakerWhitebox")]
        map_id: String,
        #[arg(long, default_value = "human-p1-024")]
        profile: String,
        #[arg(long, default_value = r"C:\Program Files\Epic Games\UE_5.7")]
        ue_root: String,
        #[arg(
            long,
            default_value = "/Users/Shared/Epic Games/UE_5.7/Engine/Binaries/Mac/UnrealEditor"
        )]
        ue_editor: String,
        #[arg(long)]
        force: bool,
        #[arg(long)]
        dry_run: bool,
    },
    #[command(about = "Run repository quality checks from the Rust toolchain.")]
    QualityGate {
        #[arg(long)]
        json: bool,
        #[arg(long)]
        require_ue: bool,
    },
    #[command(about = "Export backlog rows into GitHub issue import files.")]
    BacklogToIssues {
        #[arg(long, default_value_t = 1)]
        phase: i64,
        #[arg(long)]
        backlog: Option<PathBuf>,
        #[arg(long, default_value = "docs/issue-import")]
        out_dir: PathBuf,
        #[arg(long)]
        id_prefix: Option<String>,
        #[arg(long)]
        milestone: Option<String>,
        #[arg(long, default_value = "prototype")]
        base_label: String,
    },
    #[command(about = "Render or create GitHub issues from an issue import CSV.")]
    GithubIssueSync {
        #[arg(long = "csv", default_value = "docs/issue-import/phase1-issues.csv")]
        csv_path: PathBuf,
        #[arg(long)]
        check_existing: bool,
        #[arg(long)]
        create: bool,
        #[arg(long)]
        json: bool,
        #[arg(long)]
        limit: Option<usize>,
    },
    #[command(about = "Create the next local cycle record under docs/cycles.")]
    NewCycle {
        #[arg(long, default_value = "0")]
        phase: String,
        #[arg(long, default_value = "Codex")]
        owner: String,
        #[arg(long)]
        goal: String,
        #[arg(long)]
        gate: String,
    },
    #[command(about = "Run Unreal project generation/build checks.")]
    UnrealGate {
        #[arg(long)]
        json: bool,
        #[arg(long)]
        skip_generate: bool,
        #[arg(long)]
        include_server: bool,
        #[arg(long)]
        ue_root: Option<PathBuf>,
        #[arg(long)]
        platform: Option<String>,
    },
    #[command(about = "Launch a local Unreal smoke run.")]
    RunLocalSmoke {
        #[arg(num_args = 0.., allow_hyphen_values = true, trailing_var_arg = true)]
        args: Vec<String>,
    },
    #[command(about = "Run a sequence of local Unreal smoke profiles.")]
    RunSmokeSuite {
        #[arg(long)]
        profiles: Vec<String>,
        #[arg(long)]
        include_heavy: bool,
        #[arg(long)]
        skip_build: bool,
        #[arg(long)]
        null_rhi: bool,
        #[arg(long)]
        platform: Option<String>,
        #[arg(long)]
        ue_root: Option<PathBuf>,
        #[arg(long)]
        suite_dir: Option<PathBuf>,
    },
    #[command(about = "Export a smoke suite summary JSON as Markdown evidence.")]
    ExportSmokeSuiteMarkdown {
        suite_summary: PathBuf,
        #[arg(long)]
        out: Option<PathBuf>,
    },
    #[command(about = "Build a Frostwake visual POC planning manifest.")]
    ScaffoldFrostwakeVisualPoc {
        #[arg(long, default_value = "/Game/Maps/L_IcebreakerWhitebox")]
        source_map: String,
        #[arg(long, default_value = "/Game/Maps/L_FrostwakeVisualPOC")]
        target_map: String,
        #[arg(long, default_value = "Saved/VisualPOC/frostwake_visual_poc_plan.json")]
        out: PathBuf,
        #[arg(long)]
        write: bool,
        #[arg(long)]
        force: bool,
        #[arg(long)]
        dry_run: bool,
    },
    #[command(about = "Run transient Unreal Editor automation for the Icebreaker whitebox map.")]
    CreateIcebreakerWhitebox {
        #[arg(long)]
        ue_root: Option<PathBuf>,
        #[arg(long)]
        platform: Option<String>,
        #[arg(long)]
        print_command: bool,
        #[arg(long)]
        skip_build: bool,
    },
    #[command(
        about = "Validate the generated Icebreaker whitebox map with transient Unreal automation."
    )]
    ValidateIcebreakerWhitebox {
        #[arg(long)]
        ue_root: Option<PathBuf>,
        #[arg(long)]
        platform: Option<String>,
        #[arg(long)]
        print_command: bool,
        #[arg(long)]
        skip_build: bool,
    },
    #[command(
        about = "Create a separate Frostwake visual POC map with free/CC0-ready dressing placeholders."
    )]
    CreateFrostwakeVisualPoc {
        #[arg(long)]
        ue_root: Option<PathBuf>,
        #[arg(long)]
        platform: Option<String>,
        #[arg(long)]
        print_command: bool,
        #[arg(long)]
        skip_build: bool,
    },
    #[command(
        about = "Validate the Frostwake visual POC map exists and contains the required zones."
    )]
    ValidateFrostwakeVisualPoc {
        #[arg(long)]
        ue_root: Option<PathBuf>,
        #[arg(long)]
        platform: Option<String>,
        #[arg(long)]
        print_command: bool,
        #[arg(long)]
        skip_build: bool,
    },
    #[command(
        about = "Import the downloaded ambientCG visual POC source textures into Unreal quarantine assets."
    )]
    ImportAmbientCgVisualPocAssets {
        #[arg(long)]
        ue_root: Option<PathBuf>,
        #[arg(long)]
        platform: Option<String>,
        #[arg(long)]
        print_command: bool,
        #[arg(long)]
        skip_build: bool,
    },
    #[command(about = "Validate the ambientCG visual POC textures were imported as Unreal assets.")]
    ValidateAmbientCgVisualPocAssets {
        #[arg(long)]
        ue_root: Option<PathBuf>,
        #[arg(long)]
        platform: Option<String>,
        #[arg(long)]
        print_command: bool,
        #[arg(long)]
        skip_build: bool,
    },
    #[command(about = "Create the L_MainMenu front-end map (menu GameMode, no whitebox geometry).")]
    CreateMainMenu {
        #[arg(long)]
        ue_root: Option<PathBuf>,
        #[arg(long)]
        platform: Option<String>,
        #[arg(long)]
        print_command: bool,
        #[arg(long)]
        skip_build: bool,
    },
    #[command(
        about = "Import the quarantined Poly Haven CC0 assets (HDRI + props) into Unreal uassets."
    )]
    ImportPolyhavenCc0 {
        #[arg(long)]
        ue_root: Option<PathBuf>,
        #[arg(long)]
        platform: Option<String>,
        #[arg(long)]
        print_command: bool,
        #[arg(long)]
        skip_build: bool,
    },
}

#[derive(Clone, Copy, Debug, Eq, PartialEq, ValueEnum)]
enum PlaytestPreflightMode {
    Human,
    DryRun,
}

fn main() -> ExitCode {
    match run(Cli::parse()) {
        Ok(()) => ExitCode::SUCCESS,
        Err(message) => {
            eprintln!("{message}");
            ExitCode::FAILURE
        }
    }
}

fn run(cli: Cli) -> Result<(), String> {
    match cli.command {
        Command::ServerConfigCheck {
            config,
            schema,
            json,
        } => {
            let schema = load_json_object(&schema)?;
            let config = load_json_object(&config)?;
            let errors =
                validate_schema_value(&Value::Object(schema), &Value::Object(config.clone()), "$");
            if !errors.is_empty() {
                print_failures(&errors);
                return Err("server config validation failed".to_string());
            }

            let summary = summarize_server_config(&config);
            if json {
                println!(
                    "{}",
                    serde_json::to_string_pretty(&summary).expect("summary serializes")
                );
            } else {
                println!(
                    "[PASS] server_config name={} region={} maxPlayers={} map={} ruleset={} advertise={}",
                    display_value(summary.get("serverName")),
                    display_value(summary.get("region")),
                    display_value(summary.get("maxPlayers")),
                    display_value(summary.get("map")),
                    display_value(summary.get("ruleset")),
                    display_value(summary.get("advertise")),
                );
            }
        }
        Command::LobbyMetadataCheck {
            metadata,
            schema,
            expected_build_id,
            expected_map_id,
            json,
        } => {
            let schema = load_json_object(&schema)?;
            let metadata = load_json_object(&metadata)?;
            let errors = validate_lobby_metadata(
                &schema,
                &metadata,
                expected_build_id.as_deref(),
                expected_map_id.as_deref(),
            );
            if !errors.is_empty() {
                print_failures(&errors);
                return Err("lobby metadata validation failed".to_string());
            }

            let summary = summarize_lobby_metadata(&metadata);
            if json {
                println!(
                    "{}",
                    serde_json::to_string_pretty(&summary).expect("summary serializes")
                );
            } else {
                println!(
                    "[PASS] lobby_metadata build={} map={} players={} joinState={} mode={}",
                    display_value(summary.get("buildId")),
                    display_value(summary.get("mapId")),
                    display_value(summary.get("players")),
                    display_value(summary.get("joinState")),
                    display_value(summary.get("connectionMode")),
                );
            }
        }
        Command::LobbyJoinDecision {
            metadata,
            schema,
            expected_build_id,
            expected_map_id,
            require_accepted,
            expected_reject_reason,
            json,
        } => {
            let schema = load_json_object(&schema)?;
            let metadata = load_json_object(&metadata)?;
            let decision = decide_lobby_join(
                &schema,
                &metadata,
                expected_build_id.as_deref(),
                expected_map_id.as_deref(),
            );
            let expectation_errors = check_lobby_join_decision_expectations(
                &decision,
                require_accepted,
                expected_reject_reason.as_deref(),
            );

            if json {
                println!(
                    "{}",
                    serde_json::to_string_pretty(&decision).expect("decision serializes")
                );
            } else {
                print_lobby_join_decision(&decision);
            }

            if !expectation_errors.is_empty() {
                print_failures(&expectation_errors);
                return Err("lobby join decision validation failed".to_string());
            }
        }
        Command::AssetLedgerCheck { ledger } => {
            let errors = validate_asset_ledger(&ledger)?;
            if !errors.is_empty() {
                println!("[FAIL] asset_ledger_check");
                for error in errors {
                    println!("- {error}");
                }
                return Err("asset ledger validation failed".to_string());
            }
            println!(
                "[PASS] asset_ledger_check: {}",
                repo_relative_display(&resolve_path(&ledger))
            );
        }
        Command::ReferenceInventory {
            manifest,
            out_dir,
            max_depth,
            max_files_per_source,
            hash_files,
        } => {
            let options = ReferenceInventoryOptions {
                manifest: resolve_path(&manifest),
                out_dir: resolve_path(&out_dir),
                max_depth: max_depth.unwrap_or_else(|| env_usize("MAX_DEPTH", 3)),
                max_files_per_source: max_files_per_source
                    .unwrap_or_else(|| env_usize("MAX_FILES_PER_SOURCE", 1000)),
                hash_files: hash_files
                    || std::env::var("HASH_FILES").is_ok_and(|value| value == "1"),
            };
            let output = write_reference_inventory(&options)?;
            println!(
                "Wrote local-only inventory: {}",
                repo_relative_display(&output.inventory_path)
            );
            println!(
                "Wrote local-only summary: {}",
                repo_relative_display(&output.summary_path)
            );
            println!("Do not commit files under references/inventory/.");
        }
        Command::RedactJson { input, out, fields } => {
            let value = load_json_value(&input)?;
            let redacted = redact_json_value(&value, &normalized_redact_fields(&fields));
            let serialized =
                serde_json::to_string_pretty(&redacted).expect("redacted JSON serializes");
            if let Some(out) = out {
                if let Some(parent) = out.parent()
                    && !parent.as_os_str().is_empty()
                {
                    fs::create_dir_all(parent)
                        .map_err(|error| format!("{}: {error}", parent.display()))?;
                }
                fs::write(&out, format!("{serialized}\n"))
                    .map_err(|error| format!("{}: {error}", out.display()))?;
            } else {
                println!("{serialized}");
            }
        }
        Command::BanlistList { path } => {
            let bans = load_banlist_entries(&path)?;
            println!(
                "{}",
                serde_json::to_string_pretty(&bans).expect("banlist serializes")
            );
        }
        Command::BanlistAdd {
            path,
            platform_user_id,
            scope,
            reason,
            created_at,
        } => {
            let bans = add_banlist_entry(&path, &platform_user_id, &scope, &reason, &created_at)?;
            println!(
                "{}",
                serde_json::to_string_pretty(&bans).expect("banlist serializes")
            );
        }
        Command::SteamRegistryList => {
            let listings = steam_registry_list_servers();
            println!(
                "{}",
                serde_json::to_string_pretty(&listings).expect("listings serialize")
            );
        }
        Command::SecretScan { root } => {
            let root = root.unwrap_or_else(repo_root);
            let findings = scan_for_secrets(&root)?;
            if !findings.is_empty() {
                for finding in findings {
                    eprintln!(
                        "{}:{}: {}",
                        finding.path.display(),
                        finding.line,
                        finding.text
                    );
                }
                return Err("Potential secret found. Review before committing.".to_string());
            }
            println!("No obvious secrets found in tracked-safe paths.");
        }
        Command::LogSummary { events, out } => {
            let summary = summarize_events(&events)?;
            let serialized = serde_json::to_string_pretty(&summary).expect("summary serializes");
            if let Some(out) = out {
                if let Some(parent) = out.parent()
                    && !parent.as_os_str().is_empty()
                {
                    fs::create_dir_all(parent)
                        .map_err(|error| format!("{}: {error}", parent.display()))?;
                }
                fs::write(&out, format!("{serialized}\n"))
                    .map_err(|error| format!("{}: {error}", out.display()))?;
            } else {
                println!("{serialized}");
            }
        }
        Command::PlaytestReportUpload {
            summary,
            endpoint,
            note,
            timeout,
            send,
        } => {
            let summary = load_summary(&summary)?;
            let payload = build_playtest_report_payload(&summary, &note)?;
            println!(
                "{}",
                serde_json::to_string_pretty(&payload).expect("payload serializes")
            );
            if send {
                let response =
                    post_json_http(&endpoint, &payload, Duration::from_secs_f64(timeout))?;
                println!(
                    "{}",
                    serde_json::to_string_pretty(&response).expect("response serializes")
                );
            } else {
                println!("# dry-run: add --send to POST this payload.");
            }
        }
        Command::PlaytestSummary {
            summary,
            out,
            title,
            min_players,
            target_players,
            raw_evidence_label,
            snapshot,
            author,
            date,
            dry_run_example,
        } => {
            let summary_path = summary;
            let summary = load_summary_json(&summary_path)?;
            let markdown = render_playtest_summary_markdown(PlaytestSummaryRenderOptions {
                summary: &summary,
                summary_path: &resolve_path(&summary_path),
                title: &title,
                min_players,
                target_players,
                raw_evidence_label: raw_evidence_label.as_deref(),
                snapshot: snapshot.as_deref(),
                author: author.as_deref(),
                date: date.as_deref(),
                dry_run_example,
            });
            if let Some(out) = out {
                if let Some(parent) = out.parent()
                    && !parent.as_os_str().is_empty()
                {
                    fs::create_dir_all(parent)
                        .map_err(|error| format!("{}: {error}", parent.display()))?;
                }
                fs::write(&out, markdown).map_err(|error| format!("{}: {error}", out.display()))?;
            } else {
                print!("{markdown}");
            }
        }
        Command::PlaytestPreflight {
            summary,
            markdown,
            mode,
            min_players,
        } => {
            let summary_value = load_summary_json(&summary)?;
            let mut results = check_playtest_summary(&summary_value, &summary, mode, min_players);
            results.extend(check_playtest_markdown(
                markdown.as_deref(),
                mode,
                &summary_value,
                min_players,
            ));
            for result in &results {
                println!(
                    "[{}] {}: {}",
                    result.status.label(),
                    result.name,
                    result.detail
                );
            }
            let failures = results
                .iter()
                .filter(|result| result.status == CheckStatus::Fail)
                .count();
            let warnings = results
                .iter()
                .filter(|result| result.status == CheckStatus::Warn)
                .count();
            if failures > 0 {
                println!("[RESULT] fail: {failures} failed, {warnings} warning(s)");
                return Err("playtest preflight failed".to_string());
            }
            println!("[RESULT] pass: {warnings} warning(s)");
        }
        Command::PlaytestRunScaffold {
            run_number,
            target_players,
            port,
            res_x,
            res_y,
            build_id,
            map_id,
            profile,
            ue_root,
            ue_editor,
            force,
            dry_run,
        } => {
            let scaffold = build_playtest_run_scaffold(PlaytestRunScaffoldOptions {
                run_number,
                target_players,
                port,
                res_x,
                res_y,
                build_id: &build_id,
                map_id: &map_id,
                profile: &profile,
                ue_root: &ue_root,
                ue_editor: &ue_editor,
            })?;
            println!("Run directory: {}", scaffold.run_dir.display());
            for directory in &scaffold.directories {
                println!("Directory: {}", directory.display());
            }
            for file in &scaffold.files {
                println!("File: {}", file.path.display());
            }
            if !dry_run {
                write_playtest_run_scaffold(&scaffold, force)?;
            }
        }
        Command::QualityGate { json, require_ue } => {
            let results = run_quality_gate(require_ue);
            emit_check_results(&results, json);
            if results.iter().any(|result| result.status == "fail") {
                return Err("quality gate failed".to_string());
            }
        }
        Command::BacklogToIssues {
            phase,
            backlog,
            out_dir,
            id_prefix,
            milestone,
            base_label,
        } => {
            let backlog =
                backlog.unwrap_or_else(|| PathBuf::from(format!("docs/phase{phase}-backlog.md")));
            let id_prefix = id_prefix.unwrap_or_else(|| format!("P{phase}-"));
            let milestone = milestone.unwrap_or_else(|| format!("Phase {phase}"));
            let rows = parse_backlog_to_issue_rows(
                &resolve_path(&backlog),
                phase,
                &id_prefix,
                &milestone,
                &base_label,
            )?;
            if rows.is_empty() {
                return Err(format!(
                    "No {id_prefix} issue rows found in {}",
                    resolve_path(&backlog).display()
                ));
            }
            let out_dir = resolve_path(&out_dir);
            write_issue_import_csv(&rows, &out_dir.join(format!("phase{phase}-issues.csv")))?;
            write_issue_import_markdown(
                &rows,
                &out_dir.join(format!("phase{phase}-issues.md")),
                phase,
                &resolve_path(&backlog),
            )?;
            println!(
                "Exported {} issues to {}",
                rows.len(),
                repo_relative_display(&out_dir)
            );
        }
        Command::GithubIssueSync {
            csv_path,
            check_existing,
            create,
            json,
            limit,
        } => {
            let mut specs = load_issue_specs(&resolve_path(&csv_path))?;
            if check_existing {
                let existing = existing_issue_titles()?;
                specs.retain(|spec| !existing.contains(&spec.title));
            }
            if let Some(limit) = limit {
                specs.truncate(limit);
            }
            let commands: Vec<Vec<String>> =
                specs.iter().map(build_gh_issue_create_command).collect();
            if json {
                println!(
                    "{}",
                    serde_json::to_string_pretty(&commands).expect("commands serialize")
                );
            } else {
                for command in &commands {
                    println!("{}", shell_command(command));
                }
            }
            if !create {
                println!(
                    "# dry-run: {} issue command(s). Add --create to run them.",
                    commands.len()
                );
            } else {
                for command in commands {
                    let result = run_process("gh_issue_create", &command, None);
                    if result.status == "fail" {
                        return Err(result.detail);
                    }
                }
            }
        }
        Command::NewCycle {
            phase,
            owner,
            goal,
            gate,
        } => {
            let cycle_dir = repo_root().join("docs/cycles");
            fs::create_dir_all(&cycle_dir)
                .map_err(|error| format!("{}: {error}", cycle_dir.display()))?;
            let cycle = next_cycle_number(&cycle_dir)?;
            let date = Local::now().format("%Y-%m-%d").to_string();
            let path = cycle_dir.join(format!("{date}-cycle-{cycle}.md"));
            fs::write(
                &path,
                build_cycle_doc(cycle, &date, &phase, &owner, &goal, &gate),
            )
            .map_err(|error| format!("{}: {error}", path.display()))?;
            println!("{}", repo_relative_display(&path));
        }
        Command::UnrealGate {
            json,
            skip_generate,
            include_server,
            ue_root,
            platform,
        } => {
            let platform = platform.unwrap_or_else(host_platform);
            let ue_root = resolve_ue_root(ue_root);
            let results = run_unreal_gate(&ue_root, &platform, skip_generate, include_server);
            emit_check_results(&results, json);
            if results.iter().any(|result| result.status == "fail") {
                return Err("unreal gate failed".to_string());
            }
        }
        Command::RunLocalSmoke { args } => {
            let (mut options, describe_profile) = parse_local_smoke_args(&args)?;
            apply_smoke_profile(&mut options);
            if options.host_only {
                options.clients = 0;
            }
            if describe_profile {
                println!(
                    "{}",
                    serde_json::to_string_pretty(&describe_smoke_settings(&options))
                        .expect("profile serializes")
                );
            } else {
                run_local_smoke(&options)?;
            }
        }
        Command::RunSmokeSuite {
            profiles,
            include_heavy,
            skip_build,
            null_rhi,
            platform,
            ue_root,
            suite_dir,
        } => {
            let profiles = profiles
                .iter()
                .map(|profile| parse_smoke_profile(profile))
                .collect::<Result<Vec<_>, _>>()?;
            let exit_code = run_smoke_suite(&SmokeSuiteOptions {
                profiles,
                include_heavy,
                skip_build,
                null_rhi,
                platform,
                ue_root,
                suite_dir,
            })?;
            if exit_code != 0 {
                return Err(format!("smoke suite failed with exit code {exit_code}"));
            }
        }
        Command::ExportSmokeSuiteMarkdown { suite_summary, out } => {
            let data = load_json_value(&suite_summary)?;
            let markdown = render_smoke_suite_markdown(&data)?;
            let out = out.unwrap_or_else(|| resolve_path(&suite_summary).with_extension("md"));
            if let Some(parent) = out.parent()
                && !parent.as_os_str().is_empty()
            {
                fs::create_dir_all(parent)
                    .map_err(|error| format!("{}: {error}", parent.display()))?;
            }
            fs::write(&out, markdown).map_err(|error| format!("{}: {error}", out.display()))?;
            println!("{}", out.display());
        }
        Command::ScaffoldFrostwakeVisualPoc {
            source_map,
            target_map,
            out,
            write,
            force,
            dry_run,
        } => {
            let manifest = build_visual_poc_manifest(&source_map, &target_map, dry_run)?;
            if write {
                let out = resolve_path(&out);
                if out.exists() && !force {
                    return Err(format!(
                        "{} already exists; use --force to overwrite",
                        repo_relative_display(&out)
                    ));
                }
                if let Some(parent) = out.parent()
                    && !parent.as_os_str().is_empty()
                {
                    fs::create_dir_all(parent)
                        .map_err(|error| format!("{}: {error}", parent.display()))?;
                }
                fs::write(
                    &out,
                    format!(
                        "{}\n",
                        serde_json::to_string_pretty(&manifest).expect("manifest serializes")
                    ),
                )
                .map_err(|error| format!("{}: {error}", out.display()))?;
                println!("[PASS] wrote {}", repo_relative_display(&out));
            } else {
                println!(
                    "{}",
                    serde_json::to_string_pretty(&manifest).expect("manifest serializes")
                );
            }
        }
        Command::CreateIcebreakerWhitebox {
            ue_root,
            platform,
            print_command,
            skip_build,
        } => {
            run_unreal_editor_commandlet(
                "CreateIcebreakerWhitebox",
                ue_root,
                platform.unwrap_or_else(host_platform),
                print_command,
                skip_build,
            )?;
        }
        Command::ValidateIcebreakerWhitebox {
            ue_root,
            platform,
            print_command,
            skip_build,
        } => {
            run_unreal_editor_commandlet(
                "ValidateIcebreakerWhitebox",
                ue_root,
                platform.unwrap_or_else(host_platform),
                print_command,
                skip_build,
            )?;
        }
        Command::CreateFrostwakeVisualPoc {
            ue_root,
            platform,
            print_command,
            skip_build,
        } => {
            run_unreal_editor_commandlet(
                "CreateFrostwakeVisualPoc",
                ue_root,
                platform.unwrap_or_else(host_platform),
                print_command,
                skip_build,
            )?;
        }
        Command::ValidateFrostwakeVisualPoc {
            ue_root,
            platform,
            print_command,
            skip_build,
        } => {
            run_unreal_editor_commandlet(
                "ValidateFrostwakeVisualPoc",
                ue_root,
                platform.unwrap_or_else(host_platform),
                print_command,
                skip_build,
            )?;
        }
        Command::ImportAmbientCgVisualPocAssets {
            ue_root,
            platform,
            print_command,
            skip_build,
        } => {
            run_unreal_editor_commandlet(
                "ImportAmbientCgVisualPocAssets",
                ue_root,
                platform.unwrap_or_else(host_platform),
                print_command,
                skip_build,
            )?;
        }
        Command::ValidateAmbientCgVisualPocAssets {
            ue_root,
            platform,
            print_command,
            skip_build,
        } => {
            run_unreal_editor_commandlet(
                "ValidateAmbientCgVisualPocAssets",
                ue_root,
                platform.unwrap_or_else(host_platform),
                print_command,
                skip_build,
            )?;
        }
        Command::CreateMainMenu {
            ue_root,
            platform,
            print_command,
            skip_build,
        } => {
            run_unreal_editor_commandlet(
                "CreateMainMenu",
                ue_root,
                platform.unwrap_or_else(host_platform),
                print_command,
                skip_build,
            )?;
        }
        Command::ImportPolyhavenCc0 {
            ue_root,
            platform,
            print_command,
            skip_build,
        } => {
            run_unreal_editor_commandlet(
                "ImportPolyhavenCc0Assets",
                ue_root,
                platform.unwrap_or_else(host_platform),
                print_command,
                skip_build,
            )?;
        }
    }

    Ok(())
}

fn load_json_value(path: &Path) -> Result<Value, String> {
    let resolved = resolve_path(path);
    let text = fs::read_to_string(&resolved)
        .map_err(|error| format!("{}: {error}", resolved.display()))?;
    serde_json::from_str(&text)
        .map_err(|error| format!("{}: invalid JSON: {error}", resolved.display()))
}

fn load_json_object(path: &Path) -> Result<Map<String, Value>, String> {
    let value = load_json_value(path)?;
    value
        .as_object()
        .cloned()
        .ok_or_else(|| format!("{}: expected JSON object", resolve_path(path).display()))
}

fn resolve_path(path: &Path) -> PathBuf {
    if path.is_absolute() {
        return path.to_path_buf();
    }
    let current_candidate = std::env::current_dir()
        .expect("current directory exists")
        .join(path);
    if current_candidate.exists() {
        return current_candidate;
    }
    repo_root().join(path)
}

fn repo_root() -> PathBuf {
    Path::new(env!("CARGO_MANIFEST_DIR"))
        .ancestors()
        .nth(2)
        .expect("crate lives under repo/crates/frostwake-tools")
        .to_path_buf()
}

fn print_failures(errors: &[String]) {
    for error in errors {
        eprintln!("[FAIL] {error}");
    }
}

fn repo_relative_display(path: &Path) -> String {
    let root = repo_root();
    path.strip_prefix(&root)
        .unwrap_or(path)
        .to_string_lossy()
        .replace('\\', "/")
}

fn empty_or_pending(value: &str) -> bool {
    let normalized = value.trim().to_ascii_lowercase();
    matches!(
        normalized.as_str(),
        "" | "pending" | "unknown" | "none" | "n/a"
    )
}

fn validate_asset_ledger(path: &Path) -> Result<Vec<String>, String> {
    let resolved = resolve_path(path);
    if !resolved.exists() {
        return Ok(vec![format!(
            "{}: file missing",
            repo_relative_display(&resolved)
        )]);
    }
    if fs::metadata(&resolved)
        .map_err(|error| format!("{}: {error}", resolved.display()))?
        .len()
        == 0
    {
        return Ok(vec![format!(
            "{}: missing header",
            repo_relative_display(&resolved)
        )]);
    }

    let mut reader = csv::Reader::from_path(&resolved)
        .map_err(|error| format!("{}: {error}", resolved.display()))?;
    let headers = reader
        .headers()
        .map_err(|error| format!("{}: {error}", resolved.display()))?
        .clone();
    if headers.is_empty() {
        return Ok(vec![format!(
            "{}: missing header",
            repo_relative_display(&resolved)
        )]);
    }

    let missing: Vec<&str> = REQUIRED_ASSET_LEDGER_COLUMNS
        .iter()
        .copied()
        .filter(|column| !headers.iter().any(|header| header == *column))
        .collect();
    let extra: Vec<&str> = headers
        .iter()
        .filter(|header| !REQUIRED_ASSET_LEDGER_COLUMNS.contains(header))
        .collect();

    let mut errors = Vec::new();
    if !missing.is_empty() {
        errors.push(format!("missing columns: {}", missing.join(", ")));
    }
    if !extra.is_empty() {
        errors.push(format!("unexpected columns: {}", extra.join(", ")));
    }
    if !errors.is_empty() {
        return Ok(errors);
    }

    let mut seen_ids = HashSet::new();
    let mut row_count = 0_usize;
    for (index, record) in reader.records().enumerate() {
        let record = record.map_err(|error| format!("{}: {error}", resolved.display()))?;
        let row_number = index + 2;
        if !record.iter().any(|value| !value.trim().is_empty()) {
            continue;
        }
        row_count += 1;
        let row: BTreeMap<String, String> = headers
            .iter()
            .zip(record.iter())
            .map(|(header, value)| (header.to_string(), value.to_string()))
            .collect();
        errors.extend(validate_asset_ledger_row(&row, row_number, &mut seen_ids));
    }

    if row_count == 0 {
        errors.push(format!(
            "{}: no asset rows",
            repo_relative_display(&resolved)
        ));
    }
    Ok(errors)
}

fn validate_asset_ledger_row(
    row: &BTreeMap<String, String>,
    row_number: usize,
    seen_ids: &mut HashSet<String>,
) -> Vec<String> {
    let mut errors = Vec::new();
    let prefix = format!("row {row_number}");
    let asset_id_pattern = Regex::new(r"^[a-z0-9][a-z0-9_.-]*$").expect("valid asset id regex");
    let date_or_pending = Regex::new(r"^\d{4}-\d{2}-\d{2}$|^pending$").expect("valid date regex");

    let asset_id = ledger_value(row, "asset_id").trim();
    if !asset_id_pattern.is_match(asset_id) {
        errors.push(format!(
            "{prefix}: asset_id must be lowercase slug-like text"
        ));
    }
    if !seen_ids.insert(asset_id.to_string()) {
        errors.push(format!("{prefix}: duplicate asset_id {asset_id}"));
    }

    let adoption_state = ledger_value(row, "adoption_state")
        .trim()
        .to_ascii_lowercase();
    if !matches!(
        adoption_state.as_str(),
        "prototype" | "candidate" | "approved" | "rejected" | "replaced"
    ) {
        errors.push(format!(
            "{prefix}: invalid adoption_state {adoption_state:?}"
        ));
    }

    let rights_risk = ledger_value(row, "rights_risk").trim().to_ascii_lowercase();
    if !matches!(rights_risk.as_str(), "low" | "medium" | "high") {
        errors.push(format!("{prefix}: invalid rights_risk {rights_risk:?}"));
    }

    for field in [
        "commercial_use_allowed",
        "credit_required",
        "redistribution_allowed",
    ] {
        let value = ledger_value(row, field).trim().to_ascii_lowercase();
        if !matches!(value.as_str(), "yes" | "no" | "unknown") {
            errors.push(format!("{prefix}: {field} must be yes/no/unknown"));
        }
    }

    let license_snapshot_date = ledger_value(row, "license_snapshot_date")
        .trim()
        .to_ascii_lowercase();
    if !date_or_pending.is_match(&license_snapshot_date) {
        errors.push(format!(
            "{prefix}: license_snapshot_date must be YYYY-MM-DD or pending"
        ));
    }

    if empty_or_pending(ledger_value(row, "asset_name")) {
        errors.push(format!("{prefix}: asset_name is required"));
    }
    if empty_or_pending(ledger_value(row, "asset_type")) {
        errors.push(format!("{prefix}: asset_type is required"));
    }

    let fab_url = ledger_value(row, "fab_listing_url").trim();
    let tool_or_model = ledger_value(row, "tool_or_model").to_ascii_lowercase();
    if tool_or_model.contains("fab") || !fab_url.is_empty() {
        if !fab_url.starts_with("https://www.fab.com/listings/") {
            errors.push(format!(
                "{prefix}: Fab assets require a fab_listing_url under https://www.fab.com/listings/"
            ));
        }
        if empty_or_pending(ledger_value(row, "publisher_or_seller")) {
            errors.push(format!("{prefix}: Fab assets require publisher_or_seller"));
        }
    }

    let output_path = ledger_value(row, "output_path").trim();
    if adoption_state != "approved" && output_path.starts_with("Content/Frostwake/") {
        errors.push(format!(
            "{prefix}: non-approved assets may not point at Content/Frostwake/"
        ));
    }

    if adoption_state == "approved" {
        if ledger_value(row, "commercial_use_allowed")
            .trim()
            .to_ascii_lowercase()
            != "yes"
        {
            errors.push(format!(
                "{prefix}: approved assets require commercial_use_allowed=yes"
            ));
        }
        for field in [
            "license",
            "license_snapshot_date",
            "proof_of_purchase_path",
            "reviewer_approval",
            "final_reviewer",
            "output_path",
        ] {
            if empty_or_pending(ledger_value(row, field)) {
                errors.push(format!("{prefix}: approved assets require {field}"));
            }
        }
    }

    errors
}

fn ledger_value<'a>(row: &'a BTreeMap<String, String>, field: &str) -> &'a str {
    row.get(field).map(String::as_str).unwrap_or("")
}

#[derive(Debug)]
struct ReferenceInventoryOptions {
    manifest: PathBuf,
    out_dir: PathBuf,
    max_depth: usize,
    max_files_per_source: usize,
    hash_files: bool,
}

#[derive(Debug)]
struct ReferenceInventoryOutput {
    inventory_path: PathBuf,
    summary_path: PathBuf,
}

#[derive(Clone, Debug)]
struct InventoryRow {
    source: String,
    path: String,
    kind: String,
    size_bytes: Option<u64>,
    mtime_epoch: Option<u64>,
    sha256: String,
}

fn env_usize(name: &str, default_value: usize) -> usize {
    std::env::var(name)
        .ok()
        .and_then(|value| value.parse::<usize>().ok())
        .unwrap_or(default_value)
}

fn write_reference_inventory(
    options: &ReferenceInventoryOptions,
) -> Result<ReferenceInventoryOutput, String> {
    let sources = load_reference_sources(&options.manifest)?;
    fs::create_dir_all(&options.out_dir)
        .map_err(|error| format!("{}: {error}", options.out_dir.display()))?;

    let timestamp = Utc::now().format("%Y%m%d_%H%M%S");
    let inventory_path = options.out_dir.join(format!("inventory_{timestamp}.tsv"));
    let summary_path = options
        .out_dir
        .join(format!("inventory_{timestamp}_summary.tsv"));

    let mut rows = Vec::new();
    for source in sources {
        rows.extend(inventory_rows_for_source(&source, options)?);
    }

    fs::write(&inventory_path, render_inventory_tsv(&rows))
        .map_err(|error| format!("{}: {error}", inventory_path.display()))?;
    fs::write(&summary_path, render_inventory_summary_tsv(&rows))
        .map_err(|error| format!("{}: {error}", summary_path.display()))?;

    Ok(ReferenceInventoryOutput {
        inventory_path,
        summary_path,
    })
}

fn load_reference_sources(manifest: &Path) -> Result<Vec<String>, String> {
    let value = fs::read_to_string(manifest)
        .map_err(|error| format!("{}: {error}", manifest.display()))
        .and_then(|text| {
            serde_json::from_str::<Value>(&text)
                .map_err(|error| format!("{}: invalid JSON: {error}", manifest.display()))
        })?;
    let paths = value
        .get("paths")
        .and_then(Value::as_array)
        .ok_or_else(|| format!("{}: expected paths array", manifest.display()))?;
    let mut sources = Vec::new();
    for (index, entry) in paths.iter().enumerate() {
        let path = entry.get("path").and_then(Value::as_str).ok_or_else(|| {
            format!(
                "{}: paths[{index}].path must be a string",
                manifest.display()
            )
        })?;
        sources.push(path.to_string());
    }
    Ok(sources)
}

fn inventory_rows_for_source(
    source: &str,
    options: &ReferenceInventoryOptions,
) -> Result<Vec<InventoryRow>, String> {
    let source_path = PathBuf::from(source);
    if !source_path.exists() {
        return Ok(vec![InventoryRow {
            source: source.to_string(),
            path: source.to_string(),
            kind: "missing".to_string(),
            size_bytes: None,
            mtime_epoch: None,
            sha256: String::new(),
        }]);
    }

    let mut rows = Vec::new();
    let mut count = 0_usize;
    let mut truncated = false;
    collect_inventory_rows(
        source,
        &source_path,
        0,
        options,
        &mut count,
        &mut truncated,
        &mut rows,
    )?;
    Ok(rows)
}

fn collect_inventory_rows(
    source: &str,
    path: &Path,
    depth: usize,
    options: &ReferenceInventoryOptions,
    count: &mut usize,
    truncated: &mut bool,
    rows: &mut Vec<InventoryRow>,
) -> Result<(), String> {
    if *truncated {
        return Ok(());
    }
    if path.is_file() {
        push_inventory_file_row(source, path, options, count, truncated, rows)?;
        return Ok(());
    }
    if depth >= options.max_depth {
        return Ok(());
    }

    let entries = fs::read_dir(path).map_err(|error| format!("{}: {error}", path.display()))?;
    for entry in entries {
        let entry = entry.map_err(|error| format!("{}: {error}", path.display()))?;
        let entry_path = entry.path();
        let file_type = entry
            .file_type()
            .map_err(|error| format!("{}: {error}", entry_path.display()))?;
        if file_type.is_dir() {
            if is_reference_inventory_excluded_dir(&entry_path) {
                continue;
            }
            collect_inventory_rows(
                source,
                &entry_path,
                depth + 1,
                options,
                count,
                truncated,
                rows,
            )?;
        } else if file_type.is_file() {
            push_inventory_file_row(source, &entry_path, options, count, truncated, rows)?;
        }
        if *truncated {
            break;
        }
    }
    Ok(())
}

fn push_inventory_file_row(
    source: &str,
    path: &Path,
    options: &ReferenceInventoryOptions,
    count: &mut usize,
    truncated: &mut bool,
    rows: &mut Vec<InventoryRow>,
) -> Result<(), String> {
    *count += 1;
    if *count > options.max_files_per_source {
        rows.push(InventoryRow {
            source: source.to_string(),
            path: "__truncated__".to_string(),
            kind: "truncated".to_string(),
            size_bytes: None,
            mtime_epoch: None,
            sha256: format!("max_files_{}", options.max_files_per_source),
        });
        *truncated = true;
        return Ok(());
    }

    let metadata = fs::metadata(path).map_err(|error| format!("{}: {error}", path.display()))?;
    let size_bytes = metadata.len();
    let mtime_epoch = metadata
        .modified()
        .ok()
        .and_then(|time| time.duration_since(UNIX_EPOCH).ok())
        .map(|duration| duration.as_secs());
    let sha256 = if options.hash_files && size_bytes <= 52_428_800 {
        let bytes = fs::read(path).map_err(|error| format!("{}: {error}", path.display()))?;
        format!("{:x}", Sha256::digest(bytes))
    } else if options.hash_files {
        "skipped_large_file".to_string()
    } else {
        "skipped".to_string()
    };

    rows.push(InventoryRow {
        source: source.to_string(),
        path: path.display().to_string(),
        kind: "file".to_string(),
        size_bytes: Some(size_bytes),
        mtime_epoch,
        sha256,
    });
    Ok(())
}

fn is_reference_inventory_excluded_dir(path: &Path) -> bool {
    let Some(name) = path.file_name().and_then(|value| value.to_str()) else {
        return false;
    };
    matches!(
        name,
        ".git" | ".venv" | "node_modules" | ".mypy_cache" | "__pycache__" | "Saved" | "Logs"
    )
}

fn render_inventory_tsv(rows: &[InventoryRow]) -> String {
    let mut output = String::from("source\tpath\ttype\tsize_bytes\tmtime_epoch\tsha256\n");
    for row in rows {
        output.push_str(&format!(
            "{}\t{}\t{}\t{}\t{}\t{}\n",
            tsv_cell(&row.source),
            tsv_cell(&row.path),
            tsv_cell(&row.kind),
            row.size_bytes
                .map(|value| value.to_string())
                .unwrap_or_default(),
            row.mtime_epoch
                .map(|value| value.to_string())
                .unwrap_or_default(),
            tsv_cell(&row.sha256),
        ));
    }
    output
}

fn render_inventory_summary_tsv(rows: &[InventoryRow]) -> String {
    let mut summary: BTreeMap<(String, String), (usize, u64)> = BTreeMap::new();
    for row in rows {
        let key = (row.source.clone(), inventory_extension(&row.path));
        let entry = summary.entry(key).or_insert((0, 0));
        entry.0 += 1;
        entry.1 += row.size_bytes.unwrap_or(0);
    }

    let mut output = String::from("source\textension\tfile_count\ttotal_size_bytes\n");
    for ((source, extension), (file_count, total_size_bytes)) in summary {
        output.push_str(&format!(
            "{}\t{}\t{}\t{}\n",
            tsv_cell(&source),
            tsv_cell(&extension),
            file_count,
            total_size_bytes
        ));
    }
    output
}

fn inventory_extension(path: &str) -> String {
    let name = Path::new(path)
        .file_name()
        .and_then(|value| value.to_str())
        .unwrap_or(path);
    match name.rsplit_once('.') {
        Some((_, extension)) if !extension.is_empty() => format!(".{extension}"),
        _ => "no_ext".to_string(),
    }
}

fn tsv_cell(value: &str) -> String {
    value.replace(['\t', '\r', '\n'], " ")
}

fn normalized_redact_fields(fields: &[String]) -> Vec<String> {
    if fields.is_empty() {
        return DEFAULT_REDACT_FIELDS
            .iter()
            .map(|field| field.to_string())
            .collect();
    }
    fields
        .iter()
        .map(|field| field.to_ascii_lowercase())
        .collect()
}

fn redact_json_value(value: &Value, fields: &[String]) -> Value {
    match value {
        Value::Object(map) => {
            let mut redacted = Map::new();
            for (key, item) in map {
                let normalized_key = key.to_ascii_lowercase();
                if fields.iter().any(|field| normalized_key.contains(field)) {
                    redacted.insert(key.clone(), json!("[REDACTED]"));
                } else {
                    redacted.insert(key.clone(), redact_json_value(item, fields));
                }
            }
            Value::Object(redacted)
        }
        Value::Array(items) => Value::Array(
            items
                .iter()
                .map(|item| redact_json_value(item, fields))
                .collect(),
        ),
        _ => value.clone(),
    }
}

fn load_banlist_entries(path: &Path) -> Result<Vec<Value>, String> {
    let resolved = resolve_path(path);
    if !resolved.exists() {
        return Ok(Vec::new());
    }
    let value = load_json_value(&resolved)?;
    Ok(value
        .get("bans")
        .and_then(Value::as_array)
        .cloned()
        .unwrap_or_default())
}

fn save_banlist_entries(path: &Path, bans: &[Value]) -> Result<(), String> {
    let resolved = resolve_path(path);
    if let Some(parent) = resolved.parent()
        && !parent.as_os_str().is_empty()
    {
        fs::create_dir_all(parent).map_err(|error| format!("{}: {error}", parent.display()))?;
    }
    let payload = json!({ "bans": bans });
    fs::write(
        &resolved,
        format!(
            "{}\n",
            serde_json::to_string_pretty(&payload).expect("banlist serializes")
        ),
    )
    .map_err(|error| format!("{}: {error}", resolved.display()))
}

fn add_banlist_entry(
    path: &Path,
    platform_user_id: &str,
    scope: &str,
    reason: &str,
    created_at: &str,
) -> Result<Vec<Value>, String> {
    let mut bans = load_banlist_entries(path)?;
    bans.push(json!({
        "platform_user_id": platform_user_id,
        "scope": scope,
        "reason": reason,
        "created_at": created_at,
    }));
    save_banlist_entries(path, &bans)?;
    Ok(bans)
}

fn steam_registry_list_servers() -> Vec<Value> {
    Vec::new()
}

fn summarize_server_config(config: &Map<String, Value>) -> BTreeMap<String, Value> {
    BTreeMap::from([
        (
            "serverName".to_string(),
            value_or_null(config, "serverName"),
        ),
        ("region".to_string(), value_or_null(config, "region")),
        (
            "maxPlayers".to_string(),
            value_or_null(config, "maxPlayers"),
        ),
        ("map".to_string(), value_or_null(config, "map")),
        ("ruleset".to_string(), value_or_null(config, "ruleset")),
        ("advertise".to_string(), value_or_null(config, "advertise")),
        (
            "autoShutdownAfterMatch".to_string(),
            value_or_null(config, "autoShutdownAfterMatch"),
        ),
        (
            "hasAdminTokenEnv".to_string(),
            json!(config.get("adminTokenEnv").is_some_and(value_is_truthy)),
        ),
        (
            "banlistPath".to_string(),
            value_or_null(config, "banlistPath"),
        ),
        ("logPath".to_string(), value_or_null(config, "logPath")),
    ])
}

fn summarize_lobby_metadata(metadata: &Map<String, Value>) -> BTreeMap<String, Value> {
    BTreeMap::from([
        (
            "schemaVersion".to_string(),
            value_or_null(metadata, "schemaVersion"),
        ),
        (
            "lobbyName".to_string(),
            value_or_null(metadata, "lobbyName"),
        ),
        (
            "lobbyType".to_string(),
            value_or_null(metadata, "lobbyType"),
        ),
        ("buildId".to_string(), value_or_null(metadata, "buildId")),
        ("mapId".to_string(), value_or_null(metadata, "mapId")),
        ("ruleset".to_string(), value_or_null(metadata, "ruleset")),
        ("mode".to_string(), value_or_null(metadata, "mode")),
        (
            "difficulty".to_string(),
            value_or_null(metadata, "difficulty"),
        ),
        (
            "players".to_string(),
            json!(format!(
                "{}/{}",
                display_value(metadata.get("currentPlayers")),
                display_value(metadata.get("maxPlayers"))
            )),
        ),
        (
            "minimumCompletedMatches".to_string(),
            value_or_null(metadata, "minimumCompletedMatches"),
        ),
        (
            "joinState".to_string(),
            value_or_null(metadata, "joinState"),
        ),
        (
            "connectionMode".to_string(),
            value_or_null(metadata, "connectionMode"),
        ),
        ("official".to_string(), value_or_null(metadata, "official")),
        (
            "passworded".to_string(),
            value_or_null(metadata, "passworded"),
        ),
        ("region".to_string(), value_or_null(metadata, "region")),
        (
            "serverName".to_string(),
            value_or_null(metadata, "serverName"),
        ),
    ])
}

fn validate_lobby_metadata(
    schema: &Map<String, Value>,
    metadata: &Map<String, Value>,
    expected_build_id: Option<&str>,
    expected_map_id: Option<&str>,
) -> Vec<String> {
    let mut errors = validate_schema_value(
        &Value::Object(schema.clone()),
        &Value::Object(metadata.clone()),
        "$",
    );
    if !errors.is_empty() {
        return errors;
    }

    let max_players = read_i64(metadata, "maxPlayers");
    let current_players = read_i64(metadata, "currentPlayers");
    let lobby_type = read_str(metadata, "lobbyType");
    let minimum_completed_matches = read_i64(metadata, "minimumCompletedMatches");
    let join_state = read_str(metadata, "joinState");
    let endpoint_token = read_str(metadata, "endpointToken");
    let raw_ip_endpoint_pattern =
        Regex::new(r"\b\d{1,3}(?:\.\d{1,3}){3}:\d{2,5}\b").expect("valid raw endpoint regex");

    if max_players != FIXED_MATCH_PLAYERS {
        errors.push(format!(
            "$.maxPlayers: must be exactly {FIXED_MATCH_PLAYERS}"
        ));
    }
    if current_players > max_players {
        errors.push("$.currentPlayers: cannot exceed maxPlayers".to_string());
    }
    if lobby_type == "casual" && minimum_completed_matches != 0 {
        errors.push(
            "$.minimumCompletedMatches: casual lobbies must not require completed matches"
                .to_string(),
        );
    }
    if lobby_type == "standard" && minimum_completed_matches < STANDARD_MINIMUM_COMPLETED_MATCHES {
        errors.push(format!(
            "$.minimumCompletedMatches: standard lobbies require at least {STANDARD_MINIMUM_COMPLETED_MATCHES} completed matches"
        ));
    }
    if join_state == "open" && current_players >= max_players {
        errors.push("$.joinState: open lobby cannot already be full".to_string());
    }
    if join_state == "full" && current_players < max_players {
        errors.push("$.joinState: full lobby must have currentPlayers >= maxPlayers".to_string());
    }
    if raw_ip_endpoint_pattern.is_match(endpoint_token) {
        errors.push("$.endpointToken: must be opaque, not a raw IP:port endpoint".to_string());
    }
    if let Some(expected) = expected_build_id
        && read_str(metadata, "buildId") != expected
    {
        errors.push(format!(
            "$.buildId: mismatch expected {expected:?} got {:?}",
            read_str(metadata, "buildId")
        ));
    }
    if let Some(expected) = expected_map_id
        && read_str(metadata, "mapId") != expected
    {
        errors.push(format!(
            "$.mapId: mismatch expected {expected:?} got {:?}",
            read_str(metadata, "mapId")
        ));
    }

    errors
}

fn decide_lobby_join(
    schema: &Map<String, Value>,
    metadata: &Map<String, Value>,
    expected_build_id: Option<&str>,
    expected_map_id: Option<&str>,
) -> BTreeMap<String, Value> {
    let metadata_errors = validate_lobby_metadata(schema, metadata, None, None);
    if !metadata_errors.is_empty() {
        return lobby_join_decision_payload(
            metadata,
            expected_build_id,
            expected_map_id,
            false,
            "InvalidMetadata",
            "Lobby metadata is invalid; client travel is blocked.",
            Some(metadata_errors),
        );
    }

    if let Some(expected) = expected_build_id
        && read_str(metadata, "buildId") != expected
    {
        return lobby_join_decision_payload(
            metadata,
            expected_build_id,
            expected_map_id,
            false,
            "BuildMismatch",
            "Lobby build id does not match the local client build; client travel is blocked.",
            None,
        );
    }

    if let Some(expected) = expected_map_id
        && read_str(metadata, "mapId") != expected
    {
        return lobby_join_decision_payload(
            metadata,
            expected_build_id,
            expected_map_id,
            false,
            "MapMismatch",
            "Lobby map id does not match the local client map; client travel is blocked.",
            None,
        );
    }

    match read_str(metadata, "joinState") {
        "full" => lobby_join_decision_payload(
            metadata,
            expected_build_id,
            expected_map_id,
            false,
            "LobbyFull",
            "Lobby is full; client travel is blocked.",
            None,
        ),
        "locked" => lobby_join_decision_payload(
            metadata,
            expected_build_id,
            expected_map_id,
            false,
            "LobbyLocked",
            "Lobby is locked; client travel is blocked.",
            None,
        ),
        "in_match" => lobby_join_decision_payload(
            metadata,
            expected_build_id,
            expected_map_id,
            false,
            "AlreadyInMatch",
            "Lobby is already in match; client travel is blocked.",
            None,
        ),
        _ if read_str(metadata, "endpointToken").trim().is_empty() => lobby_join_decision_payload(
            metadata,
            expected_build_id,
            expected_map_id,
            false,
            "EndpointUnavailable",
            "Lobby does not expose an endpoint token; client travel is blocked.",
            None,
        ),
        _ => lobby_join_decision_payload(
            metadata,
            expected_build_id,
            expected_map_id,
            true,
            "None",
            "Lobby metadata is compatible; client travel may continue through the active provider.",
            None,
        ),
    }
}

fn lobby_join_decision_payload(
    metadata: &Map<String, Value>,
    expected_build_id: Option<&str>,
    expected_map_id: Option<&str>,
    accepted: bool,
    reject_reason: &str,
    message: &str,
    validation_errors: Option<Vec<String>>,
) -> BTreeMap<String, Value> {
    let mut decision = BTreeMap::from([
        ("accepted".to_string(), json!(accepted)),
        ("travelAllowed".to_string(), json!(accepted)),
        ("rejectReason".to_string(), json!(reject_reason)),
        ("message".to_string(), json!(message)),
        (
            "schemaVersion".to_string(),
            value_or_null(metadata, "schemaVersion"),
        ),
        ("buildId".to_string(), value_or_null(metadata, "buildId")),
        (
            "expectedBuildId".to_string(),
            expected_build_id.map_or(Value::Null, |value| json!(value)),
        ),
        ("mapId".to_string(), value_or_null(metadata, "mapId")),
        (
            "expectedMapId".to_string(),
            expected_map_id.map_or(Value::Null, |value| json!(value)),
        ),
        (
            "joinState".to_string(),
            value_or_null(metadata, "joinState"),
        ),
        (
            "connectionMode".to_string(),
            value_or_null(metadata, "connectionMode"),
        ),
        (
            "players".to_string(),
            json!(format!(
                "{}/{}",
                display_value(metadata.get("currentPlayers")),
                display_value(metadata.get("maxPlayers"))
            )),
        ),
        (
            "endpointAvailable".to_string(),
            json!(!read_str(metadata, "endpointToken").trim().is_empty()),
        ),
    ]);

    if let Some(errors) = validation_errors {
        decision.insert("validationErrors".to_string(), json!(errors));
    }

    decision
}

fn check_lobby_join_decision_expectations(
    decision: &BTreeMap<String, Value>,
    require_accepted: bool,
    expected_reject_reason: Option<&str>,
) -> Vec<String> {
    let mut errors = Vec::new();
    let accepted = decision
        .get("accepted")
        .and_then(Value::as_bool)
        .unwrap_or(false);
    let actual_reject_reason = decision
        .get("rejectReason")
        .and_then(Value::as_str)
        .unwrap_or("InvalidMetadata");

    if require_accepted && !accepted {
        errors.push(format!(
            "lobby join decision expected accepted=true, got rejectReason={actual_reject_reason}"
        ));
    }

    if let Some(expected) = expected_reject_reason
        && actual_reject_reason != expected
    {
        errors.push(format!(
            "lobby join decision expected rejectReason={expected}, got {actual_reject_reason}"
        ));
    }

    errors
}

fn print_lobby_join_decision(decision: &BTreeMap<String, Value>) {
    let accepted = decision
        .get("accepted")
        .and_then(Value::as_bool)
        .unwrap_or(false);
    let prefix = if accepted {
        "[JOIN-PASS]"
    } else {
        "[JOIN-REJECT]"
    };

    println!(
        "{prefix} lobby_join reason={} build={} expectedBuild={} map={} expectedMap={} players={} joinState={} mode={}",
        display_value(decision.get("rejectReason")),
        display_value(decision.get("buildId")),
        display_value(decision.get("expectedBuildId")),
        display_value(decision.get("mapId")),
        display_value(decision.get("expectedMapId")),
        display_value(decision.get("players")),
        display_value(decision.get("joinState")),
        display_value(decision.get("connectionMode")),
    );
}

fn validate_schema_value(schema: &Value, value: &Value, path: &str) -> Vec<String> {
    let Some(schema_object) = schema.as_object() else {
        return Vec::new();
    };

    match schema_object.get("type").and_then(Value::as_str) {
        Some("object") => validate_object_schema(schema_object, value, path),
        Some("array") => validate_array_schema(schema_object, value, path),
        Some("string") => validate_string_schema(schema_object, value, path),
        Some("integer") => validate_integer_schema(schema_object, value, path),
        Some("boolean") => validate_boolean_schema(value, path),
        _ => Vec::new(),
    }
}

fn validate_object_schema(schema: &Map<String, Value>, value: &Value, path: &str) -> Vec<String> {
    let Some(object) = value.as_object() else {
        return vec![format!("{path}: expected object")];
    };

    let mut errors = Vec::new();
    if let Some(required) = schema.get("required").and_then(Value::as_array) {
        for field in required.iter().filter_map(Value::as_str) {
            if !object.contains_key(field) {
                errors.push(format!("{path}.{field}: required field missing"));
            }
        }
    }

    let properties = schema
        .get("properties")
        .and_then(Value::as_object)
        .cloned()
        .unwrap_or_default();
    if schema.get("additionalProperties") == Some(&Value::Bool(false)) {
        let allowed: HashSet<_> = properties.keys().map(String::as_str).collect();
        for field in object.keys() {
            if !allowed.contains(field.as_str()) {
                errors.push(format!("{path}.{field}: additional property not allowed"));
            }
        }
    }

    for (field, field_schema) in &properties {
        if let Some(field_value) = object.get(field) {
            errors.extend(validate_schema_value(
                field_schema,
                field_value,
                &format!("{path}.{field}"),
            ));
        }
    }

    errors
}

fn validate_array_schema(schema: &Map<String, Value>, value: &Value, path: &str) -> Vec<String> {
    let Some(items) = value.as_array() else {
        return vec![format!("{path}: expected array")];
    };

    let item_schema = schema.get("items").unwrap_or(&Value::Null);
    let mut errors = Vec::new();
    for (index, item) in items.iter().enumerate() {
        errors.extend(validate_schema_value(
            item_schema,
            item,
            &format!("{path}[{index}]"),
        ));
    }
    errors
}

fn validate_string_schema(schema: &Map<String, Value>, value: &Value, path: &str) -> Vec<String> {
    let Some(text) = value.as_str() else {
        return vec![format!("{path}: expected string")];
    };

    let mut errors = Vec::new();
    if let Some(min_length) = schema.get("minLength").and_then(Value::as_u64)
        && text.chars().count() < min_length as usize
    {
        errors.push(format!("{path}: shorter than minLength {min_length}"));
    }
    if let Some(max_length) = schema.get("maxLength").and_then(Value::as_u64)
        && text.chars().count() > max_length as usize
    {
        errors.push(format!("{path}: longer than maxLength {max_length}"));
    }
    if let Some(allowed) = schema.get("enum").and_then(Value::as_array) {
        let matches = allowed.iter().any(|allowed_value| allowed_value == value);
        if !matches {
            errors.push(format!("{path}: value {text:?} not in enum"));
        }
    }
    errors
}

fn validate_integer_schema(schema: &Map<String, Value>, value: &Value, path: &str) -> Vec<String> {
    let Some(number) = value.as_i64() else {
        return vec![format!("{path}: expected integer")];
    };

    let mut errors = Vec::new();
    if let Some(minimum) = schema.get("minimum").and_then(Value::as_i64)
        && number < minimum
    {
        errors.push(format!("{path}: below minimum {minimum}"));
    }
    if let Some(maximum) = schema.get("maximum").and_then(Value::as_i64)
        && number > maximum
    {
        errors.push(format!("{path}: above maximum {maximum}"));
    }
    errors
}

fn validate_boolean_schema(value: &Value, path: &str) -> Vec<String> {
    if value.is_boolean() {
        Vec::new()
    } else {
        vec![format!("{path}: expected boolean")]
    }
}

fn value_or_null(source: &Map<String, Value>, key: &str) -> Value {
    source.get(key).cloned().unwrap_or(Value::Null)
}

fn value_is_truthy(value: &Value) -> bool {
    match value {
        Value::Null => false,
        Value::Bool(value) => *value,
        Value::Number(number) => number.as_i64().is_some_and(|value| value != 0),
        Value::String(text) => !text.is_empty(),
        Value::Array(items) => !items.is_empty(),
        Value::Object(items) => !items.is_empty(),
    }
}

fn display_value(value: Option<&Value>) -> String {
    match value {
        Some(Value::String(text)) => text.clone(),
        Some(Value::Null) | None => "null".to_string(),
        Some(other) => other.to_string(),
    }
}

fn read_i64(source: &Map<String, Value>, key: &str) -> i64 {
    source.get(key).and_then(Value::as_i64).unwrap_or_default()
}

fn read_str<'a>(source: &'a Map<String, Value>, key: &str) -> &'a str {
    source.get(key).and_then(Value::as_str).unwrap_or_default()
}

fn summarize_events(path: &Path) -> Result<BTreeMap<String, Value>, String> {
    let resolved = resolve_path(path);
    let text = fs::read_to_string(&resolved)
        .map_err(|error| format!("{}: {error}", resolved.display()))?;
    let mut counts: BTreeMap<String, i64> = BTreeMap::new();
    let mut invalid_lines = 0_i64;
    let mut first_timestamp: Option<String> = None;
    let mut last_timestamp: Option<String> = None;
    let mut first_timestamp_value: Option<DateTime<Utc>> = None;
    let mut last_timestamp_value: Option<DateTime<Utc>> = None;
    let mut first_elapsed_seconds: Option<f64> = None;
    let mut last_elapsed_seconds: Option<f64> = None;
    let mut match_started_elapsed_seconds: Option<f64> = None;
    let mut match_ended_elapsed_seconds: Option<f64> = None;
    let mut match_started_timestamp: Option<DateTime<Utc>> = None;
    let mut match_ended_timestamp: Option<DateTime<Utc>> = None;
    let mut session_metadata = Value::Null;
    let mut run_id: Option<String> = None;
    let mut build_id: Option<String> = None;
    let mut map_id: Option<String> = None;
    let mut profile: Option<String> = None;
    let mut task_repairs = 0_i64;
    let mut task_sabotages = 0_i64;
    let mut door_locks = 0_i64;
    let mut door_unlocks = 0_i64;
    let mut door_toggles = 0_i64;
    let mut bulkhead_lock_sabotages = 0_i64;
    let mut bulkhead_locked_interaction_blocks = 0_i64;
    let mut bulkhead_lock_releases = 0_i64;
    let mut bulkhead_lock_smoke_passed = 0_i64;
    let mut last_bulkhead_lock_smoke = Value::Null;
    let mut flooding_pressure_changes = 0_i64;
    let mut flooding_pressure_sabotages = 0_i64;
    let mut flooding_pressure_repairs = 0_i64;
    let mut last_flooding_pressure: Option<f64> = None;
    let mut pump_flooding_smoke_passed = 0_i64;
    let mut last_pump_flooding_smoke = Value::Null;
    let mut pve_enemy_spawns = 0_i64;
    let mut pve_enemy_attacks = 0_i64;
    let mut pve_enemy_smoke_passed = 0_i64;
    let mut last_pve_enemy_smoke = Value::Null;
    let mut players_damaged = 0_i64;
    let mut pve_spawned_events = 0_i64;
    let mut pve_damage_applied = 0_i64;
    let mut pve_damage_smoke_passed = 0_i64;
    let mut last_pve_damage_smoke = Value::Null;
    let mut item_pickups = 0_i64;
    let mut item_drops = 0_i64;
    let mut item_drop_smoke_passed = 0_i64;
    let mut last_item_drop_smoke = Value::Null;
    let mut combined_systems_smoke_passed = 0_i64;
    let mut last_combined_systems_smoke = Value::Null;
    let mut qa_bot_spawns = 0_i64;
    let mut qa_bot_moves = 0_i64;
    let mut qa_bot_interactions = 0_i64;
    let mut qa_bot_smoke_passed = 0_i64;
    let mut last_qa_bot_smoke = Value::Null;
    let mut qa_player_bot_moves = 0_i64;
    let mut qa_player_bot_interactions = 0_i64;
    let mut qa_player_bot_smoke_passed = 0_i64;
    let mut last_qa_player_bot_smoke = Value::Null;
    let mut qa_task_bot_moves = 0_i64;
    let mut qa_task_bot_interactions = 0_i64;
    let mut qa_task_bot_smoke_passed = 0_i64;
    let mut last_qa_task_bot_smoke = Value::Null;
    let mut life_action_interactions = 0_i64;
    let mut life_action_smoke_passed = 0_i64;
    let mut last_life_action_smoke = Value::Null;
    let mut last_role_assignment = Value::Null;
    let mut last_match_result = Value::Null;

    for line in text.lines() {
        let stripped = line.trim();
        if stripped.is_empty() {
            continue;
        }

        let Ok(event) = serde_json::from_str::<Value>(stripped) else {
            invalid_lines += 1;
            continue;
        };
        let Some(event_object) = event.as_object() else {
            invalid_lines += 1;
            continue;
        };

        let name = event_name(event_object);
        *counts.entry(name.clone()).or_default() += 1;
        set_once(&mut run_id, value_to_string(event_object.get("run_id")));
        set_once(&mut build_id, value_to_string(event_object.get("build_id")));
        set_once(&mut map_id, value_to_string(event_object.get("map_id")));
        set_once(&mut profile, value_to_string(event_object.get("profile")));

        let elapsed_seconds = event_object.get("elapsed_seconds").and_then(as_float);
        if let Some(elapsed_seconds) = elapsed_seconds {
            if first_elapsed_seconds.is_none() {
                first_elapsed_seconds = Some(elapsed_seconds);
            }
            last_elapsed_seconds = Some(elapsed_seconds);
        }

        let payload = normalize_payload(event_object.get("payload"));
        let payload_object = payload.as_object();

        match name.as_str() {
            "session_started" => {
                if let Some(payload_object) = payload_object {
                    session_metadata = Value::Object(payload_object.clone());
                    set_once(&mut run_id, value_to_string(payload_object.get("runId")));
                    set_once(
                        &mut build_id,
                        value_to_string(payload_object.get("buildId")),
                    );
                    set_once(&mut map_id, value_to_string(payload_object.get("mapId")));
                    set_once(&mut profile, value_to_string(payload_object.get("profile")));
                }
            }
            "match_started" => {
                match_started_elapsed_seconds = elapsed_seconds;
                match_started_timestamp = event_timestamp(event_object);
            }
            "match_ended" => {
                if payload_object.is_some() {
                    last_match_result = payload.clone();
                }
                match_ended_elapsed_seconds = elapsed_seconds;
                match_ended_timestamp = event_timestamp(event_object);
            }
            "ship_task_applied" => {
                if let Some(payload_object) = payload_object {
                    match payload_object.get("mode").and_then(Value::as_i64) {
                        Some(0) => task_repairs += 1,
                        Some(1) => task_sabotages += 1,
                        _ => {}
                    }
                }
            }
            "role_assignment_complete" => {
                if payload_object.is_some() {
                    last_role_assignment = payload.clone();
                }
            }
            "door_lock_changed" => {
                if let Some(payload_object) = payload_object {
                    match payload_object.get("locked").and_then(Value::as_bool) {
                        Some(true) => door_locks += 1,
                        Some(false) => door_unlocks += 1,
                        None => {}
                    }
                }
            }
            "door_toggled" => door_toggles += 1,
            "bulkhead_lock_sabotaged" => bulkhead_lock_sabotages += 1,
            "bulkhead_locked_interaction_blocked" => bulkhead_locked_interaction_blocks += 1,
            "bulkhead_lock_released" => bulkhead_lock_releases += 1,
            "dev_smoke_bulkhead_lock" => {
                if payload_object.is_some() {
                    if payload.get("result").and_then(Value::as_str) == Some("pass") {
                        bulkhead_lock_smoke_passed += 1;
                    }
                    last_bulkhead_lock_smoke = payload.clone();
                }
            }
            "flooding_pressure_changed" => {
                if let Some(payload_object) = payload_object {
                    flooding_pressure_changes += 1;
                    let delta_pressure = payload_object
                        .get("deltaPressure")
                        .or_else(|| payload_object.get("delta"))
                        .and_then(as_float);
                    let mode = payload_object.get("mode").and_then(Value::as_i64);
                    if delta_pressure.is_some_and(|value| value < 0.0) {
                        flooding_pressure_repairs += 1;
                    } else if delta_pressure.is_some_and(|value| value > 0.0) {
                        flooding_pressure_sabotages += 1;
                    } else if mode == Some(0) {
                        flooding_pressure_repairs += 1;
                    } else if mode == Some(1) {
                        flooding_pressure_sabotages += 1;
                    }
                    if let Some(pressure) = payload_object.get("pressure").and_then(as_float) {
                        last_flooding_pressure = Some(pressure);
                    }
                }
            }
            "dev_smoke_pump_flooding" => {
                if payload_object.is_some() {
                    if payload.get("result").and_then(Value::as_str) == Some("pass") {
                        pump_flooding_smoke_passed += 1;
                    }
                    last_pump_flooding_smoke = payload.clone();
                }
            }
            "pve_enemy_spawned" => pve_enemy_spawns += 1,
            "pve_spawned" => pve_spawned_events += 1,
            "pve_enemy_attack" => pve_enemy_attacks += 1,
            "player_damaged" => players_damaged += 1,
            "pve_damage_applied" => pve_damage_applied += 1,
            "dev_smoke_pve_enemy" => {
                if payload_object.is_some() {
                    if payload.get("result").and_then(Value::as_str) == Some("pass") {
                        pve_enemy_smoke_passed += 1;
                    }
                    last_pve_enemy_smoke = payload.clone();
                }
            }
            "dev_smoke_pve_damage" => {
                if payload_object.is_some() {
                    if payload.get("result").and_then(Value::as_str) == Some("pass") {
                        pve_damage_smoke_passed += 1;
                    }
                    last_pve_damage_smoke = payload.clone();
                }
            }
            "item_pickup" => item_pickups += 1,
            "item_dropped" => item_drops += 1,
            "dev_smoke_item_drop" => {
                if payload_object.is_some() {
                    if payload.get("result").and_then(Value::as_str) == Some("pass") {
                        item_drop_smoke_passed += 1;
                    }
                    last_item_drop_smoke = payload.clone();
                }
            }
            "dev_smoke_combined_systems" => {
                if payload_object.is_some() {
                    if payload.get("result").and_then(Value::as_str) == Some("pass") {
                        combined_systems_smoke_passed += 1;
                    }
                    last_combined_systems_smoke = payload.clone();
                }
            }
            "qa_bot_spawned" => qa_bot_spawns += 1,
            "qa_bot_moved" => qa_bot_moves += 1,
            "qa_bot_interacted" => qa_bot_interactions += 1,
            "dev_smoke_qa_bot" => {
                if payload_object.is_some() {
                    if payload.get("result").and_then(Value::as_str) == Some("pass") {
                        qa_bot_smoke_passed += 1;
                    }
                    last_qa_bot_smoke = payload.clone();
                }
            }
            "qa_player_bot_moved" => qa_player_bot_moves += 1,
            "qa_player_bot_interacted" => qa_player_bot_interactions += 1,
            "dev_smoke_qa_player_bot" => {
                if payload_object.is_some() {
                    if payload.get("result").and_then(Value::as_str) == Some("pass") {
                        qa_player_bot_smoke_passed += 1;
                    }
                    last_qa_player_bot_smoke = payload.clone();
                }
            }
            "qa_task_bot_moved" => qa_task_bot_moves += 1,
            "qa_task_bot_interacted" => qa_task_bot_interactions += 1,
            "dev_smoke_qa_task_bot" => {
                if payload_object.is_some() {
                    if payload.get("result").and_then(Value::as_str) == Some("pass") {
                        qa_task_bot_smoke_passed += 1;
                    }
                    last_qa_task_bot_smoke = payload.clone();
                }
            }
            "life_action_interacted" => life_action_interactions += 1,
            "dev_smoke_life_action" => {
                if payload_object.is_some() {
                    if payload.get("result").and_then(Value::as_str) == Some("pass") {
                        life_action_smoke_passed += 1;
                    }
                    last_life_action_smoke = payload.clone();
                }
            }
            _ => {}
        }

        let timestamp = event_object
            .get("timestamp_utc")
            .or_else(|| event_object.get("timestamp"));
        if let Some(timestamp_text) = value_to_string(timestamp) {
            if first_timestamp.is_none() {
                first_timestamp = Some(timestamp_text.clone());
            }
            last_timestamp = Some(timestamp_text);
            if let Some(timestamp_value) = timestamp.and_then(parse_timestamp) {
                if first_timestamp_value.is_none() {
                    first_timestamp_value = Some(timestamp_value);
                }
                last_timestamp_value = Some(timestamp_value);
            }
        }
    }

    let duration_seconds =
        if let (Some(first), Some(last)) = (first_elapsed_seconds, last_elapsed_seconds) {
            Some(round3((last - first).max(0.0)))
        } else if let (Some(first), Some(last)) = (first_timestamp_value, last_timestamp_value) {
            Some(round3(
                (last - first).num_milliseconds().max(0) as f64 / 1000.0,
            ))
        } else {
            None
        };

    let match_duration_seconds = if let (Some(started), Some(ended)) =
        (match_started_elapsed_seconds, match_ended_elapsed_seconds)
    {
        Some(round3((ended - started).max(0.0)))
    } else if let (Some(started), Some(ended)) = (match_started_timestamp, match_ended_timestamp) {
        Some(round3(
            (ended - started).num_milliseconds().max(0) as f64 / 1000.0,
        ))
    } else {
        None
    };

    Ok(BTreeMap::from([
        ("source".to_string(), json!(resolved.display().to_string())),
        ("run_id".to_string(), option_string(run_id)),
        ("build_id".to_string(), option_string(build_id)),
        ("map_id".to_string(), option_string(map_id)),
        ("profile".to_string(), option_string(profile)),
        ("session_metadata".to_string(), session_metadata),
        (
            "total_events".to_string(),
            json!(counts.values().sum::<i64>()),
        ),
        ("invalid_lines".to_string(), json!(invalid_lines)),
        (
            "first_timestamp".to_string(),
            option_string(first_timestamp),
        ),
        ("last_timestamp".to_string(), option_string(last_timestamp)),
        ("duration_seconds".to_string(), option_f64(duration_seconds)),
        (
            "match_duration_seconds".to_string(),
            option_f64(match_duration_seconds),
        ),
        (
            "first_elapsed_seconds".to_string(),
            option_f64(first_elapsed_seconds),
        ),
        (
            "last_elapsed_seconds".to_string(),
            option_f64(last_elapsed_seconds),
        ),
        (
            "match_started".to_string(),
            json!(count(&counts, "match_started") + count(&counts, "round_start")),
        ),
        (
            "match_ended".to_string(),
            json!(count(&counts, "match_ended") + count(&counts, "round_end")),
        ),
        (
            "match_timer_started".to_string(),
            json!(count(&counts, "match_timer_started")),
        ),
        (
            "match_timer_expired".to_string(),
            json!(count(&counts, "match_timer_expired")),
        ),
        (
            "final_approach_started".to_string(),
            json!(count(&counts, "final_approach_started")),
        ),
        (
            "players_connected".to_string(),
            json!(count(&counts, "player_connect") + count(&counts, "client_connected")),
        ),
        (
            "players_disconnected".to_string(),
            json!(count(&counts, "player_disconnect") + count(&counts, "client_disconnected")),
        ),
        (
            "deaths".to_string(),
            json!(count(&counts, "death") + count(&counts, "player_killed")),
        ),
        (
            "players_downed".to_string(),
            json!(count(&counts, "player_downed")),
        ),
        (
            "players_rescued".to_string(),
            json!(count(&counts, "player_rescued")),
        ),
        (
            "players_contained".to_string(),
            json!(count(&counts, "player_contained")),
        ),
        (
            "players_released".to_string(),
            json!(count(&counts, "player_released")),
        ),
        ("ship_task_repairs".to_string(), json!(task_repairs)),
        ("ship_task_sabotages".to_string(), json!(task_sabotages)),
        ("door_locks".to_string(), json!(door_locks)),
        ("door_unlocks".to_string(), json!(door_unlocks)),
        ("door_toggles".to_string(), json!(door_toggles)),
        (
            "bulkhead_lock_sabotages".to_string(),
            json!(bulkhead_lock_sabotages),
        ),
        (
            "bulkhead_locked_interaction_blocks".to_string(),
            json!(bulkhead_locked_interaction_blocks),
        ),
        (
            "bulkhead_lock_releases".to_string(),
            json!(bulkhead_lock_releases),
        ),
        (
            "bulkhead_lock_smoke_passed".to_string(),
            json!(bulkhead_lock_smoke_passed),
        ),
        (
            "last_bulkhead_lock_smoke".to_string(),
            last_bulkhead_lock_smoke,
        ),
        (
            "flooding_pressure_changes".to_string(),
            json!(flooding_pressure_changes),
        ),
        (
            "flooding_pressure_events".to_string(),
            json!(flooding_pressure_changes),
        ),
        (
            "flooding_pressure_sabotages".to_string(),
            json!(flooding_pressure_sabotages),
        ),
        (
            "flooding_pressure_repairs".to_string(),
            json!(flooding_pressure_repairs),
        ),
        (
            "pump_flooding_sabotages".to_string(),
            json!(flooding_pressure_sabotages),
        ),
        (
            "pump_flooding_repairs".to_string(),
            json!(flooding_pressure_repairs),
        ),
        (
            "last_flooding_pressure".to_string(),
            option_f64(last_flooding_pressure),
        ),
        (
            "pump_flooding_smoke_passed".to_string(),
            json!(pump_flooding_smoke_passed),
        ),
        (
            "last_pump_flooding_smoke".to_string(),
            last_pump_flooding_smoke,
        ),
        ("pve_enemy_spawns".to_string(), json!(pve_enemy_spawns)),
        ("pve_enemy_attacks".to_string(), json!(pve_enemy_attacks)),
        (
            "pve_enemy_smoke_passed".to_string(),
            json!(pve_enemy_smoke_passed),
        ),
        ("last_pve_enemy_smoke".to_string(), last_pve_enemy_smoke),
        ("players_damaged".to_string(), json!(players_damaged)),
        ("pve_spawned".to_string(), json!(pve_spawned_events)),
        ("pve_damage_applied".to_string(), json!(pve_damage_applied)),
        (
            "pve_damage_smoke_passed".to_string(),
            json!(pve_damage_smoke_passed),
        ),
        ("last_pve_damage_smoke".to_string(), last_pve_damage_smoke),
        ("item_pickups".to_string(), json!(item_pickups)),
        ("item_drops".to_string(), json!(item_drops)),
        (
            "item_drop_smoke_passed".to_string(),
            json!(item_drop_smoke_passed),
        ),
        ("last_item_drop_smoke".to_string(), last_item_drop_smoke),
        (
            "combined_systems_smoke_passed".to_string(),
            json!(combined_systems_smoke_passed),
        ),
        (
            "last_combined_systems_smoke".to_string(),
            last_combined_systems_smoke,
        ),
        ("qa_bot_spawns".to_string(), json!(qa_bot_spawns)),
        ("qa_bot_moves".to_string(), json!(qa_bot_moves)),
        (
            "qa_bot_interactions".to_string(),
            json!(qa_bot_interactions),
        ),
        (
            "qa_bot_smoke_passed".to_string(),
            json!(qa_bot_smoke_passed),
        ),
        ("last_qa_bot_smoke".to_string(), last_qa_bot_smoke),
        (
            "qa_player_bot_moves".to_string(),
            json!(qa_player_bot_moves),
        ),
        (
            "qa_player_bot_interactions".to_string(),
            json!(qa_player_bot_interactions),
        ),
        (
            "qa_player_bot_smoke_passed".to_string(),
            json!(qa_player_bot_smoke_passed),
        ),
        (
            "last_qa_player_bot_smoke".to_string(),
            last_qa_player_bot_smoke,
        ),
        ("qa_task_bot_moves".to_string(), json!(qa_task_bot_moves)),
        (
            "qa_task_bot_interactions".to_string(),
            json!(qa_task_bot_interactions),
        ),
        (
            "qa_task_bot_smoke_passed".to_string(),
            json!(qa_task_bot_smoke_passed),
        ),
        ("last_qa_task_bot_smoke".to_string(), last_qa_task_bot_smoke),
        (
            "life_action_interactions".to_string(),
            json!(life_action_interactions),
        ),
        (
            "life_action_smoke_passed".to_string(),
            json!(life_action_smoke_passed),
        ),
        ("last_life_action_smoke".to_string(), last_life_action_smoke),
        ("last_role_assignment".to_string(), last_role_assignment),
        ("last_match_result".to_string(), last_match_result),
        (
            "errors".to_string(),
            json!(count(&counts, "exception") + count(&counts, "error")),
        ),
        ("events_by_type".to_string(), json!(counts)),
    ]))
}

fn event_name(event: &Map<String, Value>) -> String {
    event
        .get("event")
        .or_else(|| event.get("event_type"))
        .and_then(Value::as_str)
        .unwrap_or("unknown")
        .to_string()
}

fn normalize_payload(payload: Option<&Value>) -> Value {
    match payload {
        Some(Value::String(text)) => serde_json::from_str(text).unwrap_or(Value::Null),
        Some(value) => value.clone(),
        None => Value::Null,
    }
}

fn event_timestamp(event: &Map<String, Value>) -> Option<DateTime<Utc>> {
    event
        .get("timestamp_utc")
        .or_else(|| event.get("timestamp"))
        .and_then(parse_timestamp)
}

fn parse_timestamp(value: &Value) -> Option<DateTime<Utc>> {
    let text = value.as_str()?.trim();
    if let Ok(timestamp) = DateTime::parse_from_rfc3339(text) {
        return Some(timestamp.with_timezone(&Utc));
    }
    NaiveDateTime::parse_from_str(text, "%Y-%m-%dT%H:%M:%S%.f")
        .ok()
        .map(|timestamp| timestamp.and_utc())
}

fn as_float(value: &Value) -> Option<f64> {
    if value.is_boolean() {
        return None;
    }
    value.as_f64()
}

fn set_once(target: &mut Option<String>, value: Option<String>) {
    if target.is_none() {
        *target = value;
    }
}

fn value_to_string(value: Option<&Value>) -> Option<String> {
    match value {
        Some(Value::String(text)) => Some(text.clone()),
        Some(Value::Number(number)) => Some(number.to_string()),
        Some(Value::Bool(value)) => Some(value.to_string()),
        _ => None,
    }
}

fn option_string(value: Option<String>) -> Value {
    value.map(Value::String).unwrap_or(Value::Null)
}

fn option_f64(value: Option<f64>) -> Value {
    value.map_or(Value::Null, |value| json!(value))
}

fn round3(value: f64) -> f64 {
    (value * 1000.0).round() / 1000.0
}

fn count(counts: &BTreeMap<String, i64>, key: &str) -> i64 {
    counts.get(key).copied().unwrap_or_default()
}

fn load_summary(path: &Path) -> Result<BTreeMap<String, Value>, String> {
    let resolved = resolve_path(path);
    let text = fs::read_to_string(&resolved)
        .map_err(|error| format!("{}: {error}", resolved.display()))?;
    match serde_json::from_str::<Value>(&text) {
        Ok(Value::Object(object)) => {
            if object.contains_key("event") || object.contains_key("payload") {
                return Err(
                    "input must be a frostwake-tools log-summary summary JSON object".to_string(),
                );
            }
            Ok(object.into_iter().collect())
        }
        Ok(_) => Err("input must be a frostwake-tools log-summary summary JSON object".to_string()),
        Err(_) => summarize_events(&resolved),
    }
}

fn build_playtest_report_payload(
    summary: &BTreeMap<String, Value>,
    note: &str,
) -> Result<Value, String> {
    let result = summary
        .get("last_match_result")
        .and_then(Value::as_object)
        .cloned()
        .unwrap_or_default();
    let summary_text = concise_playtest_summary(summary, &result, note);
    assert_no_risky_text(&summary_text)?;

    Ok(json!({
        "runId": summary_string(summary, "run_id", "unknown-run"),
        "buildId": summary_string(summary, "build_id", "unknown-build"),
        "mapId": summary_string(summary, "map_id", "unknown-map"),
        "playerCount": playtest_player_count(summary),
        "winner": normalize_winner(result.get("winner")),
        "summary": summary_text,
    }))
}

fn concise_playtest_summary(
    summary: &BTreeMap<String, Value>,
    result: &Map<String, Value>,
    note: &str,
) -> String {
    let mut parts = vec![
        format!("run_id={}", summary_string(summary, "run_id", "")),
        format!("profile={}", summary_string(summary, "profile", "")),
        format!(
            "players_connected={}",
            summary_i64(summary, "players_connected")
        ),
        format!(
            "players_disconnected={}",
            summary_i64(summary, "players_disconnected")
        ),
        format!("match_started={}", summary_i64(summary, "match_started")),
        format!("match_ended={}", summary_i64(summary, "match_ended")),
        format!("winner={}", normalize_winner(result.get("winner"))),
        format!(
            "reason={}",
            value_to_string(result.get("reason")).unwrap_or_default()
        ),
        format!(
            "critical_systems={}",
            result
                .get("criticalSystems")
                .map(display_json_scalar)
                .unwrap_or_default()
        ),
        format!("repairs={}", summary_i64(summary, "ship_task_repairs")),
        format!("sabotages={}", summary_i64(summary, "ship_task_sabotages")),
        format!("downs={}", summary_i64(summary, "players_downed")),
        format!("rescues={}", summary_i64(summary, "players_rescued")),
        format!(
            "duration_seconds={}",
            summary
                .get("duration_seconds")
                .map(display_json_scalar)
                .unwrap_or_default()
        ),
        format!(
            "match_duration_seconds={}",
            summary
                .get("match_duration_seconds")
                .map(display_json_scalar)
                .unwrap_or_default()
        ),
    ];
    if !note.is_empty() {
        parts.push(format!("note={note}"));
    }
    parts.join("; ")
}

fn playtest_player_count(summary: &BTreeMap<String, Value>) -> i64 {
    let connected = summary_i64(summary, "players_connected");
    if connected > 0 {
        return connected.clamp(1, 8);
    }

    let assigned_players = summary
        .get("last_role_assignment")
        .and_then(Value::as_object)
        .and_then(|assignment| assignment.get("players"))
        .and_then(value_to_i64)
        .unwrap_or_default();
    if assigned_players > 0 {
        assigned_players.clamp(1, 8)
    } else {
        1
    }
}

fn normalize_winner(value: Option<&Value>) -> &'static str {
    let text = value_to_string(value).unwrap_or_else(|| "unknown".to_string());
    match text.trim().to_lowercase().as_str() {
        "crew" => "crew",
        "agents" => "agents",
        "none" => "none",
        "unknown" => "unknown",
        "agent" | "saboteur" | "saboteurs" | "traitor" | "traitors" => "agents",
        _ => "unknown",
    }
}

fn assert_no_risky_text(text: &str) -> Result<(), String> {
    let patterns = [
        (
            "local user path",
            Regex::new(r#"(?i)(/Users/[^/\s|`]+|[A-Z]:\\Users\\[^\\\s|`]+)"#).expect("valid regex"),
        ),
        (
            "IPv4 address",
            Regex::new(r"\b(?:\d{1,3}\.){3}\d{1,3}\b").expect("valid regex"),
        ),
        (
            "SteamID64-like value",
            Regex::new(r"\b7656119\d{10}\b").expect("valid regex"),
        ),
        (
            "email address",
            Regex::new(r"\b[A-Za-z0-9._%+-]+@[A-Za-z0-9.-]+\.[A-Za-z]{2,}\b").expect("valid regex"),
        ),
        (
            "secret-like assignment",
            Regex::new(r"(?i)\b(?:token|secret|password|apikey|api_key)\s*[:=]\s*[^,\s|`]+")
                .expect("valid regex"),
        ),
    ];
    let hits: Vec<_> = patterns
        .iter()
        .filter_map(|(label, pattern)| pattern.is_match(text).then_some(*label))
        .collect();
    if hits.is_empty() {
        Ok(())
    } else {
        Err(format!(
            "refusing to upload summary with risky text: {}",
            hits.join(", ")
        ))
    }
}

fn post_json_http(endpoint: &str, payload: &Value, timeout: Duration) -> Result<Value, String> {
    let endpoint = parse_http_endpoint(endpoint)?;
    let address = format!("{}:{}", endpoint.host, endpoint.port);
    let socket_address = address
        .to_socket_addrs()
        .map_err(|error| format!("{address}: {error}"))?
        .next()
        .ok_or_else(|| format!("{address}: no socket addresses resolved"))?;
    let mut stream = TcpStream::connect_timeout(&socket_address, timeout)
        .map_err(|error| format!("upload failed: {error}"))?;
    stream
        .set_read_timeout(Some(timeout))
        .map_err(|error| format!("upload failed: {error}"))?;
    stream
        .set_write_timeout(Some(timeout))
        .map_err(|error| format!("upload failed: {error}"))?;

    let body = payload.to_string();
    let request = format!(
        "POST {} HTTP/1.1\r\nHost: {}\r\nContent-Type: application/json\r\nAccept: application/json\r\nContent-Length: {}\r\nConnection: close\r\n\r\n{}",
        endpoint.path,
        endpoint.host_header(),
        body.len(),
        body,
    );
    stream
        .write_all(request.as_bytes())
        .map_err(|error| format!("upload failed: {error}"))?;

    let mut response = Vec::new();
    stream
        .read_to_end(&mut response)
        .map_err(|error| format!("upload failed: {error}"))?;
    parse_http_json_response(&response)
}

struct HttpEndpoint {
    host: String,
    port: u16,
    path: String,
}

impl HttpEndpoint {
    fn host_header(&self) -> String {
        if self.port == 80 {
            self.host.clone()
        } else {
            format!("{}:{}", self.host, self.port)
        }
    }
}

fn parse_http_endpoint(endpoint: &str) -> Result<HttpEndpoint, String> {
    let Some(rest) = endpoint.strip_prefix("http://") else {
        return Err("only http:// endpoints are supported by this local uploader".to_string());
    };
    let (authority, path) = rest.split_once('/').unwrap_or((rest, ""));
    if authority.is_empty() {
        return Err("endpoint host is empty".to_string());
    }
    let (host, port) = if let Some((host, port)) = authority.rsplit_once(':') {
        let port = port
            .parse::<u16>()
            .map_err(|error| format!("endpoint port is invalid: {error}"))?;
        (host.to_string(), port)
    } else {
        (authority.to_string(), 80)
    };
    Ok(HttpEndpoint {
        host,
        port,
        path: format!("/{}", path),
    })
}

fn parse_http_json_response(response: &[u8]) -> Result<Value, String> {
    let response = String::from_utf8_lossy(response);
    let (head, body) = response
        .split_once("\r\n\r\n")
        .ok_or_else(|| "upload failed: malformed HTTP response".to_string())?;
    let status_line = head.lines().next().unwrap_or_default();
    let status = status_line
        .split_whitespace()
        .nth(1)
        .and_then(|value| value.parse::<u16>().ok())
        .unwrap_or_default();
    if !(200..300).contains(&status) {
        return Err(format!("upload failed: HTTP {status}: {body}"));
    }
    if body.trim().is_empty() {
        return Ok(json!({}));
    }
    serde_json::from_str(body)
        .map_err(|error| format!("upload failed: response was not JSON: {error}"))
}

fn summary_string(summary: &BTreeMap<String, Value>, key: &str, fallback: &str) -> String {
    summary
        .get(key)
        .and_then(|value| match value {
            Value::Null => None,
            _ => value_to_string(Some(value)),
        })
        .unwrap_or_else(|| fallback.to_string())
}

fn summary_i64(summary: &BTreeMap<String, Value>, key: &str) -> i64 {
    summary.get(key).and_then(value_to_i64).unwrap_or_default()
}

fn value_to_i64(value: &Value) -> Option<i64> {
    if value.is_boolean() {
        return None;
    }
    value
        .as_i64()
        .or_else(|| value.as_f64().map(|value| value as i64))
}

fn display_json_scalar(value: &Value) -> String {
    match value {
        Value::String(text) => text.clone(),
        Value::Null => String::new(),
        other => other.to_string(),
    }
}

const PLAYTEST_TELEMETRY_FIELDS: &[&str] = &[
    "run_id",
    "build_id",
    "map_id",
    "profile",
    "duration_seconds",
    "match_duration_seconds",
    "players_connected",
    "players_disconnected",
    "match_started",
    "match_ended",
    "match_timer_started",
    "match_timer_expired",
    "last_role_assignment.players",
    "last_role_assignment.saboteurs",
    "last_match_result.winner",
    "last_match_result.reason",
    "last_match_result.criticalSystems",
    "ship_task_repairs",
    "ship_task_sabotages",
    "players_damaged",
    "players_downed",
    "players_rescued",
    "players_contained",
    "players_released",
    "life_action_interactions",
    "deaths",
    "errors",
    "invalid_lines",
    "total_events",
];
const PLAYTEST_CORE_SUMMARY_FIELDS: &[&str] = &[
    "run_id",
    "build_id",
    "map_id",
    "profile",
    "players_connected",
    "players_disconnected",
    "duration_seconds",
    "match_started",
    "match_ended",
    "invalid_lines",
    "total_events",
    "events_by_type",
];
const PLAYTEST_REQUIRED_MARKDOWN_SECTIONS: &[&str] = &[
    "Header",
    "Telemetry Summary",
    "Technical Result",
    "Validity Gate",
    "Observed Flow",
    "Player Feedback",
    "Comprehension And Accessibility",
    "Keep / Cut / Change",
    "Mechanics Decision Table",
    "Top Blockers",
    "P1-024 Decision",
    "Data Handling Check",
];
const PLAYTEST_HUMAN_SIGNAL_ROWS: &[(&str, &str)] = &[
    (
        "objective_comprehension",
        "What did players think their goal was?",
    ),
    ("confusion_cost", "What was confusing?"),
    ("social_incident", "What was tense, funny, or memorable?"),
    ("repeat_play_intent", "How many wanted another round?"),
    ("keep_decision", "What should be kept?"),
    ("cut_decision", "What should be cut?"),
    (
        "change_decision",
        "What should change before the next test?",
    ),
];
const PLAYTEST_GP09_SIGNAL_ROWS: &[(&str, &str)] = &[
    ("objective_comprehension", "Objective comprehension"),
    ("next_step_clarity", "Next-step clarity"),
    ("failure_state_clarity", "Failure-state clarity"),
    ("ui_or_control_confusion", "UI or control confusion"),
    ("accessibility_blocker", "Accessibility blocker"),
    ("text_or_term_issue", "Text or term issue"),
];

fn load_summary_json(path: &Path) -> Result<BTreeMap<String, Value>, String> {
    let resolved = resolve_path(path);
    let text = fs::read_to_string(&resolved)
        .map_err(|error| format!("{}: {error}", resolved.display()))?;
    let value: Value = serde_json::from_str(&text).map_err(|error| {
        format!("input is not a single summary JSON object; run frostwake-tools log-summary --out first: {error}")
    })?;
    let Value::Object(object) = value else {
        return Err("summary JSON must contain an object".to_string());
    };
    validate_summary_shape(&object)?;
    Ok(object.into_iter().collect())
}

fn validate_summary_shape(summary: &Map<String, Value>) -> Result<(), String> {
    if summary.contains_key("event") || summary.contains_key("payload") {
        return Err(
            "input looks like a raw event record; run frostwake-tools log-summary --out first"
                .to_string(),
        );
    }
    if ![
        "total_events",
        "match_started",
        "players_connected",
        "events_by_type",
    ]
    .iter()
    .any(|field| summary.contains_key(*field))
    {
        return Err(
            "input does not look like frostwake-tools log-summary summary JSON".to_string(),
        );
    }
    Ok(())
}

struct PlaytestSummaryRenderOptions<'a> {
    summary: &'a BTreeMap<String, Value>,
    summary_path: &'a Path,
    title: &'a str,
    min_players: i64,
    target_players: Option<i64>,
    raw_evidence_label: Option<&'a str>,
    snapshot: Option<&'a str>,
    author: Option<&'a str>,
    date: Option<&'a str>,
    dry_run_example: bool,
}

fn render_playtest_summary_markdown(options: PlaytestSummaryRenderOptions<'_>) -> String {
    let summary = options.summary;
    let raw_evidence = options
        .raw_evidence_label
        .map(sanitize_markdown)
        .unwrap_or_else(|| {
            sanitize_markdown(&relative_label(
                options
                    .summary_path
                    .parent()
                    .unwrap_or(options.summary_path),
            ))
        });
    let summary_input = sanitize_markdown(&relative_label(options.summary_path));
    let date = sanitize_markdown(options.date.unwrap_or("2026-05-25"));
    let dry_run_notice = if options.dry_run_example {
        "\n> Dry-run example only. This file was generated from smoke-test telemetry, not a human playtest. Do not count it as P1-024 evidence, social validation, balance evidence, or a pass/partial/fail result.\n"
    } else {
        ""
    };
    let intro = if options.dry_run_example {
        "This dry-run demonstrates the summary format with smoke telemetry."
    } else {
        "Use this after a P1-024 run."
    };
    let validity_result = if options.dry_run_example {
        "not evaluated - smoke-test dry run"
    } else {
        "pass / partial / fail"
    };
    let decision_result = if options.dry_run_example {
        "P1-024 result: not evaluated (dry-run example)"
    } else {
        "P1-024 result: pass / partial / fail"
    };
    let next_backlog = if options.dry_run_example {
        "Next backlog item: none - run P1-024 with humans before using this as evidence"
    } else {
        "Next backlog item: P1-025 / technical rehearsal / rerun P1-024"
    };
    let generated_from = if options.dry_run_example {
        format!(
            "cargo run -p frostwake-tools -- playtest-summary {summary_input} --dry-run-example"
        )
    } else {
        "cargo run -p frostwake-tools -- log-summary Saved/Playtests/P1-024/run-01/events.jsonl --out Saved/Playtests/P1-024/run-01/summary.json\ncargo run -p frostwake-tools -- playtest-summary Saved/Playtests/P1-024/run-01/summary.json --out docs/playtests/p1-024-run-01-summary.md".to_string()
    };

    format!(
        r#"# {title}
{dry_run_notice}

{intro} Commit a completed human-test summary only after removing tester names, raw voice details, IP addresses, SteamIDs, local machine secrets, crash dump paths that expose usernames, and raw transcript text.

Raw evidence stays under `Saved/Playtests/...` and is not committed.

## Header

```text
RunId: {run_id}
Date: {date}
Local snapshot or commit: {snapshot}
Build: {build_id}
Map: {map_id}
Profile: {profile}
Host type: listen-server
Target players: {target_players}
Actual players: {actual_players}
Voice method:
Recording consent: yes / no / partial
Raw evidence path: {raw_evidence}
Summary author: {author}
```

## Telemetry Summary

Generated from:

```bash
{generated_from}
```

{telemetry_table}

Compact anonymized excerpt:

```json
{compact_json}
```

## Technical Result

{technical_table}

Technical notes:

-

## Validity Gate

Resolve each item before choosing pass, partial, or fail.

{validity_table}

Validity result:

```text
{validity_result}
```

## Observed Flow

| Moment | Timestamp or elapsed time | What happened | Decision impact |
| --- | --- | --- | --- |
| First goal comprehension |  |  |  |
| First repair or sabotage |  |  |  |
| First accusation |  |  |  |
| First down/rescue/containment |  |  |  |
| Biggest confusion |  |  |  |
| Best social moment |  |  |  |
| Worst stall |  |  |  |
| End or interruption |  |  |  |

Short narrative:

-

## Player Feedback

Aggregate only. Do not name players.

| Question | Summary |
| --- | --- |
| What did players think their goal was? |  |
| When did suspicion first appear? |  |
| What created trust? |  |
| What created distrust? |  |
| What was confusing? |  |
| What was tense, funny, or memorable? |  |
| How many wanted another round? |  |
| What should be kept? |  |
| What should be cut? |  |
| What should change before the next test? |  |

Agent-only feedback:

-

Crew-only feedback:

-

## Comprehension And Accessibility

Summarize GP-09 evidence from observer notes and post-round answers. Use `none observed` only when an observer explicitly checked the signal.

| Signal | Evidence | Follow-up |
| --- | --- | --- |
| Objective comprehension |  |  |
| Next-step clarity |  |  |
| Failure-state clarity |  |  |
| UI or control confusion |  |  |
| Accessibility blocker |  |  |
| Text or term issue |  |  |

## Keep / Cut / Change

Keep:

-

Cut:

-

Change before P1-026:

-

## Mechanics Decision Table

{mechanics_table}

## Top Blockers

List at most five. These become P1-025 candidates.

| Rank | Blocker | Severity | Evidence | Proposed fix | Owner | Retest gate |
| ---: | --- | --- | --- | --- | --- | --- |
| 1 |  | critical / high / medium / low |  |  |  |  |
| 2 |  | critical / high / medium / low |  |  |  |  |
| 3 |  | critical / high / medium / low |  |  |  |  |
| 4 |  | critical / high / medium / low |  |  |  |  |
| 5 |  | critical / high / medium / low |  |  |  |  |

## P1-024 Decision

Choose one:

- pass: {min_players} players connected, roles assigned, players interacted with the loop, useful social data was generated, and a keep/cut/change list exists.
- partial: technical or clarity issues blocked completion, but logs and observations identify what to fix.
- fail: host, connection, role assignment, movement, interaction, or logging failed before useful data.

Decision:

```text
{decision_result}
Reason:
{next_backlog}
```

## Data Handling Check

- [ ] Raw logs are not committed.
- [ ] Raw recordings are not committed.
- [ ] Tester names are removed.
- [ ] Voice transcripts are not included.
- [ ] IP addresses, SteamIDs, and local usernames are removed.
- [ ] Any moderation-sensitive details are summarized without identifying people.
- [ ] Summary is stored in `docs/playtests/` only after anonymization.

## Follow-Up

Immediate next action:

-

Retest command or plan:

```text

```
"#,
        title = options.title,
        dry_run_notice = dry_run_notice,
        intro = intro,
        run_id = summary_text(summary, "run_id"),
        date = date,
        snapshot = sanitize_markdown(options.snapshot.unwrap_or("")),
        build_id = summary_text(summary, "build_id"),
        map_id = summary_text(summary, "map_id"),
        profile = summary_text(summary, "profile"),
        target_players = options
            .target_players
            .map_or(String::new(), |value| value.to_string()),
        actual_players = summary_text(summary, "players_connected"),
        raw_evidence = raw_evidence,
        author = sanitize_markdown(options.author.unwrap_or("")),
        generated_from = generated_from,
        telemetry_table = playtest_telemetry_table(summary),
        compact_json = playtest_compact_json(summary),
        technical_table = playtest_technical_table(summary, options.min_players),
        validity_table = playtest_validity_table(summary, options.min_players),
        validity_result = validity_result,
        mechanics_table = playtest_mechanics_table(summary),
        min_players = options.min_players,
        decision_result = decision_result,
        next_backlog = next_backlog,
    )
}

fn playtest_telemetry_table(summary: &BTreeMap<String, Value>) -> String {
    let mut rows = vec!["| Field | Value |".to_string(), "| --- | --- |".to_string()];
    for field in PLAYTEST_TELEMETRY_FIELDS {
        rows.push(format!(
            "| `{field}` | {} |",
            markdown_text_value(nested_summary_value(summary, field))
        ));
    }
    rows.join("\n")
}

fn playtest_validity_table(summary: &BTreeMap<String, Value>, min_players: i64) -> String {
    let (players, _crew, saboteurs) = playtest_role_counts(summary);
    let expected = players.and_then(expected_saboteurs_for_players);
    let role_counts_match = players
        .zip(saboteurs)
        .zip(expected)
        .map(|((_, saboteurs), expected)| saboteurs == expected);
    let interactions = playtest_interaction_count(summary);
    let repair_or_sabotage =
        summary_i64(summary, "ship_task_repairs") + summary_i64(summary, "ship_task_sabotages");
    let logs_usable =
        summary_i64(summary, "total_events") > 0 && summary_i64(summary, "invalid_lines") == 0;
    let no_crash =
        summary_i64(summary, "errors") == 0 && summary_i64(summary, "invalid_lines") == 0;
    [
        "| Check | Result | Evidence |".to_string(),
        "| --- | --- | --- |".to_string(),
        format!("| `players_connected == {min_players}` | {} | players_connected={} |", bool_result(summary_i64(summary, "players_connected") == min_players), summary_i64(summary, "players_connected")),
        format!("| `match_started > 0` | {} | match_started={} |", bool_result(summary_i64(summary, "match_started") > 0), summary_i64(summary, "match_started")),
        format!("| `last_role_assignment` exists | {} | players={}, saboteurs={} |", bool_option_result(players.is_some()), players.map_or(String::new(), |value| value.to_string()), saboteurs.map_or(String::new(), |value| value.to_string())),
        format!("| Crew/agent counts match player count | {} | expected_saboteurs={} |", option_bool_result(role_counts_match), expected.map_or(String::new(), |value| value.to_string())),
        format!("| At least one repair or sabotage-relevant interaction occurred | {} | ship_task_repairs={}, ship_task_sabotages={} |", bool_result(repair_or_sabotage > 0), summary_i64(summary, "ship_task_repairs"), summary_i64(summary, "ship_task_sabotages")),
        format!("| Any interaction evidence exists | {} | interaction_count={interactions} |", bool_result(interactions > 0)),
        format!("| No crash or desync blocked basic participation | {} | errors={}, invalid_lines={}; observer confirm |", bool_result(no_crash), summary_i64(summary, "errors"), summary_i64(summary, "invalid_lines")),
        format!("| Logs are present and usable | {} | total_events={} |", bool_result(logs_usable), summary_i64(summary, "total_events")),
        "| Keep/cut/change list was produced | observer required | Fill from player feedback |".to_string(),
    ]
    .join("\n")
}

fn playtest_technical_table(summary: &BTreeMap<String, Value>, min_players: i64) -> String {
    let interactions = playtest_interaction_count(summary);
    let repair_or_sabotage =
        summary_i64(summary, "ship_task_repairs") + summary_i64(summary, "ship_task_sabotages");
    let logs_usable =
        summary_i64(summary, "total_events") > 0 && summary_i64(summary, "invalid_lines") == 0;
    [
        "| Gate | Result | Evidence |".to_string(),
        "| --- | --- | --- |".to_string(),
        format!("| Host started | {} | total_events={} |", status_result(summary_i64(summary, "total_events") > 0, false), summary_i64(summary, "total_events")),
        format!("| Clients connected | {} | players_connected={} |", status_result(summary_i64(summary, "players_connected") == min_players, summary_i64(summary, "players_connected") > 0), summary_i64(summary, "players_connected")),
        format!("| Role assignment visible in log | {} | last_role_assignment={} |", status_result(summary.get("last_role_assignment").is_some_and(|value| !value.is_null()), false), markdown_text_value(summary.get("last_role_assignment"))),
        "| Players could move | observer required | Fill from observer notes |".to_string(),
        format!("| Players could interact | {} | interaction_count={interactions}; observer confirm |", status_result(interactions > 0, true)),
        format!("| Repair or sabotage happened | {} | repairs={}, sabotages={} |", status_result(repair_or_sabotage > 0, true), summary_i64(summary, "ship_task_repairs"), summary_i64(summary, "ship_task_sabotages")),
        format!("| Match ended or meaningful interruption captured | {} | match_started={}, match_ended={} |", status_result(summary_i64(summary, "match_ended") > 0, summary_i64(summary, "match_started") > 0), summary_i64(summary, "match_started"), summary_i64(summary, "match_ended")),
        format!("| Logs reconstruct the run | {} | invalid_lines={} |", status_result(logs_usable, false), summary_i64(summary, "invalid_lines")),
        format!("| No critical crash | {} | errors={}; observer confirm |", status_result(summary_i64(summary, "errors") == 0, false), summary_i64(summary, "errors")),
    ]
    .join("\n")
}

fn playtest_mechanics_table(summary: &BTreeMap<String, Value>) -> String {
    let (players, crew, saboteurs) = playtest_role_counts(summary);
    [
        "| Area | Telemetry evidence | Observer/player evidence | Decision | Reason | Next action |".to_string(),
        "| --- | --- | --- | --- | --- | --- |".to_string(),
        format!("| Repairs and route tasks | repairs={}, route_final={} |  | keep / cut / change |  |  |", summary_i64(summary, "ship_task_repairs"), summary_i64(summary, "final_approach_started")),
        format!("| Sabotage actions | sabotages={}, winner={}, critical_systems={} |  | keep / cut / change |  |  |", summary_i64(summary, "ship_task_sabotages"), markdown_text_value(nested_summary_value(summary, "last_match_result.winner")), markdown_text_value(nested_summary_value(summary, "last_match_result.criticalSystems"))),
        format!("| Doors and bulkheads | toggles={}, locks={}, releases={} |  | keep / cut / change |  |  |", summary_i64(summary, "door_toggles"), summary_i64(summary, "door_locks"), summary_i64(summary, "bulkhead_lock_releases")),
        format!("| Flooding and pump pressure | pressure_events={}, last_pressure={} |  | keep / cut / change |  |  |", summary_i64(summary, "flooding_pressure_events"), markdown_text_value(summary.get("last_flooding_pressure"))),
        format!("| PvE, down, rescue, containment | damaged={}, downed={}, rescued={}, contained={}, released={} |  | keep / cut / change |  |  |", summary_i64(summary, "players_damaged"), summary_i64(summary, "players_downed"), summary_i64(summary, "players_rescued"), summary_i64(summary, "players_contained"), summary_i64(summary, "players_released")),
        format!("| Items and pickup/drop | pickups={}, drops={} |  | keep / cut / change |  |  |", summary_i64(summary, "item_pickups"), summary_i64(summary, "item_drops")),
        format!("| Role clarity | players={}, crew={}, agents={} |  | keep / cut / change |  |  |", players.map_or(String::new(), |value| value.to_string()), crew.map_or(String::new(), |value| value.to_string()), saboteurs.map_or(String::new(), |value| value.to_string())),
        format!("| Pacing | duration={}, match_duration={} |  | keep / cut / change |  |  |", markdown_text_value(summary.get("duration_seconds")), markdown_text_value(summary.get("match_duration_seconds"))),
        "| Social suspicion | telemetry cannot judge |  | keep / cut / change |  |  |".to_string(),
    ]
    .join("\n")
}

fn playtest_compact_json(summary: &BTreeMap<String, Value>) -> String {
    let excerpt = json!({
        "run_id": summary.get("run_id").cloned().unwrap_or(Value::Null),
        "duration_seconds": summary.get("duration_seconds").cloned().unwrap_or(Value::Null),
        "match_duration_seconds": summary.get("match_duration_seconds").cloned().unwrap_or(Value::Null),
        "players_connected": summary.get("players_connected").cloned().unwrap_or(Value::Null),
        "players_disconnected": summary.get("players_disconnected").cloned().unwrap_or(Value::Null),
        "match_started": summary.get("match_started").cloned().unwrap_or(Value::Null),
        "match_ended": summary.get("match_ended").cloned().unwrap_or(Value::Null),
        "last_role_assignment": summary.get("last_role_assignment").cloned().unwrap_or_else(|| json!({})),
        "last_match_result": summary.get("last_match_result").cloned().unwrap_or_else(|| json!({})),
    });
    serde_json::to_string_pretty(&excerpt).expect("excerpt serializes")
}

#[derive(Clone, Copy, Debug, Eq, PartialEq)]
enum CheckStatus {
    Pass,
    Warn,
    Fail,
}

impl CheckStatus {
    fn label(self) -> &'static str {
        match self {
            Self::Pass => "PASS",
            Self::Warn => "WARN",
            Self::Fail => "FAIL",
        }
    }
}

#[derive(Debug)]
struct PlaytestCheckResult {
    name: String,
    status: CheckStatus,
    detail: String,
}

fn add_playtest_check(
    results: &mut Vec<PlaytestCheckResult>,
    name: &str,
    passed: bool,
    detail: impl Into<String>,
    warn: bool,
) {
    let status = if passed {
        CheckStatus::Pass
    } else if warn {
        CheckStatus::Warn
    } else {
        CheckStatus::Fail
    };
    results.push(PlaytestCheckResult {
        name: name.to_string(),
        status,
        detail: detail.into(),
    });
}

fn check_playtest_summary(
    summary: &BTreeMap<String, Value>,
    summary_path: &Path,
    mode: PlaytestPreflightMode,
    min_players: i64,
) -> Vec<PlaytestCheckResult> {
    let mut results = Vec::new();
    add_playtest_check(
        &mut results,
        "summary_shape",
        true,
        "summary JSON shape is compatible with frostwake-tools log-summary",
        false,
    );
    add_playtest_check(
        &mut results,
        "events_present",
        summary_i64(summary, "total_events") > 0,
        format!("total_events={}", summary_i64(summary, "total_events")),
        false,
    );
    add_playtest_check(
        &mut results,
        "jsonl_parse_clean",
        summary_i64(summary, "invalid_lines") == 0,
        format!("invalid_lines={}", summary_i64(summary, "invalid_lines")),
        false,
    );
    add_playtest_check(
        &mut results,
        "no_logged_errors",
        summary_i64(summary, "errors") == 0,
        format!("errors={}", summary_i64(summary, "errors")),
        false,
    );
    add_playtest_check(
        &mut results,
        "map_id",
        summary_string(summary, "map_id", "") == "/Game/Maps/L_IcebreakerWhitebox",
        format!("map_id={:?}", summary_string(summary, "map_id", "")),
        false,
    );

    if mode == PlaytestPreflightMode::DryRun {
        add_playtest_check(
            &mut results,
            "dry_run_player_count_not_scored",
            true,
            format!(
                "players_connected={}; human minimum is not evaluated in dry-run mode",
                summary_i64(summary, "players_connected")
            ),
            false,
        );
        add_playtest_check(
            &mut results,
            "dry_run_source_allowed",
            true,
            format!("summary={}", relative_label(summary_path)),
            false,
        );
        return results;
    }

    let missing_core: Vec<_> = PLAYTEST_CORE_SUMMARY_FIELDS
        .iter()
        .filter(|field| !summary.contains_key(**field))
        .copied()
        .collect();
    let (players, _crew, saboteurs) = playtest_role_counts(summary);
    let expected = players.and_then(expected_saboteurs_for_players);
    let smoke_or_qa = smoke_or_qa_events(summary);
    let repairs_or_sabotages =
        summary_i64(summary, "ship_task_repairs") + summary_i64(summary, "ship_task_sabotages");

    add_playtest_check(
        &mut results,
        "core_summary_fields",
        missing_core.is_empty(),
        if missing_core.is_empty() {
            "all core fields present".to_string()
        } else {
            missing_core.join(", ")
        },
        false,
    );
    add_playtest_check(
        &mut results,
        "run_id",
        summary_string(summary, "run_id", "").starts_with("P1-024"),
        format!("run_id={:?}", summary_string(summary, "run_id", "")),
        false,
    );
    add_playtest_check(
        &mut results,
        "profile",
        summary_string(summary, "profile", "") == "human-p1-024",
        format!("profile={:?}", summary_string(summary, "profile", "")),
        false,
    );
    add_playtest_check(
        &mut results,
        "no_smoke_or_qa_basis",
        smoke_or_qa.is_empty(),
        if smoke_or_qa.is_empty() {
            "no smoke or QA event types found".to_string()
        } else {
            smoke_or_qa.join(", ")
        },
        false,
    );
    let source = summary_string(summary, "source", "");
    let path_ok = relative_label(summary_path).contains("Saved/Playtests/P1-024/")
        || source.contains("Saved/Playtests/P1-024/");
    add_playtest_check(
        &mut results,
        "playtest_evidence_path",
        path_ok,
        format!(
            "summary={}, source={:?}",
            relative_label(summary_path),
            source
        ),
        false,
    );
    add_playtest_check(
        &mut results,
        "required_players",
        summary_i64(summary, "players_connected") == min_players,
        format!(
            "players_connected={}, required={min_players}",
            summary_i64(summary, "players_connected")
        ),
        false,
    );
    add_playtest_check(
        &mut results,
        "match_started",
        summary_i64(summary, "match_started") > 0,
        format!("match_started={}", summary_i64(summary, "match_started")),
        false,
    );
    add_playtest_check(
        &mut results,
        "role_assignment_present",
        players.is_some() && saboteurs.is_some(),
        format!("players={players:?}, saboteurs={saboteurs:?}"),
        false,
    );
    add_playtest_check(
        &mut results,
        "role_count_matches",
        players.is_some() && saboteurs == expected,
        format!("players={players:?}, saboteurs={saboteurs:?}, expected={expected:?}"),
        false,
    );
    add_playtest_check(
        &mut results,
        "repair_or_sabotage_seen",
        repairs_or_sabotages > 0,
        format!("repairs+sabotages={repairs_or_sabotages}"),
        false,
    );
    add_playtest_check(
        &mut results,
        "interaction_seen",
        playtest_interaction_count(summary) > 0,
        format!("interaction_count={}", playtest_interaction_count(summary)),
        false,
    );
    add_playtest_check(
        &mut results,
        "disconnects_recorded",
        summary_i64(summary, "players_disconnected") >= 0,
        format!(
            "players_disconnected={}",
            summary_i64(summary, "players_disconnected")
        ),
        false,
    );
    add_playtest_check(
        &mut results,
        "match_end_optional",
        summary_i64(summary, "match_ended") > 0,
        format!(
            "match_ended={}; partial/fail summaries may still be valid",
            summary_i64(summary, "match_ended")
        ),
        true,
    );
    results
}

fn check_playtest_markdown(
    markdown_path: Option<&Path>,
    mode: PlaytestPreflightMode,
    summary: &BTreeMap<String, Value>,
    min_players: i64,
) -> Vec<PlaytestCheckResult> {
    let mut results = Vec::new();
    let Some(markdown_path) = markdown_path else {
        add_playtest_check(
            &mut results,
            "markdown_checked",
            false,
            "no --markdown file supplied; committed summary privacy/placeholders not checked",
            true,
        );
        return results;
    };
    if !markdown_path.exists() {
        add_playtest_check(
            &mut results,
            "markdown_exists",
            false,
            format!("{} does not exist", markdown_path.display()),
            false,
        );
        return results;
    }

    let text = fs::read_to_string(markdown_path).unwrap_or_default();
    let relative = relative_label(markdown_path);
    add_playtest_check(
        &mut results,
        "markdown_exists",
        true,
        relative.clone(),
        false,
    );
    add_playtest_check(
        &mut results,
        "markdown_location",
        relative.starts_with("docs/playtests/"),
        relative.clone(),
        false,
    );
    let missing_sections: Vec<_> = PLAYTEST_REQUIRED_MARKDOWN_SECTIONS
        .iter()
        .filter(|section| !text.contains(&format!("## {section}")))
        .copied()
        .collect();
    add_playtest_check(
        &mut results,
        "markdown_required_sections",
        missing_sections.is_empty(),
        if missing_sections.is_empty() {
            "all required sections present".to_string()
        } else {
            missing_sections.join(", ")
        },
        false,
    );
    add_header_match_checks(&mut results, &text, summary);

    let risky_hits = risky_markdown_hits(&text);
    add_playtest_check(
        &mut results,
        "markdown_redaction",
        risky_hits.is_empty(),
        if risky_hits.is_empty() {
            "no risky identity/path patterns found".to_string()
        } else {
            risky_hits.join(", ")
        },
        false,
    );
    let has_dry_run_notice = text.contains("Dry-run example only")
        || text.contains("smoke-test telemetry, not a human playtest");
    if mode == PlaytestPreflightMode::DryRun {
        add_playtest_check(
            &mut results,
            "dry_run_notice",
            has_dry_run_notice,
            if has_dry_run_notice {
                "dry-run warning present"
            } else {
                "missing dry-run warning"
            },
            false,
        );
        add_playtest_check(
            &mut results,
            "dry_run_decision",
            Regex::new(r"(?i)P1-024 result:\s*not evaluated\s*\(dry-run example\)")
                .unwrap()
                .is_match(&text),
            "expected not evaluated dry-run decision",
            false,
        );
        add_playtest_check(
            &mut results,
            "dry_run_not_marked_human_result",
            !Regex::new(r"(?i)P1-024 result:\s*(pass|partial|fail)\b")
                .unwrap()
                .is_match(&text),
            "no pass/partial/fail P1-024 result found",
            false,
        );
        return results;
    }

    add_playtest_check(
        &mut results,
        "no_dry_run_marker",
        !has_dry_run_notice,
        "human summary must not contain dry-run warning",
        false,
    );
    let decision_regex =
        Regex::new(r"(?i)P1-024 result:\s*(pass|partial|fail)\b").expect("valid regex");
    let decision = decision_regex
        .captures(&text)
        .and_then(|captures| captures.get(1))
        .map(|match_| match_.as_str().to_lowercase());
    add_playtest_check(
        &mut results,
        "decision_resolved",
        decision.is_some(),
        format!("decision={}", decision.clone().unwrap_or_default()),
        false,
    );
    if decision.as_deref() == Some("pass") {
        let blockers = pass_decision_objective_blockers(summary, min_players);
        add_playtest_check(
            &mut results,
            "pass_decision_matches_objective_gates",
            blockers.is_empty(),
            if blockers.is_empty() {
                "objective pass gates satisfied".to_string()
            } else {
                blockers.join(", ")
            },
            false,
        );
    }
    let signal_gaps = human_signal_markdown_gaps(&text);
    add_playtest_check(
        &mut results,
        "human_playability_signals_resolved",
        signal_gaps.is_empty(),
        if signal_gaps.is_empty() {
            "objective comprehension, confusion, social incident, repeat-play intent, and keep/cut/change evidence are filled".to_string()
        } else {
            signal_gaps.join(", ")
        },
        false,
    );
    let gp09_gaps = gp09_signal_markdown_gaps(&text);
    add_playtest_check(
        &mut results,
        "gp09_comprehension_accessibility_signals_resolved",
        gp09_gaps.is_empty(),
        if gp09_gaps.is_empty() {
            "objective, next-step, failure-state, UI/control, accessibility, and text/term evidence are filled".to_string()
        } else {
            gp09_gaps.join(", ")
        },
        false,
    );
    let unresolved = unresolved_markdown_hits(&text);
    add_playtest_check(
        &mut results,
        "no_unresolved_placeholders",
        unresolved.is_empty(),
        if unresolved.is_empty() {
            "key placeholders resolved".to_string()
        } else {
            unresolved.join(", ")
        },
        false,
    );
    results
}

fn add_header_match_checks(
    results: &mut Vec<PlaytestCheckResult>,
    text: &str,
    summary: &BTreeMap<String, Value>,
) {
    let expected = [
        ("RunId", summary_string(summary, "run_id", "")),
        ("Build", summary_string(summary, "build_id", "")),
        ("Map", summary_string(summary, "map_id", "")),
        ("Profile", summary_string(summary, "profile", "")),
        (
            "Actual players",
            summary_string(summary, "players_connected", ""),
        ),
    ];
    let mismatches: Vec<_> = expected
        .iter()
        .filter_map(|(label, expected)| {
            let actual = markdown_header_value(text, label);
            (actual.as_deref() != Some(expected.as_str()))
                .then(|| format!("{label}: expected {expected:?}, got {actual:?}"))
        })
        .collect();
    add_playtest_check(
        results,
        "markdown_header_matches_summary",
        mismatches.is_empty(),
        if mismatches.is_empty() {
            "header values match summary".to_string()
        } else {
            mismatches.join("; ")
        },
        false,
    );
}

fn markdown_header_value(text: &str, label: &str) -> Option<String> {
    text.lines().find_map(|line| {
        line.strip_prefix(&format!("{label}:"))
            .map(|value| value.trim().to_string())
    })
}

fn human_signal_markdown_gaps(text: &str) -> Vec<String> {
    PLAYTEST_HUMAN_SIGNAL_ROWS
        .iter()
        .filter_map(|(name, label)| markdown_signal_gap(text, name, label, 1))
        .collect()
}

fn gp09_signal_markdown_gaps(text: &str) -> Vec<String> {
    PLAYTEST_GP09_SIGNAL_ROWS
        .iter()
        .filter_map(|(name, label)| markdown_signal_gap(text, name, label, 1))
        .collect()
}

fn markdown_signal_gap(
    text: &str,
    signal_name: &str,
    row_label: &str,
    evidence_column: usize,
) -> Option<String> {
    let Some(cells) = markdown_table_row_by_label(text, row_label) else {
        return Some(format!("{signal_name}: missing row for {row_label}"));
    };
    let evidence = cells.get(evidence_column).map(String::as_str).unwrap_or("");
    (!markdown_cell_has_evidence(evidence))
        .then(|| format!("{signal_name}: missing evidence for {row_label}"))
}

fn markdown_table_row_by_label(text: &str, label: &str) -> Option<Vec<String>> {
    let expected = normalize_markdown_cell(label);
    text.lines().filter_map(markdown_table_cells).find(|cells| {
        cells
            .first()
            .is_some_and(|cell| normalize_markdown_cell(cell) == expected)
    })
}

fn markdown_table_cells(line: &str) -> Option<Vec<String>> {
    let trimmed = line.trim();
    if !trimmed.starts_with('|') || !trimmed.ends_with('|') {
        return None;
    }
    let mut cells = Vec::new();
    let mut cell = String::new();
    let mut escaped = false;
    for character in trimmed.chars() {
        if character == '\\' && !escaped {
            escaped = true;
            cell.push(character);
            continue;
        }
        if character == '|' && !escaped {
            cells.push(cell.trim().to_string());
            cell.clear();
        } else {
            cell.push(character);
        }
        escaped = false;
    }
    cells.push(cell.trim().to_string());

    if cells.first().is_some_and(|cell| cell.is_empty()) {
        cells.remove(0);
    }
    if cells.last().is_some_and(|cell| cell.is_empty()) {
        cells.pop();
    }
    if cells.is_empty() || cells.iter().all(|cell| markdown_separator_cell(cell)) {
        return None;
    }
    Some(cells)
}

fn markdown_separator_cell(cell: &str) -> bool {
    let trimmed = cell.trim();
    !trimmed.is_empty()
        && trimmed
            .chars()
            .all(|character| character == '-' || character == ':' || character.is_whitespace())
}

fn markdown_cell_has_evidence(cell: &str) -> bool {
    let trimmed = cell.trim();
    if trimmed.is_empty() {
        return false;
    }
    let normalized = normalize_markdown_cell(trimmed);
    if ["placeholder", "fill from", "not evaluated"]
        .iter()
        .any(|marker| normalized.contains(marker))
    {
        return false;
    }
    !matches!(
        normalized.as_str(),
        "-" | "tbd"
            | "todo"
            | "n/a"
            | "na"
            | "yes / no"
            | "yes / partial / no"
            | "pass / partial / fail"
            | "keep / cut / change"
            | "critical / high / medium / low"
    )
}

fn normalize_markdown_cell(cell: &str) -> String {
    cell.trim()
        .trim_matches('`')
        .split_whitespace()
        .collect::<Vec<_>>()
        .join(" ")
        .to_ascii_lowercase()
}

fn risky_markdown_hits(text: &str) -> Vec<String> {
    [
        ("local user path", r"/Users/[^/\s|`]+"),
        ("IPv4 address", r"\b(?:\d{1,3}\.){3}\d{1,3}\b"),
        ("SteamID64-like value", r"\b7656119\d{10}\b"),
        (
            "email address",
            r"\b[A-Za-z0-9._%+-]+@[A-Za-z0-9.-]+\.[A-Za-z]{2,}\b",
        ),
        (
            "secret-like assignment",
            r"(?i)\b(?:token|secret|password|apikey|api_key)\s*[:=]\s*[^,\s|`]+",
        ),
    ]
    .iter()
    .filter_map(|(label, pattern)| {
        Regex::new(pattern)
            .expect("valid regex")
            .is_match(text)
            .then(|| (*label).to_string())
    })
    .collect()
}

fn unresolved_markdown_hits(text: &str) -> Vec<String> {
    [
        (
            "unresolved P1-024 decision",
            r"(?i)P1-024 result:\s*pass\s*/\s*partial\s*/\s*fail",
        ),
        ("choose-one instruction still present", r"(?i)Choose one:"),
        ("observer-required placeholder", r"(?i)observer required"),
        ("fill-from placeholder", r"(?i)Fill from"),
        (
            "keep/cut/change placeholder",
            r"(?i)\|\s*keep\s*/\s*cut\s*/\s*change\s*\|",
        ),
        (
            "recording-consent placeholder",
            r"(?i)Recording consent:\s*yes\s*/\s*no",
        ),
        (
            "severity placeholder",
            r"(?i)critical\s*/\s*high\s*/\s*medium\s*/\s*low",
        ),
        ("unchecked data-handling item", r"(?m)^- \[ \] "),
    ]
    .iter()
    .filter_map(|(label, pattern)| {
        Regex::new(pattern)
            .expect("valid regex")
            .is_match(text)
            .then(|| (*label).to_string())
    })
    .collect()
}

fn pass_decision_objective_blockers(
    summary: &BTreeMap<String, Value>,
    min_players: i64,
) -> Vec<String> {
    let (players, _crew, saboteurs) = playtest_role_counts(summary);
    let expected = players.and_then(expected_saboteurs_for_players);
    let mut blockers = Vec::new();
    if summary_i64(summary, "players_connected") != min_players {
        blockers.push(format!(
            "players_connected={} != {min_players}",
            summary_i64(summary, "players_connected")
        ));
    }
    if summary_i64(summary, "match_started") <= 0 {
        blockers.push("match_started=0".to_string());
    }
    if players.is_none() || saboteurs.is_none() {
        blockers.push("role assignment missing".to_string());
    } else if saboteurs != expected {
        blockers.push(format!("saboteurs={:?}, expected={expected:?}", saboteurs));
    }
    if summary_i64(summary, "ship_task_repairs") + summary_i64(summary, "ship_task_sabotages") <= 0
    {
        blockers.push("no repair/sabotage evidence".to_string());
    }
    if playtest_interaction_count(summary) <= 0 {
        blockers.push("no interaction evidence".to_string());
    }
    if summary_i64(summary, "invalid_lines") != 0 {
        blockers.push(format!(
            "invalid_lines={}",
            summary_i64(summary, "invalid_lines")
        ));
    }
    if summary_i64(summary, "errors") != 0 {
        blockers.push(format!("errors={}", summary_i64(summary, "errors")));
    }
    blockers
}

fn smoke_or_qa_events(summary: &BTreeMap<String, Value>) -> Vec<String> {
    summary
        .get("events_by_type")
        .and_then(Value::as_object)
        .map(|events| {
            let mut names: Vec<_> = events
                .keys()
                .filter(|key| key.starts_with("dev_smoke_") || key.starts_with("qa_"))
                .cloned()
                .collect();
            names.sort();
            names
        })
        .unwrap_or_default()
}

fn playtest_role_counts(
    summary: &BTreeMap<String, Value>,
) -> (Option<i64>, Option<i64>, Option<i64>) {
    let assignment = summary
        .get("last_role_assignment")
        .and_then(Value::as_object);
    let players = assignment
        .and_then(|assignment| assignment.get("players"))
        .and_then(value_to_i64);
    let saboteurs = assignment
        .and_then(|assignment| assignment.get("saboteurs"))
        .and_then(value_to_i64);
    let crew = players
        .zip(saboteurs)
        .map(|(players, saboteurs)| (players - saboteurs).max(0));
    (players, crew, saboteurs)
}

fn expected_saboteurs_for_players(player_count: i64) -> Option<i64> {
    if player_count >= 7 {
        Some(2)
    } else if player_count >= 5 {
        Some(1)
    } else if player_count >= 0 {
        Some(0)
    } else {
        None
    }
}

fn playtest_interaction_count(summary: &BTreeMap<String, Value>) -> i64 {
    [
        "ship_task_repairs",
        "ship_task_sabotages",
        "door_toggles",
        "door_locks",
        "door_unlocks",
        "item_pickups",
        "item_drops",
        "players_downed",
        "players_rescued",
        "players_contained",
        "players_released",
    ]
    .iter()
    .map(|key| summary_i64(summary, key))
    .sum()
}

fn nested_summary_value<'a>(summary: &'a BTreeMap<String, Value>, path: &str) -> Option<&'a Value> {
    let mut parts = path.split('.');
    let first = parts.next()?;
    let mut current = summary.get(first)?;
    for part in parts {
        current = current.as_object()?.get(part)?;
    }
    Some(current)
}

fn markdown_text_value(value: Option<&Value>) -> String {
    match value {
        None | Some(Value::Null) => String::new(),
        Some(Value::Bool(value)) => {
            if *value {
                "true".to_string()
            } else {
                "false".to_string()
            }
        }
        Some(Value::Number(number)) => number.to_string(),
        Some(Value::String(text)) => sanitize_markdown(text),
        Some(value @ (Value::Array(_) | Value::Object(_))) => {
            format!(
                "`{}`",
                serde_json::to_string(value).expect("value serializes")
            )
        }
    }
}

fn summary_text(summary: &BTreeMap<String, Value>, key: &str) -> String {
    markdown_text_value(summary.get(key))
}

fn bool_result(value: bool) -> &'static str {
    if value { "yes" } else { "no" }
}

fn bool_option_result(value: bool) -> &'static str {
    bool_result(value)
}

fn option_bool_result(value: Option<bool>) -> &'static str {
    match value {
        None => "observer required",
        Some(true) => "yes",
        Some(false) => "no",
    }
}

fn status_result(value: bool, partial: bool) -> &'static str {
    if value {
        "pass"
    } else if partial {
        "partial"
    } else {
        "fail"
    }
}

fn sanitize_markdown(value: &str) -> String {
    let mut sanitized = value.replace(['\n', '\r'], " ");
    for pattern in [
        r"/Users/[^/\s|`]+",
        r"\b(?:\d{1,3}\.){3}\d{1,3}\b",
        r"\b7656119\d{10}\b",
        r"\b[A-Za-z0-9._%+-]+@[A-Za-z0-9.-]+\.[A-Za-z]{2,}\b",
        r"(?i)\b(?:token|secret|password|apikey|api_key)\s*[:=]\s*[^,\s|`]+",
    ] {
        sanitized = Regex::new(pattern)
            .expect("valid regex")
            .replace_all(&sanitized, "<redacted>")
            .into_owned();
    }
    sanitized.replace('|', "\\|")
}

fn relative_label(path: &Path) -> String {
    let resolved = resolve_path(path);
    let root = repo_root();
    resolved
        .strip_prefix(&root)
        .map(|path| path.to_string_lossy().replace('\\', "/"))
        .unwrap_or_else(|_| resolved.display().to_string())
}

#[derive(Clone, Copy)]
struct PlaytestRunScaffoldOptions<'a> {
    run_number: i64,
    target_players: i64,
    port: i64,
    res_x: i64,
    res_y: i64,
    build_id: &'a str,
    map_id: &'a str,
    profile: &'a str,
    ue_root: &'a str,
    ue_editor: &'a str,
}

struct GeneratedFile {
    path: PathBuf,
    content: String,
}

struct PlaytestRunScaffold {
    run_dir: PathBuf,
    directories: Vec<PathBuf>,
    files: Vec<GeneratedFile>,
}

fn build_playtest_run_scaffold(
    options: PlaytestRunScaffoldOptions<'_>,
) -> Result<PlaytestRunScaffold, String> {
    let slug = playtest_run_slug(options.run_number)?;
    if options.target_players != 8 {
        return Err("--target-players must be 8 for normal human playtests".to_string());
    }
    let run_id = format!("P1-024-{slug}");
    let run_dir = PathBuf::from(format!("Saved/Playtests/P1-024/{slug}"));
    let client_port = options.port + 1;
    let today = "2026-05-25";

    let files = vec![
        GeneratedFile {
            path: run_dir.join("README.md"),
            content: playtest_run_card(&run_id, &slug, options, today),
        },
        GeneratedFile {
            path: run_dir.join("commands.md"),
            content: playtest_commands_md(&run_id, &slug, options),
        },
        GeneratedFile {
            path: run_dir.join("host-notes.md"),
            content: playtest_host_notes(&run_id, options.target_players),
        },
        GeneratedFile {
            path: run_dir.join("player-brief.md"),
            content: playtest_player_brief(),
        },
        GeneratedFile {
            path: run_dir.join("preflight.ps1"),
            content: playtest_preflight_ps1(options.ue_root),
        },
        GeneratedFile {
            path: run_dir.join("host.ps1"),
            content: playtest_host_ps1(&run_id, &slug, options),
        },
        GeneratedFile {
            path: run_dir.join("client-local.ps1"),
            content: playtest_client_local_ps1(
                options.port,
                client_port,
                options.ue_root,
                options.res_x,
                options.res_y,
            ),
        },
        GeneratedFile {
            path: run_dir.join("client-lan.ps1"),
            content: playtest_client_lan_ps1(
                options.port,
                options.ue_root,
                options.res_x,
                options.res_y,
            ),
        },
        GeneratedFile {
            path: run_dir.join("after-test.ps1"),
            content: playtest_after_test_ps1(&slug, &run_id),
        },
        GeneratedFile {
            path: run_dir.join("preflight.sh"),
            content: playtest_preflight_sh(),
        },
        GeneratedFile {
            path: run_dir.join("host.sh"),
            content: playtest_host_sh(&run_id, &slug, options),
        },
        GeneratedFile {
            path: run_dir.join("client-local.sh"),
            content: playtest_client_local_sh(
                options.port,
                client_port,
                options.ue_editor,
                options.res_x,
                options.res_y,
            ),
        },
        GeneratedFile {
            path: run_dir.join("client-lan.sh"),
            content: playtest_client_lan_sh(
                options.port,
                options.ue_editor,
                options.res_x,
                options.res_y,
            ),
        },
        GeneratedFile {
            path: run_dir.join("after-test.sh"),
            content: playtest_after_test_sh(&slug),
        },
    ];

    Ok(PlaytestRunScaffold {
        run_dir,
        directories: vec![
            PathBuf::from(format!("Saved/Playtests/P1-024/{slug}/recordings")),
            PathBuf::from(format!("Saved/Playtests/P1-024/{slug}/screenshots")),
            PathBuf::from(format!("Saved/Playtests/P1-024/{slug}/crash-notes")),
        ],
        files,
    })
}

fn write_playtest_run_scaffold(scaffold: &PlaytestRunScaffold, force: bool) -> Result<(), String> {
    let root = repo_root();
    for directory in &scaffold.directories {
        fs::create_dir_all(root.join(directory))
            .map_err(|error| format!("{}: {error}", directory.display()))?;
    }
    for file in &scaffold.files {
        let path = root.join(&file.path);
        if path.exists() && !force {
            return Err(format!(
                "{} already exists; use --force to overwrite generated scaffold files",
                file.path.display()
            ));
        }
        if let Some(parent) = path.parent() {
            fs::create_dir_all(parent).map_err(|error| format!("{}: {error}", parent.display()))?;
        }
        fs::write(&path, &file.content).map_err(|error| format!("{}: {error}", path.display()))?;
    }
    Ok(())
}

fn playtest_run_slug(run_number: i64) -> Result<String, String> {
    if !(1..=999).contains(&run_number) {
        return Err("--run-number must be between 1 and 999".to_string());
    }
    Ok(format!("run-{run_number:02}"))
}

fn playtest_powershell_common_prelude() -> &'static str {
    r#"$ErrorActionPreference = "Stop"

$ScriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$RepoRoot = (Resolve-Path (Join-Path $ScriptDir "..\..\..\..")).Path
Set-Location $RepoRoot

if (-not $env:CARGO_TARGET_DIR -or $env:CARGO_TARGET_DIR.Trim() -eq "") {
  $env:CARGO_TARGET_DIR = Join-Path $RepoRoot "target\playtest-tools"
}

$CargoCommand = Get-Command cargo -ErrorAction SilentlyContinue
if ($CargoCommand) {
  $Cargo = $CargoCommand.Source
} else {
  $RepoCargo = Join-Path $RepoRoot "Tools\install\rust\cargo\bin\cargo.exe"
  $UserCargo = Join-Path $env:USERPROFILE ".cargo\bin\cargo.exe"
  if (Test-Path $RepoCargo) {
    $Cargo = $RepoCargo
  } elseif (Test-Path $UserCargo) {
    $Cargo = $UserCargo
  } else {
    throw "cargo.exe not found. Install Rust, add cargo to PATH, or install the repo-local Rust toolchain under Tools\install\rust."
  }
}

function Invoke-Cargo {
  & $Cargo @args
  if ($LASTEXITCODE -ne 0) {
    throw "cargo $($args -join ' ') failed with exit code $LASTEXITCODE"
  }
}
"#
}

fn playtest_powershell_prelude(ue_root: &str) -> String {
    format!(
        r#"{common}
if (-not $env:UE_ROOT -or $env:UE_ROOT.Trim() -eq "") {{
  $env:UE_ROOT = "{ue_root}"
}}

$UEEditor = Join-Path $env:UE_ROOT "Engine\Binaries\Win64\UnrealEditor.exe"
if (-not (Test-Path $UEEditor)) {{
  throw "UnrealEditor.exe not found at $UEEditor. Set UE_ROOT to your Unreal Engine install."
}}
"#,
        common = playtest_powershell_common_prelude(),
        ue_root = ue_root,
    )
}

fn playtest_shell_prelude() -> &'static str {
    r#"#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
REPO_ROOT="$(cd "$SCRIPT_DIR/../../../.." && pwd)"
cd "$REPO_ROOT"

export CARGO_TARGET_DIR="${CARGO_TARGET_DIR:-$REPO_ROOT/target/playtest-tools}"

CARGO_BIN="${CARGO_BIN:-$(command -v cargo || true)}"
if [[ -z "$CARGO_BIN" ]]; then
  for candidate in "$REPO_ROOT/Tools/install/rust/cargo/bin/cargo" "$HOME/.cargo/bin/cargo"; do
    if [[ -x "$candidate" ]]; then
      CARGO_BIN="$candidate"
      break
    fi
  done
fi
if [[ -z "$CARGO_BIN" ]]; then
  echo "cargo not found. Install Rust, add cargo to PATH, or set CARGO_BIN." >&2
  exit 1
fi
"#
}

fn playtest_preflight_ps1(ue_root: &str) -> String {
    format!(
        "{}\nInvoke-Cargo run -p frostwake-tools -- quality-gate --require-ue\nInvoke-Cargo run -p frostwake-tools -- unreal-gate --skip-generate --platform Win64 --include-server\nInvoke-Cargo run -p frostwake-tools -- run-local-smoke --profile ready8 --skip-build --null-rhi --platform Win64\n",
        playtest_powershell_prelude(ue_root)
    )
}

fn playtest_host_ps1(run_id: &str, slug: &str, options: PlaytestRunScaffoldOptions<'_>) -> String {
    format!(
        r#"{prelude}
& $UEEditor `
  "$RepoRoot\Frostwake.uproject" `
  "{map_id}?listen" `
  -game `
  -windowed `
  -ResX={res_x} `
  -ResY={res_y} `
  -NoLiveCoding `
  -nop4 `
  -port={port} `
  "-FrostwakeEventLog=$RepoRoot\Saved\Playtests\P1-024\{slug}\events.jsonl" `
  -FrostwakeRunId={run_id} `
  "-FrostwakeBuildId={build_id}" `
  "-FrostwakeMapId={map_id}" `
  -FrostwakeProfile={profile} `
  -FrostwakeAutoReady `
  -FrostwakeLobbyMinPlayers={target_players}
"#,
        prelude = playtest_powershell_prelude(options.ue_root),
        map_id = options.map_id,
        res_x = options.res_x,
        res_y = options.res_y,
        port = options.port,
        slug = slug,
        run_id = run_id,
        build_id = options.build_id,
        profile = options.profile,
        target_players = options.target_players,
    )
}

fn playtest_client_local_ps1(
    port: i64,
    client_port: i64,
    ue_root: &str,
    res_x: i64,
    res_y: i64,
) -> String {
    format!(
        r#"{prelude}
& $UEEditor `
  "$RepoRoot\Frostwake.uproject" `
  "127.0.0.1:{port}" `
  -game `
  -windowed `
  -ResX={res_x} `
  -ResY={res_y} `
  -NoLiveCoding `
  -nop4 `
  -port={client_port}
"#,
        prelude = playtest_powershell_prelude(ue_root),
    )
}

fn playtest_client_lan_ps1(port: i64, ue_root: &str, res_x: i64, res_y: i64) -> String {
    format!(
        r#"param(
  [Parameter(Mandatory = $true)]
  [string]$HostLanIp
)

{prelude}
& $UEEditor `
  "$RepoRoot\Frostwake.uproject" `
  "$($HostLanIp):{port}" `
  -game `
  -windowed `
  -ResX={res_x} `
  -ResY={res_y} `
  -NoLiveCoding `
  -nop4
"#,
        prelude = playtest_powershell_prelude(ue_root),
    )
}

fn playtest_after_test_ps1(slug: &str, run_id: &str) -> String {
    format!(
        r#"{prelude}
Invoke-Cargo run -p frostwake-tools -- log-summary Saved\Playtests\P1-024\{slug}\events.jsonl
Invoke-Cargo run -p frostwake-tools -- log-summary Saved\Playtests\P1-024\{slug}\events.jsonl --out Saved\Playtests\P1-024\{slug}\summary.json
Invoke-Cargo run -p frostwake-tools -- playtest-summary Saved\Playtests\P1-024\{slug}\summary.json --out docs\playtests\p1-024-{slug}-summary.md
Invoke-Cargo run -p frostwake-tools -- playtest-preflight Saved\Playtests\P1-024\{slug}\summary.json --markdown docs\playtests\p1-024-{slug}-summary.md
Invoke-Cargo run -p frostwake-tools -- playtest-report-upload Saved\Playtests\P1-024\{slug}\summary.json

Write-Host "Review and anonymize docs\playtests\p1-024-{slug}-summary.md before committing."
Write-Host "Review the backend report payload dry-run before using playtest-report-upload --send."
Write-Host "RunId: {run_id}"
"#,
        prelude = playtest_powershell_common_prelude(),
        slug = slug,
        run_id = run_id,
    )
}

fn playtest_preflight_sh() -> String {
    format!(
        "{}\n\"$CARGO_BIN\" run -p frostwake-tools -- quality-gate --require-ue\n\"$CARGO_BIN\" run -p frostwake-tools -- unreal-gate --skip-generate --include-server\n\"$CARGO_BIN\" run -p frostwake-tools -- run-local-smoke --profile ready8 --skip-build --null-rhi\n",
        playtest_shell_prelude()
    )
}

fn playtest_host_sh(run_id: &str, slug: &str, options: PlaytestRunScaffoldOptions<'_>) -> String {
    format!(
        r#"{prelude}
UE_EDITOR="{ue_editor}"

"$UE_EDITOR" \
  "$REPO_ROOT/Frostwake.uproject" \
  "{map_id}?listen" \
  -game \
  -windowed \
  -ResX={res_x} \
  -ResY={res_y} \
  -NoLiveCoding \
  -nop4 \
  -port={port} \
  -FrostwakeEventLog="$REPO_ROOT/Saved/Playtests/P1-024/{slug}/events.jsonl" \
  -FrostwakeRunId={run_id} \
  -FrostwakeBuildId="{build_id}" \
  -FrostwakeMapId="{map_id}" \
  -FrostwakeProfile={profile} \
  -FrostwakeAutoReady \
  -FrostwakeLobbyMinPlayers={target_players}
"#,
        prelude = playtest_shell_prelude(),
        ue_editor = options.ue_editor,
        map_id = options.map_id,
        res_x = options.res_x,
        res_y = options.res_y,
        port = options.port,
        slug = slug,
        run_id = run_id,
        build_id = options.build_id,
        profile = options.profile,
        target_players = options.target_players,
    )
}

fn playtest_client_local_sh(
    port: i64,
    client_port: i64,
    ue_editor: &str,
    res_x: i64,
    res_y: i64,
) -> String {
    format!(
        r#"{prelude}
UE_EDITOR="{ue_editor}"

"$UE_EDITOR" \
  "$REPO_ROOT/Frostwake.uproject" \
  "127.0.0.1:{port}" \
  -game \
  -windowed \
  -ResX={res_x} \
  -ResY={res_y} \
  -NoLiveCoding \
  -nop4 \
  -port={client_port}
"#,
        prelude = playtest_shell_prelude(),
    )
}

fn playtest_client_lan_sh(port: i64, ue_editor: &str, res_x: i64, res_y: i64) -> String {
    format!(
        r#"{prelude}
HOST_LAN_IP="${{1:-}}"
if [ -z "$HOST_LAN_IP" ]; then
  echo "Usage: $0 <host-lan-ip>" >&2
  exit 2
fi

UE_EDITOR="{ue_editor}"

"$UE_EDITOR" \
  "$REPO_ROOT/Frostwake.uproject" \
  "$HOST_LAN_IP:{port}" \
  -game \
  -windowed \
  -ResX={res_x} \
  -ResY={res_y} \
  -NoLiveCoding \
  -nop4
"#,
        prelude = playtest_shell_prelude(),
    )
}

fn playtest_after_test_sh(slug: &str) -> String {
    format!(
        r#"{prelude}
"$CARGO_BIN" run -p frostwake-tools -- log-summary Saved/Playtests/P1-024/{slug}/events.jsonl
"$CARGO_BIN" run -p frostwake-tools -- log-summary Saved/Playtests/P1-024/{slug}/events.jsonl --out Saved/Playtests/P1-024/{slug}/summary.json
"$CARGO_BIN" run -p frostwake-tools -- playtest-summary Saved/Playtests/P1-024/{slug}/summary.json --out docs/playtests/p1-024-{slug}-summary.md
"$CARGO_BIN" run -p frostwake-tools -- playtest-preflight Saved/Playtests/P1-024/{slug}/summary.json --markdown docs/playtests/p1-024-{slug}-summary.md
"$CARGO_BIN" run -p frostwake-tools -- playtest-report-upload Saved/Playtests/P1-024/{slug}/summary.json
"#,
        prelude = playtest_shell_prelude(),
    )
}

fn playtest_commands_md(
    run_id: &str,
    slug: &str,
    options: PlaytestRunScaffoldOptions<'_>,
) -> String {
    let client_port = options.port + 1;
    format!(
        r#"# {run_id} Commands

Run every command from the repository root on Windows PowerShell.

## Host Preflight

```powershell
cargo run -p frostwake-tools -- quality-gate --require-ue
cargo run -p frostwake-tools -- unreal-gate --skip-generate --platform Win64 --include-server
cargo run -p frostwake-tools -- run-local-smoke --profile ready8 --skip-build --null-rhi --platform Win64
```

## Environment

Set `UE_ROOT` if Unreal is not installed at the default path.

```powershell
$env:UE_ROOT = "{ue_root}"
```

## Listen Host

```powershell
.\Saved\Playtests\P1-024\{slug}\host.ps1
```

Manual equivalent:

```powershell
$UEEditor = Join-Path $env:UE_ROOT "Engine\Binaries\Win64\UnrealEditor.exe"
& $UEEditor `
  "$PWD\Frostwake.uproject" `
  "{map_id}?listen" `
  -game `
  -windowed `
  -ResX={res_x} `
  -ResY={res_y} `
  -NoLiveCoding `
  -nop4 `
  -port={port} `
  "-FrostwakeEventLog=$PWD\Saved\Playtests\P1-024\{slug}\events.jsonl" `
  -FrostwakeRunId={run_id} `
  "-FrostwakeBuildId={build_id}" `
  "-FrostwakeMapId={map_id}" `
  -FrostwakeProfile={profile} `
  -FrostwakeAutoReady `
  -FrostwakeLobbyMinPlayers={target_players}
```

## Same-Machine Client

```powershell
.\Saved\Playtests\P1-024\{slug}\client-local.ps1
```

Manual equivalent:

```powershell
$UEEditor = Join-Path $env:UE_ROOT "Engine\Binaries\Win64\UnrealEditor.exe"
& $UEEditor `
  "$PWD\Frostwake.uproject" `
  "127.0.0.1:{port}" `
  -game `
  -windowed `
  -ResX={res_x} `
  -ResY={res_y} `
  -NoLiveCoding `
  -nop4 `
  -port={client_port}
```

For additional same-machine clients, increment the client port.

## LAN Client

```powershell
.\Saved\Playtests\P1-024\{slug}\client-lan.ps1 -HostLanIp <host-lan-ip>
```

## After-Test Summary

```powershell
.\Saved\Playtests\P1-024\{slug}\after-test.ps1
```

The after-test script prints a backend report payload dry-run. It does not upload unless `cargo run -p frostwake-tools -- playtest-report-upload` is rerun manually with `--send`.
"#,
        ue_root = options.ue_root,
        map_id = options.map_id,
        res_x = options.res_x,
        res_y = options.res_y,
        port = options.port,
        build_id = options.build_id,
        profile = options.profile,
        target_players = options.target_players,
    )
}

fn playtest_run_card(
    run_id: &str,
    slug: &str,
    options: PlaytestRunScaffoldOptions<'_>,
    date: &str,
) -> String {
    let expected_agents = if options.target_players >= 7 { 2 } else { 1 };
    let expected_crew = (options.target_players - expected_agents).max(0);
    format!(
        r#"# {run_id} Run Card

Date prepared: {date}

This directory is local-only raw playtest evidence. It is under `Saved/Playtests/...`, which is ignored by git. Do not copy raw logs, recordings, tester names, call links, host IPs, or transcripts into committed files.

## Header

```text
RunId: {run_id}
Build: {build_id}
Commit or local snapshot:
Host machine:
Host LAN IP:
Port: {port}
Map: {map_id}
Profile: {profile}
Target players: {target_players}
Expected crew: {expected_crew}
Expected agents: {expected_agents}
Voice method:
Recording consent: yes / no
Observer A, timing and logs:
Observer B, comprehension and social dynamics:
```

## Before Launch

Run from the repository root on Windows:

```powershell
cargo run -p frostwake-tools -- quality-gate --require-ue
cargo run -p frostwake-tools -- unreal-gate --skip-generate --platform Win64 --include-server
cargo run -p frostwake-tools -- run-local-smoke --profile ready8 --skip-build --null-rhi --platform Win64
```

## During Run

- Keep observer notes in `host-notes.md`.
- Keep raw logs and recordings in this local directory only.
- Do not coach players mid-round unless the match is blocked.
- Do not reveal roles.
- Do not score proximity-voice quality from this Phase 1 run.

## After Run

```powershell
.\Saved\Playtests\P1-024\{slug}\after-test.ps1
```

This also prints a backend report payload dry-run for review. It does not upload data.
"#,
        build_id = options.build_id,
        port = options.port,
        map_id = options.map_id,
        profile = options.profile,
        target_players = options.target_players,
    )
}

fn playtest_host_notes(run_id: &str, target_players: i64) -> String {
    format!(
        r#"# {run_id} Host Notes

Keep this file local. Commit only the anonymized summary after preflight passes.

## Run Header

```text
RunId: {run_id}
Target players: {target_players}
Actual players:
Voice method:
Recording consent:
Observer A:
Observer B:
```

## Timeline

| Time | Event | Notes |
| --- | --- | --- |
|  | First goal comprehension |  |
|  | First repair or sabotage |  |
|  | First accusation |  |
|  | First down, containment, or rescue |  |
|  | Biggest confusion |  |
|  | Best social moment |  |
|  | Worst stall |  |
|  | End or interruption |  |

## Comprehension And Accessibility

Record observations during the run and confirm them in the post-round survey.

| Signal | Observation |
| --- | --- |
| Objective comprehension |  |
| Next-step clarity |  |
| Failure-state clarity |  |
| UI or control confusion |  |
| Accessibility blocker |  |
| Text or term issue |  |
"#
    )
}

fn playtest_player_brief() -> String {
    r#"# Player Brief

Read once before the round.

```text
This is a greybox prototype. Visuals and UI are temporary.

Crew: keep the ship alive and progress the route. Repair systems, share information, and watch for suspicious behavior.

Agent: prevent the ship from succeeding. Use sabotage, timing, isolation, and false explanations. Do not simply hide forever; create situations other players can discuss.

Everyone: speak naturally, call out discoveries, and say what you think happened. If something is confusing, say it out loud rather than silently waiting.
```
"#
    .to_string()
}

#[derive(Debug, Eq, PartialEq)]
struct SecretFinding {
    path: PathBuf,
    line: usize,
    text: String,
}

fn scan_for_secrets(root: &Path) -> Result<Vec<SecretFinding>, String> {
    let root = root
        .canonicalize()
        .map_err(|error| format!("{}: {error}", root.display()))?;
    let secret_pattern = secret_pattern();
    let mut findings = Vec::new();
    scan_path_for_secrets(&root, &root, &secret_pattern, &mut findings)?;
    findings.sort_by(|left, right| left.path.cmp(&right.path).then(left.line.cmp(&right.line)));
    Ok(findings)
}

fn scan_path_for_secrets(
    root: &Path,
    path: &Path,
    secret_pattern: &Regex,
    findings: &mut Vec<SecretFinding>,
) -> Result<(), String> {
    if should_skip_secret_scan_path(root, path) {
        return Ok(());
    }

    let metadata = fs::metadata(path).map_err(|error| format!("{}: {error}", path.display()))?;
    if metadata.is_dir() {
        let entries = fs::read_dir(path).map_err(|error| format!("{}: {error}", path.display()))?;
        for entry in entries {
            let entry = entry.map_err(|error| format!("{}: {error}", path.display()))?;
            scan_path_for_secrets(root, &entry.path(), secret_pattern, findings)?;
        }
        return Ok(());
    }

    if !metadata.is_file() {
        return Ok(());
    }

    let bytes = fs::read(path).map_err(|error| format!("{}: {error}", path.display()))?;
    let text = String::from_utf8_lossy(&bytes);
    for (index, line) in text.lines().enumerate() {
        if secret_pattern.is_match(line) {
            findings.push(SecretFinding {
                path: path
                    .strip_prefix(root)
                    .map(Path::to_path_buf)
                    .unwrap_or_else(|_| path.to_path_buf()),
                line: index + 1,
                text: line.trim().to_string(),
            });
        }
    }

    Ok(())
}

fn secret_pattern() -> Regex {
    let pieces = [
        concat!("EXTERNAL_CHAT_BOT_TOKEN", "=", ".+"),
        concat!("SecurityToken", "=", "[A-Fa-f0-9]{24,}"),
        concat!("aws", "_access", "_key", "_id"),
        concat!("aws", "_secret", "_access", "_key"),
        concat!("BEGIN ", "(RSA|OPENSSH|PRIVATE)", " KEY"),
        "xox[baprs]-",
        concat!("ghp_", "[A-Za-z0-9_]+"),
    ];
    Regex::new(&pieces.join("|")).expect("valid secret regex")
}

fn should_skip_secret_scan_path(root: &Path, path: &Path) -> bool {
    let Ok(relative) = path.strip_prefix(root) else {
        return true;
    };
    let relative_text = relative.to_string_lossy().replace('\\', "/");
    if relative_text.is_empty() {
        return false;
    }
    if relative_text.ends_with(".lock") {
        return true;
    }
    let excluded_prefixes = [
        ".git",
        ".venv",
        "Binaries",
        "DerivedDataCache",
        "Intermediate",
        "Saved",
        "Script",
        "references/private",
        "references/inventory",
        "Tools/install",
        "target",
    ];
    if excluded_prefixes
        .iter()
        .any(|prefix| relative_text == *prefix || relative_text.starts_with(&format!("{prefix}/")))
    {
        return true;
    }

    let lower = relative_text.to_ascii_lowercase();
    let binary_suffixes = [
        ".7z", ".bin", ".db", ".dll", ".dmg", ".exe", ".jpg", ".jpeg", ".log", ".mp4", ".pak",
        ".pdb", ".png", ".sig", ".uasset", ".ucas", ".umap", ".utoc", ".wav", ".zip",
    ];
    binary_suffixes.iter().any(|suffix| lower.ends_with(suffix))
}

#[derive(Clone, Debug)]
struct GateResult {
    name: String,
    status: String,
    detail: String,
}

fn gate_result(
    name: impl Into<String>,
    status: impl Into<String>,
    detail: impl Into<String>,
) -> GateResult {
    GateResult {
        name: name.into(),
        status: status.into(),
        detail: detail.into(),
    }
}

fn emit_check_results(results: &[GateResult], json_output: bool) {
    if json_output {
        let payload: Vec<Value> = results
            .iter()
            .map(|result| {
                json!({
                    "name": result.name,
                    "status": result.status,
                    "detail": result.detail,
                })
            })
            .collect();
        println!(
            "{}",
            serde_json::to_string_pretty(&payload).expect("results serialize")
        );
        return;
    }

    for result in results {
        println!(
            "[{}] {}: {}",
            result.status.to_ascii_uppercase(),
            result.name,
            result.detail
        );
    }
}

fn command_output_text(output: &std::process::Output) -> String {
    let stdout = String::from_utf8_lossy(&output.stdout).trim().to_string();
    let stderr = String::from_utf8_lossy(&output.stderr).trim().to_string();
    [stdout, stderr]
        .into_iter()
        .filter(|part| !part.is_empty())
        .collect::<Vec<_>>()
        .join("\n")
}

fn run_process(name: &str, command: &[String], envs: Option<&[(String, String)]>) -> GateResult {
    if command.is_empty() {
        return gate_result(name, "fail", "empty command");
    }
    let mut process = ProcessCommand::new(&command[0]);
    process.args(&command[1..]).current_dir(repo_root());
    if let Some(envs) = envs {
        for (key, value) in envs {
            process.env(key, value);
        }
    }
    match process.output() {
        Ok(output) => {
            let detail = command_output_text(&output);
            if output.status.success() {
                gate_result(
                    name,
                    "pass",
                    if detail.is_empty() {
                        "ok".to_string()
                    } else {
                        detail
                    },
                )
            } else {
                gate_result(
                    name,
                    "fail",
                    if detail.is_empty() {
                        format!("exit code {}", output.status.code().unwrap_or(-1))
                    } else {
                        detail
                    },
                )
            }
        }
        Err(error) => gate_result(name, "fail", format!("{}: {error}", command[0])),
    }
}

fn run_process_blockable(
    name: &str,
    command: &[String],
    blocked_needle: Option<&str>,
) -> GateResult {
    let result = run_process(name, command, None);
    if result.status == "fail"
        && blocked_needle.is_some_and(|needle| result.detail.contains(needle))
    {
        return gate_result(
            name,
            "blocked",
            "Launcher UE distribution does not support Server targets",
        );
    }
    if result.status == "pass" {
        gate_result(name, "pass", summarize_process_output(&result.detail))
    } else {
        gate_result(
            name,
            result.status,
            summarize_process_output(&result.detail),
        )
    }
}

fn summarize_process_output(output: &str) -> String {
    if output.trim().is_empty() {
        return "ok".to_string();
    }
    let important: Vec<&str> = output
        .lines()
        .filter(|line| {
            [
                "Result:",
                "Error:",
                "ERROR:",
                "Failed",
                "Succeeded",
                "Server targets are not currently supported",
            ]
            .iter()
            .any(|token| line.contains(token))
        })
        .collect();
    let lines: Vec<&str> = if important.is_empty() {
        output.lines().collect()
    } else {
        important
    };
    let start = lines.len().saturating_sub(12);
    lines[start..].join("\n")
}

fn local_cargo_path() -> Option<PathBuf> {
    let executable = if cfg!(windows) { "cargo.exe" } else { "cargo" };
    let local = repo_root()
        .join("Tools/install/rust/cargo/bin")
        .join(executable);
    if local.exists() { Some(local) } else { None }
}

fn cargo_command(args: &[&str]) -> (Vec<String>, Vec<(String, String)>) {
    if let Some(cargo) = local_cargo_path() {
        let rust_root = repo_root().join("Tools/install/rust");
        let mut command = vec![cargo.to_string_lossy().to_string()];
        command.extend(args.iter().map(|arg| (*arg).to_string()));
        return (
            command,
            vec![
                (
                    "CARGO_HOME".to_string(),
                    rust_root.join("cargo").to_string_lossy().to_string(),
                ),
                (
                    "RUSTUP_HOME".to_string(),
                    rust_root.join("rustup").to_string_lossy().to_string(),
                ),
            ],
        );
    }
    let mut command = vec!["cargo".to_string()];
    command.extend(args.iter().map(|arg| (*arg).to_string()));
    (command, Vec::new())
}

fn run_cargo_gate(name: &str, args: &[&str]) -> GateResult {
    let (command, envs) = cargo_command(args);
    run_process(name, &command, Some(&envs))
}

fn run_quality_gate(require_ue: bool) -> Vec<GateResult> {
    let config = match load_json_value(Path::new("docs/quality-gates.json")) {
        Ok(Value::Object(config)) => config,
        Ok(_) => {
            return vec![gate_result(
                "quality_config",
                "fail",
                "docs/quality-gates.json must be an object",
            )];
        }
        Err(error) => return vec![gate_result("quality_config", "fail", error)],
    };

    let mut results = vec![
        check_required_files(&config),
        check_json_files(&config),
        check_schema_examples(&config),
        check_no_retired_source_files(),
        check_retired_file_references(&config),
        run_cargo_gate("rust_fmt", &["fmt", "--all", "--check"]),
        run_cargo_gate("rust_tests", &["test", "--workspace"]),
        check_git_ignored(&config),
        check_source_control_safety(),
        check_shell_files(&config),
        check_powershell_files(&config),
        check_stale_terms(&config),
        check_unreal_metadata(),
        check_unreal_cpp_heuristics(),
        check_cycle_records(&config),
    ];

    match validate_asset_ledger(Path::new("docs/asset-ledger-candidates.csv")) {
        Ok(errors) if errors.is_empty() => results.push(gate_result(
            "asset_ledger_check",
            "pass",
            "docs/asset-ledger-candidates.csv",
        )),
        Ok(errors) => results.push(gate_result("asset_ledger_check", "fail", errors.join("\n"))),
        Err(error) => results.push(gate_result("asset_ledger_check", "fail", error)),
    }

    match scan_for_secrets(&repo_root()) {
        Ok(findings) if findings.is_empty() => results.push(gate_result(
            "secret_scan",
            "pass",
            "No obvious secrets found in tracked-safe paths.",
        )),
        Ok(findings) => results.push(gate_result(
            "secret_scan",
            "fail",
            findings
                .iter()
                .take(80)
                .map(|finding| {
                    format!(
                        "{}:{}: {}",
                        finding.path.display(),
                        finding.line,
                        finding.text
                    )
                })
                .collect::<Vec<_>>()
                .join("\n"),
        )),
        Err(error) => results.push(gate_result("secret_scan", "fail", error)),
    }

    results.push(run_process(
        "git_diff_check",
        &["git", "diff", "--check"]
            .iter()
            .map(|s| s.to_string())
            .collect::<Vec<_>>(),
        None,
    ));
    results.push(run_process(
        "git_cached_diff_check",
        &["git", "diff", "--cached", "--check"]
            .iter()
            .map(|s| s.to_string())
            .collect::<Vec<_>>(),
        None,
    ));
    results.push(check_unreal_installed(require_ue));
    results.push(check_cpp_toolchain(require_ue));
    results
}

fn config_array<'a>(config: &'a Map<String, Value>, key: &str) -> Vec<&'a str> {
    config
        .get(key)
        .and_then(Value::as_array)
        .map(|items| items.iter().filter_map(Value::as_str).collect())
        .unwrap_or_default()
}

fn check_required_files(config: &Map<String, Value>) -> GateResult {
    let missing: Vec<String> = config_array(config, "required_files")
        .into_iter()
        .filter(|item| !repo_root().join(item).exists())
        .map(str::to_string)
        .collect();
    if missing.is_empty() {
        gate_result(
            "required_files",
            "pass",
            format!(
                "{} files present",
                config_array(config, "required_files").len()
            ),
        )
    } else {
        gate_result("required_files", "fail", missing.join(", "))
    }
}

fn check_json_files(config: &Map<String, Value>) -> GateResult {
    let mut errors = Vec::new();
    let files = config_array(config, "json_files");
    for item in &files {
        if let Err(error) = load_json_value(Path::new(item)) {
            errors.push(format!("{item}: {error}"));
        }
    }
    if errors.is_empty() {
        gate_result(
            "json_parse",
            "pass",
            format!("{} JSON files parsed", files.len()),
        )
    } else {
        gate_result("json_parse", "fail", errors.join("\n"))
    }
}

fn check_schema_examples(config: &Map<String, Value>) -> GateResult {
    let mut errors = Vec::new();
    let examples = config
        .get("schema_examples")
        .and_then(Value::as_array)
        .cloned()
        .unwrap_or_default();
    for pair in &examples {
        let Some(schema_path) = pair.get("schema").and_then(Value::as_str) else {
            errors.push("schema_examples item missing schema".to_string());
            continue;
        };
        let Some(example_path) = pair.get("example").and_then(Value::as_str) else {
            errors.push("schema_examples item missing example".to_string());
            continue;
        };
        match (
            load_json_value(Path::new(schema_path)),
            load_json_value(Path::new(example_path)),
        ) {
            (Ok(schema), Ok(example)) => errors.extend(
                validate_schema_value(&schema, &example, "$")
                    .into_iter()
                    .map(|error| format!("{example_path}: {error}")),
            ),
            (Err(error), _) | (_, Err(error)) => errors.push(error),
        }
    }
    if errors.is_empty() {
        gate_result(
            "schema_examples",
            "pass",
            format!("{} examples validate", examples.len()),
        )
    } else {
        gate_result("schema_examples", "fail", errors.join("\n"))
    }
}

fn check_no_retired_source_files() -> GateResult {
    let command = [
        "git",
        "ls-files",
        "--cached",
        "--others",
        "--exclude-standard",
    ]
    .iter()
    .map(|item| item.to_string())
    .collect::<Vec<_>>();
    let result = run_process("git_ls_files", &command, None);
    if result.status == "fail" {
        return gate_result("retired_source_files", "fail", result.detail);
    }
    let forbidden_names = ["package.json", "package-lock.json", "tsconfig.json"];
    let forbidden_suffixes = [".js", ".mjs", ".py", ".sh", ".ts", ".tsx"];
    let findings: Vec<String> = result
        .detail
        .lines()
        .filter(|rel| {
            let path = repo_root().join(rel);
            if !path.exists() {
                return false;
            }
            let lower = rel.to_ascii_lowercase();
            let name = Path::new(rel)
                .file_name()
                .and_then(|name| name.to_str())
                .unwrap_or("")
                .to_ascii_lowercase();
            forbidden_names.contains(&name.as_str())
                || forbidden_suffixes
                    .iter()
                    .any(|suffix| lower.ends_with(suffix))
        })
        .map(str::to_string)
        .collect();
    if findings.is_empty() {
        gate_result(
            "retired_source_files",
            "pass",
            "no tracked or pending retired script/Node source files",
        )
    } else {
        gate_result("retired_source_files", "fail", findings.join("\n"))
    }
}

fn check_retired_file_references(config: &Map<String, Value>) -> GateResult {
    let retired = config_array(config, "retired_file_references");
    if retired.is_empty() {
        return gate_result("retired_file_references", "pass", "no retired files listed");
    }
    let mut files = Vec::new();
    if let Err(error) = iter_quality_text_files(&repo_root(), &mut files) {
        return gate_result("retired_file_references", "fail", error);
    }
    let mut findings = Vec::new();
    for path in files {
        let rel = repo_relative_display(&path);
        if rel == "docs/quality-gates.json" || rel.starts_with("docs/cycles/") {
            continue;
        }
        let text = fs::read_to_string(&path).unwrap_or_default();
        let normalized = text.replace('\\', "/");
        for retired_path in &retired {
            if normalized.contains(retired_path) {
                findings.push(format!("{rel}: {retired_path}"));
            }
        }
    }
    if findings.is_empty() {
        gate_result(
            "retired_file_references",
            "pass",
            "no active references to retired tool paths",
        )
    } else {
        gate_result(
            "retired_file_references",
            "fail",
            findings.into_iter().take(80).collect::<Vec<_>>().join("\n"),
        )
    }
}

fn check_git_ignored(config: &Map<String, Value>) -> GateResult {
    let mut failures = Vec::new();
    for item in config_array(config, "required_directories_ignored_by_git") {
        let candidates = if item.ends_with('/') {
            vec![item.to_string()]
        } else {
            vec![item.to_string(), format!("{item}/")]
        };
        let ignored = candidates.iter().any(|candidate| {
            ProcessCommand::new("git")
                .args(["check-ignore", "-q", candidate])
                .current_dir(repo_root())
                .status()
                .is_ok_and(|status| status.success())
        });
        if !ignored {
            failures.push(item.to_string());
        }
    }
    if failures.is_empty() {
        gate_result(
            "git_ignored_private_paths",
            "pass",
            "private/generated paths ignored",
        )
    } else {
        gate_result("git_ignored_private_paths", "fail", failures.join(", "))
    }
}

fn check_source_control_safety() -> GateResult {
    let command = [
        "git",
        "ls-files",
        "--cached",
        "--others",
        "--exclude-standard",
    ]
    .iter()
    .map(|item| item.to_string())
    .collect::<Vec<_>>();
    let result = run_process("git_ls_files", &command, None);
    if result.status == "fail" {
        return gate_result("source_control_safety", "fail", result.detail);
    }
    let forbidden_prefixes = [
        "references/private/",
        "references/inventory/",
        "Tools/install/",
    ];
    let forbidden_suffixes = [
        ".7z", ".db", ".dll", ".dmg", ".exe", ".log", ".pak", ".pdb", ".sig", ".ucas", ".utoc",
        ".zip",
    ];
    let mut failures = Vec::new();
    for rel in result.detail.lines() {
        let lower = rel.to_ascii_lowercase();
        if forbidden_prefixes
            .iter()
            .any(|prefix| rel.starts_with(prefix))
        {
            failures.push(format!("{rel}: forbidden path"));
        }
        if forbidden_suffixes
            .iter()
            .any(|suffix| lower.ends_with(suffix))
        {
            failures.push(format!("{rel}: forbidden binary/package/log suffix"));
        }
        if repo_root().join(rel).is_symlink() {
            let target = repo_root()
                .join(rel)
                .canonicalize()
                .unwrap_or_else(|_| repo_root().join(rel));
            if target.to_string_lossy().contains("references/private") {
                failures.push(format!("{rel}: symlink points into private references"));
            }
        }
    }
    if failures.is_empty() {
        gate_result(
            "source_control_safety",
            "pass",
            "no forbidden tracked-safe paths",
        )
    } else {
        gate_result(
            "source_control_safety",
            "fail",
            failures.into_iter().take(80).collect::<Vec<_>>().join("\n"),
        )
    }
}

fn check_shell_files(config: &Map<String, Value>) -> GateResult {
    let files = config_array(config, "shell_files");
    let mut errors = Vec::new();
    for item in &files {
        let result = run_process(
            "shell_syntax",
            &["bash".to_string(), "-n".to_string(), (*item).to_string()],
            None,
        );
        if result.status == "fail" {
            errors.push(format!("{item}: {}", result.detail));
        }
    }
    if errors.is_empty() {
        gate_result(
            "shell_syntax",
            "pass",
            format!("{} shell files parsed", files.len()),
        )
    } else {
        gate_result("shell_syntax", "fail", errors.join("\n"))
    }
}

fn powershell_static_errors(path: &Path) -> Vec<String> {
    let text = match fs::read_to_string(path) {
        Ok(text) => text,
        Err(error) => return vec![format!("{error}")],
    };
    let lines: Vec<&str> = text.lines().collect();
    if lines.is_empty() {
        return vec!["empty file".to_string()];
    }
    let mut errors = Vec::new();
    for (index, line) in lines.iter().enumerate() {
        if line.trim_end().ends_with('`') {
            if index + 1 == lines.len() {
                errors.push(format!(
                    "line {}: trailing line-continuation backtick at EOF",
                    index + 1
                ));
            } else if lines[index + 1].trim().is_empty() {
                errors.push(format!(
                    "line {}: line-continuation backtick followed by a blank line",
                    index + 1
                ));
            }
        }
    }
    if !text.contains("Set-StrictMode") {
        errors.push("missing Set-StrictMode".to_string());
    }
    if !text.contains("$ErrorActionPreference") {
        errors.push("missing $ErrorActionPreference".to_string());
    }
    errors
}

fn find_on_path(names: &[&str]) -> Option<PathBuf> {
    let path_value = std::env::var_os("PATH")?;
    let mut candidate_names = Vec::new();
    for name in names {
        candidate_names.push((*name).to_string());
        if cfg!(windows) && !name.to_ascii_lowercase().ends_with(".exe") {
            candidate_names.push(format!("{name}.exe"));
        }
    }
    for directory in std::env::split_paths(&path_value) {
        for name in &candidate_names {
            let candidate = directory.join(name);
            if candidate.exists() {
                return Some(candidate);
            }
        }
    }
    None
}

fn ps_quote(path: &Path) -> String {
    format!("'{}'", path.to_string_lossy().replace('\'', "''"))
}

fn check_powershell_files(config: &Map<String, Value>) -> GateResult {
    let files = config_array(config, "powershell_files");
    let parser = find_on_path(&["pwsh", "powershell"]);
    let mut errors = Vec::new();
    for item in &files {
        let path = repo_root().join(item);
        errors.extend(
            powershell_static_errors(&path)
                .into_iter()
                .map(|error| format!("{item}: {error}")),
        );
        if let Some(parser) = &parser {
            let script = format!(
                "$errors = $null; $null = [System.Management.Automation.PSParser]::Tokenize((Get-Content -Raw -LiteralPath {}), [ref]$errors); if ($errors) {{ $errors | ForEach-Object {{ Write-Error $_ }}; exit 1 }}",
                ps_quote(&path)
            );
            let result = run_process(
                "powershell_parse",
                &[
                    parser.to_string_lossy().to_string(),
                    "-NoProfile".to_string(),
                    "-NonInteractive".to_string(),
                    "-Command".to_string(),
                    script,
                ],
                None,
            );
            if result.status == "fail" {
                errors.push(format!("{item}: {}", result.detail));
            }
        }
    }
    if !errors.is_empty() {
        return gate_result("powershell_syntax", "fail", errors.join("\n"));
    }
    if let Some(parser) = parser {
        gate_result(
            "powershell_syntax",
            "pass",
            format!(
                "{} PowerShell files parsed with {}",
                files.len(),
                parser
                    .file_name()
                    .and_then(|name| name.to_str())
                    .unwrap_or("PowerShell")
            ),
        )
    } else {
        gate_result(
            "powershell_syntax",
            "pass",
            format!("{} PowerShell files passed static checks", files.len()),
        )
    }
}

fn quality_file_excluded(path: &Path) -> bool {
    let Ok(relative) = path.strip_prefix(repo_root()) else {
        return true;
    };
    let rel = relative.to_string_lossy().replace('\\', "/");
    let excluded_prefixes = [
        ".git",
        "Binaries",
        "DerivedDataCache",
        "Intermediate",
        "Saved",
        "Script",
        "Tools/install",
        "target",
        "references/private",
        "references/inventory",
    ];
    let excluded_names = [
        ".git",
        ".mypy_cache",
        "__pycache__",
        "dist",
        "node_modules",
        "target",
    ];
    excluded_prefixes
        .iter()
        .any(|prefix| rel == *prefix || rel.starts_with(&format!("{prefix}/")))
        || relative.components().any(|component| {
            excluded_names.contains(&component.as_os_str().to_string_lossy().as_ref())
        })
}

fn iter_quality_text_files(root: &Path, files: &mut Vec<PathBuf>) -> Result<(), String> {
    if quality_file_excluded(root) {
        return Ok(());
    }
    let metadata = fs::metadata(root).map_err(|error| format!("{}: {error}", root.display()))?;
    if metadata.is_dir() {
        for entry in fs::read_dir(root).map_err(|error| format!("{}: {error}", root.display()))? {
            let entry = entry.map_err(|error| format!("{}: {error}", root.display()))?;
            iter_quality_text_files(&entry.path(), files)?;
        }
    } else if metadata.is_file() {
        let suffixes = [
            ".cs",
            ".cpp",
            ".h",
            ".ini",
            ".json",
            ".md",
            ".mjs",
            ".ps1",
            ".rs",
            ".sh",
            ".toml",
            ".txt",
            ".ts",
            ".uproject",
            ".yaml",
            ".yml",
        ];
        let name = root
            .file_name()
            .and_then(|name| name.to_str())
            .unwrap_or("");
        let suffix = root
            .extension()
            .and_then(|value| value.to_str())
            .map(|value| format!(".{value}"));
        if name == "Frostwake.uproject"
            || suffix
                .as_deref()
                .is_some_and(|suffix| suffixes.contains(&suffix))
        {
            files.push(root.to_path_buf());
        }
    }
    Ok(())
}

fn check_stale_terms(config: &Map<String, Value>) -> GateResult {
    let terms: Vec<String> = config_array(config, "stale_terms")
        .into_iter()
        .map(|term| term.to_ascii_lowercase())
        .collect();
    let allowed = [
        "docs/ip-risk.md",
        "docs/ip-boundary.md",
        "docs/quality-gates.json",
        "docs/reference-policy.md",
        "references/README.md",
        "references/external-paths.json",
        "references/private-copy-manifest.md",
    ];
    let mut files = Vec::new();
    if let Err(error) = iter_quality_text_files(&repo_root(), &mut files) {
        return gate_result("stale_terms", "fail", error);
    }
    let mut findings = Vec::new();
    for path in files {
        let rel = repo_relative_display(&path);
        if allowed.contains(&rel.as_str()) {
            continue;
        }
        let text = fs::read_to_string(&path)
            .unwrap_or_default()
            .to_ascii_lowercase();
        for term in &terms {
            if text.contains(term) {
                findings.push(format!("{rel}: {term}"));
            }
        }
    }
    if findings.is_empty() {
        gate_result("stale_terms", "pass", "no stale core terms found")
    } else {
        gate_result(
            "stale_terms",
            "fail",
            findings.into_iter().take(80).collect::<Vec<_>>().join("\n"),
        )
    }
}

fn check_unreal_metadata() -> GateResult {
    let mut errors = Vec::new();
    let project = match load_json_value(Path::new("Frostwake.uproject")) {
        Ok(Value::Object(project)) => project,
        Ok(_) => {
            return gate_result(
                "unreal_metadata",
                "fail",
                "Frostwake.uproject must be a JSON object",
            );
        }
        Err(error) => return gate_result("unreal_metadata", "fail", error),
    };
    let modules: Vec<String> = project
        .get("Modules")
        .and_then(Value::as_array)
        .into_iter()
        .flatten()
        .filter_map(|module| {
            module
                .get("Name")
                .and_then(Value::as_str)
                .map(str::to_string)
        })
        .collect();
    for module in &modules {
        if !repo_root()
            .join("Source")
            .join(module)
            .join(format!("{module}.Build.cs"))
            .exists()
        {
            errors.push(format!("missing Build.cs for module {module}"));
        }
    }

    let target_files = fs::read_dir(repo_root().join("Source"))
        .ok()
        .into_iter()
        .flatten()
        .filter_map(Result::ok)
        .map(|entry| entry.path())
        .filter(|path| {
            path.file_name()
                .and_then(|name| name.to_str())
                .is_some_and(|name| name.ends_with(".Target.cs"))
        })
        .collect::<Vec<_>>();
    for target_file in &target_files {
        let text = fs::read_to_string(target_file).unwrap_or_default();
        if !modules
            .iter()
            .any(|module| text.contains(&format!("ExtraModuleNames.Add(\"{module}\")")))
        {
            errors.push(format!(
                "{}: no project module in ExtraModuleNames",
                repo_relative_display(target_file)
            ));
        }
    }

    let steam_enabled = project
        .get("Plugins")
        .and_then(Value::as_array)
        .into_iter()
        .flatten()
        .find(|plugin| plugin.get("Name").and_then(Value::as_str) == Some("OnlineSubsystemSteam"))
        .and_then(|plugin| plugin.get("Enabled"))
        .and_then(Value::as_bool);
    if steam_enabled != Some(false) {
        errors.push("OnlineSubsystemSteam must remain disabled during Phase 1".to_string());
    }

    let default_engine =
        fs::read_to_string(repo_root().join("Config/DefaultEngine.ini")).unwrap_or_default();
    if !default_engine.contains("DefaultPlatformService=Null") {
        errors.push("DefaultPlatformService=Null missing".to_string());
    }
    if !default_engine.contains("MaxPlayers=8") {
        errors.push("MaxPlayers=8 missing".to_string());
    }

    if errors.is_empty() {
        gate_result(
            "unreal_metadata",
            "pass",
            format!(
                "{} module(s), {} target(s)",
                modules.len(),
                target_files.len()
            ),
        )
    } else {
        gate_result("unreal_metadata", "fail", errors.join("\n"))
    }
}

fn collect_files_with_extension(
    root: &Path,
    extension: &str,
    files: &mut Vec<PathBuf>,
) -> Result<(), String> {
    if !root.exists() {
        return Ok(());
    }
    let metadata = fs::metadata(root).map_err(|error| format!("{}: {error}", root.display()))?;
    if metadata.is_dir() {
        for entry in fs::read_dir(root).map_err(|error| format!("{}: {error}", root.display()))? {
            let entry = entry.map_err(|error| format!("{}: {error}", root.display()))?;
            collect_files_with_extension(&entry.path(), extension, files)?;
        }
    } else if root.extension().and_then(|value| value.to_str()) == Some(extension) {
        files.push(root.to_path_buf());
    }
    Ok(())
}

fn check_unreal_cpp_heuristics() -> GateResult {
    let source_root = repo_root().join("Source");
    let mut errors = Vec::new();
    let mut headers = Vec::new();
    let mut cpps = Vec::new();
    if let Err(error) = collect_files_with_extension(&source_root, "h", &mut headers) {
        return gate_result("unreal_cpp_heuristics", "fail", error);
    }
    if let Err(error) = collect_files_with_extension(&source_root, "cpp", &mut cpps) {
        return gate_result("unreal_cpp_heuristics", "fail", error);
    }
    for header in &headers {
        let text = fs::read_to_string(header).unwrap_or_default();
        if ["UCLASS", "USTRUCT", "UENUM"]
            .iter()
            .any(|marker| text.contains(marker))
        {
            let expected = format!(
                "#include \"{}.generated.h\"",
                header
                    .file_stem()
                    .and_then(|stem| stem.to_str())
                    .unwrap_or_default()
            );
            if !text.contains(&expected) {
                errors.push(format!(
                    "{}: missing {expected}",
                    repo_relative_display(header)
                ));
            }
        }
    }

    let secret_team_declared = headers.iter().any(|header| {
        fs::read_to_string(header)
            .unwrap_or_default()
            .contains("SecretTeam")
    });
    if secret_team_declared {
        let replication_text = cpps
            .iter()
            .map(|path| fs::read_to_string(path).unwrap_or_default())
            .collect::<Vec<_>>()
            .join("\n");
        if !replication_text.contains("DOREPLIFETIME_CONDITION")
            || !replication_text.contains("SecretTeam")
            || !replication_text.contains("COND_OwnerOnly")
        {
            errors.push("SecretTeam must replicate with COND_OwnerOnly".to_string());
        }
    }

    if errors.is_empty() {
        gate_result(
            "unreal_cpp_heuristics",
            "pass",
            "basic Unreal C++ heuristics passed",
        )
    } else {
        gate_result(
            "unreal_cpp_heuristics",
            "fail",
            errors.into_iter().take(80).collect::<Vec<_>>().join("\n"),
        )
    }
}

fn check_cycle_records(config: &Map<String, Value>) -> GateResult {
    let cycle_dir = repo_root().join("docs/cycles");
    let cycle_regex = Regex::new(r"cycle-(\d+)\.md$").expect("valid cycle regex");
    let mut cycle_files = fs::read_dir(&cycle_dir)
        .ok()
        .into_iter()
        .flatten()
        .filter_map(Result::ok)
        .map(|entry| entry.path())
        .filter(|path| path.file_name().and_then(|name| name.to_str()) != Some("template.md"))
        .filter_map(|path| {
            let name = path.file_name()?.to_str()?;
            let number = cycle_regex
                .captures(name)?
                .get(1)?
                .as_str()
                .parse::<i64>()
                .ok()?;
            Some((number, path))
        })
        .collect::<Vec<_>>();
    cycle_files.sort_by_key(|(number, _)| *number);
    let Some((_, latest)) = cycle_files.last() else {
        return gate_result("cycle_records", "fail", "no cycle records found");
    };
    let text = fs::read_to_string(latest).unwrap_or_default();
    let retired = config_array(config, "retired_file_references");
    let findings = cycle_record_findings(&text, &retired);
    if findings.is_empty() {
        gate_result(
            "cycle_records",
            "pass",
            format!("latest cycle record {}", repo_relative_display(latest)),
        )
    } else {
        gate_result(
            "cycle_records",
            "fail",
            format!("{}: {}", repo_relative_display(latest), findings.join("; ")),
        )
    }
}

fn cycle_record_findings(text: &str, retired_paths: &[&str]) -> Vec<String> {
    let mut findings = Vec::new();
    for term in [
        "Goal:",
        "Gate:",
        "## Evidence",
        "## Decisions",
        "## Next Cycle",
    ] {
        if !text.contains(term) {
            findings.push(format!("missing {term}"));
        }
    }

    let normalized = text.replace('\\', "/");
    if !normalized.contains("cargo run -p frostwake-tools -- quality-gate") {
        findings.push(
            "missing current quality gate command: cargo run -p frostwake-tools -- quality-gate"
                .to_string(),
        );
    }
    for retired_path in retired_paths {
        if normalized.contains(retired_path) {
            findings.push(format!("retired tool reference: {retired_path}"));
        }
    }

    for line in text.lines() {
        let trimmed = line.trim();
        for field in [
            "- Cycle:",
            "- Date:",
            "- Phase:",
            "- Owner:",
            "- Goal:",
            "- Gate:",
            "- First action:",
        ] {
            if trimmed == field {
                findings.push(format!("empty cycle field: {field}"));
            }
        }
        if trimmed.contains("| not run |  |") || trimmed.contains("| not available |  |") {
            findings.push(format!("unexplained skipped verification: {trimmed}"));
        }
    }

    findings
}

fn check_unreal_installed(require_ue: bool) -> GateResult {
    let mut candidates = Vec::new();
    for env_name in ["UE_ROOT", "UNREAL_ENGINE_ROOT"] {
        if let Some(value) = std::env::var_os(env_name) {
            candidates.push(PathBuf::from(value));
        }
    }
    if cfg!(windows) {
        for env_name in ["ProgramFiles", "ProgramW6432"] {
            if let Some(base) = std::env::var_os(env_name) {
                collect_ue_roots(PathBuf::from(base).join("Epic Games"), &mut candidates);
            }
        }
        collect_ue_roots(
            PathBuf::from(r"C:\Program Files\Epic Games"),
            &mut candidates,
        );
    } else if cfg!(target_os = "macos") {
        collect_ue_roots(PathBuf::from("/Users/Shared/Epic Games"), &mut candidates);
    } else {
        collect_ue_roots(PathBuf::from("/opt"), &mut candidates);
    }
    let existing: Vec<PathBuf> = candidates
        .into_iter()
        .filter(|path| path.exists())
        .collect();
    if !existing.is_empty() {
        return gate_result(
            "unreal_installed",
            "pass",
            existing
                .iter()
                .map(|path| path.display().to_string())
                .collect::<Vec<_>>()
                .join(", "),
        );
    }
    let detail = "UE install not found. Set UE_ROOT or UNREAL_ENGINE_ROOT if installed elsewhere.";
    if require_ue {
        gate_result("unreal_installed", "fail", detail)
    } else {
        gate_result("unreal_installed", "warn", detail)
    }
}

fn collect_ue_roots(base: PathBuf, candidates: &mut Vec<PathBuf>) {
    if !base.exists() {
        candidates.push(base.join("UE_5.7"));
        return;
    }
    candidates.push(base.join("UE_5.7"));
    if let Ok(entries) = fs::read_dir(base) {
        for entry in entries.flatten() {
            let path = entry.path();
            if path
                .file_name()
                .and_then(|name| name.to_str())
                .is_some_and(|name| name.starts_with("UE_"))
            {
                candidates.push(path);
            }
        }
    }
}

fn check_cpp_toolchain(require_ue: bool) -> GateResult {
    if cfg!(windows) {
        if let Some(cl) = find_on_path(&["cl"]) {
            return gate_result(
                "cpp_toolchain",
                "pass",
                format!("MSVC compiler on PATH: {}", cl.display()),
            );
        }
        let vswhere = PathBuf::from(
            std::env::var_os("ProgramFiles(x86)")
                .unwrap_or_else(|| r"C:\Program Files (x86)".into()),
        )
        .join("Microsoft Visual Studio/Installer/vswhere.exe");
        if vswhere.exists() {
            let command = vec![
                vswhere.to_string_lossy().to_string(),
                "-latest".to_string(),
                "-products".to_string(),
                "*".to_string(),
                "-requires".to_string(),
                "Microsoft.VisualStudio.Workload.NativeGame".to_string(),
                "-property".to_string(),
                "installationPath".to_string(),
            ];
            let result = run_process("vswhere", &command, None);
            if result.status == "pass" && !result.detail.trim().is_empty() {
                return gate_result(
                    "cpp_toolchain",
                    "pass",
                    format!(
                        "Visual Studio native game workload: {}",
                        result.detail.trim()
                    ),
                );
            }
        }
        let detail = "Visual Studio C++ toolchain not found. Install Visual Studio 2022 with Game development with C++.";
        return gate_result(
            "cpp_toolchain",
            if require_ue { "fail" } else { "warn" },
            detail,
        );
    }
    if cfg!(target_os = "macos") {
        let result = run_process(
            "xcodebuild",
            &["xcodebuild".to_string(), "-version".to_string()],
            None,
        );
        if result.status == "pass" {
            return gate_result(
                "cpp_toolchain",
                "pass",
                result
                    .detail
                    .lines()
                    .next()
                    .unwrap_or("xcodebuild")
                    .to_string(),
            );
        }
        return gate_result(
            "cpp_toolchain",
            if require_ue { "fail" } else { "warn" },
            result.detail,
        );
    }
    if let Some(cxx) = find_on_path(&["clang++", "g++"]) {
        gate_result("cpp_toolchain", "pass", cxx.display().to_string())
    } else {
        gate_result(
            "cpp_toolchain",
            if require_ue { "fail" } else { "warn" },
            "C++ compiler not found on PATH",
        )
    }
}

#[derive(Clone, Debug, Eq, PartialEq)]
struct IssueRow {
    issue_id: String,
    section: String,
    task: String,
    done_when: String,
    phase: i64,
    source_path: PathBuf,
    milestone: String,
    base_label: String,
}

impl IssueRow {
    fn title(&self) -> String {
        format!("{}: {}", self.issue_id, self.task)
    }

    fn labels(&self) -> String {
        format!(
            "phase-{}, {}, {}",
            self.phase,
            slug_label(&self.section),
            self.base_label
        )
        .replace(", ", ",")
    }

    fn body(&self) -> String {
        let source = repo_relative_display(&self.source_path);
        [
            format!("Source: `{source}`"),
            String::new(),
            "Done when:".to_string(),
            String::new(),
            self.done_when.clone(),
            String::new(),
            "Validation evidence:".to_string(),
            String::new(),
            "- [ ] Command output or cycle link added".to_string(),
            "- [ ] Result recorded in a cycle file".to_string(),
            "- [ ] No raw logs, recordings, or personal identifiers committed".to_string(),
        ]
        .join("\n")
    }
}

fn slug_label(value: &str) -> String {
    let regex = Regex::new(r"[^a-z0-9]+").expect("valid slug regex");
    let slug = regex
        .replace_all(&value.to_ascii_lowercase(), "-")
        .trim_matches('-')
        .to_string();
    if slug.is_empty() {
        "unsorted".to_string()
    } else {
        slug
    }
}

fn parse_backlog_to_issue_rows(
    path: &Path,
    phase: i64,
    id_prefix: &str,
    milestone: &str,
    base_label: &str,
) -> Result<Vec<IssueRow>, String> {
    let text = fs::read_to_string(path).map_err(|error| format!("{}: {error}", path.display()))?;
    let row_pattern = Regex::new(r"^\|\s*([A-Z]+\d+-\d{3})\s*\|\s*(.*?)\s*\|\s*(.*?)\s*\|$")
        .expect("valid backlog row regex");
    let section_pattern = Regex::new(r"^##\s+(.+?)\s*$").expect("valid section regex");
    let mut rows = Vec::new();
    let mut current_section = milestone.to_string();
    for line in text.lines() {
        if let Some(captures) = section_pattern.captures(line) {
            current_section = captures[1].to_string();
            continue;
        }
        let Some(captures) = row_pattern.captures(line) else {
            continue;
        };
        let issue_id = captures[1].to_string();
        if !issue_id.starts_with(id_prefix) {
            continue;
        }
        rows.push(IssueRow {
            issue_id,
            section: current_section.clone(),
            task: captures[2].to_string(),
            done_when: captures[3].to_string(),
            phase,
            source_path: path.to_path_buf(),
            milestone: milestone.to_string(),
            base_label: base_label.to_string(),
        });
    }
    Ok(rows)
}

fn write_issue_import_csv(rows: &[IssueRow], path: &Path) -> Result<(), String> {
    if let Some(parent) = path.parent()
        && !parent.as_os_str().is_empty()
    {
        fs::create_dir_all(parent).map_err(|error| format!("{}: {error}", parent.display()))?;
    }
    let mut writer = csv::WriterBuilder::new()
        .has_headers(true)
        .terminator(csv::Terminator::Any(b'\n'))
        .from_path(path)
        .map_err(|error| format!("{}: {error}", path.display()))?;
    writer
        .write_record(["title", "body", "labels", "milestone"])
        .map_err(|error| format!("{}: {error}", path.display()))?;
    for row in rows {
        writer
            .write_record([row.title(), row.body(), row.labels(), row.milestone.clone()])
            .map_err(|error| format!("{}: {error}", path.display()))?;
    }
    writer
        .flush()
        .map_err(|error| format!("{}: {error}", path.display()))
}

fn write_issue_import_markdown(
    rows: &[IssueRow],
    path: &Path,
    phase: i64,
    source_path: &Path,
) -> Result<(), String> {
    if let Some(parent) = path.parent()
        && !parent.as_os_str().is_empty()
    {
        fs::create_dir_all(parent).map_err(|error| format!("{}: {error}", parent.display()))?;
    }
    let mut lines = vec![
        format!("# Phase {phase} Issue Import"),
        String::new(),
        format!("Generated from `{}`.", repo_relative_display(source_path)),
        String::new(),
        "| ID | Section | Title | Labels |".to_string(),
        "| --- | --- | --- | --- |".to_string(),
    ];
    for row in rows {
        lines.push(format!(
            "| {} | {} | {} | `{}` |",
            row.issue_id,
            row.section,
            row.task,
            row.labels()
        ));
    }
    lines.push(String::new());
    fs::write(path, lines.join("\n")).map_err(|error| format!("{}: {error}", path.display()))
}

#[derive(Clone, Debug, Eq, PartialEq)]
struct IssueSpec {
    title: String,
    body: String,
    labels: Vec<String>,
    milestone: String,
}

fn load_issue_specs(path: &Path) -> Result<Vec<IssueSpec>, String> {
    let mut reader =
        csv::Reader::from_path(path).map_err(|error| format!("{}: {error}", path.display()))?;
    let mut specs = Vec::new();
    for row in reader.deserialize::<BTreeMap<String, String>>() {
        let row = row.map_err(|error| format!("{}: {error}", path.display()))?;
        let title = row
            .get("title")
            .map(|value| value.trim())
            .unwrap_or_default();
        if title.is_empty() {
            continue;
        }
        let body = row
            .get("body")
            .map(|value| value.trim())
            .unwrap_or_default()
            .to_string();
        let labels = row
            .get("labels")
            .map(String::as_str)
            .unwrap_or_default()
            .split(',')
            .map(str::trim)
            .filter(|value| !value.is_empty())
            .map(str::to_string)
            .collect();
        let milestone = row
            .get("milestone")
            .map(|value| value.trim())
            .unwrap_or_default()
            .to_string();
        specs.push(IssueSpec {
            title: title.to_string(),
            body,
            labels,
            milestone,
        });
    }
    Ok(specs)
}

fn build_gh_issue_create_command(spec: &IssueSpec) -> Vec<String> {
    let mut command = vec![
        "gh".to_string(),
        "issue".to_string(),
        "create".to_string(),
        "--title".to_string(),
        spec.title.clone(),
        "--body".to_string(),
        spec.body.clone(),
    ];
    for label in &spec.labels {
        command.extend(["--label".to_string(), label.clone()]);
    }
    if !spec.milestone.is_empty() {
        command.extend(["--milestone".to_string(), spec.milestone.clone()]);
    }
    command
}

fn shell_quote(part: &str) -> String {
    if !part.is_empty()
        && part.chars().all(|ch| {
            ch.is_ascii_alphanumeric() || matches!(ch, '_' | '-' | '.' | '/' | ':' | '\\')
        })
    {
        return part.to_string();
    }
    format!("'{}'", part.replace('\'', "'\"'\"'"))
}

fn shell_command(command: &[String]) -> String {
    command
        .iter()
        .map(|part| shell_quote(part))
        .collect::<Vec<_>>()
        .join(" ")
}

fn existing_issue_titles() -> Result<HashSet<String>, String> {
    let command = [
        "gh", "issue", "list", "--state", "all", "--limit", "1000", "--json", "title",
    ]
    .iter()
    .map(|item| item.to_string())
    .collect::<Vec<_>>();
    let result = run_process("gh_issue_list", &command, None);
    if result.status == "fail" {
        return Err(result.detail);
    }
    let payload: Value = serde_json::from_str(&result.detail)
        .map_err(|error| format!("gh issue list returned invalid JSON: {error}"))?;
    Ok(payload
        .as_array()
        .into_iter()
        .flatten()
        .filter_map(|item| item.get("title").and_then(Value::as_str).map(str::trim))
        .filter(|title| !title.is_empty())
        .map(str::to_string)
        .collect())
}

fn next_cycle_number(cycle_dir: &Path) -> Result<i64, String> {
    let pattern = Regex::new(r"^\d{4}-\d{2}-\d{2}-cycle-(\d+)\.md$").expect("valid cycle regex");
    let mut max_number = -1;
    if cycle_dir.exists() {
        for entry in
            fs::read_dir(cycle_dir).map_err(|error| format!("{}: {error}", cycle_dir.display()))?
        {
            let entry = entry.map_err(|error| format!("{}: {error}", cycle_dir.display()))?;
            let Some(name) = entry.file_name().to_str().map(str::to_string) else {
                continue;
            };
            if let Some(captures) = pattern.captures(&name)
                && let Ok(number) = captures[1].parse::<i64>()
            {
                max_number = max_number.max(number);
            }
        }
    }
    Ok(max_number + 1)
}

fn build_cycle_doc(
    cycle: i64,
    date: &str,
    phase: &str,
    owner: &str,
    goal: &str,
    gate: &str,
) -> String {
    format!(
        r#"# Cycle {cycle}: {goal}

## Header

- Cycle: {cycle}
- Date: {date}
- Phase: {phase}
- Owner: {owner}
- Goal: {goal}
- Gate: {gate}

## Planned Scope

-

## Files Changed

-

## Verification

| Check | Result | Notes |
| --- | --- | --- |
| `cargo run -p frostwake-tools -- quality-gate` | not run |  |
| Unreal build | not available |  |
| Multiplayer smoke test | not run |  |
| Bot/client run | not run |  |
| Human playtest | not run |  |

## Evidence

-

## Decisions

| Label | Decision | Reason | Next Action |
| --- | --- | --- | --- |
| blocker |  |  |  |
| keep |  |  |  |
| change |  |  |  |
| cut |  |  |  |
| defer |  |  |  |

## Backlog Updates

-

## Next Cycle

- Goal:
- First action:
"#
    )
}

fn host_platform() -> String {
    if cfg!(windows) {
        "Win64".to_string()
    } else if cfg!(target_os = "macos") {
        "Mac".to_string()
    } else if cfg!(target_os = "linux") {
        "Linux".to_string()
    } else {
        std::env::consts::OS.to_string()
    }
}

fn candidate_ue_roots() -> Vec<PathBuf> {
    let mut candidates = Vec::new();
    for env_name in ["UE_ROOT", "UNREAL_ENGINE_ROOT"] {
        if let Some(value) = std::env::var_os(env_name) {
            candidates.push(PathBuf::from(value));
        }
    }
    if cfg!(windows) {
        for env_name in ["ProgramFiles", "ProgramW6432"] {
            if let Some(base) = std::env::var_os(env_name) {
                candidates.push(PathBuf::from(base).join("Epic Games/UE_5.7"));
            }
        }
        candidates.push(PathBuf::from(r"C:\Program Files\Epic Games\UE_5.7"));
    } else if cfg!(target_os = "macos") {
        candidates.push(PathBuf::from("/Users/Shared/Epic Games/UE_5.7"));
        let shared = PathBuf::from("/Users/Shared/Epic Games");
        if let Ok(entries) = fs::read_dir(shared) {
            for entry in entries.flatten() {
                let path = entry.path();
                if path
                    .file_name()
                    .and_then(|name| name.to_str())
                    .is_some_and(|name| name.starts_with("UE_"))
                {
                    candidates.push(path);
                }
            }
        }
    } else {
        candidates.push(PathBuf::from("/opt/UE_5.7"));
        candidates.push(
            PathBuf::from(std::env::var_os("HOME").unwrap_or_default()).join("Epic Games/UE_5.7"),
        );
    }
    let mut seen = HashSet::new();
    candidates
        .into_iter()
        .filter(|path| seen.insert(path.to_string_lossy().to_string()))
        .collect()
}

fn resolve_ue_root(override_path: Option<PathBuf>) -> PathBuf {
    if let Some(path) = override_path {
        return path;
    }
    candidate_ue_roots()
        .into_iter()
        .find(|path| path.exists())
        .unwrap_or_else(|| PathBuf::from(r"C:\Program Files\Epic Games\UE_5.7"))
}

fn unreal_build_script(ue_root: &Path, platform: &str) -> PathBuf {
    match platform {
        "Win64" => ue_root.join("Engine/Build/BatchFiles/Build.bat"),
        "Mac" => ue_root.join("Engine/Build/BatchFiles/Mac/Build.sh"),
        "Linux" => ue_root.join("Engine/Build/BatchFiles/Linux/Build.sh"),
        _ => ue_root.join("Engine/Build/BatchFiles/Build.sh"),
    }
}

fn generate_project_files_script(ue_root: &Path, platform: &str) -> PathBuf {
    match platform {
        "Win64" => ue_root.join("Engine/Build/BatchFiles/GenerateProjectFiles.bat"),
        "Mac" => ue_root.join("Engine/Build/BatchFiles/Mac/GenerateProjectFiles.sh"),
        "Linux" => ue_root.join("Engine/Build/BatchFiles/Linux/GenerateProjectFiles.sh"),
        _ => ue_root.join("Engine/Build/BatchFiles/GenerateProjectFiles.sh"),
    }
}

fn unreal_editor_path(ue_root: &Path, platform: &str) -> PathBuf {
    match platform {
        "Win64" => ue_root.join("Engine/Binaries/Win64/UnrealEditor.exe"),
        "Mac" => ue_root.join("Engine/Binaries/Mac/UnrealEditor"),
        "Linux" => ue_root.join("Engine/Binaries/Linux/UnrealEditor"),
        other => ue_root
            .join("Engine/Binaries")
            .join(other)
            .join("UnrealEditor"),
    }
}

fn unreal_editor_cmd_path(ue_root: &Path, platform: &str) -> PathBuf {
    match platform {
        "Win64" => ue_root.join("Engine/Binaries/Win64/UnrealEditor-Cmd.exe"),
        "Mac" => ue_root.join("Engine/Binaries/Mac/UnrealEditor-Cmd"),
        "Linux" => ue_root.join("Engine/Binaries/Linux/UnrealEditor-Cmd"),
        other => ue_root
            .join("Engine/Binaries")
            .join(other)
            .join("UnrealEditor-Cmd"),
    }
}

fn run_unreal_gate(
    ue_root: &Path,
    platform: &str,
    skip_generate: bool,
    include_server: bool,
) -> Vec<GateResult> {
    let project = repo_root().join("Frostwake.uproject");
    let build = unreal_build_script(ue_root, platform);
    let generate = generate_project_files_script(ue_root, platform);
    let mut results = vec![
        gate_result(
            "ue_root",
            if ue_root.exists() { "pass" } else { "fail" },
            ue_root.display().to_string(),
        ),
        gate_result(
            "build_script",
            if build.exists() { "pass" } else { "fail" },
            build.display().to_string(),
        ),
        gate_result(
            "project",
            if project.exists() { "pass" } else { "fail" },
            project.display().to_string(),
        ),
    ];
    if results.iter().any(|result| result.status == "fail") {
        return results;
    }
    if !skip_generate {
        results.push(run_process_blockable(
            "generate_project_files",
            &[
                generate.to_string_lossy().to_string(),
                format!("-project={}", project.display()),
                "-game".to_string(),
            ],
            None,
        ));
    }
    let build_base = |target: &str| -> Vec<String> {
        vec![
            build.to_string_lossy().to_string(),
            target.to_string(),
            platform.to_string(),
            "Development".to_string(),
            format!("-Project={}", project.display()),
            "-WaitMutex".to_string(),
        ]
    };
    results.push(run_process_blockable(
        "build_editor",
        &build_base("FrostwakeEditor"),
        None,
    ));
    results.push(run_process_blockable(
        "build_game",
        &build_base("Frostwake"),
        None,
    ));
    if include_server {
        results.push(run_process_blockable(
            "build_server",
            &build_base("FrostwakeServer"),
            Some("Server targets are not currently supported from this engine distribution"),
        ));
    }
    results
}

#[derive(Clone, Copy, Debug, Eq, PartialEq)]
enum SmokeProfile {
    Ready5,
    Ready6,
    Ready8,
    Combined5,
    Combined8,
    QaBot,
    QaPlayerBot,
    QaTaskBot,
    MatchTimer,
    LifeAction,
    Practice,
    SinglePlayer,
    FatalSabotage,
}

impl SmokeProfile {
    fn as_str(self) -> &'static str {
        match self {
            SmokeProfile::Ready5 => "ready5",
            SmokeProfile::Ready6 => "ready6",
            SmokeProfile::Ready8 => "ready8",
            SmokeProfile::Combined5 => "combined5",
            SmokeProfile::Combined8 => "combined8",
            SmokeProfile::QaBot => "qa-bot",
            SmokeProfile::QaPlayerBot => "qa-player-bot",
            SmokeProfile::QaTaskBot => "qa-task-bot",
            SmokeProfile::MatchTimer => "match-timer",
            SmokeProfile::LifeAction => "life-action",
            SmokeProfile::Practice => "practice",
            SmokeProfile::SinglePlayer => "single-player",
            SmokeProfile::FatalSabotage => "fatal-sabotage",
        }
    }
}

#[allow(dead_code)]
fn parse_smoke_profile(value: &str) -> Result<SmokeProfile, String> {
    match value {
        "ready5" => Ok(SmokeProfile::Ready5),
        "ready6" => Ok(SmokeProfile::Ready6),
        "ready8" => Ok(SmokeProfile::Ready8),
        "combined5" => Ok(SmokeProfile::Combined5),
        "combined8" => Ok(SmokeProfile::Combined8),
        "qa-bot" => Ok(SmokeProfile::QaBot),
        "qa-player-bot" => Ok(SmokeProfile::QaPlayerBot),
        "qa-task-bot" => Ok(SmokeProfile::QaTaskBot),
        "match-timer" => Ok(SmokeProfile::MatchTimer),
        "life-action" => Ok(SmokeProfile::LifeAction),
        "practice" => Ok(SmokeProfile::Practice),
        "single-player" => Ok(SmokeProfile::SinglePlayer),
        "fatal-sabotage" => Ok(SmokeProfile::FatalSabotage),
        _ => Err(format!(
            "unknown smoke profile {value:?}; expected ready5, ready6, ready8, combined5, combined8, qa-bot, qa-player-bot, qa-task-bot, match-timer, life-action, practice, single-player, or fatal-sabotage"
        )),
    }
}

#[allow(dead_code)]
#[derive(Clone, Debug)]
struct LocalSmokeOptions {
    profile: Option<SmokeProfile>,
    clients: i64,
    duration: u64,
    startup_timeout: u64,
    match_timeout: u64,
    client_launch_spacing: f64,
    auto_start_min_players: Option<i64>,
    expected_players: Option<i64>,
    expected_saboteurs: Option<i64>,
    map: String,
    port: i64,
    log_dir: Option<PathBuf>,
    run_id: Option<String>,
    build_id: String,
    skip_build: bool,
    ue_root: Option<PathBuf>,
    platform: String,
    null_rhi: bool,
    windowed: bool,
    host_only: bool,
    smoke_interact: bool,
    smoke_down_rescue: bool,
    smoke_containment: bool,
    smoke_route_complete: bool,
    smoke_fatal_sabotage: bool,
    smoke_bulkhead_lock: bool,
    smoke_pump_flooding: bool,
    smoke_pve_enemy: bool,
    smoke_pve_damage: bool,
    smoke_item_drop: bool,
    smoke_combined_systems: bool,
    smoke_qa_bot: bool,
    smoke_qa_player_bot: bool,
    smoke_qa_task_bot: bool,
    smoke_match_timer: bool,
    smoke_life_action: bool,
    smoke_eat: bool,
    smoke_damage_type: bool,
    smoke_survival: bool,
    smoke_effect: bool,
    no_auto_start: bool,
    auto_ready: bool,
    lobby_min_players: Option<i64>,
    lobby_dev_saboteurs: Option<i64>,
    practice_mode: bool,
    single_player_mode: bool,
}

fn default_local_smoke_options() -> LocalSmokeOptions {
    LocalSmokeOptions {
        profile: None,
        clients: 0,
        duration: 45,
        startup_timeout: 45,
        match_timeout: 45,
        client_launch_spacing: 3.0,
        auto_start_min_players: None,
        expected_players: None,
        expected_saboteurs: None,
        map: "/Game/Maps/L_IcebreakerWhitebox".to_string(),
        port: 7777,
        log_dir: None,
        run_id: None,
        build_id: "local-dev".to_string(),
        skip_build: false,
        ue_root: None,
        platform: host_platform(),
        null_rhi: false,
        windowed: false,
        host_only: false,
        smoke_interact: false,
        smoke_down_rescue: false,
        smoke_containment: false,
        smoke_route_complete: false,
        smoke_fatal_sabotage: false,
        smoke_bulkhead_lock: false,
        smoke_pump_flooding: false,
        smoke_pve_enemy: false,
        smoke_pve_damage: false,
        smoke_item_drop: false,
        smoke_combined_systems: false,
        smoke_qa_bot: false,
        smoke_qa_player_bot: false,
        smoke_qa_task_bot: false,
        smoke_match_timer: false,
        smoke_life_action: false,
        smoke_eat: false,
        smoke_damage_type: false,
        smoke_survival: false,
        smoke_effect: false,
        no_auto_start: false,
        auto_ready: false,
        lobby_min_players: None,
        lobby_dev_saboteurs: None,
        practice_mode: false,
        single_player_mode: false,
    }
}

fn take_smoke_arg(args: &[String], index: &mut usize, name: &str) -> Result<String, String> {
    *index += 1;
    args.get(*index)
        .cloned()
        .ok_or_else(|| format!("{name} requires a value"))
}

fn parse_local_smoke_args(args: &[String]) -> Result<(LocalSmokeOptions, bool), String> {
    let mut options = default_local_smoke_options();
    let mut describe_profile = false;
    let mut index = 0;
    while index < args.len() {
        let arg = args[index].as_str();
        match arg {
            "--profile" => options.profile = Some(parse_smoke_profile(&take_smoke_arg(args, &mut index, arg)?)?),
            "--describe-profile" => describe_profile = true,
            "--clients" => options.clients = take_smoke_arg(args, &mut index, arg)?.parse().map_err(|_| "--clients must be an integer".to_string())?,
            "--duration" => options.duration = take_smoke_arg(args, &mut index, arg)?.parse().map_err(|_| "--duration must be an integer".to_string())?,
            "--startup-timeout" => options.startup_timeout = take_smoke_arg(args, &mut index, arg)?.parse().map_err(|_| "--startup-timeout must be an integer".to_string())?,
            "--match-timeout" => options.match_timeout = take_smoke_arg(args, &mut index, arg)?.parse().map_err(|_| "--match-timeout must be an integer".to_string())?,
            "--client-launch-spacing" => options.client_launch_spacing = take_smoke_arg(args, &mut index, arg)?.parse().map_err(|_| "--client-launch-spacing must be a number".to_string())?,
            "--auto-start-min-players" => options.auto_start_min_players = Some(take_smoke_arg(args, &mut index, arg)?.parse().map_err(|_| "--auto-start-min-players must be an integer".to_string())?),
            "--expected-players" => options.expected_players = Some(take_smoke_arg(args, &mut index, arg)?.parse().map_err(|_| "--expected-players must be an integer".to_string())?),
            "--expected-saboteurs" => options.expected_saboteurs = Some(take_smoke_arg(args, &mut index, arg)?.parse().map_err(|_| "--expected-saboteurs must be an integer".to_string())?),
            "--map" => options.map = take_smoke_arg(args, &mut index, arg)?,
            "--port" => options.port = take_smoke_arg(args, &mut index, arg)?.parse().map_err(|_| "--port must be an integer".to_string())?,
            "--log-dir" => options.log_dir = Some(PathBuf::from(take_smoke_arg(args, &mut index, arg)?)),
            "--run-id" => options.run_id = Some(take_smoke_arg(args, &mut index, arg)?),
            "--build-id" => options.build_id = take_smoke_arg(args, &mut index, arg)?,
            "--skip-build" => options.skip_build = true,
            "--ue-root" => options.ue_root = Some(PathBuf::from(take_smoke_arg(args, &mut index, arg)?)),
            "--platform" => options.platform = take_smoke_arg(args, &mut index, arg)?,
            "--null-rhi" => options.null_rhi = true,
            "--windowed" => options.windowed = true,
            "--host-only" => options.host_only = true,
            "--smoke-interact" => options.smoke_interact = true,
            "--smoke-down-rescue" => options.smoke_down_rescue = true,
            "--smoke-containment" => options.smoke_containment = true,
            "--smoke-route-complete" => options.smoke_route_complete = true,
            "--smoke-fatal-sabotage" => options.smoke_fatal_sabotage = true,
            "--smoke-bulkhead-lock" => options.smoke_bulkhead_lock = true,
            "--smoke-pump-flooding" => options.smoke_pump_flooding = true,
            "--smoke-pve-enemy" => options.smoke_pve_enemy = true,
            "--smoke-pve-damage" => options.smoke_pve_damage = true,
            "--smoke-item-drop" => options.smoke_item_drop = true,
            "--smoke-combined-systems" => options.smoke_combined_systems = true,
            "--smoke-qa-bot" => options.smoke_qa_bot = true,
            "--smoke-qa-player-bot" => options.smoke_qa_player_bot = true,
            "--smoke-qa-task-bot" => options.smoke_qa_task_bot = true,
            "--smoke-match-timer" => options.smoke_match_timer = true,
            "--smoke-life-action" => options.smoke_life_action = true,
            "--smoke-eat" => options.smoke_eat = true,
            "--smoke-damage-type" => options.smoke_damage_type = true,
            "--smoke-survival" => options.smoke_survival = true,
            "--smoke-effect" => options.smoke_effect = true,
            "--no-auto-start" => options.no_auto_start = true,
            "--auto-ready" => options.auto_ready = true,
            "--lobby-min-players" => options.lobby_min_players = Some(take_smoke_arg(args, &mut index, arg)?.parse().map_err(|_| "--lobby-min-players must be an integer".to_string())?),
            "--lobby-dev-saboteurs" => options.lobby_dev_saboteurs = Some(take_smoke_arg(args, &mut index, arg)?.parse().map_err(|_| "--lobby-dev-saboteurs must be an integer".to_string())?),
            "--practice-mode" => options.practice_mode = true,
            "--single-player" => options.single_player_mode = true,
            "--help" | "-h" => return Err("Usage: frostwake-tools run-local-smoke [--profile NAME] [--describe-profile] [smoke options...]".to_string()),
            other => return Err(format!("unknown run-local-smoke argument: {other}")),
        }
        index += 1;
    }
    Ok((options, describe_profile))
}

#[allow(dead_code)]
fn apply_smoke_profile(options: &mut LocalSmokeOptions) {
    let Some(profile) = options.profile else {
        return;
    };
    match profile {
        SmokeProfile::Ready5 => {
            options.clients = 4;
            options.no_auto_start = true;
            options.auto_ready = true;
            options.lobby_min_players = Some(5);
            options.expected_players = Some(5);
            options.expected_saboteurs = Some(1);
            options.duration = 5;
            options.startup_timeout = 60;
            options.match_timeout = 120;
            options.client_launch_spacing = 1.5;
        }
        SmokeProfile::Ready6 => {
            options.clients = 5;
            options.no_auto_start = true;
            options.auto_ready = true;
            options.lobby_min_players = Some(6);
            options.expected_players = Some(6);
            options.expected_saboteurs = Some(1);
            options.duration = 5;
            options.startup_timeout = 75;
            options.match_timeout = 180;
            options.client_launch_spacing = 1.25;
        }
        SmokeProfile::Ready8 => {
            options.clients = 7;
            options.no_auto_start = true;
            options.auto_ready = true;
            options.lobby_min_players = Some(8);
            options.expected_players = Some(8);
            options.expected_saboteurs = Some(2);
            options.duration = 5;
            options.startup_timeout = 90;
            options.match_timeout = 240;
            options.client_launch_spacing = 1.25;
        }
        SmokeProfile::Combined5 => {
            apply_smoke_profile_for_base(options, SmokeProfile::Ready5);
            options.smoke_combined_systems = true;
        }
        SmokeProfile::Combined8 => {
            apply_smoke_profile_for_base(options, SmokeProfile::Ready8);
            options.smoke_combined_systems = true;
        }
        SmokeProfile::QaBot => {
            options.clients = 0;
            options.host_only = true;
            options.smoke_qa_bot = true;
            options.duration = 5;
            options.startup_timeout = 60;
            options.match_timeout = 120;
        }
        SmokeProfile::QaPlayerBot => {
            apply_smoke_profile_for_base(options, SmokeProfile::Ready5);
            options.smoke_qa_player_bot = true;
        }
        SmokeProfile::QaTaskBot => {
            apply_smoke_profile_for_base(options, SmokeProfile::Ready5);
            options.smoke_qa_task_bot = true;
        }
        SmokeProfile::MatchTimer => {
            options.clients = 0;
            options.host_only = true;
            options.smoke_match_timer = true;
            options.duration = 1;
            options.startup_timeout = 60;
            options.match_timeout = 30;
        }
        SmokeProfile::LifeAction => {
            apply_smoke_profile_for_base(options, SmokeProfile::Ready5);
            options.smoke_life_action = true;
        }
        SmokeProfile::Practice => {
            options.clients = 0;
            options.host_only = true;
            options.no_auto_start = true;
            options.practice_mode = true;
            options.expected_players = Some(1);
            options.expected_saboteurs = Some(0);
            options.duration = 5;
            options.startup_timeout = 60;
            options.match_timeout = 60;
        }
        SmokeProfile::SinglePlayer => {
            options.clients = 0;
            options.host_only = true;
            options.no_auto_start = true;
            options.single_player_mode = true;
            options.expected_players = Some(1);
            options.expected_saboteurs = Some(0);
            options.duration = 5;
            options.startup_timeout = 60;
            options.match_timeout = 60;
        }
        SmokeProfile::FatalSabotage => {
            options.clients = 0;
            options.host_only = true;
            options.auto_start_min_players = Some(1);
            options.expected_players = Some(1);
            options.expected_saboteurs = Some(1);
            options.smoke_fatal_sabotage = true;
            options.duration = 5;
            options.startup_timeout = 90;
            options.match_timeout = 120;
        }
    }
}

#[allow(dead_code)]
fn apply_smoke_profile_for_base(options: &mut LocalSmokeOptions, base: SmokeProfile) {
    let original = options.profile;
    options.profile = Some(base);
    apply_smoke_profile(options);
    options.profile = original;
}

#[allow(dead_code)]
fn describe_smoke_settings(options: &LocalSmokeOptions) -> Value {
    json!({
        "profile": options.profile.map(SmokeProfile::as_str),
        "clients": options.clients,
        "duration": options.duration,
        "startup_timeout": options.startup_timeout,
        "match_timeout": options.match_timeout,
        "client_launch_spacing": options.client_launch_spacing,
        "auto_start_min_players": options.auto_start_min_players,
        "expected_players": options.expected_players,
        "expected_saboteurs": options.expected_saboteurs,
        "map": options.map,
        "run_id": options.run_id,
        "build_id": options.build_id,
        "host_only": options.host_only,
        "smoke_interact": options.smoke_interact,
        "smoke_down_rescue": options.smoke_down_rescue,
        "smoke_containment": options.smoke_containment,
        "smoke_route_complete": options.smoke_route_complete,
        "smoke_fatal_sabotage": options.smoke_fatal_sabotage,
        "smoke_bulkhead_lock": options.smoke_bulkhead_lock,
        "smoke_pump_flooding": options.smoke_pump_flooding,
        "smoke_pve_enemy": options.smoke_pve_enemy,
        "smoke_pve_damage": options.smoke_pve_damage,
        "smoke_item_drop": options.smoke_item_drop,
        "smoke_combined_systems": options.smoke_combined_systems,
        "smoke_qa_bot": options.smoke_qa_bot,
        "smoke_qa_player_bot": options.smoke_qa_player_bot,
        "smoke_qa_task_bot": options.smoke_qa_task_bot,
        "smoke_match_timer": options.smoke_match_timer,
        "smoke_life_action": options.smoke_life_action,
        "no_auto_start": options.no_auto_start,
        "auto_ready": options.auto_ready,
        "lobby_min_players": options.lobby_min_players,
        "lobby_dev_saboteurs": options.lobby_dev_saboteurs,
        "practice_mode": options.practice_mode,
        "single_player_mode": options.single_player_mode,
    })
}

#[allow(dead_code)]
fn build_editor_target(ue_root: &Path, platform: &str) -> Result<(), String> {
    let project = repo_root().join("Frostwake.uproject");
    let build = unreal_build_script(ue_root, platform);
    let command = vec![
        build.to_string_lossy().to_string(),
        "FrostwakeEditor".to_string(),
        platform.to_string(),
        "Development".to_string(),
        format!("-Project={}", project.display()),
        "-WaitMutex".to_string(),
        "-NoLiveCoding".to_string(),
    ];
    let result = run_process("build_editor", &command, None);
    if result.status == "pass" {
        Ok(())
    } else {
        Err(format!("Editor build failed: {}", result.detail))
    }
}

#[allow(dead_code)]
fn make_smoke_log_dir(value: &Option<PathBuf>) -> Result<PathBuf, String> {
    let path = if let Some(value) = value {
        resolve_path(value)
    } else {
        repo_root()
            .join("Saved/SmokeTests")
            .join(format!("local-{}", Local::now().format("%Y%m%d-%H%M%S")))
    };
    fs::create_dir_all(&path).map_err(|error| format!("{}: {error}", path.display()))?;
    Ok(path)
}

#[allow(dead_code)]
fn common_unreal_args(options: &LocalSmokeOptions) -> Vec<String> {
    let mut values = vec![
        "-game",
        "-stdout",
        "-FullStdOutLogOutput",
        "-Unattended",
        "-NoSound",
        "-NoSplash",
        "-NoLiveCoding",
        "-nop4",
    ]
    .into_iter()
    .map(str::to_string)
    .collect::<Vec<_>>();
    if options.null_rhi {
        values.push("-nullrhi".to_string());
    }
    if options.windowed {
        values.extend(
            ["-windowed", "-ResX=960", "-ResY=540"]
                .into_iter()
                .map(str::to_string),
        );
    }
    values
}

#[allow(dead_code)]
fn open_logged_process(name: &str, command: &[String], log_path: &Path) -> Result<Child, String> {
    let mut log =
        fs::File::create(log_path).map_err(|error| format!("{}: {error}", log_path.display()))?;
    writeln!(log, "$ {}\n", command.join(" "))
        .map_err(|error| format!("{}: {error}", log_path.display()))?;
    let stdout = log
        .try_clone()
        .map_err(|error| format!("{}: {error}", log_path.display()))?;
    let child = ProcessCommand::new(&command[0])
        .args(&command[1..])
        .current_dir(repo_root())
        .stdout(Stdio::from(stdout))
        .stderr(Stdio::from(log))
        .spawn()
        .map_err(|error| format!("{}: {error}", command[0]))?;
    println!(
        "[START] {name}: pid={} log={}",
        child.id(),
        log_path.display()
    );
    Ok(child)
}

#[allow(dead_code)]
fn close_child(child: &mut Child) {
    if child.try_wait().ok().flatten().is_some() {
        return;
    }
    let _ = child.kill();
    let _ = child.wait();
}

#[allow(dead_code)]
fn log_contains(path: &Path, needles: &[&str]) -> bool {
    fs::read_to_string(path)
        .map(|text| needles.iter().any(|needle| text.contains(needle)))
        .unwrap_or(false)
}

#[allow(dead_code)]
fn log_has_failure(path: &Path) -> bool {
    log_contains(
        path,
        &[
            "Fatal error",
            "Unhandled Exception",
            "Critical error",
            "appError called",
        ],
    )
}

#[allow(dead_code)]
fn wait_for_log(path: &Path, needles: &[&str], timeout_seconds: u64) -> bool {
    let deadline = std::time::Instant::now() + Duration::from_secs(timeout_seconds);
    while std::time::Instant::now() < deadline {
        if log_contains(path, needles) {
            return true;
        }
        std::thread::sleep(Duration::from_secs(1));
    }
    false
}

#[allow(dead_code)]
fn last_role_assignment(events_log: &Path) -> Option<Value> {
    let text = fs::read_to_string(events_log).ok()?;
    let mut latest = None;
    for line in text.lines() {
        let Ok(event) = serde_json::from_str::<Value>(line) else {
            continue;
        };
        if event.get("event").and_then(Value::as_str) == Some("role_assignment_complete")
            && event.get("payload").is_some_and(Value::is_object)
        {
            latest = event.get("payload").cloned();
        }
    }
    latest
}

#[allow(dead_code)]
fn run_local_smoke(options: &LocalSmokeOptions) -> Result<(), String> {
    if options.clients < 0 {
        return Err("--clients must be >= 0".to_string());
    }
    let project = repo_root().join("Frostwake.uproject");
    if !project.exists() {
        return Err(format!("Missing project: {}", project.display()));
    }
    let ue_root = resolve_ue_root(options.ue_root.clone());
    let editor = unreal_editor_path(&ue_root, &options.platform);
    if !editor.exists() {
        return Err(format!("Missing UnrealEditor: {}", editor.display()));
    }
    if !options.skip_build {
        build_editor_target(&ue_root, &options.platform)?;
    }

    let log_dir = make_smoke_log_dir(&options.log_dir)?;
    let run_id = options.run_id.clone().unwrap_or_else(|| {
        log_dir
            .file_name()
            .and_then(|name| name.to_str())
            .unwrap_or("local-smoke")
            .to_string()
    });
    let profile = options
        .profile
        .map(SmokeProfile::as_str)
        .unwrap_or("custom");
    let common = common_unreal_args(options);
    let auto_start_min_players = options.auto_start_min_players.unwrap_or_else(|| {
        if options.clients > 0 {
            options.clients + 1
        } else {
            1
        }
    });
    let host_log = log_dir.join("listen_host.log");
    let events_log = log_dir.join("events.jsonl");
    let mut host_command = vec![
        editor.to_string_lossy().to_string(),
        project.to_string_lossy().to_string(),
        format!("{}?listen", options.map),
    ];
    host_command.extend(common.clone());
    host_command.extend([
        format!("-port={}", options.port),
        format!("-FrostwakeEventLog={}", events_log.display()),
        format!("-FrostwakeRunId={run_id}"),
        format!("-FrostwakeBuildId={}", options.build_id),
        format!("-FrostwakeMapId={}", options.map),
        format!("-FrostwakeProfile={profile}"),
    ]);
    if !options.no_auto_start {
        host_command.extend([
            "-FrostwakeAutoStart".to_string(),
            format!("-FrostwakeAutoStartMinPlayers={auto_start_min_players}"),
            "-FrostwakeDevSaboteurs=1".to_string(),
        ]);
    }
    for (enabled, flag) in [
        (options.smoke_interact, "-FrostwakeSmokeInteract"),
        (options.smoke_down_rescue, "-FrostwakeSmokeDownRescue"),
        (options.smoke_containment, "-FrostwakeSmokeContainment"),
        (options.smoke_route_complete, "-FrostwakeSmokeRouteComplete"),
        (options.smoke_fatal_sabotage, "-FrostwakeSmokeFatalSabotage"),
        (options.smoke_bulkhead_lock, "-FrostwakeSmokeBulkheadLock"),
        (options.smoke_pump_flooding, "-FrostwakeSmokePumpFlooding"),
        (options.smoke_item_drop, "-FrostwakeSmokeItemDrop"),
        (
            options.smoke_combined_systems,
            "-FrostwakeSmokeCombinedSystems",
        ),
        (options.smoke_qa_bot, "-FrostwakeSmokeQaBot"),
        (options.smoke_qa_player_bot, "-FrostwakeSmokeQaPlayerBot"),
        (options.smoke_qa_task_bot, "-FrostwakeSmokeQaTaskBot"),
        (options.smoke_life_action, "-FrostwakeSmokeLifeAction"),
        (options.smoke_eat, "-FrostwakeSmokeEat"),
        (options.smoke_damage_type, "-FrostwakeSmokeDamageType"),
        (options.smoke_survival, "-FrostwakeSmokeSurvival"),
        (options.smoke_effect, "-FrostwakeSmokeEffect"),
        (options.auto_ready, "-FrostwakeAutoReady"),
        (options.practice_mode, "-FrostwakePracticeMode"),
        (options.single_player_mode, "-FrostwakeSinglePlayer"),
    ] {
        if enabled {
            host_command.push(flag.to_string());
        }
    }
    if options.smoke_pve_enemy || options.smoke_pve_damage {
        host_command.push("-FrostwakeSmokePveEnemy".to_string());
    }
    if options.smoke_pve_damage {
        host_command.push("-FrostwakeSmokePvEDamage".to_string());
    }
    if options.smoke_match_timer {
        host_command.push("-FrostwakeMatchDurationSeconds=1.0".to_string());
    }
    if let Some(value) = options.lobby_min_players {
        host_command.push(format!("-FrostwakeLobbyMinPlayers={value}"));
    }
    if let Some(value) = options.lobby_dev_saboteurs {
        host_command.push(format!("-FrostwakeLobbyDevSaboteurs={value}"));
    }

    let mut processes = Vec::new();
    let result = (|| -> Result<(), String> {
        let host = open_logged_process("listen_host", &host_command, &host_log)?;
        processes.push(host);
        if !wait_for_log(
            &host_log,
            &[
                "IpNetDriver listening on port",
                &format!("listening on port {}", options.port),
                "Load map complete /Game/Maps/L_IcebreakerWhitebox",
            ],
            options.startup_timeout,
        ) || processes[0].try_wait().ok().flatten().is_some()
        {
            return Err(format!(
                "[FAIL] listen host did not reach startup evidence within {}s",
                options.startup_timeout
            ));
        }
        for index in 1..=options.clients {
            let client_log = log_dir.join(format!("client_{index:02}.log"));
            let mut client_command = vec![
                editor.to_string_lossy().to_string(),
                project.to_string_lossy().to_string(),
                format!("127.0.0.1:{}", options.port),
            ];
            client_command.extend(common.clone());
            client_command.push(format!("-port={}", options.port + index));
            let child =
                open_logged_process(&format!("client_{index:02}"), &client_command, &client_log)?;
            processes.push(child);
            std::thread::sleep(Duration::from_secs_f64(options.client_launch_spacing));
        }
        if !wait_for_log(
            &host_log,
            &["dev_auto_start", "role_assignment_complete"],
            options.match_timeout,
        ) {
            return Err(format!(
                "[FAIL] match did not auto-start within {}s",
                options.match_timeout
            ));
        }
        let smoke_checks = [
            (
                options.smoke_interact,
                &["dev_smoke_task_interaction", "ship_task_applied"][..],
                "task smoke interaction",
            ),
            (
                options.smoke_down_rescue,
                &["dev_smoke_down_rescue", "player_rescued"][..],
                "down/rescue smoke",
            ),
            (
                options.smoke_containment,
                &["dev_smoke_containment", "player_released"][..],
                "containment smoke",
            ),
            (
                options.smoke_route_complete,
                &["dev_smoke_route_complete", "final_approach_started"][..],
                "route completion smoke",
            ),
            (
                options.smoke_fatal_sabotage,
                &["dev_smoke_fatal_sabotage", "fatal_ship_state"][..],
                "fatal sabotage smoke",
            ),
            (
                options.smoke_bulkhead_lock,
                &["dev_smoke_bulkhead_lock result=pass"][..],
                "bulkhead lock smoke",
            ),
            (
                options.smoke_pump_flooding,
                &["dev_smoke_pump_flooding result=pass"][..],
                "pump/flooding smoke",
            ),
            (
                options.smoke_pve_enemy || options.smoke_pve_damage,
                &["dev_smoke_pve_enemy result=pass"][..],
                "PvE enemy smoke",
            ),
            (
                options.smoke_item_drop,
                &["dev_smoke_item_drop result=pass"][..],
                "item drop smoke",
            ),
            (
                options.smoke_combined_systems,
                &["dev_smoke_combined_systems result=pass"][..],
                "combined systems smoke",
            ),
            (
                options.smoke_qa_bot,
                &["dev_smoke_qa_bot result=pass"][..],
                "QA bot smoke",
            ),
            (
                options.smoke_qa_player_bot,
                &["dev_smoke_qa_player_bot result=pass"][..],
                "QA player bot smoke",
            ),
            (
                options.smoke_qa_task_bot,
                &["dev_smoke_qa_task_bot result=pass"][..],
                "QA task bot smoke",
            ),
            (
                options.smoke_match_timer,
                &["match_timer_expired", "timer_expired"][..],
                "match timer smoke",
            ),
            (
                options.smoke_life_action,
                &["dev_smoke_life_action result=pass"][..],
                "life action smoke",
            ),
        ];
        for (enabled, needles, label) in smoke_checks {
            if enabled && !wait_for_log(&host_log, needles, options.match_timeout) {
                return Err(format!(
                    "[FAIL] {label} did not complete within {}s",
                    options.match_timeout
                ));
            }
        }
        if options.expected_players.is_some() || options.expected_saboteurs.is_some() {
            let assignment = last_role_assignment(&events_log).ok_or_else(|| {
                "[FAIL] no role_assignment_complete event found in events.jsonl".to_string()
            })?;
            if let Some(expected) = options.expected_players
                && assignment.get("players").and_then(Value::as_i64) != Some(expected)
            {
                return Err(format!(
                    "[FAIL] expected players={expected}, got {}",
                    display_value(assignment.get("players"))
                ));
            }
            if let Some(expected) = options.expected_saboteurs
                && assignment.get("saboteurs").and_then(Value::as_i64) != Some(expected)
            {
                return Err(format!(
                    "[FAIL] expected saboteurs={expected}, got {}",
                    display_value(assignment.get("saboteurs"))
                ));
            }
        }
        println!("[RUN] holding smoke processes for {}s", options.duration);
        std::thread::sleep(Duration::from_secs(options.duration));
        let failed_logs = fs::read_dir(&log_dir)
            .map_err(|error| format!("{}: {error}", log_dir.display()))?
            .filter_map(Result::ok)
            .map(|entry| entry.path())
            .filter(|path| {
                path.extension().and_then(|ext| ext.to_str()) == Some("log")
                    && log_has_failure(path)
            })
            .collect::<Vec<_>>();
        if !failed_logs.is_empty() {
            return Err(format!(
                "[FAIL] fatal/crash terms found:\n{}",
                failed_logs
                    .iter()
                    .map(|path| format!("  - {}", path.display()))
                    .collect::<Vec<_>>()
                    .join("\n")
            ));
        }
        println!(
            "[PASS] local smoke launch completed. logs={}",
            log_dir.display()
        );
        Ok(())
    })();
    for child in processes.iter_mut().rev() {
        close_child(child);
    }
    result
}

#[allow(dead_code)]
struct SmokeSuiteOptions {
    profiles: Vec<SmokeProfile>,
    include_heavy: bool,
    skip_build: bool,
    null_rhi: bool,
    platform: Option<String>,
    ue_root: Option<PathBuf>,
    suite_dir: Option<PathBuf>,
}

#[allow(dead_code)]
fn resolve_suite_profiles(options: &SmokeSuiteOptions) -> Vec<SmokeProfile> {
    if !options.profiles.is_empty() {
        return options.profiles.clone();
    }
    let mut profiles = vec![
        SmokeProfile::QaBot,
        SmokeProfile::QaPlayerBot,
        SmokeProfile::MatchTimer,
    ];
    if options.include_heavy {
        profiles.extend([
            SmokeProfile::QaTaskBot,
            SmokeProfile::LifeAction,
            SmokeProfile::Combined5,
            SmokeProfile::Ready8,
            SmokeProfile::Combined8,
        ]);
    }
    profiles
}

#[allow(dead_code)]
fn make_suite_dir(value: &Option<PathBuf>) -> Result<PathBuf, String> {
    let path = if let Some(value) = value {
        resolve_path(value)
    } else {
        repo_root()
            .join("Saved/SmokeSuites")
            .join(format!("suite-{}", Local::now().format("%Y%m%d-%H%M%S")))
    };
    fs::create_dir_all(&path).map_err(|error| format!("{}: {error}", path.display()))?;
    Ok(path)
}

#[allow(dead_code)]
fn run_command_capture(command: &[String]) -> Result<(i32, String, String), String> {
    let output = ProcessCommand::new(&command[0])
        .args(&command[1..])
        .current_dir(repo_root())
        .output()
        .map_err(|error| format!("{}: {error}", command[0]))?;
    Ok((
        output.status.code().unwrap_or(-1),
        String::from_utf8_lossy(&output.stdout).to_string(),
        String::from_utf8_lossy(&output.stderr).to_string(),
    ))
}

#[allow(dead_code)]
fn run_smoke_suite(options: &SmokeSuiteOptions) -> Result<i32, String> {
    let profiles = resolve_suite_profiles(options);
    let suite_dir = make_suite_dir(&options.suite_dir)?;
    let exe = std::env::current_exe().map_err(|error| format!("current exe: {error}"))?;
    let mut results = Vec::new();
    let mut exit_code = 0;
    for (index, profile) in profiles.iter().enumerate() {
        let profile_dir = suite_dir.join(format!("{:02}-{}", index + 1, profile.as_str()));
        let mut command = vec![
            exe.to_string_lossy().to_string(),
            "run-local-smoke".to_string(),
            "--profile".to_string(),
            profile.as_str().to_string(),
            "--log-dir".to_string(),
            profile_dir.to_string_lossy().to_string(),
        ];
        if let Some(platform) = &options.platform {
            command.extend(["--platform".to_string(), platform.clone()]);
        }
        if let Some(ue_root) = &options.ue_root {
            command.extend([
                "--ue-root".to_string(),
                ue_root.to_string_lossy().to_string(),
            ]);
        }
        if options.skip_build {
            command.push("--skip-build".to_string());
        }
        if options.null_rhi {
            command.push("--null-rhi".to_string());
        }
        println!(
            "[SUITE] running profile={} log_dir={}",
            profile.as_str(),
            profile_dir.display()
        );
        let (code, stdout, stderr) = run_command_capture(&command)?;
        print!("{stdout}");
        eprint!("{stderr}");
        let events_path = profile_dir.join("events.jsonl");
        let summary = if events_path.exists() {
            match summarize_events(&events_path) {
                Ok(summary) => Value::Object(summary.into_iter().collect()),
                Err(error) => json!({"summary_error": error}),
            }
        } else {
            Value::Null
        };
        results.push(json!({
            "profile": profile.as_str(),
            "returncode": code,
            "log_dir": profile_dir.to_string_lossy(),
            "events": events_path.to_string_lossy(),
            "summary": summary,
        }));
        if code != 0 {
            exit_code = code;
            break;
        }
    }
    let suite_summary = json!({
        "suite_dir": suite_dir.to_string_lossy(),
        "profiles": profiles.iter().map(|profile| profile.as_str()).collect::<Vec<_>>(),
        "passed": exit_code == 0,
        "results": results,
    });
    let summary_path = suite_dir.join("suite_summary.json");
    fs::write(
        &summary_path,
        format!(
            "{}\n",
            serde_json::to_string_pretty(&suite_summary).expect("suite summary serializes")
        ),
    )
    .map_err(|error| format!("{}: {error}", summary_path.display()))?;
    println!("[SUITE] summary={}", summary_path.display());
    Ok(exit_code)
}

#[allow(dead_code)]
fn nested_summary(summary: Option<&Value>, key: &str) -> String {
    summary
        .and_then(|summary| summary.get(key))
        .and_then(|value| {
            if value.is_null() {
                None
            } else {
                Some(display_value(Some(value)))
            }
        })
        .unwrap_or_default()
}

#[allow(dead_code)]
fn render_smoke_suite_markdown(data: &Value) -> Result<String, String> {
    let object = data
        .as_object()
        .ok_or_else(|| "suite summary must be a JSON object".to_string())?;
    let profiles = object
        .get("profiles")
        .and_then(Value::as_array)
        .into_iter()
        .flatten()
        .filter_map(Value::as_str)
        .collect::<Vec<_>>()
        .join(", ");
    let mut lines = vec![
        "# Smoke Suite Evidence".to_string(),
        String::new(),
        format!("- Suite: `{}`", display_value(object.get("suite_dir"))),
        format!("- Passed: `{}`", display_value(object.get("passed"))),
        format!("- Profiles: `{profiles}`"),
        String::new(),
        "| Profile | Return | Match Started | Roles | Smoke Pass | Invalid Lines | Log Dir |"
            .to_string(),
        "| --- | ---: | ---: | --- | ---: | ---: | --- |".to_string(),
    ];
    for result in object
        .get("results")
        .and_then(Value::as_array)
        .into_iter()
        .flatten()
    {
        let summary = result.get("summary");
        let smoke_pass = [
            "qa_bot_smoke_passed",
            "qa_player_bot_smoke_passed",
            "qa_task_bot_smoke_passed",
            "combined_systems_smoke_passed",
            "item_drop_smoke_passed",
            "pump_flooding_smoke_passed",
            "pve_enemy_smoke_passed",
            "bulkhead_lock_smoke_passed",
        ]
        .into_iter()
        .map(|key| nested_summary(summary, key))
        .find(|value| !value.is_empty())
        .unwrap_or_default();
        let roles = summary
            .and_then(|summary| summary.get("last_role_assignment"))
            .and_then(Value::as_object)
            .map(|assignment| {
                format!(
                    "{}p/{}s",
                    display_value(assignment.get("players")),
                    display_value(assignment.get("saboteurs"))
                )
            })
            .unwrap_or_default();
        lines.push(format!(
            "| {} | {} | {} | {} | {} | {} | `{}` |",
            display_value(result.get("profile")),
            display_value(result.get("returncode")),
            nested_summary(summary, "match_started"),
            roles,
            smoke_pass,
            nested_summary(summary, "invalid_lines"),
            display_value(result.get("log_dir")),
        ));
    }
    lines.push(String::new());
    Ok(lines.join("\n"))
}

fn build_visual_poc_manifest(
    source_map: &str,
    target_map: &str,
    dry_run: bool,
) -> Result<Value, String> {
    if source_map != "/Game/Maps/L_IcebreakerWhitebox" {
        return Err(
            "source_map must remain /Game/Maps/L_IcebreakerWhitebox for this scaffold".to_string(),
        );
    }
    if target_map == source_map {
        return Err("target_map must not equal source_map".to_string());
    }
    if !target_map.starts_with("/Game/Maps/") {
        return Err("target_map must be under /Game/Maps/".to_string());
    }
    if target_map.contains("IcebreakerWhitebox") {
        return Err("target_map must not look like the automation whitebox map".to_string());
    }
    Ok(json!({
        "mode": if dry_run { "dry-run" } else { "plan" },
        "source_map": source_map,
        "target_map": target_map,
        "will_modify_unreal_assets": false,
        "will_change_default_maps": false,
        "quarantine_root": "Content/ThirdParty/Quarantine",
        "project_art_root": "Content/Frostwake",
        "poc_zones": [
            {
                "zone_id": "central-corridor",
                "purpose": "Social choke point and proximity voice suspicion",
                "source_whitebox_area": "Mess / Workshop / central passage",
                "art_direction": "near-future bulkheads, emergency LEDs, frost on windows, readable sight breaks",
                "acceptance": "players can read it as the main meeting and accusation space within 5 seconds",
            },
            {
                "zone_id": "engine-fuel-battery-bay",
                "purpose": "Core systems pressure and sabotage readability",
                "source_whitebox_area": "Engine Room / Fuel Store / Boiler",
                "art_direction": "industrial ship machinery, battery racks, fuel lines, pumps, repair panels",
                "acceptance": "players can identify at least two interactable system targets without UI explanation",
            },
            {
                "zone_id": "exterior-ice-access",
                "purpose": "High-risk exterior route and isolation pressure",
                "source_whitebox_area": "Stern Work Deck / Ice Field Access",
                "art_direction": "snowy metal deck, floodlights, containers, safety rails, whiteout boundary",
                "acceptance": "players immediately understand that leaving the vessel is useful but dangerous",
            },
        ],
        "required_ledger_fields": [
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
        ],
        "blocked_actions": [
            "Do not rename or overwrite /Game/Maps/L_IcebreakerWhitebox.",
            "Do not change GameDefaultMap or EditorStartupMap for the visual POC.",
            "Do not import third-party assets into Content/Frostwake until approved.",
            "Do not purchase or import paid assets without a fresh listing/license check.",
        ],
        "next_execute_step": "Create the target map in Unreal Editor only after selecting assets and recording candidate ledger entries. This command intentionally does not create .umap files.",
    }))
}

fn unreal_commandlet_command(commandlet: &str, ue_root: &Path, platform: &str) -> Vec<String> {
    vec![
        unreal_editor_cmd_path(ue_root, platform)
            .to_string_lossy()
            .to_string(),
        repo_root()
            .join("Frostwake.uproject")
            .to_string_lossy()
            .to_string(),
        format!("-run={commandlet}"),
        "-unattended".to_string(),
        "-nop4".to_string(),
    ]
}

fn run_unreal_editor_commandlet(
    commandlet: &str,
    ue_root: Option<PathBuf>,
    platform: String,
    print_command: bool,
    skip_build: bool,
) -> Result<(), String> {
    let ue_root = resolve_ue_root(ue_root);
    let command = unreal_commandlet_command(commandlet, &ue_root, &platform);
    if print_command {
        println!("{}", shell_command(&command));
        return Ok(());
    }
    if !skip_build {
        build_editor_target(&ue_root, &platform)?;
    }
    let result = run_process(commandlet, &command, None);
    if result.status == "pass" {
        println!("[PASS] {commandlet}");
        Ok(())
    } else {
        Err(result.detail)
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    use std::net::TcpListener;
    use std::thread;
    use std::time::{SystemTime, UNIX_EPOCH};

    fn load_fixture(path: &str) -> Map<String, Value> {
        load_json_object(Path::new(path)).expect("fixture loads")
    }

    #[test]
    fn server_config_example_matches_schema() {
        let schema = load_fixture("Tools/ops/server_config.schema.json");
        let example = load_fixture("Tools/ops/server_config.example.json");

        assert_eq!(
            validate_schema_value(&Value::Object(schema), &Value::Object(example), "$"),
            Vec::<String>::new()
        );
    }

    #[test]
    fn server_config_summary_matches_legacy_contract() {
        let example = load_fixture("Tools/ops/server_config.example.json");
        let summary = summarize_server_config(&example);

        assert_eq!(summary["maxPlayers"], json!(8));
        assert_eq!(summary["map"], json!("L_IcebreakerWhitebox"));
        assert_eq!(summary["hasAdminTokenEnv"], json!(true));
    }

    #[test]
    fn lobby_metadata_example_matches_schema_and_domain_rules() {
        let schema = load_fixture("Tools/ops/lobby_metadata.schema.json");
        let example = load_fixture("Tools/ops/lobby_metadata.example.json");

        assert_eq!(
            validate_lobby_metadata(
                &schema,
                &example,
                Some("Frostwake-Win64-Development-local"),
                Some("L_IcebreakerWhitebox")
            ),
            Vec::<String>::new()
        );
    }

    #[test]
    fn lobby_metadata_rejects_build_mismatch() {
        let schema = load_fixture("Tools/ops/lobby_metadata.schema.json");
        let example = load_fixture("Tools/ops/lobby_metadata.example.json");
        let errors = validate_lobby_metadata(&schema, &example, Some("DifferentBuild"), None);

        assert!(errors.iter().any(|error| error.contains("buildId")));
        assert!(errors.iter().any(|error| error.contains("mismatch")));
    }

    #[test]
    fn lobby_metadata_rejects_raw_ip_endpoint() {
        let schema = load_fixture("Tools/ops/lobby_metadata.schema.json");
        let mut example = load_fixture("Tools/ops/lobby_metadata.example.json");
        example.insert("endpointToken".to_string(), json!("127.0.0.1:7777"));
        let errors = validate_lobby_metadata(&schema, &example, None, None);

        assert!(errors.iter().any(|error| error.contains("endpointToken")));
    }

    #[test]
    fn lobby_metadata_rejects_inconsistent_player_counts() {
        let schema = load_fixture("Tools/ops/lobby_metadata.schema.json");
        let mut example = load_fixture("Tools/ops/lobby_metadata.example.json");
        example.insert("currentPlayers".to_string(), json!(8));
        example.insert("joinState".to_string(), json!("open"));
        let errors = validate_lobby_metadata(&schema, &example, None, None);

        assert!(
            errors
                .iter()
                .any(|error| error.contains("open lobby cannot already be full"))
        );
    }

    #[test]
    fn lobby_metadata_rejects_standard_lobby_below_experience_floor() {
        let schema = load_fixture("Tools/ops/lobby_metadata.schema.json");
        let mut example = load_fixture("Tools/ops/lobby_metadata.example.json");
        example.insert("lobbyType".to_string(), json!("standard"));
        example.insert("minimumCompletedMatches".to_string(), json!(49));
        let errors = validate_lobby_metadata(&schema, &example, None, None);

        assert!(
            errors
                .iter()
                .any(|error| error.contains("standard lobbies require at least 50"))
        );
    }

    #[test]
    fn lobby_metadata_accepts_madman_mode() {
        let schema = load_fixture("Tools/ops/lobby_metadata.schema.json");
        let mut example = load_fixture("Tools/ops/lobby_metadata.example.json");
        example.insert("mode".to_string(), json!("madman"));
        example.insert("difficulty".to_string(), json!("hard"));
        let errors = validate_lobby_metadata(&schema, &example, None, None);

        assert_eq!(errors, Vec::<String>::new());
    }

    #[test]
    fn lobby_metadata_rejects_unknown_mode() {
        let schema = load_fixture("Tools/ops/lobby_metadata.schema.json");
        let mut example = load_fixture("Tools/ops/lobby_metadata.example.json");
        example.insert("mode".to_string(), json!("chaos"));
        let errors = validate_lobby_metadata(&schema, &example, None, None);

        assert!(errors.iter().any(|error| error.contains("mode")));
        assert!(errors.iter().any(|error| error.contains("not in enum")));
    }

    #[test]
    fn lobby_metadata_rejects_unknown_difficulty() {
        let schema = load_fixture("Tools/ops/lobby_metadata.schema.json");
        let mut example = load_fixture("Tools/ops/lobby_metadata.example.json");
        example.insert("difficulty".to_string(), json!("nightmare"));
        let errors = validate_lobby_metadata(&schema, &example, None, None);

        assert!(errors.iter().any(|error| error.contains("difficulty")));
    }

    #[test]
    fn lobby_join_decision_accepts_matching_open_lobby() {
        let schema = load_fixture("Tools/ops/lobby_metadata.schema.json");
        let example = load_fixture("Tools/ops/lobby_metadata.example.json");

        let decision = decide_lobby_join(
            &schema,
            &example,
            Some("Frostwake-Win64-Development-local"),
            Some("L_IcebreakerWhitebox"),
        );

        assert_eq!(decision["accepted"], json!(true));
        assert_eq!(decision["travelAllowed"], json!(true));
        assert_eq!(decision["rejectReason"], json!("None"));
        assert!(decision.get("validationErrors").is_none());
    }

    #[test]
    fn lobby_join_decision_rejects_build_mismatch_before_travel() {
        let schema = load_fixture("Tools/ops/lobby_metadata.schema.json");
        let example = load_fixture("Tools/ops/lobby_metadata.example.json");

        let decision = decide_lobby_join(&schema, &example, Some("DifferentBuild"), None);

        assert_eq!(decision["accepted"], json!(false));
        assert_eq!(decision["travelAllowed"], json!(false));
        assert_eq!(decision["rejectReason"], json!("BuildMismatch"));
        assert_eq!(decision["expectedBuildId"], json!("DifferentBuild"));
    }

    #[test]
    fn lobby_join_decision_rejects_map_mismatch_before_travel() {
        let schema = load_fixture("Tools/ops/lobby_metadata.schema.json");
        let example = load_fixture("Tools/ops/lobby_metadata.example.json");

        let decision = decide_lobby_join(&schema, &example, None, Some("DifferentMap"));

        assert_eq!(decision["accepted"], json!(false));
        assert_eq!(decision["travelAllowed"], json!(false));
        assert_eq!(decision["rejectReason"], json!("MapMismatch"));
        assert_eq!(decision["expectedMapId"], json!("DifferentMap"));
    }

    #[test]
    fn lobby_join_decision_rejects_closed_join_states() {
        let schema = load_fixture("Tools/ops/lobby_metadata.schema.json");
        let example = load_fixture("Tools/ops/lobby_metadata.example.json");

        let mut full = example.clone();
        full.insert("currentPlayers".to_string(), json!(8));
        full.insert("joinState".to_string(), json!("full"));
        assert_eq!(
            decide_lobby_join(&schema, &full, None, None)["rejectReason"],
            json!("LobbyFull")
        );

        let mut locked = example.clone();
        locked.insert("joinState".to_string(), json!("locked"));
        assert_eq!(
            decide_lobby_join(&schema, &locked, None, None)["rejectReason"],
            json!("LobbyLocked")
        );

        let mut in_match = example;
        in_match.insert("joinState".to_string(), json!("in_match"));
        assert_eq!(
            decide_lobby_join(&schema, &in_match, None, None)["rejectReason"],
            json!("AlreadyInMatch")
        );
    }

    #[test]
    fn lobby_join_decision_expectations_fail_on_unexpected_reason() {
        let decision = BTreeMap::from([
            ("accepted".to_string(), json!(false)),
            ("rejectReason".to_string(), json!("BuildMismatch")),
        ]);

        let errors = check_lobby_join_decision_expectations(&decision, false, Some("MapMismatch"));

        assert!(
            errors
                .iter()
                .any(|error| error.contains("expected rejectReason=MapMismatch"))
        );
    }

    #[test]
    fn asset_ledger_accepts_current_candidates() {
        let errors = validate_asset_ledger(Path::new("docs/asset-ledger-candidates.csv"))
            .expect("asset ledger validates");

        assert_eq!(errors, Vec::<String>::new());
    }

    #[test]
    fn asset_ledger_rejects_duplicate_ids_and_production_candidate_paths() {
        let root = temp_root("asset_ledger_duplicate");
        let path = root.join("ledger.csv");
        let mut row = valid_asset_ledger_row("local-test-asset");
        row.insert(
            "output_path".to_string(),
            "Content/Frostwake/Props/TestAsset.uasset".to_string(),
        );
        write_asset_ledger_fixture(&path, &[row.clone(), row]);

        let errors = validate_asset_ledger(&path).expect("asset ledger checked");

        assert!(
            errors
                .iter()
                .any(|error| error.contains("duplicate asset_id local-test-asset"))
        );
        assert!(
            errors
                .iter()
                .any(|error| error.contains("non-approved assets may not point"))
        );
    }

    #[test]
    fn asset_ledger_requires_approved_license_evidence() {
        let root = temp_root("asset_ledger_approved");
        let path = root.join("ledger.csv");
        let mut row = valid_asset_ledger_row("approved-test-asset");
        row.insert("adoption_state".to_string(), "approved".to_string());
        row.insert("commercial_use_allowed".to_string(), "no".to_string());
        row.insert(
            "output_path".to_string(),
            "Content/Frostwake/Approved/Test.uasset".to_string(),
        );
        write_asset_ledger_fixture(&path, &[row]);

        let errors = validate_asset_ledger(&path).expect("asset ledger checked");

        assert!(
            errors
                .iter()
                .any(|error| error.contains("approved assets require commercial_use_allowed=yes"))
        );
        assert!(
            errors
                .iter()
                .any(|error| error.contains("approved assets require license"))
        );
        assert!(
            errors
                .iter()
                .any(|error| error.contains("approved assets require proof_of_purchase_path"))
        );
    }

    #[test]
    fn reference_inventory_records_missing_sources() {
        let root = temp_root("reference_inventory_missing");
        let manifest = root.join("external-paths.json");
        let missing = root.join("missing-source");
        fs::write(
            &manifest,
            json!({"paths": [{"path": missing.to_string_lossy()}]}).to_string(),
        )
        .expect("write manifest");

        let output = write_reference_inventory(&ReferenceInventoryOptions {
            manifest,
            out_dir: root.join("inventory"),
            max_depth: 3,
            max_files_per_source: 1000,
            hash_files: false,
        })
        .expect("write inventory");
        let inventory = fs::read_to_string(output.inventory_path).expect("read inventory");

        assert!(inventory.contains("missing"));
        assert!(inventory.contains(&missing.to_string_lossy().to_string()));
    }

    #[test]
    fn reference_inventory_scans_files_and_skips_excluded_dirs() {
        let root = temp_root("reference_inventory_files");
        let source = root.join("source");
        fs::create_dir_all(source.join("nested")).expect("nested dir");
        fs::create_dir_all(source.join("Saved")).expect("saved dir");
        fs::write(source.join("a.txt"), "alpha").expect("write txt");
        fs::write(source.join("nested").join("b.rs"), "fn main() {}\n").expect("write rs");
        fs::write(source.join("Saved").join("skip.log"), "skip").expect("write skipped");
        let manifest = root.join("external-paths.json");
        fs::write(
            &manifest,
            json!({"paths": [{"path": source.to_string_lossy()}]}).to_string(),
        )
        .expect("write manifest");

        let output = write_reference_inventory(&ReferenceInventoryOptions {
            manifest,
            out_dir: root.join("inventory"),
            max_depth: 3,
            max_files_per_source: 1000,
            hash_files: false,
        })
        .expect("write inventory");
        let inventory = fs::read_to_string(&output.inventory_path).expect("read inventory");
        let summary = fs::read_to_string(&output.summary_path).expect("read summary");

        assert!(inventory.contains("a.txt"));
        assert!(inventory.contains("b.rs"));
        assert!(!inventory.contains("skip.log"));
        assert!(inventory.contains("skipped"));
        assert!(summary.contains(".txt"));
        assert!(summary.contains(".rs"));
    }

    #[test]
    fn redaction_recurses_into_nested_mappings_and_lists() {
        let value = json!({
            "server": "local",
            "token": "secret-token",
            "nested": {
                "steam_id": "76561190000000000",
                "notes": "keep me"
            },
            "reports": [
                {"ip": "127.0.0.1", "summary": "keep summary"},
                {"chat_account_id": "abc123"}
            ]
        });

        let redacted = redact_json_value(&value, &normalized_redact_fields(&[]));

        assert_eq!(redacted["token"], json!("[REDACTED]"));
        assert_eq!(redacted["nested"]["steam_id"], json!("[REDACTED]"));
        assert_eq!(redacted["nested"]["notes"], json!("keep me"));
        assert_eq!(redacted["reports"][0]["ip"], json!("[REDACTED]"));
        assert_eq!(redacted["reports"][0]["summary"], json!("keep summary"));
        assert_eq!(
            redacted["reports"][1]["chat_account_id"],
            json!("[REDACTED]")
        );
    }

    #[test]
    fn banlist_roundtrip() {
        let root = temp_root("banlist_roundtrip");
        let path = root.join("banlist.json");

        assert_eq!(
            load_banlist_entries(&path).expect("missing banlist"),
            Vec::<Value>::new()
        );
        let saved = add_banlist_entry(
            &path,
            "hash-local-1",
            "private_test",
            "voice_abuse",
            "2026-05-25T00:00:00Z",
        )
        .expect("add ban");
        let loaded = load_banlist_entries(&path).expect("load banlist");

        assert_eq!(loaded, saved);
        assert_eq!(loaded[0]["platform_user_id"], json!("hash-local-1"));
        assert_eq!(loaded[0]["scope"], json!("private_test"));
    }

    #[test]
    fn steam_registry_phase1_returns_empty_listing() {
        assert_eq!(steam_registry_list_servers(), Vec::<Value>::new());
    }

    #[test]
    fn secret_scan_reports_obvious_token_assignments() {
        let root = temp_root("secret_scan_reports");
        let file = root.join("config.env");
        fs::write(
            &file,
            concat!("EXTERNAL_CHAT_BOT_TOKEN", "=", "secret-value\n"),
        )
        .expect("write fixture");

        let findings = scan_for_secrets(&root).expect("scan succeeds");

        assert_eq!(findings.len(), 1);
        assert_eq!(findings[0].path, PathBuf::from("config.env"));
        assert_eq!(findings[0].line, 1);
    }

    #[test]
    fn secret_scan_skips_generated_private_and_install_paths() {
        let root = temp_root("secret_scan_skips");
        let venv_dir = root.join(".venv");
        let private_dir = root.join("references/private");
        let install_dir = root.join("Tools/install");
        let saved_dir = root.join("Saved");
        let content_dir = root.join("Content/Maps");
        fs::create_dir_all(&venv_dir).expect("venv dir");
        fs::create_dir_all(&private_dir).expect("private dir");
        fs::create_dir_all(&install_dir).expect("install dir");
        fs::create_dir_all(&saved_dir).expect("saved dir");
        fs::create_dir_all(&content_dir).expect("content dir");
        fs::write(
            venv_dir.join("config.env"),
            concat!("EXTERNAL_CHAT_BOT_TOKEN", "=", "secret-value\n"),
        )
        .expect("write venv fixture");
        fs::write(
            private_dir.join("config.env"),
            concat!("EXTERNAL_CHAT_BOT_TOKEN", "=", "secret-value\n"),
        )
        .expect("write private fixture");
        fs::write(
            install_dir.join("config.env"),
            concat!("EXTERNAL_CHAT_BOT_TOKEN", "=", "secret-value\n"),
        )
        .expect("write install fixture");
        fs::write(
            saved_dir.join("config.env"),
            concat!("EXTERNAL_CHAT_BOT_TOKEN", "=", "secret-value\n"),
        )
        .expect("write saved fixture");
        fs::write(
            content_dir.join("L_Test.umap"),
            concat!("EXTERNAL_CHAT_BOT_TOKEN", "=", "secret-value\n"),
        )
        .expect("write binary fixture");

        let findings = scan_for_secrets(&root).expect("scan succeeds");

        assert_eq!(findings, Vec::<SecretFinding>::new());
    }

    #[test]
    fn cycle_record_findings_rejects_retired_tools_and_empty_skips() {
        let retired_path = ["Tools/", "quality_gate.py"].concat();
        let text = format!(
            r#"# Cycle 65: Evidence gate fixture

## Header

- Cycle: 65
- Date: 2026-05-26
- Phase: QA
- Owner: Codex
- Goal: Exercise evidence checks
- Gate: Latest cycle rejects stale commands

## Verification

| Check | Result | Notes |
| --- | --- | --- |
| `python3 {retired_path}` | not run |  |

## Evidence

-

## Decisions

| Label | Decision | Reason | Next Action |
| --- | --- | --- | --- |
| keep |  |  |  |

## Next Cycle

- Goal: Continue
- First action:
"#
        );

        let findings = cycle_record_findings(&text, &[retired_path.as_str()]);

        assert!(
            findings
                .iter()
                .any(|finding| finding.contains("missing current quality gate command"))
        );
        assert!(
            findings
                .iter()
                .any(|finding| finding.contains(&format!("retired tool reference: {retired_path}")))
        );
        assert!(
            findings
                .iter()
                .any(|finding| finding.contains("unexplained skipped verification"))
        );
        assert!(
            findings
                .iter()
                .any(|finding| finding.contains("empty cycle field: - First action:"))
        );
    }

    #[test]
    fn cycle_record_findings_accepts_current_rust_evidence() {
        let text = r#"# Cycle 65: Evidence gate fixture

## Header

- Cycle: 65
- Date: 2026-05-26
- Phase: QA
- Owner: Codex
- Goal: Exercise evidence checks
- Gate: Latest cycle records current Rust checks

## Verification

| Check | Result | Notes |
| --- | --- | --- |
| `cargo run -p frostwake-tools -- quality-gate` | pass | Latest cycle record uses the Rust gate |
| Unreal build | not run | Documentation and Rust tooling only |

## Evidence

- Latest cycle points to current commands and explains skipped checks.

## Decisions

| Label | Decision | Reason | Next Action |
| --- | --- | --- | --- |
| keep | Latest cycle evidence guard | Prevent stale commands in active evidence | Keep in quality gate |

## Next Cycle

- Goal: Continue evidence review
- First action: Re-run the quality gate
"#;

        let retired_path = ["Tools/", "quality_gate.py"].concat();
        assert_eq!(
            cycle_record_findings(text, &[retired_path.as_str()]),
            Vec::<String>::new()
        );
    }

    #[test]
    fn log_summary_matches_human_playtest_fixture() {
        let summary = summarize_events(Path::new("tests/fixtures/p1_024_human_events.jsonl"))
            .expect("summary");

        assert_eq!(summary["run_id"], json!("P1-024-run-01"));
        // p1_024 is an immutable pre-rename recording — build_id stays "AbyssLock-..." (do not rename).
        assert_eq!(
            summary["build_id"],
            json!("AbyssLock-Win64-Development-local")
        );
        assert_eq!(summary["map_id"], json!("/Game/Maps/L_IcebreakerWhitebox"));
        assert_eq!(summary["profile"], json!("human-p1-024"));
        assert_eq!(summary["players_connected"], json!(6));
        assert_eq!(summary["players_disconnected"], json!(1));
        assert_eq!(summary["match_started"], json!(1));
        assert_eq!(summary["match_ended"], json!(1));
        assert_eq!(summary["match_timer_started"], json!(0));
        assert_eq!(summary["match_timer_expired"], json!(0));
        assert_eq!(summary["ship_task_repairs"], json!(1));
        assert_eq!(summary["ship_task_sabotages"], json!(1));
        assert_eq!(summary["door_toggles"], json!(1));
        assert_eq!(summary["players_downed"], json!(1));
        assert_eq!(summary["players_rescued"], json!(1));
        assert_eq!(summary["life_action_interactions"], json!(0));
        assert_eq!(summary["invalid_lines"], json!(0));
        assert_eq!(
            summary["last_role_assignment"],
            json!({"players": 6, "saboteurs": 1})
        );
        assert_eq!(
            summary["last_match_result"],
            json!({"winner": "crew", "reason": "route_complete"})
        );
        assert_eq!(summary["duration_seconds"], json!(182.0));
        assert_eq!(summary["match_duration_seconds"], json!(160.0));
    }

    #[test]
    fn log_summary_counts_invalid_jsonl_without_dropping_valid_events() {
        let root = temp_root("log_summary_invalid");
        let events = root.join("events.jsonl");
        fs::write(
            &events,
            [
                json!({"event": "match_started", "elapsed_seconds": 1.0}).to_string(),
                "{not json".to_string(),
                json!({"event": "match_ended", "elapsed_seconds": 3.5, "payload": {"winner": "crew"}})
                    .to_string(),
            ]
            .join("\n")
                + "\n",
        )
        .expect("write events");

        let summary = summarize_events(&events).expect("summary");

        assert_eq!(summary["invalid_lines"], json!(1));
        assert_eq!(summary["match_started"], json!(1));
        assert_eq!(summary["match_ended"], json!(1));
        assert_eq!(summary["match_duration_seconds"], json!(2.5));
    }

    #[test]
    fn log_summary_preserves_fatal_ship_state_context() {
        let root = temp_root("log_summary_fatal_ship_state");
        let events = root.join("events.jsonl");
        fs::write(
            &events,
            [
                json!({"event": "match_started", "elapsed_seconds": 1.0}).to_string(),
                json!({
                    "event": "match_ended",
                    "elapsed_seconds": 9.0,
                    "payload": {
                        "winner": "saboteur",
                        "reason": "fatal_ship_state",
                        "criticalSystems": ["power"],
                        "systemConditions": {
                            "hull": 0.650,
                            "power": 0.0
                        }
                    }
                })
                .to_string(),
            ]
            .join("\n")
                + "\n",
        )
        .expect("write events");

        let summary = summarize_events(&events).expect("summary");
        let result = summary["last_match_result"]
            .as_object()
            .expect("match result");

        assert_eq!(result["criticalSystems"], json!(["power"]));
        assert_eq!(result["systemConditions"]["power"], json!(0.0));
        assert!(
            concise_playtest_summary(&summary, result, "").contains("critical_systems=[\"power\"]")
        );
    }

    #[test]
    fn playtest_report_payload_matches_backend_contract() {
        let summary = summarize_events(Path::new("tests/fixtures/p1_024_human_events.jsonl"))
            .expect("summary");
        let payload =
            build_playtest_report_payload(&summary, "anonymized keep/cut/change recorded")
                .expect("payload");

        assert_eq!(payload["runId"], json!("P1-024-run-01"));
        // p1_024 is an immutable pre-rename recording — buildId stays "AbyssLock-..." (do not rename).
        assert_eq!(
            payload["buildId"],
            json!("AbyssLock-Win64-Development-local")
        );
        assert_eq!(payload["mapId"], json!("/Game/Maps/L_IcebreakerWhitebox"));
        assert_eq!(payload["playerCount"], json!(6));
        assert_eq!(payload["winner"], json!("crew"));
        assert!(
            payload["summary"]
                .as_str()
                .expect("summary")
                .contains("repairs=1")
        );
        assert!(
            payload["summary"]
                .as_str()
                .expect("summary")
                .contains("sabotages=1")
        );
    }

    #[test]
    fn playtest_report_load_summary_accepts_raw_jsonl_events() {
        let summary =
            load_summary(Path::new("tests/fixtures/p1_024_human_events.jsonl")).expect("summary");

        assert_eq!(summary["run_id"], json!("P1-024-run-01"));
        assert_eq!(summary["players_connected"], json!(6));
    }

    #[test]
    fn playtest_report_rejects_risky_summary_text() {
        let mut summary = summarize_events(Path::new("tests/fixtures/p1_024_human_events.jsonl"))
            .expect("summary");
        summary.insert("profile".to_string(), json!("host-127.0.0.1"));

        let error =
            build_playtest_report_payload(&summary, "").expect_err("risky profile should fail");

        assert!(error.contains("IPv4 address"));
    }

    #[test]
    fn playtest_report_post_json_http_sends_payload() {
        let listener = TcpListener::bind("127.0.0.1:0").expect("bind listener");
        let port = listener.local_addr().expect("local addr").port();
        let handle = thread::spawn(move || {
            let (mut stream, _) = listener.accept().expect("accept request");
            let mut request = [0_u8; 4096];
            let bytes = stream.read(&mut request).expect("read request");
            let request = String::from_utf8_lossy(&request[..bytes]);
            assert!(request.starts_with("POST /v1/playtest-reports HTTP/1.1"));
            assert!(request.contains("\"runId\":\"P1-024-run-01\""));
            stream
                .write_all(
                    b"HTTP/1.1 202 Accepted\r\nContent-Type: application/json\r\nContent-Length: 33\r\nConnection: close\r\n\r\n{\"accepted\":true,\"id\":\"ptr_test\"}",
                )
                .expect("write response");
        });

        let response = post_json_http(
            &format!("http://127.0.0.1:{port}/v1/playtest-reports"),
            &json!({"runId": "P1-024-run-01"}),
            Duration::from_secs(5),
        )
        .expect("post succeeds");
        handle.join().expect("server thread");

        assert_eq!(response, json!({"accepted": true, "id": "ptr_test"}));
    }

    #[test]
    fn playtest_summary_markdown_contains_expected_fixture_fields() {
        let summary = summarize_events(Path::new("tests/fixtures/p1_024_human_events.jsonl"))
            .expect("summary");
        let markdown = render_playtest_summary_markdown(PlaytestSummaryRenderOptions {
            summary: &summary,
            summary_path: Path::new("Saved/Playtests/P1-024/run-01/summary.json"),
            title: "P1-024 Fixture Summary",
            min_players: 6,
            target_players: Some(6),
            raw_evidence_label: None,
            snapshot: Some("test-snapshot"),
            author: Some("Test"),
            date: Some("2026-05-25"),
            dry_run_example: false,
        });

        assert!(markdown.contains("# P1-024 Fixture Summary"));
        assert!(markdown.contains("RunId: P1-024-run-01"));
        assert!(markdown.contains("Actual players: 6"));
        assert!(markdown.contains("| `players_connected` | 6 |"));
        assert!(markdown.contains("| `ship_task_repairs` | 1 |"));
        assert!(markdown.contains("## Comprehension And Accessibility"));
        assert!(markdown.contains("| Objective comprehension |"));
        assert!(markdown.contains("## P1-024 Decision"));
    }

    #[test]
    fn playtest_summary_rejects_raw_event_json_input() {
        let root = temp_root("playtest_summary_raw");
        let raw = root.join("raw.json");
        fs::write(
            &raw,
            json!({"event": "match_started", "payload": {}}).to_string(),
        )
        .expect("write raw");

        let error = load_summary_json(&raw).expect_err("raw event should fail");

        assert!(error.contains("raw event record"));
    }

    #[test]
    fn playtest_preflight_accepts_resolved_human_summary() {
        let mut summary = summarize_events(Path::new("tests/fixtures/p1_024_human_events.jsonl"))
            .expect("summary");
        summary.insert(
            "source".to_string(),
            json!("Saved/Playtests/P1-024/run-01/events.jsonl"),
        );
        let markdown = resolved_playtest_markdown(&summary);
        let root = temp_root("playtest_preflight_resolved");
        let docs_dir = repo_root().join("docs/playtests");
        fs::create_dir_all(&docs_dir).expect("docs dir");
        let markdown_path = docs_dir.join(format!(
            "p1-024-rust-test-{}-summary.md",
            SystemTime::now()
                .duration_since(UNIX_EPOCH)
                .expect("clock")
                .as_nanos()
        ));
        fs::write(&markdown_path, markdown).expect("write markdown");
        let summary_path = root.join("Saved/Playtests/P1-024/run-01/summary.json");
        fs::create_dir_all(summary_path.parent().expect("parent")).expect("summary parent");
        fs::write(
            &summary_path,
            serde_json::to_string(&summary).expect("summary serializes"),
        )
        .expect("write summary");
        let mut results =
            check_playtest_summary(&summary, &summary_path, PlaytestPreflightMode::Human, 6);
        results.extend(check_playtest_markdown(
            Some(&markdown_path),
            PlaytestPreflightMode::Human,
            &summary,
            6,
        ));
        fs::remove_file(&markdown_path).ok();

        assert!(
            results
                .iter()
                .all(|result| result.status != CheckStatus::Fail)
        );
        assert!(
            results
                .iter()
                .any(|result| result.name == "decision_resolved"
                    && result.status == CheckStatus::Pass)
        );
        assert!(results.iter().any(|result| {
            result.name == "no_unresolved_placeholders" && result.status == CheckStatus::Pass
        }));
        assert!(results.iter().any(|result| {
            result.name == "human_playability_signals_resolved"
                && result.status == CheckStatus::Pass
        }));
        assert!(results.iter().any(|result| {
            result.name == "gp09_comprehension_accessibility_signals_resolved"
                && result.status == CheckStatus::Pass
        }));
    }

    #[test]
    fn playtest_preflight_rejects_generated_placeholder_summary() {
        let mut summary = summarize_events(Path::new("tests/fixtures/p1_024_human_events.jsonl"))
            .expect("summary");
        summary.insert(
            "source".to_string(),
            json!("Saved/Playtests/P1-024/run-01/events.jsonl"),
        );
        let markdown = render_playtest_summary_markdown(PlaytestSummaryRenderOptions {
            summary: &summary,
            summary_path: Path::new("Saved/Playtests/P1-024/run-01/summary.json"),
            title: "P1-024 Generated Summary",
            min_players: 6,
            target_players: Some(6),
            raw_evidence_label: None,
            snapshot: None,
            author: None,
            date: Some("2026-05-25"),
            dry_run_example: false,
        });
        let docs_dir = repo_root().join("docs/playtests");
        fs::create_dir_all(&docs_dir).expect("docs dir");
        let markdown_path = docs_dir.join(format!(
            "p1-024-rust-generated-{}-summary.md",
            SystemTime::now()
                .duration_since(UNIX_EPOCH)
                .expect("clock")
                .as_nanos()
        ));
        fs::write(&markdown_path, markdown).expect("write markdown");
        let results = check_playtest_markdown(
            Some(&markdown_path),
            PlaytestPreflightMode::Human,
            &summary,
            6,
        );
        fs::remove_file(&markdown_path).ok();

        assert!(results.iter().any(|result| {
            result.name == "no_unresolved_placeholders" && result.status == CheckStatus::Fail
        }));
    }

    #[test]
    fn playtest_preflight_rejects_blank_human_signal_rows() {
        let mut summary = summarize_events(Path::new("tests/fixtures/p1_024_human_events.jsonl"))
            .expect("summary");
        summary.insert(
            "source".to_string(),
            json!("Saved/Playtests/P1-024/run-01/events.jsonl"),
        );
        let markdown = resolved_playtest_markdown(&summary).replace(
            "| How many wanted another round? | Most fixture players wanted one more round. |",
            "| How many wanted another round? |  |",
        );
        let docs_dir = repo_root().join("docs/playtests");
        fs::create_dir_all(&docs_dir).expect("docs dir");
        let markdown_path = docs_dir.join(format!(
            "p1-024-rust-blank-signal-{}-summary.md",
            SystemTime::now()
                .duration_since(UNIX_EPOCH)
                .expect("clock")
                .as_nanos()
        ));
        fs::write(&markdown_path, markdown).expect("write markdown");
        let results = check_playtest_markdown(
            Some(&markdown_path),
            PlaytestPreflightMode::Human,
            &summary,
            6,
        );
        fs::remove_file(&markdown_path).ok();

        assert!(results.iter().any(|result| {
            result.name == "human_playability_signals_resolved"
                && result.status == CheckStatus::Fail
        }));
    }

    #[test]
    fn playtest_preflight_rejects_blank_gp09_signal_rows() {
        let mut summary = summarize_events(Path::new("tests/fixtures/p1_024_human_events.jsonl"))
            .expect("summary");
        summary.insert(
            "source".to_string(),
            json!("Saved/Playtests/P1-024/run-01/events.jsonl"),
        );
        let markdown = resolved_playtest_markdown(&summary).replace(
            "| Objective comprehension | Players could state the crew and agent goals after the round. | Keep the short brief. |",
            "| Objective comprehension |  | Keep the short brief. |",
        );
        let docs_dir = repo_root().join("docs/playtests");
        fs::create_dir_all(&docs_dir).expect("docs dir");
        let markdown_path = docs_dir.join(format!(
            "p1-024-rust-blank-gp09-{}-summary.md",
            SystemTime::now()
                .duration_since(UNIX_EPOCH)
                .expect("clock")
                .as_nanos()
        ));
        fs::write(&markdown_path, markdown).expect("write markdown");
        let results = check_playtest_markdown(
            Some(&markdown_path),
            PlaytestPreflightMode::Human,
            &summary,
            6,
        );
        fs::remove_file(&markdown_path).ok();

        assert!(results.iter().any(|result| {
            result.name == "gp09_comprehension_accessibility_signals_resolved"
                && result.status == CheckStatus::Fail
        }));
    }

    #[test]
    fn playtest_run_scaffold_generates_windows_first_scripts() {
        let scaffold = fixture_scaffold();
        let preflight = scaffold_file(&scaffold, "preflight.ps1");

        assert!(preflight.contains("target\\playtest-tools"));
        assert!(preflight.contains("--platform Win64"));
        assert!(preflight.contains("--include-server"));
        assert!(preflight.contains("--profile ready8"));
        assert!(
            preflight.contains("Invoke-Cargo run -p frostwake-tools -- quality-gate --require-ue")
        );
    }

    #[test]
    fn playtest_run_scaffold_host_script_includes_telemetry_identity() {
        let scaffold = fixture_scaffold();
        let host = scaffold_file(&scaffold, "host.ps1");

        assert!(host.contains("-FrostwakeRunId=P1-024-run-01"));
        assert!(host.contains("-FrostwakeBuildId=Frostwake-Win64-Development-local"));
        assert!(host.contains("-FrostwakeMapId=/Game/Maps/L_IcebreakerWhitebox"));
        assert!(host.contains("-FrostwakeProfile=human-p1-024"));
        assert!(host.contains(
            "-FrostwakeEventLog=$RepoRoot\\Saved\\Playtests\\P1-024\\run-01\\events.jsonl"
        ));
        assert!(host.contains("-FrostwakeLobbyMinPlayers=8"));
    }

    #[test]
    fn playtest_run_scaffold_after_test_uses_rust_tools() {
        let scaffold = fixture_scaffold();
        let after_test = scaffold_file(&scaffold, "after-test.ps1");

        assert!(after_test.contains("target\\playtest-tools"));
        assert!(after_test.contains("Invoke-Cargo run -p frostwake-tools -- log-summary"));
        assert!(after_test.contains("Invoke-Cargo run -p frostwake-tools -- playtest-summary"));
        assert!(after_test.contains("Invoke-Cargo run -p frostwake-tools -- playtest-preflight"));
        assert!(
            after_test.contains("Invoke-Cargo run -p frostwake-tools -- playtest-report-upload")
        );
        assert!(after_test.contains("docs\\playtests\\p1-024-run-01-summary.md"));
    }

    #[test]
    fn playtest_run_scaffold_command_card_documents_windows_manual_equivalent() {
        let scaffold = fixture_scaffold();
        let commands = scaffold_file(&scaffold, "commands.md");

        assert!(commands.contains("Host Preflight"));
        assert!(commands.contains("--platform Win64"));
        assert!(commands.contains(".\\Saved\\Playtests\\P1-024\\run-01\\host.ps1"));
        assert!(commands.contains("-FrostwakeRunId=P1-024-run-01"));
        assert!(commands.contains("backend report payload dry-run"));
    }

    #[test]
    fn playtest_run_scaffold_host_notes_capture_gp09_signals() {
        let scaffold = fixture_scaffold();
        let notes = scaffold_file(&scaffold, "host-notes.md");

        assert!(notes.contains("## Comprehension And Accessibility"));
        assert!(notes.contains("| Objective comprehension |"));
        assert!(notes.contains("| Accessibility blocker |"));
        assert!(notes.contains("| Text or term issue |"));
    }

    fn temp_root(label: &str) -> PathBuf {
        let unique = SystemTime::now()
            .duration_since(UNIX_EPOCH)
            .expect("system clock")
            .as_nanos();
        let path = std::env::temp_dir().join(format!("frostwake_tools_{label}_{unique}"));
        fs::create_dir_all(&path).expect("temp root");
        path
    }

    fn valid_asset_ledger_row(asset_id: &str) -> BTreeMap<String, String> {
        let values = [
            ("asset_id", asset_id),
            ("asset_name", "Local Test Asset"),
            ("asset_type", "model"),
            ("created_at", "2026-05-25"),
            ("creator", "Test"),
            ("tool_or_model", "Internal"),
            ("tool_version", "1"),
            ("prompt_summary", "none"),
            ("reference_used", "no"),
            ("reference_source", "none"),
            ("output_path", "pending"),
            ("modification_history", "none"),
            ("license", "pending"),
            ("fab_listing_url", ""),
            ("publisher_or_seller", "pending"),
            ("license_snapshot_date", "pending"),
            ("package_version", "pending"),
            ("permitted_engine_scope", "pending"),
            ("proof_of_purchase_path", "pending"),
            ("reviewer_approval", "pending"),
            ("commercial_use_allowed", "unknown"),
            ("credit_required", "unknown"),
            ("redistribution_allowed", "unknown"),
            ("training_data_restriction", "none"),
            ("rights_risk", "low"),
            ("adoption_state", "candidate"),
            ("final_reviewer", "pending"),
            ("notes", "none"),
        ];
        values
            .into_iter()
            .map(|(key, value)| (key.to_string(), value.to_string()))
            .collect()
    }

    fn write_asset_ledger_fixture(path: &Path, rows: &[BTreeMap<String, String>]) {
        let mut text = REQUIRED_ASSET_LEDGER_COLUMNS.join(",");
        text.push('\n');
        for row in rows {
            let values: Vec<&str> = REQUIRED_ASSET_LEDGER_COLUMNS
                .iter()
                .map(|column| row.get(*column).map(String::as_str).unwrap_or(""))
                .collect();
            text.push_str(&values.join(","));
            text.push('\n');
        }
        fs::write(path, text).expect("write asset ledger fixture");
    }

    fn resolved_playtest_markdown(summary: &BTreeMap<String, Value>) -> String {
        format!(
            r#"# P1-024 Resolved Fixture Summary

## Header

```text
RunId: {run_id}
Date: 2026-05-25
Local snapshot or commit: test-snapshot
Build: {build_id}
Map: {map_id}
Profile: {profile}
Host type: listen-server
Target players: 6
Actual players: {players_connected}
Voice method: external voice call
Recording consent: yes
Raw evidence path: Saved/Playtests/P1-024/run-01
Summary author: Test
```

## Telemetry Summary

Fixture telemetry reviewed.

## Technical Result

All objective gates passed in fixture data.

## Validity Gate

Validity result:

```text
pass
```

## Observed Flow

Players connected, roles assigned, repairs and sabotage occurred, and the match ended.

## Player Feedback

Aggregate only. Do not name players.

| Question | Summary |
| --- | --- |
| What did players think their goal was? | Crew keep route progress moving; agents prevent success. |
| When did suspicion first appear? | After the first sabotage and repair disagreement. |
| What created trust? | Calling out repairs and moving together. |
| What created distrust? | Isolation plus conflicting explanations. |
| What was confusing? | Fixture notes say the next useful repair target needed clearer wording. |
| What was tense, funny, or memorable? | Players argued over whether the sabotage was accidental. |
| How many wanted another round? | Most fixture players wanted one more round. |
| What should be kept? | Repair and sabotage pressure. |
| What should be cut? | No fixture cuts. |
| What should change before the next test? | Improve UI labels. |

## Comprehension And Accessibility

| Signal | Evidence | Follow-up |
| --- | --- | --- |
| Objective comprehension | Players could state the crew and agent goals after the round. | Keep the short brief. |
| Next-step clarity | Players found at least one useful repair or sabotage action. | Add clearer next-target wording later. |
| Failure-state clarity | Players could connect sabotage pressure to ship failure risk. | Keep tracking end-state explanations. |
| UI or control confusion | Repair target labels caused the main fixture confusion. | Improve labels before the next test. |
| Accessibility blocker | none observed in fixture evidence | Recheck readability and controls with humans. |
| Text or term issue | Use crew, agent, route progress, repair, and sabotage consistently. | Keep glossary terms. |

## Keep / Cut / Change

Keep: repair and sabotage pressure.

Cut: no fixture cuts.

Change: improve UI labels.

## Mechanics Decision Table

| Area | Decision |
| --- | --- |
| Repairs | keep |
| Sabotage | keep |

## Top Blockers

| Rank | Blocker | Severity | Evidence | Proposed fix | Owner | Retest gate |
| ---: | --- | --- | --- | --- | --- | --- |
| 1 | None from fixture | low | fixture | continue | Codex | rerun |

## P1-024 Decision

Decision:

```text
P1-024 result: pass
Reason: fixture satisfies objective gates
Next backlog item: P1-025
```

## Data Handling Check

- [x] Raw logs are not committed.
- [x] Raw recordings are not committed.
- [x] Tester names are removed.
- [x] Voice transcripts are not included.
- [x] IP addresses, SteamIDs, and local usernames are removed.
- [x] Any moderation-sensitive details are summarized without identifying people.
- [x] Summary is stored in `docs/playtests/` only after anonymization.
"#,
            run_id = summary_string(summary, "run_id", ""),
            build_id = summary_string(summary, "build_id", ""),
            map_id = summary_string(summary, "map_id", ""),
            profile = summary_string(summary, "profile", ""),
            players_connected = summary_string(summary, "players_connected", ""),
        )
    }

    fn fixture_scaffold() -> PlaytestRunScaffold {
        build_playtest_run_scaffold(PlaytestRunScaffoldOptions {
            run_number: 1,
            target_players: 8,
            port: 7777,
            res_x: 1280,
            res_y: 720,
            build_id: "Frostwake-Win64-Development-local",
            map_id: "/Game/Maps/L_IcebreakerWhitebox",
            profile: "human-p1-024",
            ue_root: r"C:\Program Files\Epic Games\UE_5.7",
            ue_editor: "/Users/Shared/Epic Games/UE_5.7/Engine/Binaries/Mac/UnrealEditor",
        })
        .expect("scaffold")
    }

    fn scaffold_file<'a>(scaffold: &'a PlaytestRunScaffold, name: &str) -> &'a str {
        scaffold
            .files
            .iter()
            .find(|file| file.path.file_name().and_then(|value| value.to_str()) == Some(name))
            .expect("file exists")
            .content
            .as_str()
    }

    #[test]
    fn backlog_issue_export_slug_and_parse_match_contract() {
        let root = temp_root("backlog_issue_export");
        let backlog = root.join("phase2-backlog.md");
        fs::write(
            &backlog,
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
            .join("\n"),
        )
        .expect("write backlog");

        let rows =
            parse_backlog_to_issue_rows(&backlog, 2, "P2-", "Phase 2", "platform").expect("rows");

        assert_eq!(
            slug_label("  Phase 2: Steam Lobby  "),
            "phase-2-steam-lobby"
        );
        assert_eq!(rows.len(), 1);
        assert_eq!(rows[0].title(), "P2-001: Enable Steam dev config safely");
        assert_eq!(rows[0].section, "Steam Lobby");
        assert_eq!(rows[0].labels(), "phase-2,steam-lobby,platform");
        assert!(rows[0].body().contains("phase2-backlog.md"));
    }

    #[test]
    fn github_issue_sync_builds_shell_safe_commands() {
        let spec = IssueSpec {
            title: "P1-999: spaced title".to_string(),
            body: "Body text".to_string(),
            labels: vec!["phase-1".to_string(), "prototype".to_string()],
            milestone: "Phase 1".to_string(),
        };

        let command = build_gh_issue_create_command(&spec);

        assert_eq!(
            &command[..6],
            [
                "gh",
                "issue",
                "create",
                "--title",
                "P1-999: spaced title",
                "--body"
            ]
        );
        assert!(command.contains(&"--label".to_string()));
        assert!(command.contains(&"phase-1".to_string()));
        assert_eq!(
            shell_command(&[
                "gh".to_string(),
                "issue".to_string(),
                "create".to_string(),
                "--title".to_string(),
                "P1-999: spaced title".to_string()
            ]),
            "gh issue create --title 'P1-999: spaced title'"
        );
    }

    #[test]
    fn whitebox_automation_uses_unreal_cpp_commandlet() {
        let command =
            unreal_commandlet_command("CreateIcebreakerWhitebox", Path::new("C:/UE_5.7"), "Win64");
        let joined = command.join(" ").to_ascii_lowercase();

        assert!(command.contains(&"-run=CreateIcebreakerWhitebox".to_string()));
        assert!(!joined.contains("python"));
        assert!(!joined.contains(".py"));
    }

    #[test]
    fn visual_poc_automation_uses_unreal_cpp_commandlets() {
        let create =
            unreal_commandlet_command("CreateFrostwakeVisualPoc", Path::new("C:/UE_5.7"), "Win64");
        let validate = unreal_commandlet_command(
            "ValidateFrostwakeVisualPoc",
            Path::new("C:/UE_5.7"),
            "Win64",
        );

        assert!(create.contains(&"-run=CreateFrostwakeVisualPoc".to_string()));
        assert!(validate.contains(&"-run=ValidateFrostwakeVisualPoc".to_string()));
        assert!(
            unreal_commandlet_command(
                "ImportAmbientCgVisualPocAssets",
                Path::new("C:/UE_5.7"),
                "Win64"
            )
            .contains(&"-run=ImportAmbientCgVisualPocAssets".to_string())
        );
        assert!(
            unreal_commandlet_command(
                "ValidateAmbientCgVisualPocAssets",
                Path::new("C:/UE_5.7"),
                "Win64"
            )
            .contains(&"-run=ValidateAmbientCgVisualPocAssets".to_string())
        );
    }

    #[test]
    fn smoke_profile_descriptions_match_expected_launch_shapes() {
        let mut life_action = LocalSmokeOptions {
            profile: Some(SmokeProfile::LifeAction),
            clients: 0,
            duration: 45,
            startup_timeout: 45,
            match_timeout: 45,
            client_launch_spacing: 3.0,
            auto_start_min_players: None,
            expected_players: None,
            expected_saboteurs: None,
            map: "/Game/Maps/L_IcebreakerWhitebox".to_string(),
            port: 7777,
            log_dir: None,
            run_id: None,
            build_id: "local-dev".to_string(),
            skip_build: false,
            ue_root: None,
            platform: "Win64".to_string(),
            null_rhi: false,
            windowed: false,
            host_only: false,
            smoke_interact: false,
            smoke_down_rescue: false,
            smoke_containment: false,
            smoke_route_complete: false,
            smoke_fatal_sabotage: false,
            smoke_bulkhead_lock: false,
            smoke_pump_flooding: false,
            smoke_pve_enemy: false,
            smoke_pve_damage: false,
            smoke_item_drop: false,
            smoke_combined_systems: false,
            smoke_qa_bot: false,
            smoke_qa_player_bot: false,
            smoke_qa_task_bot: false,
            smoke_match_timer: false,
            smoke_life_action: false,
            smoke_eat: false,
            smoke_damage_type: false,
            smoke_survival: false,
            smoke_effect: false,
            no_auto_start: false,
            auto_ready: false,
            lobby_min_players: None,
            lobby_dev_saboteurs: None,
            practice_mode: false,
            single_player_mode: false,
        };
        apply_smoke_profile(&mut life_action);
        let profile = describe_smoke_settings(&life_action);
        assert_eq!(profile["clients"], json!(4));
        assert_eq!(profile["expected_players"], json!(5));
        assert_eq!(profile["expected_saboteurs"], json!(1));
        assert_eq!(profile["auto_ready"], json!(true));
        assert_eq!(profile["smoke_life_action"], json!(true));

        life_action.profile = Some(SmokeProfile::Practice);
        apply_smoke_profile(&mut life_action);
        let practice = describe_smoke_settings(&life_action);
        assert_eq!(practice["clients"], json!(0));
        assert_eq!(practice["expected_players"], json!(1));
        assert_eq!(practice["expected_saboteurs"], json!(0));
        assert_eq!(practice["practice_mode"], json!(true));

        life_action.profile = Some(SmokeProfile::SinglePlayer);
        apply_smoke_profile(&mut life_action);
        let single_player = describe_smoke_settings(&life_action);
        assert_eq!(single_player["clients"], json!(0));
        assert_eq!(single_player["expected_players"], json!(1));
        assert_eq!(single_player["expected_saboteurs"], json!(0));
        assert_eq!(single_player["single_player_mode"], json!(true));

        let mut fatal_sabotage = default_local_smoke_options();
        fatal_sabotage.profile = Some(SmokeProfile::FatalSabotage);
        apply_smoke_profile(&mut fatal_sabotage);
        let fatal = describe_smoke_settings(&fatal_sabotage);
        assert_eq!(fatal["clients"], json!(0));
        assert_eq!(fatal["host_only"], json!(true));
        assert_eq!(fatal["auto_start_min_players"], json!(1));
        assert_eq!(fatal["expected_players"], json!(1));
        assert_eq!(fatal["expected_saboteurs"], json!(1));
        assert_eq!(fatal["smoke_fatal_sabotage"], json!(true));
    }

    #[test]
    fn visual_poc_manifest_blocks_whitebox_overwrite() {
        let error = build_visual_poc_manifest(
            "/Game/Maps/L_IcebreakerWhitebox",
            "/Game/Maps/L_IcebreakerWhitebox_Copy",
            true,
        )
        .expect_err("whitebox-like target rejected");

        assert!(error.contains("automation whitebox"));
    }
}
