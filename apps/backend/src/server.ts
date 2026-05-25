import { randomUUID } from "node:crypto";
import { createServer, type IncomingMessage, type Server, type ServerResponse } from "node:http";

import {
  validateModerationReport,
  validatePlaytestReport,
  type ModerationReport,
  type PlaytestReport,
} from "./validation.js";

const MAX_BODY_BYTES = 32 * 1024;
const SERVICE_NAME = "frostwake-backend";

class RequestError extends Error {
  constructor(
    readonly statusCode: number,
    readonly code: string,
    message: string,
  ) {
    super(message);
  }
}

export interface StoredReport<T> {
  id: string;
  receivedAt: string;
  payload: T;
}

export interface BackendState {
  readonly startedAt: string;
  readonly version: string;
  playtestReports: StoredReport<PlaytestReport>[];
  moderationReports: StoredReport<ModerationReport>[];
}

export function createBackendState(version = "0.1.0", now: () => Date = () => new Date()): BackendState {
  return {
    startedAt: now().toISOString(),
    version,
    playtestReports: [],
    moderationReports: [],
  };
}

export function createAppServer(options: { state?: BackendState; now?: () => Date } = {}): Server {
  const now = options.now ?? (() => new Date());
  const state = options.state ?? createBackendState("0.1.0", now);

  return createServer(async (request, response) => {
    try {
      await routeRequest(request, response, state, now);
    } catch (error) {
      if (error instanceof RequestError) {
        sendJson(response, error.statusCode, {
          error: error.code,
          message: error.message,
        });
        return;
      }
      sendJson(response, 500, {
        error: "internal_error",
        message: error instanceof Error ? error.message : "unknown error",
      });
    }
  });
}

async function routeRequest(
  request: IncomingMessage,
  response: ServerResponse,
  state: BackendState,
  now: () => Date,
): Promise<void> {
  const method = request.method ?? "GET";
  const url = new URL(request.url ?? "/", "http://localhost");

  if (method === "GET" && url.pathname === "/healthz") {
    sendJson(response, 200, {
      ok: true,
      service: SERVICE_NAME,
      version: state.version,
      startedAt: state.startedAt,
    });
    return;
  }

  if (method === "POST" && url.pathname === "/v1/playtest-reports") {
    const body = await readJsonBody(request);
    const validation = validatePlaytestReport(body);
    if (!validation.ok) {
      sendJson(response, 422, { error: "invalid_playtest_report", details: validation.errors });
      return;
    }

    const stored = storeReport(state.playtestReports, validation.value, "ptr", now);
    sendJson(response, 202, { accepted: true, id: stored.id });
    return;
  }

  if (method === "POST" && url.pathname === "/v1/moderation/reports") {
    const body = await readJsonBody(request);
    const validation = validateModerationReport(body);
    if (!validation.ok) {
      sendJson(response, 422, { error: "invalid_moderation_report", details: validation.errors });
      return;
    }

    const stored = storeReport(state.moderationReports, validation.value, "mod", now);
    sendJson(response, 202, { accepted: true, id: stored.id });
    return;
  }

  const banlistMatch = /^\/v1\/banlists\/([^/]+)$/.exec(url.pathname);
  if (method === "GET" && banlistMatch) {
    const scope = banlistMatch[1] ?? "";
    if (!["official", "private_test", "local"].includes(scope)) {
      sendJson(response, 404, { error: "unknown_banlist_scope" });
      return;
    }
    sendJson(response, 200, { scope, updatedAt: now().toISOString(), entries: [] });
    return;
  }

  if (method === "GET" && url.pathname === "/v1/news") {
    sendJson(response, 200, { items: [] });
    return;
  }

  if (method === "GET" && url.pathname === "/v1/fleet/servers") {
    sendJson(response, 200, { items: [] });
    return;
  }

  sendJson(response, 404, { error: "not_found" });
}

function storeReport<T>(target: StoredReport<T>[], payload: T, prefix: string, now: () => Date): StoredReport<T> {
  const stored = {
    id: `${prefix}_${randomUUID()}`,
    receivedAt: now().toISOString(),
    payload,
  };
  target.push(stored);
  return stored;
}

async function readJsonBody(request: IncomingMessage): Promise<unknown> {
  const chunks: Buffer[] = [];
  let totalBytes = 0;

  for await (const chunk of request) {
    const buffer = Buffer.isBuffer(chunk) ? chunk : Buffer.from(chunk);
    totalBytes += buffer.byteLength;
    if (totalBytes > MAX_BODY_BYTES) {
      throw new RequestError(413, "request_too_large", `request body exceeds ${MAX_BODY_BYTES} bytes`);
    }
    chunks.push(buffer);
  }

  const raw = Buffer.concat(chunks).toString("utf8");
  if (!raw.trim()) {
    return {};
  }

  try {
    return JSON.parse(raw) as unknown;
  } catch {
    throw new RequestError(400, "invalid_json", "request body must be valid JSON");
  }
}

function sendJson(response: ServerResponse, statusCode: number, body: unknown): void {
  const serialized = JSON.stringify(body);
  response.writeHead(statusCode, {
    "content-type": "application/json; charset=utf-8",
    "content-length": Buffer.byteLength(serialized).toString(),
    "x-frostwake-authority": "non-authoritative",
  });
  response.end(serialized);
}
