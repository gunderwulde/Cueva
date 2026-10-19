"""Inspect the packaged APK without installing it on a device."""
from pathlib import Path
import hashlib
import json
import os
import re
import subprocess
import sys
import zipfile

root = Path(__file__).resolve().parents[1]
apk = Path(sys.argv[1]) if len(sys.argv) > 1 else root / 'Builds/Quest3/Android_ASTC/LaGarma-arm64.apk'
sdk = Path(os.environ.get('ANDROID_HOME', str(Path.home() / 'AppData/Local/Android/Sdk')))
build_tools = sdk / 'build-tools/36.0.0'
java_home = Path(os.environ.get('JAVA_HOME', str(Path.home() / 'AppData/Local/Android/jdk-21')))

def command(*args):
    return subprocess.check_output([str(a) for a in args], encoding='utf-8', errors='replace', stderr=subprocess.STDOUT)

badging = command(build_tools / 'aapt2.exe', 'dump', 'badging', apk)
manifest = command(build_tools / 'aapt2.exe', 'dump', 'xmltree', apk, '--file', 'AndroidManifest.xml')
assert "name='com.overlat.memoria'" in badging
assert re.search(r"native-code:\s*'arm64-v8a'\s*$", badging, re.M)
assert 'com.oculus.intent.category.VR' in manifest
assert 'android.hardware.vr.headtracking' in manifest
assert 'com.oculus.supportedDevices' in manifest and 'quest3|quest3s' in manifest
signature = command(java_home / 'bin/java.exe', '-jar', build_tools / 'lib/apksigner.jar', 'verify', '--verbose', apk)
assert 'Verifies' in signature

with zipfile.ZipFile(apk) as archive:
    entries = archive.infolist()
    payload = next(item for item in entries if item.filename == 'assets/main.obb.png')
    assert payload.file_size > 500_000_000, 'Cooked game content missing'
    libraries = [item.filename for item in entries if item.filename.startswith('lib/')]
    assert 'lib/arm64-v8a/libUnreal.so' in libraries
    assert not any('wwise' in name.lower() or 'AkSoundEngine' in name for name in libraries)

listing_path = Path(sys.argv[3]) if len(sys.argv) > 3 else root / 'Diagnostics/UE58-Quest3-container.csv'
listing_bytes = listing_path.read_bytes()
listing = listing_bytes.decode('utf-16' if listing_bytes.startswith(b'\xff\xfe') else 'utf-8-sig')
maps = ['Start', 'Menu', 'Valley', 'Zona_I', 'Zona_IV', 'Zona_IX', 'Zona_Test']
for name in maps:
    assert '/Assets/Scenes/' + name + '.umap' in listing, 'Missing map: ' + name
sequences = ['Menu', 'Valley', 'Zona_I', 'Zona_IV_Animation', 'Zona_IX', 'Zona_Test']
for name in sequences:
    assert '/Assets/Sequences/' + name + '.uasset' in listing, 'Missing sequence: ' + name
wave_count = len(re.findall(r'/NativeAudio/Waves/[^,\r\n]+\.uasset,', listing))
cue_count = len(re.findall(r'/NativeAudio/Cues/[^,\r\n]+\.uasset,', listing))
assert wave_count == 110 and cue_count == 84
version_code = re.search(r"versionCode='(\d+)'", badging).group(1)
if int(version_code) >= 2:
    for asset in ['Input/IA_SelectLeft', 'Input/IA_SelectRight', 'Input/IMC_Quest', 'M_SelectionRay']:
        assert '/NativeVR/' + asset + '.uasset' in listing
digest = hashlib.sha256()
with apk.open('rb') as source:
    for block in iter(lambda: source.read(8 * 1024 * 1024), b''):
        digest.update(block)
report = {
    'apk': str(apk.resolve()), 'bytes': apk.stat().st_size,
    'sha256': digest.hexdigest(), 'version_code': version_code, 'signature_verified': True,
    'architecture': 'arm64-v8a', 'package': 'com.overlat.memoria',
    'headsets': ['Quest 3', 'Quest 3S'], 'vr_manifest_verified': True,
    'embedded_game_data_bytes': payload.file_size, 'cooked_maps': maps,
    'cooked_sequences': sequences,
    'native_waves_in_container': wave_count, 'native_cues_in_container': cue_count,
    'device_test': 'Not performed; user requested APK preparation only',
}
(Path(sys.argv[2]) if len(sys.argv) > 2 else root / 'Diagnostics/UE58-Quest3-apk-validation.json').write_text(json.dumps(report, indent=2), encoding='utf-8')
print(json.dumps(report, indent=2))
