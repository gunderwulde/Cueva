"""Verify the Wwise Quest archive, including the cooked audio payload."""
from pathlib import Path
import hashlib
import json
import os
import re
import subprocess
import sys
import zipfile

root = Path(__file__).resolve().parents[1]
apk = Path(sys.argv[1])
output = Path(sys.argv[2])
container_csv = Path(sys.argv[3])
files_csv = Path(sys.argv[4])
sdk = Path(os.environ.get('ANDROID_HOME', str(Path.home() / 'AppData/Local/Android/Sdk')))
build_tools = sdk / 'build-tools/36.0.0'
java_home = Path(os.environ.get('JAVA_HOME', str(Path.home() / 'AppData/Local/Android/jdk-21')))

def command(*args):
    return subprocess.check_output([str(a) for a in args], encoding='utf-8', errors='replace', stderr=subprocess.STDOUT)

def listing(path):
    data = path.read_bytes()
    return data.decode('utf-16' if data.startswith(b'\xff\xfe') else 'utf-8-sig')

badging = command(build_tools / 'aapt2.exe', 'dump', 'badging', apk)
manifest = command(build_tools / 'aapt2.exe', 'dump', 'xmltree', apk, '--file', 'AndroidManifest.xml')
assert "name='com.overlat.memoria'" in badging
assert re.search(r"native-code:\s*'arm64-v8a'\s*$", badging, re.M)
assert 'com.oculus.intent.category.VR' in manifest
assert 'android.hardware.vr.headtracking' in manifest
assert 'com.oculus.supportedDevices' in manifest and 'quest3|quest3s' in manifest
version_code = re.search(r"versionCode='(\d+)'", badging).group(1)
version_name = re.search(r"versionName='([^']+)'", badging).group(1)
assert version_code == '4' and version_name == '1.1.0'
signature = command(java_home / 'bin/java.exe', '-jar', build_tools / 'lib/apksigner.jar', 'verify', '--verbose', apk)
assert 'Verifies' in signature

with zipfile.ZipFile(apk) as archive:
    entries = archive.infolist()
    payload = next(item for item in entries if item.filename == 'assets/main.obb.png')
    assert payload.file_size > 500_000_000, 'Cooked game content missing'
    assert 'lib/arm64-v8a/libUnreal.so' in archive.namelist()

assets = listing(container_csv)
files = listing(files_csv)
maps = ['Start', 'Menu', 'Valley', 'Zona_I', 'Zona_IV', 'Zona_IX', 'Zona_Test']
sequences = ['Menu', 'Valley', 'Zona_I', 'Zona_IV_Animation', 'Zona_IX', 'Zona_Test']
for name in maps:
    assert '/Assets/Scenes/' + name + '.umap' in assets, 'Missing map: ' + name
for name in sequences:
    assert '/Assets/Sequences/' + name + '.uasset' in assets, 'Missing sequence: ' + name
for name in ['Input/IA_SelectLeft', 'Input/IA_SelectRight', 'Input/IMC_Quest', 'M_SelectionRay']:
    assert '/NativeVR/' + name + '.uasset' in assets
audit = json.loads((root / 'Diagnostics/Wwise-restoration-audit.json').read_text(encoding='utf-8'))
events = [item['name'] for item in audit['events'] if item['unreal_asset']]
assert len(events) == 63
for item in audit['events']:
    if item['unreal_asset']:
        cooked_path = item['unreal_asset'].split('.')[0].replace('/Game/', '/Content/', 1) + '.uasset'
        assert cooked_path in assets, 'Missing Wwise asset: ' + cooked_path
for name in ['FADES', 'InitBank']:
    assert '/Audio/' + name + '.uasset' in assets, 'Missing Wwise asset: ' + name
assert '/WwiseAudio/InitBank.uasset' in assets, 'Configured InitBank asset missing'
wave_count = len(re.findall(r'/NativeAudio/Waves/[^,\r\n]+\.uasset,', assets))
cue_count = len(re.findall(r'/NativeAudio/Cues/[^,\r\n]+\.uasset,', assets))
assert wave_count == 0 and cue_count == 0, 'Native audio fallback unexpectedly cooked'
bank_paths = sorted(set(re.findall(r'[^,"\r\n]*WwiseAudio/[^,"\r\n]*\.bnk', files)))
media_paths = sorted(set(re.findall(r'[^,"\r\n]*WwiseAudio/[^,"\r\n]*\.wem', files)))
assert bank_paths and media_paths, 'Wwise bank/media payload missing'
assert any(path.endswith('/Init.bnk') for path in bank_paths)
for language in ['English(US)', 'Spanish']:
    assert any('/' + language + '/' in path for path in bank_paths), 'Missing voice banks: ' + language
    assert any('/' + language + '/' in path for path in media_paths), 'Missing voice media: ' + language
assert not any('/Windows/' in path or '/Mac/' in path for path in bank_paths + media_paths)
extracted = root / 'Diagnostics/Wwise-Quest-extracted-audio/LaGarma/Content/WwiseAudio'
generated = root / 'LaGarma_UE58_Android/LaGarma_WwiseProject/GeneratedSoundBanks/Android'
payload_hashes = []
for packaged in sorted(extracted.rglob('*')):
    if packaged.suffix.lower() not in ['.bnk', '.wem']:
        continue
    relative = packaged.relative_to(extracted)
    source = generated / relative
    assert source.is_file(), 'Packaged audio is not from the Android generation: ' + str(relative)
    expected_hash = hashlib.sha256(source.read_bytes()).hexdigest()
    assert hashlib.sha256(packaged.read_bytes()).hexdigest() == expected_hash, 'Audio payload mismatch: ' + str(relative)
    payload_hashes.append({'path': relative.as_posix(), 'sha256': expected_hash})
assert len(payload_hashes) == len(bank_paths) + len(media_paths)
(root / 'Diagnostics/Wwise-Quest-audio-hashes.json').write_text(json.dumps(payload_hashes, indent=2), encoding='utf-8')
digest = hashlib.sha256()
with apk.open('rb') as source:
    for block in iter(lambda: source.read(8 * 1024 * 1024), b''):
        digest.update(block)
report = {
    'apk': str(apk.resolve()), 'bytes': apk.stat().st_size,
    'sha256': digest.hexdigest(), 'version_code': version_code, 'version_name': version_name,
    'signature_verified': True, 'architecture': 'arm64-v8a', 'package': 'com.overlat.memoria',
    'target_headsets': ['Quest 3', 'Quest 3S'], 'vr_manifest_verified': True,
    'embedded_game_data_bytes': payload.file_size, 'cooked_maps': maps, 'cooked_sequences': sequences,
    'audio_backend': 'Wwise 2025.1.11.9262.4509', 'wwise_events': len(events),
    'wwise_banks': len(bank_paths), 'wwise_media': len(media_paths),
    'wwise_languages': ['English(US)', 'Spanish'],
    'wwise_packaged_files_match_android_generation': len(payload_hashes),
    'wwise_packaging': 'Shared additional files within pak',
    'native_waves_in_container': wave_count, 'native_cues_in_container': cue_count,
    'device_test': 'Not performed; user requested APK preparation only',
}
output.write_text(json.dumps(report, indent=2), encoding='utf-8')
print(json.dumps(report, indent=2))
