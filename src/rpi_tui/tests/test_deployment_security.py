from pathlib import Path

root = Path(__file__).resolve().parents[3]
makefile = (root / "src/rpi_tui/Makefile").read_text()
manifest = (root / "android_app/app/src/main/AndroidManifest.xml").read_text()
network_config = (root / "android_app/app/src/main/res/xml/network_security_config.xml").read_text()
retrofit = (root / "android_app/app/src/main/java/com/apiguard/apiary/network/RetrofitClient.kt").read_text()
installer = (root / "src/rpi_tui/install_apiary.sh").read_text()
root_installer = (root / "hive_monitor_installer.sh").read_text()
legacy_api = (root / "src/webui/legacy_api.php").read_text()

assert "sudo chmod 0750 /var/log/apiaryguard" in makefile
assert "sudo chmod 777 /var/log/apiaryguard" not in makefile
assert 'android:allowBackup="false"' in manifest
assert 'android:usesCleartextTraffic="false"' in manifest
assert 'android:networkSecurityConfig="@xml/network_security_config"' in manifest
assert "cleartextTrafficPermitted=\"false\"" in network_config
assert "cleartextTrafficPermitted=\"true\"" not in network_config
assert 'return "https://$host:$port/"' in retrofit
assert 'return "http://' not in retrofit
assert "legacy_api.php" in installer and "apiary_api_key" in installer
assert "User=apiaryguard" in installer and "Group=apiaryguard" in installer and "ProtectSystem=strict" in installer
assert "ReadWritePaths=/var/lib/apiaryguard /var/log/apiaryguard" in installer
assert "EnvironmentFile=/etc/apiaryguard/apiary.env" not in installer
assert "sudo install -d -o apiaryguard -g www-data -m 0750 /var/lib/apiaryguard" in installer
assert "sudo install -d -o www-data -g www-data -m 0700 /var/log/apiaryguard-web" in installer
assert "chmod 777" not in installer and "User=root" not in installer
assert 'chmod 0600 "$BACKUP_FILE"' in root_installer
assert 'install -d -m 0700 "$CONFIG_DIR" "$BACKUP_DIR"' in root_installer
assert "git pull --ff-only" in root_installer
assert "remote get-url origin" in root_installer
assert 'rm -rf -- "$INSTALL_DIR"' in root_installer
assert '"$HOME" == "/"' in root_installer
assert "hash_equals" in legacy_api and "HTTP_X_API_KEY" in legacy_api
assert "FILTER_VALIDATE_INT" in legacy_api and "hive_id = :hive_id" in legacy_api
assert "hive_id LIKE" not in legacy_api
print("Deployment, legacy API and TLS security defaults: OK")
