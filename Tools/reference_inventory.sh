#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
MANIFEST="${ROOT_DIR}/references/external-paths.json"
OUT_DIR="${ROOT_DIR}/references/inventory"
OUT_FILE="${OUT_DIR}/inventory_$(date +%Y%m%d_%H%M%S).tsv"
SUMMARY_FILE="${OUT_FILE%.tsv}_summary.tsv"
MAX_DEPTH="${MAX_DEPTH:-3}"
MAX_FILES_PER_SOURCE="${MAX_FILES_PER_SOURCE:-1000}"
HASH_FILES="${HASH_FILES:-0}"

mkdir -p "${OUT_DIR}"

if ! command -v jq >/dev/null 2>&1; then
  echo "jq is required" >&2
  exit 2
fi

printf "source\tpath\ttype\tsize_bytes\tmtime_epoch\tsha256\n" > "${OUT_FILE}"

jq -r '.paths[].path' "${MANIFEST}" | while IFS= read -r source_path; do
  if [[ ! -e "${source_path}" ]]; then
    printf "%s\t%s\t%s\t%s\t%s\t%s\n" "${source_path}" "${source_path}" "missing" "" "" "" >> "${OUT_FILE}"
    continue
  fi

  count=0
  while IFS= read -r -d '' file_path; do
    count=$((count + 1))
    if [[ "${count}" -gt "${MAX_FILES_PER_SOURCE}" ]]; then
      printf "%s\t%s\t%s\t%s\t%s\t%s\n" "${source_path}" "__truncated__" "truncated" "" "" "max_files_${MAX_FILES_PER_SOURCE}" >> "${OUT_FILE}"
      break
    fi
    size_bytes="$(stat -f '%z' "${file_path}" 2>/dev/null || printf '')"
    mtime_epoch="$(stat -f '%m' "${file_path}" 2>/dev/null || printf '')"
    if [[ "${HASH_FILES}" == "1" && -n "${size_bytes}" && "${size_bytes}" -le 52428800 ]]; then
      sha256="$(shasum -a 256 "${file_path}" | awk '{print $1}')"
    elif [[ "${HASH_FILES}" == "1" ]]; then
      sha256="skipped_large_file"
    else
      sha256="skipped"
    fi
    printf "%s\t%s\t%s\t%s\t%s\t%s\n" "${source_path}" "${file_path}" "file" "${size_bytes}" "${mtime_epoch}" "${sha256}" >> "${OUT_FILE}"
  done < <(
    find "${source_path}" -maxdepth "${MAX_DEPTH}" \
      \( -name .git -o -name .venv -o -name node_modules -o -name .mypy_cache -o -name __pycache__ -o -name Saved -o -name Logs \) -prune \
      -o -type f -print0 2>/dev/null
  )
done

echo "Wrote local-only inventory: ${OUT_FILE}"
awk -F '\t' '
  NR > 1 {
    ext = "no_ext"
    n = split($2, parts, "/")
    name = parts[n]
    if (name ~ /\./) {
      ext = name
      sub(/^.*\./, ".", ext)
    }
    key = $1 "\t" ext
    counts[key] += 1
    bytes[key] += $4
  }
  END {
    print "source\textension\tfile_count\ttotal_size_bytes"
    for (key in counts) {
      print key "\t" counts[key] "\t" bytes[key]
    }
  }
' "${OUT_FILE}" > "${SUMMARY_FILE}"
echo "Wrote local-only summary: ${SUMMARY_FILE}"
echo "Do not commit files under references/inventory/."
