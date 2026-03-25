#!/usr/bin/env bash
set -euo pipefail

CFG="${1:-default.conf}"
BASE_DIR="$(cd "$(dirname "$CFG")" 2>/dev/null && pwd || pwd)"
CFG_PATH="$CFG"
if [[ "$CFG" != /* ]]; then
  CFG_PATH="$BASE_DIR/$CFG"
fi

cd "$BASE_DIR"

BIN=""
if [[ -x "./webserv" ]]; then
  BIN="./webserv"
elif [[ -x "./webserver" ]]; then
  BIN="./webserver"
else
  echo "FAIL: No server binary found (expected ./webserv or ./webserver)" >&2
  exit 1
fi

need_cmd() {
  command -v "$1" >/dev/null 2>&1 || { echo "FAIL: missing required command: $1" >&2; exit 1; }
}

need_cmd awk
need_cmd sed
need_cmd grep
need_cmd curl
need_cmd sha256sum
need_cmd ls
need_cmd stat
need_cmd mkdir
need_cmd rm
need_cmd head
need_cmd tail
need_cmd tr
need_cmd wc
need_cmd date
need_cmd sleep
need_cmd pkill
need_cmd pgrep
need_cmd nc

# --- tiny config parser (best-effort) ---
get_first_listen_port() {
  # Matches: listen localhost:8080;
  awk '/^[[:space:]]*listen[[:space:]]+/{
    gsub(";","",$0);
    for (i=1;i<=NF;i++) if ($i ~ /:[0-9]+$/) { split($i,a,":"); print a[2]; exit }
  }' "$CFG_PATH" 2>/dev/null || true
}

get_server_root() {
  awk 'BEGIN{in_server=0}
    /^[[:space:]]*server[[:space:]]*\{/{in_server=1}
    in_server && /^[[:space:]]*root[[:space:]]+/{gsub(";","",$2); print $2; exit}
  ' "$CFG_PATH" 2>/dev/null || true
}

get_location_value() {
  # args: location_path key
  local loc="$1" key="$2"
  awk -v LOC="$loc" -v KEY="$key" '
    function trim(s){ sub(/^[ \t\r\n]+/,"",s); sub(/[ \t\r\n]+$/,"",s); return s }
    BEGIN{in_loc=0; depth=0}
    /^[[:space:]]*location[[:space:]]+/{
      # location /upload {
      if ($2==LOC) { in_loc=1 }
    }
    in_loc {
      if (index($0,"{")>0) depth++
      if (index($0,"}")>0) { depth--; if (depth<=0) { in_loc=0 } }
      if ($1==KEY) { val=$2; gsub(";","",val); print val; exit }
    }
  ' "$CFG_PATH" 2>/dev/null || true
}

PORT="$(get_first_listen_port)"
ROOT="$(get_server_root)"
UPLOAD_STORE="$(get_location_value /upload upload_store)"
CGI_ROOT="$(get_location_value /cgi-bin root)"
if [[ -z "$CGI_ROOT" ]]; then
  CGI_ROOT="$(get_location_value /cgi-bin alias)"
fi

if [[ -z "$PORT" ]]; then PORT=8080; fi

SERVER="localhost"
URL_BASE="http://$SERVER:$PORT"

PASS=0
FAIL=0
FAIL_NOTES=()

say() { printf '%s\n' "$*"; }
pass() { PASS=$((PASS+1)); say "PASS: $*"; }
fail() {
  FAIL=$((FAIL+1))
  local name="$1"; shift
  local note="$*"
  say "FAIL: $name"
  [[ -n "$note" ]] && say "      $note"
  FAIL_NOTES+=("$name :: $note")
}

expect_status() {
  # args: name url expected_substring [curl_args...]
  local name="$1" url="$2" expected="$3"; shift 3
  local status
  status="$(curl -sS -i --max-time 5 "$@" "$url" | head -n 1 | tr -d '\r' || true)"
  if [[ "$status" == *"$expected"* ]]; then
    pass "$name ($status)"
  else
    local hint=""
    if [[ "$name" == GET\ /index.html* ]]; then
      hint="(hint: create 'www/index.html' or configure an 'index' directive)"
    fi
    fail "$name" "expected '$expected' got '${status:-<empty>}' $hint"
  fi
}

raw_request_status() {
  # args: request_bytes
  # prints status line (or empty)
  local req="$1"
  printf "%b" "$req" | nc -q 2 "$SERVER" "$PORT" 2>/dev/null | head -n 1 | tr -d '\r' || true
}

start_server() {
  # Stop any existing instance for this config
  pkill -f "^${BIN} ${CFG}$" 2>/dev/null || true
  pkill -f "^${BIN} ${CFG_PATH}$" 2>/dev/null || true

  nohup "$BIN" "$CFG" > /tmp/webserv_config_missing_check.log 2>&1 &
  SERVER_PID=$!
  # wait briefly for listen
  for _ in 1 2 3 4 5; do
    if curl -sS --max-time 1 "$URL_BASE/" >/dev/null 2>&1; then
      return 0
    fi
    sleep 0.2
  done
  say "--- server log ---"
  tail -n 60 /tmp/webserv_config_missing_check.log || true
  return 1
}

stop_server() {
  if [[ -n "${SERVER_PID:-}" ]]; then
    kill "$SERVER_PID" 2>/dev/null || true
    sleep 0.2
  fi
  pkill -f "^${BIN} ${CFG}$" 2>/dev/null || true
  pkill -f "^${BIN} ${CFG_PATH}$" 2>/dev/null || true
}

cleanup() {
  stop_server
}
trap cleanup EXIT

say "Config: $CFG_PATH"
say "Binary:  $BIN"
say "Port:    $PORT"
say "Root:    ${ROOT:-<not found>}"
say "Upload:  ${UPLOAD_STORE:-<not found>}"
say "CGI dir: ${CGI_ROOT:-<not found>}"
say ""

if ! start_server; then
  fail "Server start" "server did not respond on $URL_BASE/ (check /tmp/webserv_config_missing_check.log)"
  say "\n=== Summary ==="
  say "Passed: $PASS"
  say "Failed: $FAIL"
  exit 1
fi

# --- Basic GETs ---
expect_status "GET /" "$URL_BASE/" "200"
expect_status "GET /index.html" "$URL_BASE/index.html" "200"
expect_status "GET /index.html?query" "$URL_BASE/index.html?foo=bar" "200"

# --- Header limit / malformed header behavior ---
# Oversized header block should return 431, and should not be an empty response.
OVERSIZE_REQ=$'GET / HTTP/1.1\r\nHost: localhost\r\n'"$(printf 'X-Fill: %.0sA' {1..9000})"$'\r\n\r\n'
status="$(raw_request_status "$OVERSIZE_REQ")"
if [[ "$status" == *"431"* ]]; then
  pass "Oversized headers ($status)"
elif [[ -z "$status" ]]; then
  fail "Oversized headers" "no status line returned (likely connection closed before error response is generated)"
else
  fail "Oversized headers" "expected 431 got '$status'"
fi

# --- HTTP/1.0 Host optional (many testers expect this) ---
status="$(raw_request_status $'GET / HTTP/1.0\r\n\r\n')"
if [[ "$status" == *"200"* ]]; then
  pass "HTTP/1.0 without Host ($status)"
else
  fail "HTTP/1.0 without Host" "expected 200 got '${status:-<empty>}' (likely your reader rejects requestLineEnd==headerEnd for headerless requests)"
fi

# --- Upload tests (config-driven) ---
if [[ -n "$UPLOAD_STORE" ]]; then
  mkdir -p "$UPLOAD_STORE" 2>/dev/null || true
  test -w "$UPLOAD_STORE" 2>/dev/null || fail "upload_store writable" "upload_store is not writable: $UPLOAD_STORE"

  if [[ -f "./123.jpg" ]]; then
    rm -f "$UPLOAD_STORE/123.jpg" 2>/dev/null || true

    # Your server expects POST /upload/<filename> (not just /upload)
    status_line="$(curl -sS -i --max-time 10 -X POST -H 'Content-Type: image/jpeg' --data-binary @123.jpg "$URL_BASE/upload/123.jpg" | head -n 1 | tr -d '\r' || true)"
    if [[ "$status_line" == *"201"* ]]; then
      pass "POST /upload/123.jpg ($status_line)"
    else
      fail "POST /upload/123.jpg" "expected 201 Created got '${status_line:-<empty>}'"
    fi

    if [[ -f "$UPLOAD_STORE/123.jpg" ]]; then
      if sha256sum -c <(sha256sum 123.jpg | awk '{print $1"  '$UPLOAD_STORE'/123.jpg"}') >/dev/null 2>&1; then
        pass "Uploaded file matches sha256"
      else
        fail "Uploaded file matches sha256" "file written but content differs"
      fi
    else
      fail "Uploaded file exists" "expected file at $UPLOAD_STORE/123.jpg"
    fi

    # Overwrite should return 204 No Content in your ResponseBuilder
    status_line="$(curl -sS -i --max-time 10 -X POST -H 'Content-Type: image/jpeg' --data-binary @123.jpg "$URL_BASE/upload/123.jpg" | head -n 1 | tr -d '\r' || true)"
    if [[ "$status_line" == *"204"* ]]; then
      pass "POST overwrite /upload/123.jpg ($status_line)"
    else
      fail "POST overwrite /upload/123.jpg" "expected 204 got '${status_line:-<empty>}'"
    fi

    status_line="$(curl -sS -i --max-time 10 -X DELETE "$URL_BASE/upload/123.jpg" | head -n 1 | tr -d '\r' || true)"
    if [[ "$status_line" == *"204"* ]]; then
      pass "DELETE /upload/123.jpg ($status_line)"
    else
      fail "DELETE /upload/123.jpg" "expected 204 got '${status_line:-<empty>}'"
    fi

  else
    fail "Upload test" "./123.jpg not found (place a test image in $BASE_DIR/123.jpg)"
  fi

  # Chunked upload probe to /upload/<filename>
  # If this returns empty, chunked decode likely waits for trailers (common bug).
  status="$(raw_request_status $'POST /upload/chunk.txt HTTP/1.1\r\nHost: localhost\r\nContent-Type: application/octet-stream\r\nTransfer-Encoding: chunked\r\n\r\n4\r\nWiki\r\n5\r\npedia\r\n0\r\n\r\n')"
  if [[ "$status" == *"201"* || "$status" == *"204"* ]]; then
    pass "Chunked POST /upload/chunk.txt ($status)"
  elif [[ -z "$status" ]]; then
    fail "Chunked POST /upload/chunk.txt" "no response (likely chunked decoder waits for '\\r\\n\\r\\n' after the 0-chunk; should accept immediate '\\r\\n')"
  else
    fail "Chunked POST /upload/chunk.txt" "expected 201/204 got '$status'"
  fi
else
  fail "Upload tests" "no upload_store configured for location /upload"
fi

# --- CGI probe (best-effort) ---
# Create a tiny CGI script if possible, then request it.
CGI_DIR="$CGI_ROOT"
if [[ -n "$CGI_DIR" ]]; then
  mkdir -p "$CGI_DIR" 2>/dev/null || true
  if [[ -w "$CGI_DIR" ]]; then
    CGI_SCRIPT="$CGI_DIR/hello.py"
    if [[ ! -f "$CGI_SCRIPT" ]]; then
      cat > "$CGI_SCRIPT" <<'EOF'
#!/usr/bin/env python3
print("Content-Type: text/plain\r\n")
print("hello")
EOF
      chmod +x "$CGI_SCRIPT" 2>/dev/null || true
    fi
    status_line="$(curl -sS -i --max-time 5 "$URL_BASE/cgi-bin/hello.py" | head -n 1 | tr -d '\r' || true)"
    if [[ "$status_line" == *"200"* ]]; then
      pass "CGI GET /cgi-bin/hello.py ($status_line)"
    else
      fail "CGI GET /cgi-bin/hello.py" "got '${status_line:-<empty>}' (if 404: config probably needs 'alias' for /cgi-bin)"
    fi
  else
    fail "CGI probe" "cgi root not writable: $CGI_DIR"
  fi
else
  fail "CGI probe" "no cgi-bin root configured"
fi

say "\n=== Summary ==="
say "Passed: $PASS"
say "Failed: $FAIL"
if (( FAIL > 0 )); then
  say "\nFailures (with hints):"
  for item in "${FAIL_NOTES[@]}"; do
    say "- $item"
  done
  say "\nLog: /tmp/webserv_config_missing_check.log"
fi
