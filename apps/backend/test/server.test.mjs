import assert from "node:assert/strict";
import { test } from "node:test";

import { createAppServer } from "../dist/server.js";

async function withServer(callback) {
  const server = createAppServer({
    now: () => new Date("2026-05-25T00:00:00.000Z"),
  });
  await new Promise((resolve) => server.listen(0, "127.0.0.1", resolve));
  const address = server.address();
  try {
    await callback(`http://127.0.0.1:${address.port}`);
  } finally {
    await new Promise((resolve, reject) => server.close((error) => (error ? reject(error) : resolve())));
  }
}

test("healthz returns non-authoritative service status", async () => {
  await withServer(async (baseUrl) => {
    const response = await fetch(`${baseUrl}/healthz`);
    const body = await response.json();

    assert.equal(response.status, 200);
    assert.equal(response.headers.get("x-frostwake-authority"), "non-authoritative");
    assert.equal(body.ok, true);
    assert.equal(body.service, "frostwake-backend");
  });
});

test("accepts anonymized playtest reports", async () => {
  await withServer(async (baseUrl) => {
    const response = await fetch(`${baseUrl}/v1/playtest-reports`, {
      method: "POST",
      headers: { "content-type": "application/json" },
      body: JSON.stringify({
        runId: "P1-024-run-01",
        buildId: "AbyssLock-Win64-Development-local",
        mapId: "/Game/Maps/L_IcebreakerWhitebox",
        playerCount: 6,
        winner: "crew",
        summary: "Six players connected, roles assigned, repair and sabotage happened.",
      }),
    });
    const body = await response.json();

    assert.equal(response.status, 202);
    assert.equal(body.accepted, true);
    assert.match(body.id, /^ptr_/);
  });
});

test("rejects playtest reports with risky identity data", async () => {
  await withServer(async (baseUrl) => {
    const response = await fetch(`${baseUrl}/v1/playtest-reports`, {
      method: "POST",
      headers: { "content-type": "application/json" },
      body: JSON.stringify({
        runId: "P1-024-run-01",
        buildId: "AbyssLock-Win64-Development-local",
        mapId: "/Game/Maps/L_IcebreakerWhitebox",
        playerCount: 6,
        summary: "Host was 192.168.0.12 and should be redacted.",
      }),
    });
    const body = await response.json();

    assert.equal(response.status, 422);
    assert.equal(body.error, "invalid_playtest_report");
    assert.match(body.details.join(" "), /IPv4 address/);
  });
});

test("accepts moderation report metadata", async () => {
  await withServer(async (baseUrl) => {
    const response = await fetch(`${baseUrl}/v1/moderation/reports`, {
      method: "POST",
      headers: { "content-type": "application/json" },
      body: JSON.stringify({
        runId: "P1-024-run-01",
        reporterLocalId: "observer-a",
        reportedLocalId: "player-4",
        category: "griefing",
        summary: "Repeated deliberate team blocking was observed.",
      }),
    });
    const body = await response.json();

    assert.equal(response.status, 202);
    assert.equal(body.accepted, true);
    assert.match(body.id, /^mod_/);
  });
});

test("read-only metadata endpoints return empty local prototype data", async () => {
  await withServer(async (baseUrl) => {
    const [banlist, news, fleet] = await Promise.all([
      fetch(`${baseUrl}/v1/banlists/private_test`).then((response) => response.json()),
      fetch(`${baseUrl}/v1/news`).then((response) => response.json()),
      fetch(`${baseUrl}/v1/fleet/servers`).then((response) => response.json()),
    ]);

    assert.deepEqual(banlist.entries, []);
    assert.deepEqual(news.items, []);
    assert.deepEqual(fleet.items, []);
  });
});
