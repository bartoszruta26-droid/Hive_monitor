#!/usr/bin/env python3
import json
import os
import pathlib
import socket
import sqlite3
import subprocess
import tempfile
import time
import urllib.error
import urllib.request

ROOT = pathlib.Path(__file__).resolve().parents[2]
WEBUI = ROOT / "src" / "webui"
KEY = "test-api-key-0123456789abcdef"

def request(url, key=None, method="GET"):
    headers = {}
    if key is not None:
        headers["X-API-Key"] = key
    req = urllib.request.Request(url, headers=headers, method=method)
    try:
        with urllib.request.urlopen(req, timeout=2) as response:
            return response.status, response.read().decode("utf-8")
    except urllib.error.HTTPError as error:
        return error.code, error.read().decode("utf-8")

with tempfile.TemporaryDirectory(prefix="apiary-legacy-test-") as tmp:
    db_path = pathlib.Path(tmp) / "apiary.db"
    connection = sqlite3.connect(db_path)
    connection.executescript("""
        CREATE TABLE raw_data (id INTEGER PRIMARY KEY, timestamp INTEGER, hive_id TEXT);
        CREATE TABLE aggregated_data (
            id INTEGER PRIMARY KEY, timestamp_start INTEGER, timestamp_end INTEGER,
            hive_id TEXT, agg_type INTEGER, temperature_avg REAL
        );
        INSERT INTO aggregated_data VALUES (1, 100, 160, 'UL-1', 1, 34.5);
        INSERT INTO aggregated_data VALUES (2, 100, 160, 'UL-2', 1, 31.5);
        INSERT INTO raw_data VALUES (1, 100, 'UL-1');
    """)
    connection.commit()
    connection.close()

    with socket.socket() as sock:
        sock.bind(("127.0.0.1", 0))
        port = sock.getsockname()[1]
    env = os.environ.copy()
    env["APIARY_API_KEY"] = KEY
    env["APIARY_DB_PATH"] = str(db_path)
    server = subprocess.Popen(
        ["php", "-S", f"127.0.0.1:{port}", "-t", str(WEBUI)],
        env=env, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL
    )
    base = f"http://127.0.0.1:{port}/legacy_api.php"
    try:
        for _ in range(50):
            try:
                with socket.create_connection(("127.0.0.1", port), timeout=0.1):
                    break
            except OSError:
                time.sleep(0.05)
        else:
            raise AssertionError("PHP test server did not start")

        assert request(base)[0] == 401
        assert request(base, "wrong-key")[0] == 401
        status, body = request(base, KEY)
        assert status == 200 and json.loads(body)["hive_id"] == "UL-2"

        status, body = request(base + "?hive_id=UL-1", KEY)
        assert status == 200 and json.loads(body)["hive_id"] == "UL-1"
        assert request(base + "?hive_id=%25", KEY)[0] == 400
        assert request(base + "?limit=100000", KEY)[0] == 400
        assert request(base + "?limit=0", KEY)[0] == 400
        assert request(base + "?action=unknown", KEY)[0] == 400
        assert request(base, KEY, "POST")[0] == 405
        status, body = request(base + "?action=stats", KEY)
        assert status == 200 and json.loads(body)["raw_count"] == 1
    finally:
        server.terminate()
        try:
            server.wait(timeout=3)
        except subprocess.TimeoutExpired:
            server.kill()
            server.wait()

print("Legacy PHP API authentication and input-boundary regressions: OK")
