export interface PlaytestReport {
  runId: string;
  buildId: string;
  mapId: string;
  playerCount: number;
  winner?: "crew" | "agents" | "none" | "unknown";
  summary: string;
}

export interface ModerationReport {
  runId: string;
  reporterLocalId: string;
  reportedLocalId?: string;
  category: "voice_abuse" | "griefing" | "cheating" | "harassment" | "other";
  summary: string;
}

export type ValidationResult<T> =
  | { ok: true; value: T }
  | { ok: false; errors: string[] };

const PLAYTEST_WINNERS = new Set(["crew", "agents", "none", "unknown"]);
const MODERATION_CATEGORIES = new Set(["voice_abuse", "griefing", "cheating", "harassment", "other"]);

const RISKY_TEXT_PATTERNS: Array<[string, RegExp]> = [
  ["local user path", /\/Users\/[^/\s|`]+|[A-Z]:\\Users\\[^\\\s|`]+/i],
  ["IPv4 address", /\b(?:\d{1,3}\.){3}\d{1,3}\b/],
  ["SteamID64-like value", /\b7656119\d{10}\b/],
  ["email address", /\b[A-Za-z0-9._%+-]+@[A-Za-z0-9.-]+\.[A-Za-z]{2,}\b/],
  ["secret-like assignment", /\b(?:token|secret|password|apikey|api_key)\s*[:=]\s*[^,\s|`]+/i],
];

export function validatePlaytestReport(input: unknown): ValidationResult<PlaytestReport> {
  const errors: string[] = [];
  if (!isRecord(input)) {
    return { ok: false, errors: ["payload must be an object"] };
  }

  rejectUnknownFields(input, ["runId", "buildId", "mapId", "playerCount", "winner", "summary"], errors);
  const runId = readString(input, "runId", 1, 64, errors);
  const buildId = readString(input, "buildId", 1, 128, errors);
  const mapId = readString(input, "mapId", 1, 128, errors);
  const playerCount = readInteger(input, "playerCount", 1, 8, errors);
  const summary = readString(input, "summary", 1, 8000, errors);
  const winner = readOptionalEnum(input, "winner", PLAYTEST_WINNERS, errors);
  rejectRiskyText("summary", summary, errors);

  if (errors.length > 0) {
    return { ok: false, errors };
  }

  return {
    ok: true,
    value: winner === undefined ? { runId, buildId, mapId, playerCount, summary } : { runId, buildId, mapId, playerCount, winner, summary },
  };
}

export function validateModerationReport(input: unknown): ValidationResult<ModerationReport> {
  const errors: string[] = [];
  if (!isRecord(input)) {
    return { ok: false, errors: ["payload must be an object"] };
  }

  rejectUnknownFields(input, ["runId", "reporterLocalId", "reportedLocalId", "category", "summary"], errors);
  const runId = readString(input, "runId", 1, 64, errors);
  const reporterLocalId = readString(input, "reporterLocalId", 1, 64, errors);
  const reportedLocalId = readOptionalString(input, "reportedLocalId", 1, 64, errors);
  const category = readRequiredEnum(input, "category", MODERATION_CATEGORIES, errors);
  const summary = readString(input, "summary", 1, 4000, errors);
  rejectRiskyText("summary", summary, errors);

  if (errors.length > 0) {
    return { ok: false, errors };
  }

  return {
    ok: true,
    value:
      reportedLocalId === undefined
        ? { runId, reporterLocalId, category: category as ModerationReport["category"], summary }
        : { runId, reporterLocalId, reportedLocalId, category: category as ModerationReport["category"], summary },
  };
}

function isRecord(value: unknown): value is Record<string, unknown> {
  return typeof value === "object" && value !== null && !Array.isArray(value);
}

function rejectUnknownFields(input: Record<string, unknown>, allowedFields: string[], errors: string[]): void {
  const allowed = new Set(allowedFields);
  for (const field of Object.keys(input)) {
    if (!allowed.has(field)) {
      errors.push(`${field}: unknown field`);
    }
  }
}

function readString(input: Record<string, unknown>, field: string, minLength: number, maxLength: number, errors: string[]): string {
  const value = input[field];
  if (typeof value !== "string") {
    errors.push(`${field}: expected string`);
    return "";
  }
  if (value.length < minLength) {
    errors.push(`${field}: shorter than ${minLength}`);
  }
  if (value.length > maxLength) {
    errors.push(`${field}: longer than ${maxLength}`);
  }
  return value;
}

function readOptionalString(
  input: Record<string, unknown>,
  field: string,
  minLength: number,
  maxLength: number,
  errors: string[],
): string | undefined {
  if (!(field in input)) {
    return undefined;
  }
  return readString(input, field, minLength, maxLength, errors);
}

function readInteger(input: Record<string, unknown>, field: string, minimum: number, maximum: number, errors: string[]): number {
  const value = input[field];
  if (!Number.isInteger(value)) {
    errors.push(`${field}: expected integer`);
    return 0;
  }
  const numberValue = Number(value);
  if (numberValue < minimum) {
    errors.push(`${field}: below ${minimum}`);
  }
  if (numberValue > maximum) {
    errors.push(`${field}: above ${maximum}`);
  }
  return numberValue;
}

function readOptionalEnum(
  input: Record<string, unknown>,
  field: string,
  allowed: Set<string>,
  errors: string[],
): PlaytestReport["winner"] | undefined {
  if (!(field in input)) {
    return undefined;
  }
  const value = input[field];
  if (typeof value !== "string" || !allowed.has(value)) {
    errors.push(`${field}: unsupported value`);
    return undefined;
  }
  return value as PlaytestReport["winner"];
}

function readRequiredEnum(input: Record<string, unknown>, field: string, allowed: Set<string>, errors: string[]): string {
  const value = input[field];
  if (typeof value !== "string" || !allowed.has(value)) {
    errors.push(`${field}: unsupported value`);
    return "";
  }
  return value;
}

function rejectRiskyText(field: string, value: string, errors: string[]): void {
  for (const [label, pattern] of RISKY_TEXT_PATTERNS) {
    if (pattern.test(value)) {
      errors.push(`${field}: contains ${label}`);
    }
  }
}
