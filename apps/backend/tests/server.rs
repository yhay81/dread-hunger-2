use axum::Router;
use axum::body::{Body, to_bytes};
use axum::http::{HeaderMap, Method, Request, StatusCode};
use chrono::{TimeZone, Utc};
use frostwake_backend::{AppState, MockSteamIdentityConfig, create_router};
use serde_json::{Value, json};
use tower::ServiceExt;

fn fixed_state() -> AppState {
    AppState::with_now("0.1.0", || {
        Utc.with_ymd_and_hms(2026, 5, 25, 0, 0, 0).unwrap()
    })
}

fn fixed_app() -> Router {
    create_router(fixed_state())
}

fn fixed_app_with_mock_steam_identity() -> Router {
    create_router(
        fixed_state()
            .with_mock_steam_identity(MockSteamIdentityConfig::new(480, "test-subject-hash-salt")),
    )
}

async fn request_json(app: Router, request: Request<Body>) -> (StatusCode, HeaderMap, Value) {
    let response = app.oneshot(request).await.expect("request succeeds");
    let status = response.status();
    let headers = response.headers().clone();
    let bytes = to_bytes(response.into_body(), usize::MAX)
        .await
        .expect("body reads");
    let value = serde_json::from_slice(&bytes).expect("response is JSON");
    (status, headers, value)
}

fn json_request(method: Method, uri: &str, body: Value) -> Request<Body> {
    Request::builder()
        .method(method)
        .uri(uri)
        .header("content-type", "application/json")
        .body(Body::from(body.to_string()))
        .expect("valid request")
}

#[tokio::test]
async fn healthz_returns_non_authoritative_service_status() {
    let (status, headers, body) = request_json(
        fixed_app(),
        Request::builder()
            .method(Method::GET)
            .uri("/healthz")
            .body(Body::empty())
            .expect("valid request"),
    )
    .await;

    assert_eq!(status, StatusCode::OK);
    assert_eq!(headers["x-frostwake-authority"], "non-authoritative");
    assert_eq!(body["ok"], true);
    assert_eq!(body["service"], "frostwake-backend");
    assert!(
        body["startedAt"].as_str().is_some(),
        "healthz must report startedAt as an ISO-8601 string per openapi.yaml HealthResponse"
    );
}

#[tokio::test]
async fn accepts_anonymized_playtest_reports() {
    let (status, _, body) = request_json(
        fixed_app(),
        json_request(
            Method::POST,
            "/v1/playtest-reports",
            json!({
                "runId": "P1-024-run-01",
                "buildId": "Frostwake-Win64-Development-local",
                "mapId": "/Game/Maps/L_IcebreakerWhitebox",
                "playerCount": 6,
                "winner": "crew",
                "summary": "Six players connected, roles assigned, repair and sabotage happened."
            }),
        ),
    )
    .await;

    assert_eq!(status, StatusCode::ACCEPTED);
    assert_eq!(body["accepted"], true);
    assert!(body["id"].as_str().is_some_and(|id| id.starts_with("ptr_")));
}

#[tokio::test]
async fn rejects_playtest_reports_with_risky_identity_data() {
    let (status, _, body) = request_json(
        fixed_app(),
        json_request(
            Method::POST,
            "/v1/playtest-reports",
            json!({
                "runId": "P1-024-run-01",
                "buildId": "Frostwake-Win64-Development-local",
                "mapId": "/Game/Maps/L_IcebreakerWhitebox",
                "playerCount": 6,
                "summary": "Host was 192.168.0.12 and should be redacted."
            }),
        ),
    )
    .await;

    assert_eq!(status, StatusCode::UNPROCESSABLE_ENTITY);
    assert_eq!(body["error"], "invalid_playtest_report");
    assert!(body["details"].to_string().contains("IPv4 address"));
}

#[tokio::test]
async fn rejects_playtest_reports_with_risky_identity_fields() {
    let (status, _, body) = request_json(
        fixed_app(),
        json_request(
            Method::POST,
            "/v1/playtest-reports",
            json!({
                "runId": "P1-024-76561190000000000",
                "buildId": "Frostwake-Win64-Development-local",
                "mapId": "C:\\Users\\tester\\PrivateMap.umap",
                "playerCount": 6,
                "summary": "Six players connected, roles assigned, repair and sabotage happened."
            }),
        ),
    )
    .await;

    assert_eq!(status, StatusCode::UNPROCESSABLE_ENTITY);
    assert_eq!(body["error"], "invalid_playtest_report");
    let details = body["details"].to_string();
    assert!(details.contains("runId: contains SteamID64-like value"));
    assert!(details.contains("mapId: contains local user path"));
}

#[tokio::test]
async fn accepts_moderation_report_metadata() {
    let (status, _, body) = request_json(
        fixed_app(),
        json_request(
            Method::POST,
            "/v1/moderation/reports",
            json!({
                "runId": "P1-024-run-01",
                "reporterLocalId": "observer-a",
                "reportedLocalId": "player-4",
                "category": "griefing",
                "summary": "Repeated deliberate team blocking was observed."
            }),
        ),
    )
    .await;

    assert_eq!(status, StatusCode::ACCEPTED);
    assert_eq!(body["accepted"], true);
    assert!(body["id"].as_str().is_some_and(|id| id.starts_with("mod_")));
}

#[tokio::test]
async fn accepts_voice_moderation_report_with_structured_evidence() {
    let (status, _, body) = request_json(
        fixed_app(),
        json_request(
            Method::POST,
            "/v1/moderation/reports",
            json!({
                "runId": "P2-voice-run-01",
                "matchId": "match-local-001",
                "buildId": "Frostwake-Win64-Development-local",
                "mapId": "L_IcebreakerWhitebox",
                "reporterLocalId": "player-2",
                "reportedLocalId": "player-5",
                "reporterSlot": 2,
                "reportedSlot": 5,
                "matchTimeSeconds": 430,
                "voiceState": "proximity",
                "actionTaken": "muted",
                "recentEventIds": ["evt-door-014", "evt-task-092", "evt-voice-window-003"],
                "category": "voice_abuse",
                "summary": "Reporter muted the player after disruptive proximity voice."
            }),
        ),
    )
    .await;

    assert_eq!(status, StatusCode::ACCEPTED);
    assert_eq!(body["accepted"], true);
    assert!(body["id"].as_str().is_some_and(|id| id.starts_with("mod_")));
}

#[tokio::test]
async fn rejects_moderation_reports_with_risky_identity_evidence() {
    let (status, _, body) = request_json(
        fixed_app(),
        json_request(
            Method::POST,
            "/v1/moderation/reports",
            json!({
                "runId": "P2-voice-run-01",
                "reporterLocalId": "player-2",
                "reportedLocalId": "76561190000000000",
                "category": "voice_abuse",
                "summary": "Reporter muted the player after disruptive proximity voice.",
                "recentEventIds": ["evt-76561190000000000"]
            }),
        ),
    )
    .await;

    assert_eq!(status, StatusCode::UNPROCESSABLE_ENTITY);
    assert_eq!(body["error"], "invalid_moderation_report");
    assert!(body["details"].to_string().contains("SteamID64-like value"));
}

#[tokio::test]
async fn rejects_moderation_reports_with_raw_voice_fields() {
    let (status, _, body) = request_json(
        fixed_app(),
        json_request(
            Method::POST,
            "/v1/moderation/reports",
            json!({
                "runId": "P2-voice-run-01",
                "reporterLocalId": "player-2",
                "reportedLocalId": "player-5",
                "category": "voice_abuse",
                "summary": "Reporter muted the player after disruptive proximity voice.",
                "rawVoiceTranscript": "verbatim audio text must not be accepted"
            }),
        ),
    )
    .await;

    assert_eq!(status, StatusCode::UNPROCESSABLE_ENTITY);
    assert_eq!(body["error"], "invalid_moderation_report");
    assert!(
        body["details"]
            .to_string()
            .contains("rawVoiceTranscript: unknown field")
    );
}

#[tokio::test]
async fn steam_identity_endpoint_requires_mock_config() {
    let (status, _, body) = request_json(
        fixed_app(),
        json_request(
            Method::POST,
            "/v1/identity/steam/session-ticket",
            json!({
                "ticketHex": "feedfacecafebeef",
                "purpose": "moderation",
                "localRunId": "P2-auth-run-01"
            }),
        ),
    )
    .await;

    assert_eq!(status, StatusCode::SERVICE_UNAVAILABLE);
    assert_eq!(body["error"], "steam_identity_not_configured");
}

#[tokio::test]
async fn steam_identity_endpoint_rejects_invalid_ticket() {
    let (status, _, body) = request_json(
        fixed_app_with_mock_steam_identity(),
        json_request(
            Method::POST,
            "/v1/identity/steam/session-ticket",
            json!({
                "ticketHex": "deadbeef",
                "purpose": "moderation"
            }),
        ),
    )
    .await;

    assert_eq!(status, StatusCode::UNAUTHORIZED);
    assert_eq!(body["error"], "invalid_steam_ticket");
}

#[tokio::test]
async fn steam_identity_endpoint_rejects_missing_app_ownership() {
    let (status, _, body) = request_json(
        fixed_app_with_mock_steam_identity(),
        json_request(
            Method::POST,
            "/v1/identity/steam/session-ticket",
            json!({
                "ticketHex": "0badc0de",
                "purpose": "support"
            }),
        ),
    )
    .await;

    assert_eq!(status, StatusCode::FORBIDDEN);
    assert_eq!(body["error"], "app_ownership_required");
}

#[tokio::test]
async fn steam_identity_endpoint_returns_hash_without_raw_identity_or_ticket_echo() {
    let (status, _, body) = request_json(
        fixed_app_with_mock_steam_identity(),
        json_request(
            Method::POST,
            "/v1/identity/steam/session-ticket",
            json!({
                "ticketHex": "feedfacecafebeef",
                "purpose": "server_metadata",
                "localRunId": "P2-auth-run-01"
            }),
        ),
    )
    .await;

    assert_eq!(status, StatusCode::OK);
    assert_eq!(body["verified"], true);
    assert_eq!(body["ownsApp"], true);
    assert_eq!(body["expiresAt"], "2026-05-25T00:15:00.000Z");
    assert!(
        body["subjectHash"]
            .as_str()
            .is_some_and(|value| value.starts_with("sub_"))
    );
    assert!(
        body["proofId"]
            .as_str()
            .is_some_and(|value| value.starts_with("sip_"))
    );
    let serialized = body.to_string();
    assert!(!serialized.contains("feedfacecafebeef"));
    assert!(!serialized.contains("7656119"));
    assert!(body.get("ticketHex").is_none());
    assert!(body.get("steamId").is_none());
}

#[tokio::test]
async fn steam_identity_endpoint_rejects_risky_local_run_id() {
    let (status, _, body) = request_json(
        fixed_app_with_mock_steam_identity(),
        json_request(
            Method::POST,
            "/v1/identity/steam/session-ticket",
            json!({
                "ticketHex": "feedfacecafebeef",
                "purpose": "moderation",
                "localRunId": "P2-auth-76561190000000000"
            }),
        ),
    )
    .await;

    assert_eq!(status, StatusCode::UNPROCESSABLE_ENTITY);
    assert_eq!(body["error"], "invalid_steam_identity_request");
    assert!(body["details"].to_string().contains("SteamID64-like value"));
}

#[tokio::test]
async fn returns_a_client_error_for_invalid_json() {
    let (status, _, body) = request_json(
        fixed_app(),
        Request::builder()
            .method(Method::POST)
            .uri("/v1/playtest-reports")
            .header("content-type", "application/json")
            .body(Body::from("{not json"))
            .expect("valid request"),
    )
    .await;

    assert_eq!(status, StatusCode::BAD_REQUEST);
    assert_eq!(body["error"], "invalid_json");
}

#[tokio::test]
async fn returns_request_too_large_for_oversized_json_bodies() {
    let (status, _, body) = request_json(
        fixed_app(),
        json_request(
            Method::POST,
            "/v1/playtest-reports",
            json!({
                "runId": "P1-024-run-01",
                "buildId": "Frostwake-Win64-Development-local",
                "mapId": "/Game/Maps/L_IcebreakerWhitebox",
                "playerCount": 6,
                "summary": "x".repeat(40000)
            }),
        ),
    )
    .await;

    assert_eq!(status, StatusCode::PAYLOAD_TOO_LARGE);
    assert_eq!(body["error"], "request_too_large");
}

#[tokio::test]
async fn creates_and_lists_casual_matchmaking_lobbies() {
    let app = fixed_app();
    let (create_status, _, created) = request_json(
        app.clone(),
        json_request(
            Method::POST,
            "/v1/matchmaking/lobbies",
            json!({
                "lobbyName": "Friday Casual",
                "lobbyType": "casual",
                "hostLocalId": "host-1",
                "hostCompletedMatches": 3,
                "buildId": "Frostwake-Win64-Development-local",
                "mapId": "L_IcebreakerWhitebox",
                "endpointToken": "local-lobby-token-001"
            }),
        ),
    )
    .await;

    assert_eq!(create_status, StatusCode::CREATED);
    assert!(
        created["lobby"]["id"]
            .as_str()
            .is_some_and(|id| id.starts_with("lob_"))
    );
    assert_eq!(created["lobby"]["lobbyName"], "Friday Casual");
    assert_eq!(created["lobby"]["lobbyType"], "casual");
    assert_eq!(created["lobby"]["requiredPlayers"], 8);
    assert_eq!(created["lobby"]["currentPlayers"], 1);
    assert_eq!(created["lobby"]["minimumCompletedMatches"], 0);
    assert_eq!(created["lobby"]["status"], "waiting");

    let (list_status, _, listed) = request_json(
        app,
        Request::builder()
            .method(Method::GET)
            .uri("/v1/matchmaking/lobbies?lobbyType=casual")
            .body(Body::empty())
            .expect("valid request"),
    )
    .await;

    assert_eq!(list_status, StatusCode::OK);
    assert_eq!(listed["items"].as_array().expect("items").len(), 1);
    assert_eq!(listed["items"][0]["id"], created["lobby"]["id"]);
    assert!(listed["items"][0].get("endpointToken").is_none());
}

#[tokio::test]
async fn standard_lobbies_require_fifty_completed_matches_and_become_ready_at_eight_players() {
    let app = fixed_app();
    let (rejected_status, _, rejected) = request_json(
        app.clone(),
        json_request(
            Method::POST,
            "/v1/matchmaking/lobbies",
            json!({
                "lobbyName": "Standard Run",
                "lobbyType": "standard",
                "hostLocalId": "host-1",
                "hostCompletedMatches": 49,
                "buildId": "Frostwake-Win64-Development-local",
                "mapId": "L_IcebreakerWhitebox",
                "endpointToken": "standard-token-001"
            }),
        ),
    )
    .await;

    assert_eq!(rejected_status, StatusCode::UNPROCESSABLE_ENTITY);
    assert_eq!(rejected["error"], "invalid_lobby_create_request");
    assert!(rejected["details"].to_string().contains("at least 50"));

    let (_, _, created) = request_json(
        app.clone(),
        json_request(
            Method::POST,
            "/v1/matchmaking/lobbies",
            json!({
                "lobbyName": "Standard Run",
                "lobbyType": "standard",
                "hostLocalId": "host-1",
                "hostCompletedMatches": 50,
                "buildId": "Frostwake-Win64-Development-local",
                "mapId": "L_IcebreakerWhitebox",
                "endpointToken": "standard-token-001"
            }),
        ),
    )
    .await;
    let lobby_id = created["lobby"]["id"].as_str().expect("lobby id");

    let (low_status, _, low_join) = request_json(
        app.clone(),
        json_request(
            Method::POST,
            &format!("/v1/matchmaking/lobbies/{lobby_id}/join"),
            json!({ "playerLocalId": "player-low", "completedMatches": 12 }),
        ),
    )
    .await;
    assert_eq!(low_status, StatusCode::FORBIDDEN);
    assert_eq!(low_join["error"], "experience_required");
    assert_eq!(low_join["requiredCompletedMatches"], 50);

    let mut latest_join = Value::Null;
    for player_index in 2..=8 {
        let (join_status, _, joined) = request_json(
            app.clone(),
            json_request(
                Method::POST,
                &format!("/v1/matchmaking/lobbies/{lobby_id}/join"),
                json!({ "playerLocalId": format!("player-{player_index}"), "completedMatches": 50 }),
            ),
        )
        .await;
        assert_eq!(join_status, StatusCode::OK);
        latest_join = joined;
    }

    assert_eq!(latest_join["matchStartEligible"], true);
    assert_eq!(latest_join["lobby"]["currentPlayers"], 8);
    assert_eq!(latest_join["lobby"]["status"], "ready_to_start");
    assert_eq!(latest_join["lobby"]["endpointToken"], "standard-token-001");
}

#[tokio::test]
async fn leaving_matchmaking_lobbies_updates_readiness_and_removes_empty_lobbies() {
    let app = fixed_app();
    let (_, _, created) = request_json(
        app.clone(),
        json_request(
            Method::POST,
            "/v1/matchmaking/lobbies",
            json!({
                "lobbyName": "Leave Test",
                "lobbyType": "casual",
                "hostLocalId": "host-1",
                "hostCompletedMatches": 0,
                "buildId": "Frostwake-Win64-Development-local",
                "mapId": "L_IcebreakerWhitebox",
                "endpointToken": "leave-token-001"
            }),
        ),
    )
    .await;
    let lobby_id = created["lobby"]["id"].as_str().expect("lobby id");

    for player_index in 2..=8 {
        let (join_status, _, _) = request_json(
            app.clone(),
            json_request(
                Method::POST,
                &format!("/v1/matchmaking/lobbies/{lobby_id}/join"),
                json!({ "playerLocalId": format!("player-{player_index}"), "completedMatches": 0 }),
            ),
        )
        .await;
        assert_eq!(join_status, StatusCode::OK);
    }

    let (leave_status, _, left) = request_json(
        app.clone(),
        json_request(
            Method::POST,
            &format!("/v1/matchmaking/lobbies/{lobby_id}/leave"),
            json!({ "playerLocalId": "player-8" }),
        ),
    )
    .await;

    assert_eq!(leave_status, StatusCode::OK);
    assert_eq!(left["left"], true);
    assert_eq!(left["lobby"]["currentPlayers"], 7);
    assert_eq!(left["lobby"]["status"], "waiting");
    assert!(left["lobby"].get("endpointToken").is_none());

    for player_local_id in [
        "host-1", "player-2", "player-3", "player-4", "player-5", "player-6", "player-7",
    ] {
        let (status, _, _) = request_json(
            app.clone(),
            json_request(
                Method::POST,
                &format!("/v1/matchmaking/lobbies/{lobby_id}/leave"),
                json!({ "playerLocalId": player_local_id }),
            ),
        )
        .await;
        assert_eq!(status, StatusCode::OK);
    }

    let (_, _, listed) = request_json(
        app,
        Request::builder()
            .method(Method::GET)
            .uri("/v1/matchmaking/lobbies")
            .body(Body::empty())
            .expect("valid request"),
    )
    .await;
    assert_eq!(listed["items"], json!([]));
}

#[tokio::test]
async fn joining_unknown_lobby_returns_not_found() {
    let (status, _, body) = request_json(
        fixed_app(),
        json_request(
            Method::POST,
            "/v1/matchmaking/lobbies/does-not-exist/join",
            json!({ "playerLocalId": "player-x", "completedMatches": 50 }),
        ),
    )
    .await;
    assert_eq!(status, StatusCode::NOT_FOUND);
    assert_eq!(body["error"], "lobby_not_found");
}

#[tokio::test]
async fn leaving_unknown_lobby_returns_not_found() {
    let (status, _, body) = request_json(
        fixed_app(),
        json_request(
            Method::POST,
            "/v1/matchmaking/lobbies/does-not-exist/leave",
            json!({ "playerLocalId": "player-x" }),
        ),
    )
    .await;
    assert_eq!(status, StatusCode::NOT_FOUND);
    assert_eq!(body["error"], "lobby_not_found");
}

#[tokio::test]
async fn banlist_unknown_scope_returns_not_found() {
    let (status, _, body) = request_json(
        fixed_app(),
        Request::builder()
            .method(Method::GET)
            .uri("/v1/banlists/bogus-scope")
            .body(Body::empty())
            .expect("valid request"),
    )
    .await;
    assert_eq!(status, StatusCode::NOT_FOUND);
    assert_eq!(body["error"], "unknown_banlist_scope");
}

#[tokio::test]
async fn joining_a_full_lobby_returns_conflict() {
    let app = fixed_app();
    let (_, _, created) = request_json(
        app.clone(),
        json_request(
            Method::POST,
            "/v1/matchmaking/lobbies",
            json!({
                "lobbyName": "Full Test",
                "lobbyType": "casual",
                "hostLocalId": "host-1",
                "hostCompletedMatches": 0,
                "buildId": "Frostwake-Win64-Development-local",
                "mapId": "L_IcebreakerWhitebox",
                "endpointToken": "full-token-001"
            }),
        ),
    )
    .await;
    let lobby_id = created["lobby"]["id"]
        .as_str()
        .expect("lobby id")
        .to_string();

    // Fill to capacity: host + players 2..=8 == 8 == required_players.
    for player_index in 2..=8 {
        let (join_status, _, _) = request_json(
            app.clone(),
            json_request(
                Method::POST,
                &format!("/v1/matchmaking/lobbies/{lobby_id}/join"),
                json!({ "playerLocalId": format!("player-{player_index}"), "completedMatches": 0 }),
            ),
        )
        .await;
        assert_eq!(join_status, StatusCode::OK);
    }

    // The next join exceeds capacity and must be rejected before any travel.
    let (status, _, body) = request_json(
        app,
        json_request(
            Method::POST,
            &format!("/v1/matchmaking/lobbies/{lobby_id}/join"),
            json!({ "playerLocalId": "player-9", "completedMatches": 0 }),
        ),
    )
    .await;
    assert_eq!(status, StatusCode::CONFLICT);
    assert_eq!(body["error"], "lobby_full");
}

#[tokio::test]
async fn read_only_metadata_endpoints_return_empty_local_prototype_data() {
    let app = fixed_app();

    let (_, _, banlist) = request_json(
        app.clone(),
        Request::builder()
            .method(Method::GET)
            .uri("/v1/banlists/private_test")
            .body(Body::empty())
            .expect("valid request"),
    )
    .await;
    let (_, _, news) = request_json(
        app.clone(),
        Request::builder()
            .method(Method::GET)
            .uri("/v1/news")
            .body(Body::empty())
            .expect("valid request"),
    )
    .await;
    let (_, _, fleet) = request_json(
        app,
        Request::builder()
            .method(Method::GET)
            .uri("/v1/fleet/servers")
            .body(Body::empty())
            .expect("valid request"),
    )
    .await;

    assert_eq!(banlist["entries"], json!([]));
    assert_eq!(news["items"], json!([]));
    assert_eq!(fleet["items"], json!([]));
}
