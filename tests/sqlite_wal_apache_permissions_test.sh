#!/usr/bin/env bash
# Integration regression: a collector process writes in WAL mode while the
# Apache identity (www-data) opens the same database read-only.
set -euo pipefail

if [[ "${EUID}" -ne 0 ]]; then
    echo "Run this integration test as root (for example: sudo bash $0)." >&2
    exit 2
fi

for command in useradd usermod runuser install stat php python3; do
    command -v "$command" >/dev/null || {
        echo "Missing required command: $command" >&2
        exit 2
    }
done

getent group www-data >/dev/null || {
    echo "The www-data group is required." >&2
    exit 2
}

if ! id apiaryguard >/dev/null 2>&1; then
    useradd --system --no-create-home --home-dir /nonexistent \
        --shell /usr/sbin/nologin --groups www-data apiaryguard
else
    usermod -a -G www-data apiaryguard
fi

tmp_dir="$(mktemp -d)"
# apiaryguard must traverse the root-owned temporary parent; the database
# subdirectory below remains restricted by its own 2770 mode.
chmod 0711 "$tmp_dir"
writer_pid=""
cleanup() {
    if [[ -n "$writer_pid" ]]; then
        kill "$writer_pid" 2>/dev/null || true
        wait "$writer_pid" 2>/dev/null || true
    fi
    rm -rf -- "$tmp_dir"
}
trap cleanup EXIT

db_dir="$tmp_dir/db"
db_path="$db_dir/apiary.db"
install -d -o apiaryguard -g www-data -m 2770 "$db_dir"

# Keep a writer connection open after committing so WAL and SHM remain present
# while the Apache identity attempts a concurrent read.
runuser -u apiaryguard -- env DB_PATH="$db_path" bash -c '
    umask 0007
    python3 - <<'"'"'PY'"'"'
import os
import sqlite3
import time

path = os.environ["DB_PATH"]
db = sqlite3.connect(path)
mode = db.execute("PRAGMA journal_mode=WAL").fetchone()[0]
assert mode.lower() == "wal", mode
db.execute("CREATE TABLE IF NOT EXISTS probe (value TEXT NOT NULL)")
db.execute("INSERT INTO probe(value) VALUES (?)", ("wal-visible",))
db.commit()
print("WAL_READY", flush=True)
time.sleep(15)
db.close()
PY
' >"$tmp_dir/writer.log" 2>&1 &
writer_pid=$!

for _ in $(seq 1 100); do
    if grep -q WAL_READY "$tmp_dir/writer.log" && [[ -e "$db_path-wal" && -e "$db_path-shm" ]]; then
        break
    fi
    sleep 0.1
done

grep -q WAL_READY "$tmp_dir/writer.log" || {
    cat "$tmp_dir/writer.log" >&2
    echo "SQLite writer did not become ready." >&2
    exit 1
}
[[ -e "$db_path-wal" && -e "$db_path-shm" ]] || {
    echo "SQLite WAL/SHM sidecars were not created." >&2
    exit 1
}

for file in "$db_path" "$db_path-wal" "$db_path-shm"; do
    [[ "$(stat -c '%G' "$file")" == "www-data" ]] || {
        echo "Unexpected group on $file: $(stat -c '%G' "$file")" >&2
        exit 1
    }
    [[ "$(stat -c '%a' "$file")" == "660" ]] || {
        echo "Unexpected mode on $file: $(stat -c '%a' "$file")" >&2
        exit 1
    }
done

runuser -u www-data -- php -r '
$db = new SQLite3($argv[1], SQLITE3_OPEN_READONLY);
$result = $db->querySingle("SELECT value FROM probe LIMIT 1");
if ($result !== "wal-visible") {
    fwrite(STDERR, "Unexpected SQLite result: " . var_export($result, true) . PHP_EOL);
    exit(1);
}
$db->close();
' "$db_path"

echo "SQLite WAL/SHM permissions and concurrent Apache read: OK"
