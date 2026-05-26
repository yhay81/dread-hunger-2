use std::env;
use std::error::Error;

use frostwake_backend::{AppState, create_router};
use tokio::net::TcpListener;

#[tokio::main]
async fn main() -> Result<(), Box<dyn Error>> {
    let port = env::var("PORT")
        .ok()
        .and_then(|value| value.parse::<u16>().ok())
        .unwrap_or(8787);
    let host = env::var("HOST").unwrap_or_else(|_| "127.0.0.1".to_string());
    let listener = TcpListener::bind((host.as_str(), port)).await?;

    println!(
        "{}",
        serde_json::json!({
            "event": "backend_started",
            "host": host,
            "port": port,
        })
    );

    axum::serve(listener, create_router(AppState::new("0.1.0"))).await?;
    Ok(())
}
