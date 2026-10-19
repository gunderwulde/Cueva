"""Publish migration metadata only after all Wwise and APK checks pass."""
from pathlib import Path
import json
import re
import shutil

root = Path(__file__).resolve().parents[1]
diagnostics = root / 'Diagnostics'
def read(name):
    return json.loads((diagnostics / name).read_text(encoding='utf-8-sig'))
def write(name, value):
    (diagnostics / name).write_text(json.dumps(value, indent=2), encoding='utf-8')

apk = read('Wwise-Quest-apk-validation.json')
assets = read('Wwise-validation.json')
restore = read('Wwise-assets-restored.json')
playback = read('Wwise-playback-smoke.json')
assert apk['signature_verified'] and apk['wwise_events'] == 63 and apk['wwise_banks'] > 0
assert assets['quest_configuration_valid'] and assets['event_sections'] == 43 and len(assets['curves']) == 3
assert playback['passed'] and all(row['ok'] for row in restore['blueprints'] + restore['maps'])
package_log = (diagnostics / 'Wwise-Quest-Package.log').read_text(encoding='utf-8-sig', errors='replace')
assert 'AutomationTool exiting with ExitCode=0 (Success)' in package_log
cook_summary = re.findall(r'Success - (\d+) error\(s\), (\d+) warning\(s\)', package_log)
assert cook_summary and int(cook_summary[-1][0]) == 0

audit = read('Wwise-restoration-audit.json')
audit.update({
    'status': 'Wwise restored and Android APK packaged/validated',
    'installed_authoring_version': '2025.1.11.9262',
    'installed_integration_version': '2025.1.11.9262.4509',
    'installed_platforms': ['Windows x64 vc170', 'Windows x64 vc180', 'Android ARM64'],
    'native_audio_backup': 'Backups/UE58_NativeAudio_1.0.2_BeforeWwise',
    'bank_generation': 'Windows and Android; English(US) and Spanish; exit 0',
    'asset_validation': 'Wwise-validation.json',
    'playback_validation': 'Wwise-playback-smoke.json',
    'apk_validation': 'Wwise-Quest-apk-validation.json',
})
write('Wwise-restoration-audit.json', audit)
status = read('migration-status.json')
status.update({
    'audio_backend': 'Wwise 2025.1.11.9262.4509',
    'ue58_cooked_native_sound_cues': 0,
    'native_audio_history': {'sound_cues': 84, 'source_wavs': 110, 'backup': audit['native_audio_backup']},
    'ue58_wwise_events': 63,
    'ue58_wwise_game_parameter': 'FADES',
    'android_build': 'Succeeded: ARM64 Wwise build, cook, package, archive and APK validation',
    'quest_revision': '1.1.0, Android versionCode 4',
    'apk': apk['apk'],
    'apk_validation': 'Wwise-Quest-apk-validation.json',
    'final_build_log': 'Wwise-Quest-Package.log',
    'wwise_asset_validation': 'Wwise-validation.json',
    'wwise_playback_smoke_passed': True,
    'wwise_languages': apk['wwise_languages'],
    'android_cook_errors': int(cook_summary[-1][0]),
    'android_cook_warnings': int(cook_summary[-1][1]),
    'wwise_packaging': 'Shared additional files in pak; bPackageAsBulkData=False',
    'additional_niagara_repair': 'NiniaTipo2_System2 skeletal sample variable and attribute trimming',
    'device_test': 'Initial native-audio APK confirmed working by user; Wwise 1.1.0 pending headset test',
})
status.pop('ue58_native_sound_cues', None)
status['native_audio_history']['attenuation_curves'] = status.pop('native_attenuation_curves', 23)
status['editor_plugin_build'] = 'Succeeded with Wwise 2025.1 and Quest support'
write('migration-status.json', status)
notes = root / 'MIGRATION.txt'
history = notes.read_text(encoding='utf-8-sig')
header = '''MEMORIA - Current Wwise revision 1.1.0 (Android versionCode 4)

Project: LaGarma_UE58_Android/LaGarma.uproject (Unreal 5.8.3)
APK: Builds/Quest3-Wwise/Android_ASTC/LaGarma-arm64.apk
Target: Meta Quest 3 / Quest 3S, Android ARM64, OpenXR, ASTC.

Wwise 2025.1.11.9262 Authoring and SDK are installed in C:/Audiokinetic.
Unreal integration 2025.1.11.9262.4509 is installed in the project's Plugins folder.
The Wwise 2019 authoring project was migrated with its object GUIDs preserved.
Windows and Android banks were generated successfully for English(US) and Spanish.
The 63 original Unreal event assets now use real AkAudioEvent objects.
All six used sequences contain real Wwise tracks: 43 event sections and 3 FADES
curves, with authored time ranges and curve keys checked after a fresh reload.
All 21 Blueprints and 7 level Blueprints compile; all 7 maps reload successfully.
Wwise playback smoke check accepted effects and voices in both languages.
The Android archive includes shared Wwise banks/media and excludes native fallback audio.
Wwise bPackageAsBulkData=False avoids copying shared banks into individual event assets.
NiniaTipo2_System2 also received the legacy skeletal-surface Niagara repair.
MSAA 4x, controller selection rays/input, floor tracking and sequence assignments remain.

The previous native-audio project is backed up at
Backups/UE58_NativeAudio_1.0.2_BeforeWwise. Earlier APKs remain in their build folders.
The MemoriaNative plugin retains OpenXR interaction and the language bridge;
its old native audio implementation is unused by the migrated maps/sequences.

Rebuild (use an explicit archive folder to preserve earlier APKs):
  powershell -ExecutionPolicy Bypass -File .\\Tools\\BuildQuest3.ps1 -ArchiveDirectory .\\Builds\\Quest3-Wwise
Requires Unreal 5.8, Android SDK 36, NDK 27.2.12479018 and JDK 21.
Regenerate SoundBanks in Wwise for Windows and Android after authoring changes.
Do not rerun the one-time native-audio conversion/bootstrap scripts on this project.

Validation: Diagnostics/Wwise-validation.json, Wwise-assets-restored.json,
Wwise-playback-smoke.json, Wwise-generate-banks-restored.log,
Wwise-Quest-Package.log and Wwise-Quest-apk-validation.json.
Physical headset audio audition and performance testing remain pending.

--- Historical native-audio migration notes (superseded by Wwise above) ---

'''
if not history.startswith('MEMORIA - Current Wwise revision'):
    notes.write_text(header + history, encoding='utf-8')
archive_dir = Path(apk['apk']).parent
shutil.copy2(notes, archive_dir / 'MIGRATION.txt')
shutil.copy2(diagnostics / 'Wwise-Quest-apk-validation.json', archive_dir / 'apk-validation.json')
print('Wwise migration metadata and archive notes updated.')
