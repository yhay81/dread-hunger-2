use std::collections::HashSet;
use std::sync::{Arc, LazyLock, Mutex};

use axum::body::Bytes;
use axum::extract::{Path, Query, Request, State};
use axum::http::{HeaderValue, StatusCode};
use axum::middleware::{self, Next};
use axum::response::{IntoResponse, Response};
use axum::routing::{get, post};
use axum::{Json, Router};
use chrono::{DateTime, Duration, SecondsFormat, Utc};
use regex::Regex;
use serde::{Deserialize, Serialize};
use serde_json::{Map, Value, json};
use uuid::Uuid;

const MAX_BODY_BYTES: usize = 32 * 1024;
const FIXED_MATCH_PLAYERS: u8 = 8;
const STANDARD_MINIMUM_COMPLETED_MATCHES: u32 = 50;
const SERVICE_NAME: &str = "frostwake-backend";
const MOCK_STEAM_TICKET_SUCCESS: &str = "feedfacecafebeef";
const MOCK_STEAM_TICKET_INVALID: &str = "deadbeef";
const MOCK_STEAM_TICKET_NO_OWNERSHIP: &str = "0badc0de";
const STEAM_IDENTITY_PROOF_TTL_MINUTES: i64 = 15;

static PLAYTEST_WINNERS: LazyLock<HashSet<&'static str>> =
    LazyLock::new(|| HashSet::from(["crew", "agents", "none", "unknown"]));
static MODERATION_CATEGORIES: LazyLock<HashSet<&'static str>> =
    LazyLock::new(|| HashSet::from(["voice_abuse", "griefing", "cheating", "harassment", "other"]));
static MODERATION_VOICE_STATES: LazyLock<HashSet<&'static str>> = LazyLock::new(|| {
    HashSet::from([
        "not_applicable",
        "lobby",
        "proximity",
        "downed",
        "contained",
        "dead_spectator",
        "saboteur_private",
        "unknown",
    ])
});
static MODERATION_ACTIONS: LazyLock<HashSet<&'static str>> = LazyLock::new(|| {
    HashSet::from([
        "none",
        "muted",
        "blocked",
        "kick_requested",
        "kicked",
        "ban_requested",
    ])
});
static STEAM_IDENTITY_PURPOSES: LazyLock<HashSet<&'static str>> =
    LazyLock::new(|| HashSet::from(["moderation", "server_metadata", "support"]));
static RISKY_TEXT_PATTERNS: LazyLock<Vec<(&'static str, Regex)>> = LazyLock::new(|| {
    vec![
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
    ]
});

#[derive(Clone)]
pub struct AppState {
    inner: Arc<Mutex<BackendState>>,
    now: Arc<dyn Fn() -> DateTime<Utc> + Send + Sync>,
}

#[derive(Clone, Debug)]
pub struct MockSteamIdentityConfig {
    app_id: u32,
    subject_hash_salt: String,
}

impl MockSteamIdentityConfig {
    pub fn new(app_id: u32, subject_hash_salt: impl Into<String>) -> Self {
        Self {
            app_id,
            subject_hash_salt: subject_hash_salt.into(),
        }
    }
}

impl AppState {
    pub fn new(version: impl Into<String>) -> Self {
        Self::with_now(version, Utc::now)
    }

    pub fn with_now<F>(version: impl Into<String>, now: F) -> Self
    where
        F: Fn() -> DateTime<Utc> + Send + Sync + 'static,
    {
        let now = Arc::new(now);
        let started_at = iso_timestamp(now());
        Self {
            inner: Arc::new(Mutex::new(BackendState {
                started_at,
                version: version.into(),
                playtest_reports: Vec::new(),
                moderation_reports: Vec::new(),
                matchmaking_lobbies: Vec::new(),
                steam_identity: None,
            })),
            now,
        }
    }

    pub fn with_mock_steam_identity(self, config: MockSteamIdentityConfig) -> Self {
        self.inner
            .lock()
            .expect("backend state mutex poisoned")
            .steam_identity = Some(config);
        self
    }

    fn now_iso(&self) -> String {
        iso_timestamp((self.now)())
    }
}

#[derive(Debug)]
struct BackendState {
    started_at: String,
    version: String,
    playtest_reports: Vec<StoredReport>,
    moderation_reports: Vec<StoredReport>,
    matchmaking_lobbies: Vec<StoredMatchmakingLobby>,
    steam_identity: Option<MockSteamIdentityConfig>,
}

#[derive(Debug, Serialize)]
#[serde(rename_all = "camelCase")]
struct StoredReport {
    id: String,
    received_at: String,
    payload: Value,
}

#[derive(Clone, Copy, Debug, Deserialize, Eq, PartialEq, Serialize)]
#[serde(rename_all = "snake_case")]
enum LobbyType {
    Casual,
    Standard,
}

impl LobbyType {
    fn parse(value: &str) -> Option<Self> {
        match value {
            "casual" => Some(Self::Casual),
            "standard" => Some(Self::Standard),
            _ => None,
        }
    }

    fn minimum_completed_matches(self) -> u32 {
        match self {
            Self::Casual => 0,
            Self::Standard => STANDARD_MINIMUM_COMPLETED_MATCHES,
        }
    }
}

#[derive(Clone, Copy, Debug, Eq, PartialEq, Serialize)]
#[serde(rename_all = "snake_case")]
enum MatchmakingLobbyStatus {
    Waiting,
    ReadyToStart,
}

#[derive(Debug)]
struct StoredLobbyMember {
    local_id: String,
    _completed_matches: u32,
    _joined_at: String,
}

#[derive(Debug)]
struct StoredMatchmakingLobby {
    id: String,
    lobby_name: String,
    lobby_type: LobbyType,
    minimum_completed_matches: u32,
    required_players: u8,
    build_id: String,
    map_id: String,
    endpoint_token: String,
    status: MatchmakingLobbyStatus,
    created_at: String,
    updated_at: String,
    members: Vec<StoredLobbyMember>,
}

#[derive(Debug, Serialize)]
#[serde(rename_all = "camelCase")]
struct MatchmakingLobbyView {
    id: String,
    lobby_name: String,
    lobby_type: LobbyType,
    minimum_completed_matches: u32,
    required_players: u8,
    current_players: usize,
    build_id: String,
    map_id: String,
    status: MatchmakingLobbyStatus,
    created_at: String,
    updated_at: String,
}

#[derive(Debug, Serialize)]
#[serde(rename_all = "camelCase")]
struct MatchmakingLobbyJoinView {
    #[serde(flatten)]
    lobby: MatchmakingLobbyView,
    endpoint_token: String,
}

#[derive(Debug)]
struct LobbyCreateRequest {
    lobby_name: String,
    lobby_type: LobbyType,
    host_local_id: String,
    host_completed_matches: u32,
    build_id: String,
    map_id: String,
    endpoint_token: String,
}

#[derive(Debug)]
struct LobbyJoinRequest {
    player_local_id: String,
    completed_matches: u32,
}

#[derive(Debug)]
struct LobbyLeaveRequest {
    player_local_id: String,
}

#[derive(Debug)]
struct SteamIdentityRequest {
    ticket_hex: String,
    _purpose: String,
    _local_run_id: Option<String>,
}

#[derive(Debug)]
struct ApiError {
    status: StatusCode,
    body: Value,
}

impl ApiError {
    fn new(status: StatusCode, body: Value) -> Self {
        Self { status, body }
    }
}

impl IntoResponse for ApiError {
    fn into_response(self) -> Response {
        (self.status, Json(self.body)).into_response()
    }
}

#[derive(Debug, Deserialize)]
#[serde(rename_all = "camelCase")]
struct LobbyListQuery {
    lobby_type: Option<String>,
}

pub fn create_router(state: AppState) -> Router {
    Router::new()
        .route("/healthz", get(healthz))
        .route("/v1/playtest-reports", post(create_playtest_report))
        .route("/v1/moderation/reports", post(create_moderation_report))
        .route(
            "/v1/identity/steam/session-ticket",
            post(verify_steam_session_ticket),
        )
        .route(
            "/v1/matchmaking/lobbies",
            get(list_lobbies).post(create_lobby),
        )
        .route("/v1/matchmaking/lobbies/{lobby_id}/join", post(join_lobby))
        .route(
            "/v1/matchmaking/lobbies/{lobby_id}/leave",
            post(leave_lobby),
        )
        .route("/v1/banlists/{scope}", get(get_banlist))
        .route("/v1/news", get(get_news))
        .route("/v1/fleet/servers", get(get_fleet_servers))
        .fallback(not_found)
        .layer(middleware::from_fn(authority_header))
        .with_state(state)
}

async fn authority_header(request: Request, next: Next) -> Response {
    let mut response = next.run(request).await;
    response.headers_mut().insert(
        "x-frostwake-authority",
        HeaderValue::from_static("non-authoritative"),
    );
    response
}

async fn healthz(State(state): State<AppState>) -> Response {
    let state = state.inner.lock().expect("backend state mutex poisoned");
    Json(json!({
        "ok": true,
        "service": SERVICE_NAME,
        "version": state.version,
        "startedAt": state.started_at,
    }))
    .into_response()
}

async fn create_playtest_report(
    State(state): State<AppState>,
    body: Bytes,
) -> Result<Response, ApiError> {
    let payload = parse_json_body(body)?;
    let payload = validate_playtest_report(&payload).map_err(|errors| {
        ApiError::new(
            StatusCode::UNPROCESSABLE_ENTITY,
            json!({ "error": "invalid_playtest_report", "details": errors }),
        )
    })?;
    let id = store_report(
        &state,
        ReportKind::Playtest,
        payload,
        "ptr",
        state.now_iso(),
    );

    Ok((
        StatusCode::ACCEPTED,
        Json(json!({ "accepted": true, "id": id })),
    )
        .into_response())
}

async fn create_moderation_report(
    State(state): State<AppState>,
    body: Bytes,
) -> Result<Response, ApiError> {
    let payload = parse_json_body(body)?;
    let payload = validate_moderation_report(&payload).map_err(|errors| {
        ApiError::new(
            StatusCode::UNPROCESSABLE_ENTITY,
            json!({ "error": "invalid_moderation_report", "details": errors }),
        )
    })?;
    let id = store_report(
        &state,
        ReportKind::Moderation,
        payload,
        "mod",
        state.now_iso(),
    );

    Ok((
        StatusCode::ACCEPTED,
        Json(json!({ "accepted": true, "id": id })),
    )
        .into_response())
}

async fn verify_steam_session_ticket(
    State(state): State<AppState>,
    body: Bytes,
) -> Result<Response, ApiError> {
    let payload = parse_json_body(body)?;
    let request = validate_steam_identity_request(&payload).map_err(|errors| {
        ApiError::new(
            StatusCode::UNPROCESSABLE_ENTITY,
            json!({ "error": "invalid_steam_identity_request", "details": errors }),
        )
    })?;
    let config = state
        .inner
        .lock()
        .expect("backend state mutex poisoned")
        .steam_identity
        .clone();
    let Some(config) = config else {
        return Ok((
            StatusCode::SERVICE_UNAVAILABLE,
            Json(json!({
                "error": "steam_identity_not_configured",
                "message": "Steam identity verification is not configured for this backend",
            })),
        )
            .into_response());
    };

    match request.ticket_hex.as_str() {
        MOCK_STEAM_TICKET_SUCCESS => {}
        MOCK_STEAM_TICKET_NO_OWNERSHIP => {
            return Ok((
                StatusCode::FORBIDDEN,
                Json(json!({ "error": "app_ownership_required" })),
            )
                .into_response());
        }
        MOCK_STEAM_TICKET_INVALID => {
            return Ok((
                StatusCode::UNAUTHORIZED,
                Json(json!({ "error": "invalid_steam_ticket" })),
            )
                .into_response());
        }
        _ => {
            return Ok((
                StatusCode::UNAUTHORIZED,
                Json(json!({ "error": "invalid_steam_ticket" })),
            )
                .into_response());
        }
    }

    let expires_at =
        iso_timestamp((state.now)() + Duration::minutes(STEAM_IDENTITY_PROOF_TTL_MINUTES));
    Ok(Json(json!({
        "verified": true,
        "ownsApp": true,
        "subjectHash": mock_subject_hash(config.app_id, &config.subject_hash_salt, &request.ticket_hex),
        "proofId": format!("sip_{}", Uuid::new_v4()),
        "expiresAt": expires_at,
    }))
    .into_response())
}

async fn list_lobbies(
    State(state): State<AppState>,
    Query(query): Query<LobbyListQuery>,
) -> Result<Response, ApiError> {
    let lobby_type_filter = match query.lobby_type.as_deref() {
        None => None,
        Some(value) => Some(LobbyType::parse(value).ok_or_else(|| {
            ApiError::new(
                StatusCode::BAD_REQUEST,
                json!({
                    "error": "invalid_lobby_type",
                    "message": "lobbyType must be casual or standard",
                }),
            )
        })?),
    };

    let state = state.inner.lock().expect("backend state mutex poisoned");
    let items: Vec<_> = state
        .matchmaking_lobbies
        .iter()
        .filter(|lobby| lobby_type_filter.is_none_or(|filter| lobby.lobby_type == filter))
        .map(to_lobby_view)
        .collect();

    Ok(Json(json!({ "items": items })).into_response())
}

async fn create_lobby(State(state): State<AppState>, body: Bytes) -> Result<Response, ApiError> {
    let payload = parse_json_body(body)?;
    let request = validate_lobby_create_request(&payload).map_err(|errors| {
        ApiError::new(
            StatusCode::UNPROCESSABLE_ENTITY,
            json!({ "error": "invalid_lobby_create_request", "details": errors }),
        )
    })?;
    let timestamp = state.now_iso();

    let mut lobby = StoredMatchmakingLobby {
        id: format!("lob_{}", Uuid::new_v4()),
        lobby_name: request.lobby_name,
        lobby_type: request.lobby_type,
        minimum_completed_matches: request.lobby_type.minimum_completed_matches(),
        required_players: FIXED_MATCH_PLAYERS,
        build_id: request.build_id,
        map_id: request.map_id,
        endpoint_token: request.endpoint_token,
        status: MatchmakingLobbyStatus::Waiting,
        created_at: timestamp.clone(),
        updated_at: timestamp.clone(),
        members: vec![StoredLobbyMember {
            local_id: request.host_local_id,
            _completed_matches: request.host_completed_matches,
            _joined_at: timestamp.clone(),
        }],
    };
    refresh_lobby_status(&mut lobby, timestamp);

    let response_lobby = to_lobby_join_view(&lobby);
    state
        .inner
        .lock()
        .expect("backend state mutex poisoned")
        .matchmaking_lobbies
        .push(lobby);

    Ok((
        StatusCode::CREATED,
        Json(json!({ "lobby": response_lobby })),
    )
        .into_response())
}

async fn join_lobby(
    State(state): State<AppState>,
    Path(lobby_id): Path<String>,
    body: Bytes,
) -> Result<Response, ApiError> {
    let payload = parse_json_body(body)?;
    let request = validate_lobby_join_request(&payload).map_err(|errors| {
        ApiError::new(
            StatusCode::UNPROCESSABLE_ENTITY,
            json!({ "error": "invalid_lobby_join_request", "details": errors }),
        )
    })?;
    let timestamp = state.now_iso();
    let mut state = state.inner.lock().expect("backend state mutex poisoned");
    let Some(lobby) = state
        .matchmaking_lobbies
        .iter_mut()
        .find(|candidate| candidate.id == lobby_id)
    else {
        return Ok((
            StatusCode::NOT_FOUND,
            Json(json!({ "error": "lobby_not_found" })),
        )
            .into_response());
    };

    if request.completed_matches < lobby.minimum_completed_matches {
        return Ok((
            StatusCode::FORBIDDEN,
            Json(json!({
                "error": "experience_required",
                "requiredCompletedMatches": lobby.minimum_completed_matches,
            })),
        )
            .into_response());
    }

    if lobby
        .members
        .iter()
        .any(|member| member.local_id == request.player_local_id)
    {
        return Ok(Json(json!({
            "joined": false,
            "alreadyJoined": true,
            "matchStartEligible": lobby.status == MatchmakingLobbyStatus::ReadyToStart,
            "lobby": to_lobby_join_view(lobby),
        }))
        .into_response());
    }

    if lobby.members.len() >= usize::from(lobby.required_players) {
        return Ok((StatusCode::CONFLICT, Json(json!({ "error": "lobby_full" }))).into_response());
    }

    lobby.members.push(StoredLobbyMember {
        local_id: request.player_local_id,
        _completed_matches: request.completed_matches,
        _joined_at: timestamp.clone(),
    });
    refresh_lobby_status(lobby, timestamp);

    Ok(Json(json!({
        "joined": true,
        "matchStartEligible": lobby.status == MatchmakingLobbyStatus::ReadyToStart,
        "lobby": to_lobby_join_view(lobby),
    }))
    .into_response())
}

async fn leave_lobby(
    State(state): State<AppState>,
    Path(lobby_id): Path<String>,
    body: Bytes,
) -> Result<Response, ApiError> {
    let payload = parse_json_body(body)?;
    let request = validate_lobby_leave_request(&payload).map_err(|errors| {
        ApiError::new(
            StatusCode::UNPROCESSABLE_ENTITY,
            json!({ "error": "invalid_lobby_leave_request", "details": errors }),
        )
    })?;
    let timestamp = state.now_iso();
    let mut state = state.inner.lock().expect("backend state mutex poisoned");
    let Some(lobby_index) = state
        .matchmaking_lobbies
        .iter()
        .position(|candidate| candidate.id == lobby_id)
    else {
        return Ok((
            StatusCode::NOT_FOUND,
            Json(json!({ "error": "lobby_not_found" })),
        )
            .into_response());
    };

    let lobby = &mut state.matchmaking_lobbies[lobby_index];
    let Some(member_index) = lobby
        .members
        .iter()
        .position(|member| member.local_id == request.player_local_id)
    else {
        return Ok(Json(json!({
            "left": false,
            "alreadyAbsent": true,
            "lobby": to_lobby_view(lobby),
        }))
        .into_response());
    };

    lobby.members.remove(member_index);
    if lobby.members.is_empty() {
        state.matchmaking_lobbies.remove(lobby_index);
        return Ok(Json(json!({ "left": true, "lobbyDeleted": true })).into_response());
    }

    let lobby = &mut state.matchmaking_lobbies[lobby_index];
    refresh_lobby_status(lobby, timestamp);
    Ok(Json(json!({
        "left": true,
        "lobbyDeleted": false,
        "lobby": to_lobby_view(lobby),
    }))
    .into_response())
}

async fn get_banlist(State(state): State<AppState>, Path(scope): Path<String>) -> Response {
    if !matches!(scope.as_str(), "official" | "private_test" | "local") {
        return (
            StatusCode::NOT_FOUND,
            Json(json!({ "error": "unknown_banlist_scope" })),
        )
            .into_response();
    }

    Json(json!({
        "scope": scope,
        "updatedAt": state.now_iso(),
        "entries": [],
    }))
    .into_response()
}

async fn get_news() -> Response {
    Json(json!({ "items": [] })).into_response()
}

async fn get_fleet_servers() -> Response {
    Json(json!({ "items": [] })).into_response()
}

async fn not_found() -> Response {
    (StatusCode::NOT_FOUND, Json(json!({ "error": "not_found" }))).into_response()
}

enum ReportKind {
    Playtest,
    Moderation,
}

fn store_report(
    state: &AppState,
    kind: ReportKind,
    payload: Value,
    prefix: &str,
    received_at: String,
) -> String {
    let id = format!("{}_{}", prefix, Uuid::new_v4());
    let stored = StoredReport {
        id: id.clone(),
        received_at,
        payload,
    };

    let mut state = state.inner.lock().expect("backend state mutex poisoned");
    match kind {
        ReportKind::Playtest => state.playtest_reports.push(stored),
        ReportKind::Moderation => state.moderation_reports.push(stored),
    }
    id
}

fn parse_json_body(body: Bytes) -> Result<Value, ApiError> {
    if body.len() > MAX_BODY_BYTES {
        return Err(ApiError::new(
            StatusCode::PAYLOAD_TOO_LARGE,
            json!({
                "error": "request_too_large",
                "message": format!("request body exceeds {MAX_BODY_BYTES} bytes"),
            }),
        ));
    }

    if body.iter().all(u8::is_ascii_whitespace) {
        return Ok(json!({}));
    }

    serde_json::from_slice(&body).map_err(|_| {
        ApiError::new(
            StatusCode::BAD_REQUEST,
            json!({
                "error": "invalid_json",
                "message": "request body must be valid JSON",
            }),
        )
    })
}

fn validate_playtest_report(input: &Value) -> Result<Value, Vec<String>> {
    let Some(input) = input.as_object() else {
        return Err(vec!["payload must be an object".to_string()]);
    };

    let mut errors = Vec::new();
    reject_unknown_fields(
        input,
        &[
            "runId",
            "buildId",
            "mapId",
            "playerCount",
            "winner",
            "summary",
        ],
        &mut errors,
    );
    let run_id = read_string(input, "runId", 1, 64, &mut errors);
    let build_id = read_string(input, "buildId", 1, 128, &mut errors);
    let map_id = read_string(input, "mapId", 1, 128, &mut errors);
    let player_count = read_integer(input, "playerCount", 1, 8, &mut errors);
    let summary = read_string(input, "summary", 1, 8000, &mut errors);
    let winner = read_optional_enum(input, "winner", &PLAYTEST_WINNERS, &mut errors);
    reject_risky_text("runId", &run_id, &mut errors);
    reject_risky_text("buildId", &build_id, &mut errors);
    reject_risky_text("mapId", &map_id, &mut errors);
    reject_risky_text("summary", &summary, &mut errors);

    if !errors.is_empty() {
        return Err(errors);
    }

    let mut report = Map::from_iter([
        ("runId".to_string(), json!(run_id)),
        ("buildId".to_string(), json!(build_id)),
        ("mapId".to_string(), json!(map_id)),
        ("playerCount".to_string(), json!(player_count)),
        ("summary".to_string(), json!(summary)),
    ]);
    if let Some(winner) = winner {
        report.insert("winner".to_string(), json!(winner));
    }

    Ok(Value::Object(report))
}

fn validate_moderation_report(input: &Value) -> Result<Value, Vec<String>> {
    let Some(input) = input.as_object() else {
        return Err(vec!["payload must be an object".to_string()]);
    };

    let mut errors = Vec::new();
    reject_unknown_fields(
        input,
        &[
            "runId",
            "reporterLocalId",
            "reportedLocalId",
            "matchId",
            "buildId",
            "mapId",
            "matchTimeSeconds",
            "reporterSlot",
            "reportedSlot",
            "voiceState",
            "actionTaken",
            "recentEventIds",
            "category",
            "summary",
        ],
        &mut errors,
    );
    let run_id = read_string(input, "runId", 1, 64, &mut errors);
    let reporter_local_id = read_string(input, "reporterLocalId", 1, 64, &mut errors);
    let reported_local_id = read_optional_string(input, "reportedLocalId", 1, 64, &mut errors);
    let match_id = read_optional_string(input, "matchId", 1, 64, &mut errors);
    let build_id = read_optional_string(input, "buildId", 1, 128, &mut errors);
    let map_id = read_optional_string(input, "mapId", 1, 128, &mut errors);
    let match_time_seconds = read_optional_integer(input, "matchTimeSeconds", 0, 7200, &mut errors);
    let reporter_slot = read_optional_integer(input, "reporterSlot", 1, 8, &mut errors);
    let reported_slot = read_optional_integer(input, "reportedSlot", 1, 8, &mut errors);
    let voice_state =
        read_optional_enum(input, "voiceState", &MODERATION_VOICE_STATES, &mut errors);
    let action_taken = read_optional_enum(input, "actionTaken", &MODERATION_ACTIONS, &mut errors);
    let recent_event_ids =
        read_optional_string_array(input, "recentEventIds", 16, 1, 64, &mut errors);
    let category = read_required_enum(input, "category", &MODERATION_CATEGORIES, &mut errors);
    let summary = read_string(input, "summary", 1, 4000, &mut errors);
    reject_risky_text("runId", &run_id, &mut errors);
    reject_risky_text("reporterLocalId", &reporter_local_id, &mut errors);
    if let Some(value) = reported_local_id.as_deref() {
        reject_risky_text("reportedLocalId", value, &mut errors);
    }
    if let Some(value) = match_id.as_deref() {
        reject_risky_text("matchId", value, &mut errors);
    }
    if let Some(value) = build_id.as_deref() {
        reject_risky_text("buildId", value, &mut errors);
    }
    if let Some(value) = map_id.as_deref() {
        reject_risky_text("mapId", value, &mut errors);
    }
    if let Some(values) = recent_event_ids.as_deref() {
        for (index, value) in values.iter().enumerate() {
            reject_risky_text(&format!("recentEventIds[{index}]"), value, &mut errors);
        }
    }
    reject_risky_text("summary", &summary, &mut errors);

    if !errors.is_empty() {
        return Err(errors);
    }

    let mut report = Map::from_iter([
        ("runId".to_string(), json!(run_id)),
        ("reporterLocalId".to_string(), json!(reporter_local_id)),
        ("category".to_string(), json!(category)),
        ("summary".to_string(), json!(summary)),
    ]);
    if let Some(reported_local_id) = reported_local_id {
        report.insert("reportedLocalId".to_string(), json!(reported_local_id));
    }
    if let Some(match_id) = match_id {
        report.insert("matchId".to_string(), json!(match_id));
    }
    if let Some(build_id) = build_id {
        report.insert("buildId".to_string(), json!(build_id));
    }
    if let Some(map_id) = map_id {
        report.insert("mapId".to_string(), json!(map_id));
    }
    if let Some(match_time_seconds) = match_time_seconds {
        report.insert("matchTimeSeconds".to_string(), json!(match_time_seconds));
    }
    if let Some(reporter_slot) = reporter_slot {
        report.insert("reporterSlot".to_string(), json!(reporter_slot));
    }
    if let Some(reported_slot) = reported_slot {
        report.insert("reportedSlot".to_string(), json!(reported_slot));
    }
    if let Some(voice_state) = voice_state {
        report.insert("voiceState".to_string(), json!(voice_state));
    }
    if let Some(action_taken) = action_taken {
        report.insert("actionTaken".to_string(), json!(action_taken));
    }
    if let Some(recent_event_ids) = recent_event_ids {
        report.insert("recentEventIds".to_string(), json!(recent_event_ids));
    }

    Ok(Value::Object(report))
}

fn validate_steam_identity_request(input: &Value) -> Result<SteamIdentityRequest, Vec<String>> {
    let Some(input) = input.as_object() else {
        return Err(vec!["payload must be an object".to_string()]);
    };

    let mut errors = Vec::new();
    reject_unknown_fields(input, &["ticketHex", "purpose", "localRunId"], &mut errors);
    let ticket_hex = read_string(input, "ticketHex", 2, 4096, &mut errors);
    let purpose = read_required_enum(input, "purpose", &STEAM_IDENTITY_PURPOSES, &mut errors);
    let local_run_id = read_optional_string(input, "localRunId", 1, 64, &mut errors);

    if !ticket_hex.is_empty() {
        if ticket_hex.len() % 2 != 0 {
            errors.push("ticketHex: expected even-length hex string".to_string());
        }
        if !ticket_hex
            .as_bytes()
            .iter()
            .all(|byte| byte.is_ascii_hexdigit())
        {
            errors.push("ticketHex: expected hex string".to_string());
        }
    }
    if let Some(value) = local_run_id.as_deref() {
        reject_risky_text("localRunId", value, &mut errors);
    }

    if !errors.is_empty() {
        return Err(errors);
    }

    Ok(SteamIdentityRequest {
        ticket_hex: ticket_hex.to_ascii_lowercase(),
        _purpose: purpose,
        _local_run_id: local_run_id,
    })
}

fn validate_lobby_create_request(input: &Value) -> Result<LobbyCreateRequest, Vec<String>> {
    let Some(input) = input.as_object() else {
        return Err(vec!["payload must be an object".to_string()]);
    };

    let mut errors = Vec::new();
    reject_unknown_fields(
        input,
        &[
            "lobbyName",
            "lobbyType",
            "hostLocalId",
            "hostCompletedMatches",
            "buildId",
            "mapId",
            "endpointToken",
        ],
        &mut errors,
    );
    let lobby_name = read_string(input, "lobbyName", 1, 64, &mut errors);
    let lobby_type = read_lobby_type(input, "lobbyType", &mut errors);
    let host_local_id = read_string(input, "hostLocalId", 1, 64, &mut errors);
    let host_completed_matches =
        read_integer(input, "hostCompletedMatches", 0, 100_000, &mut errors);
    let build_id = read_string(input, "buildId", 1, 128, &mut errors);
    let map_id = read_string(input, "mapId", 1, 128, &mut errors);
    let endpoint_token = read_string(input, "endpointToken", 1, 128, &mut errors);

    reject_risky_text("lobbyName", &lobby_name, &mut errors);
    reject_risky_text("hostLocalId", &host_local_id, &mut errors);
    reject_risky_text("endpointToken", &endpoint_token, &mut errors);
    if lobby_type == Some(LobbyType::Standard)
        && host_completed_matches < STANDARD_MINIMUM_COMPLETED_MATCHES
    {
        errors.push(format!(
            "hostCompletedMatches: standard lobbies require at least {STANDARD_MINIMUM_COMPLETED_MATCHES}"
        ));
    }

    if !errors.is_empty() {
        return Err(errors);
    }

    Ok(LobbyCreateRequest {
        lobby_name,
        lobby_type: lobby_type.expect("lobby type was validated"),
        host_local_id,
        host_completed_matches,
        build_id,
        map_id,
        endpoint_token,
    })
}

fn validate_lobby_join_request(input: &Value) -> Result<LobbyJoinRequest, Vec<String>> {
    let Some(input) = input.as_object() else {
        return Err(vec!["payload must be an object".to_string()]);
    };

    let mut errors = Vec::new();
    reject_unknown_fields(input, &["playerLocalId", "completedMatches"], &mut errors);
    let player_local_id = read_string(input, "playerLocalId", 1, 64, &mut errors);
    let completed_matches = read_integer(input, "completedMatches", 0, 100_000, &mut errors);
    reject_risky_text("playerLocalId", &player_local_id, &mut errors);

    if !errors.is_empty() {
        return Err(errors);
    }

    Ok(LobbyJoinRequest {
        player_local_id,
        completed_matches,
    })
}

fn validate_lobby_leave_request(input: &Value) -> Result<LobbyLeaveRequest, Vec<String>> {
    let Some(input) = input.as_object() else {
        return Err(vec!["payload must be an object".to_string()]);
    };

    let mut errors = Vec::new();
    reject_unknown_fields(input, &["playerLocalId"], &mut errors);
    let player_local_id = read_string(input, "playerLocalId", 1, 64, &mut errors);
    reject_risky_text("playerLocalId", &player_local_id, &mut errors);

    if !errors.is_empty() {
        return Err(errors);
    }

    Ok(LobbyLeaveRequest { player_local_id })
}

fn reject_unknown_fields(
    input: &Map<String, Value>,
    allowed_fields: &[&str],
    errors: &mut Vec<String>,
) {
    let allowed_fields: HashSet<_> = allowed_fields.iter().copied().collect();
    for field in input.keys() {
        if !allowed_fields.contains(field.as_str()) {
            errors.push(format!("{field}: unknown field"));
        }
    }
}

fn read_string(
    input: &Map<String, Value>,
    field: &str,
    min_length: usize,
    max_length: usize,
    errors: &mut Vec<String>,
) -> String {
    let Some(value) = input.get(field).and_then(Value::as_str) else {
        errors.push(format!("{field}: expected string"));
        return String::new();
    };

    if value.len() < min_length {
        errors.push(format!("{field}: shorter than {min_length}"));
    }
    if value.len() > max_length {
        errors.push(format!("{field}: longer than {max_length}"));
    }
    value.to_string()
}

fn read_optional_string(
    input: &Map<String, Value>,
    field: &str,
    min_length: usize,
    max_length: usize,
    errors: &mut Vec<String>,
) -> Option<String> {
    if !input.contains_key(field) {
        return None;
    }
    Some(read_string(input, field, min_length, max_length, errors))
}

fn read_integer(
    input: &Map<String, Value>,
    field: &str,
    minimum: u32,
    maximum: u32,
    errors: &mut Vec<String>,
) -> u32 {
    let Some(value) = input.get(field).and_then(Value::as_u64) else {
        errors.push(format!("{field}: expected integer"));
        return 0;
    };
    if value > u64::from(u32::MAX) {
        errors.push(format!("{field}: above {maximum}"));
        return 0;
    }
    let value = value as u32;
    if value < minimum {
        errors.push(format!("{field}: below {minimum}"));
    }
    if value > maximum {
        errors.push(format!("{field}: above {maximum}"));
    }
    value
}

fn read_optional_integer(
    input: &Map<String, Value>,
    field: &str,
    minimum: u32,
    maximum: u32,
    errors: &mut Vec<String>,
) -> Option<u32> {
    if !input.contains_key(field) {
        return None;
    }
    Some(read_integer(input, field, minimum, maximum, errors))
}

fn read_optional_string_array(
    input: &Map<String, Value>,
    field: &str,
    max_items: usize,
    min_length: usize,
    max_length: usize,
    errors: &mut Vec<String>,
) -> Option<Vec<String>> {
    if !input.contains_key(field) {
        return None;
    }
    let Some(values) = input.get(field).and_then(Value::as_array) else {
        errors.push(format!("{field}: expected array"));
        return None;
    };
    if values.len() > max_items {
        errors.push(format!("{field}: more than {max_items} items"));
    }

    let mut strings = Vec::with_capacity(values.len());
    for (index, value) in values.iter().enumerate() {
        let Some(value) = value.as_str() else {
            errors.push(format!("{field}[{index}]: expected string"));
            continue;
        };
        if value.len() < min_length {
            errors.push(format!("{field}[{index}]: shorter than {min_length}"));
        }
        if value.len() > max_length {
            errors.push(format!("{field}[{index}]: longer than {max_length}"));
        }
        strings.push(value.to_string());
    }
    Some(strings)
}

fn read_optional_enum(
    input: &Map<String, Value>,
    field: &str,
    allowed: &HashSet<&'static str>,
    errors: &mut Vec<String>,
) -> Option<String> {
    if !input.contains_key(field) {
        return None;
    }
    let Some(value) = input.get(field).and_then(Value::as_str) else {
        errors.push(format!("{field}: unsupported value"));
        return None;
    };
    if !allowed.contains(value) {
        errors.push(format!("{field}: unsupported value"));
        return None;
    }
    Some(value.to_string())
}

fn read_required_enum(
    input: &Map<String, Value>,
    field: &str,
    allowed: &HashSet<&'static str>,
    errors: &mut Vec<String>,
) -> String {
    let Some(value) = input.get(field).and_then(Value::as_str) else {
        errors.push(format!("{field}: unsupported value"));
        return String::new();
    };
    if !allowed.contains(value) {
        errors.push(format!("{field}: unsupported value"));
        return String::new();
    }
    value.to_string()
}

fn read_lobby_type(
    input: &Map<String, Value>,
    field: &str,
    errors: &mut Vec<String>,
) -> Option<LobbyType> {
    let Some(value) = input.get(field).and_then(Value::as_str) else {
        errors.push(format!("{field}: unsupported value"));
        return None;
    };
    let parsed = LobbyType::parse(value);
    if parsed.is_none() {
        errors.push(format!("{field}: unsupported value"));
    }
    parsed
}

fn reject_risky_text(field: &str, value: &str, errors: &mut Vec<String>) {
    for (label, pattern) in RISKY_TEXT_PATTERNS.iter() {
        if pattern.is_match(value) {
            errors.push(format!("{field}: contains {label}"));
        }
    }
}

fn mock_subject_hash(app_id: u32, subject_hash_salt: &str, ticket_hex: &str) -> String {
    let app_id = app_id.to_string();
    let mut hash = 0xcbf2_9ce4_8422_2325_u64;
    for value in [subject_hash_salt, app_id.as_str(), ticket_hex] {
        for byte in value.as_bytes() {
            hash ^= u64::from(*byte);
            hash = hash.wrapping_mul(0x0000_0100_0000_01b3);
        }
        hash ^= 0xff;
        hash = hash.wrapping_mul(0x0000_0100_0000_01b3);
    }
    format!("sub_{hash:016x}")
}

fn refresh_lobby_status(lobby: &mut StoredMatchmakingLobby, timestamp: String) {
    lobby.status = if lobby.members.len() >= usize::from(lobby.required_players) {
        MatchmakingLobbyStatus::ReadyToStart
    } else {
        MatchmakingLobbyStatus::Waiting
    };
    lobby.updated_at = timestamp;
}

fn to_lobby_view(lobby: &StoredMatchmakingLobby) -> MatchmakingLobbyView {
    MatchmakingLobbyView {
        id: lobby.id.clone(),
        lobby_name: lobby.lobby_name.clone(),
        lobby_type: lobby.lobby_type,
        minimum_completed_matches: lobby.minimum_completed_matches,
        required_players: lobby.required_players,
        current_players: lobby.members.len(),
        build_id: lobby.build_id.clone(),
        map_id: lobby.map_id.clone(),
        status: lobby.status,
        created_at: lobby.created_at.clone(),
        updated_at: lobby.updated_at.clone(),
    }
}

fn to_lobby_join_view(lobby: &StoredMatchmakingLobby) -> MatchmakingLobbyJoinView {
    MatchmakingLobbyJoinView {
        lobby: to_lobby_view(lobby),
        endpoint_token: lobby.endpoint_token.clone(),
    }
}

fn iso_timestamp(timestamp: DateTime<Utc>) -> String {
    timestamp.to_rfc3339_opts(SecondsFormat::Millis, true)
}
