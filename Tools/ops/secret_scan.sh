#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"

PATTERN='(EXTERNAL_CHAT_BOT_TOKEN=.+|SecurityToken=[A-Fa-f0-9]{24,}|aws_access_key_id|aws_secret_access_key|BEGIN (RSA|OPENSSH|PRIVATE) KEY|xox[baprs]-|ghp_[A-Za-z0-9_]+)'

if rg -n --hidden \
  --glob '!references/private/**' \
  --glob '!references/inventory/**' \
  --glob '!.git/**' \
  --glob '!*.lock' \
  --glob '!Tools/ops/secret_scan.sh' \
  "${PATTERN}" "${ROOT_DIR}"; then
  echo "Potential secret found. Review before committing." >&2
  exit 1
fi

echo "No obvious secrets found in tracked-safe paths."
