from pathlib import Path

root = Path(__file__).resolve().parents[3]
makefile = (root / "src/rpi_tui/Makefile").read_text()
manifest = (root / "android_app/app/src/main/AndroidManifest.xml").read_text()
installer = (root / "src/rpi_tui/install_apiary.sh").read_text()

assert "sudo chmod 0750 /var/log/apiaryguard" in makefile
assert "sudo chmod 777 /var/log/apiaryguard" not in makefile
assert 'android:allowBackup="false"' in manifest
assert 'android:allowBackup="true"' not in manifest
assert "set -e" in installer
print("Deployment security defaults: OK")
