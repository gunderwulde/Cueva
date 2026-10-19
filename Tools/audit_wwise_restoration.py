"""Inventory preserved Wwise sources before switching the working audio backend."""
from pathlib import Path
import json
import xml.etree.ElementTree as ET

root = Path(__file__).resolve().parents[1]
project = root / 'LaGarma_UE58_Android'
wwise = project / 'LaGarma_WwiseProject'
document = ET.parse(wwise / 'LaGarma_WwiseProject.wproj').getroot()
manifest = json.loads((root / 'Diagnostics/native-audio-manifest.json').read_text(encoding='utf-8-sig'))
inventory = json.loads((root / 'Diagnostics/UE422-asset-audit.json').read_text(encoding='utf-8-sig'))['inventory']
assets = {row['path'].split('.')[-1]: row['path'] for row in inventory if row['class'] == 'AkAudioEvent'}
events = [{'name': e['name'], 'wwise_guid': e['id'], 'unreal_asset': assets.get(e['name'])}
          for e in manifest['events']]

def walk(node):
    yield node
    for child in node.get('children', []):
        yield from walk(child)

sources = set()
for event in manifest['events']:
    for action in event['actions']:
        if 'target' in action:
            for node in walk(action['target']):
                for source in node.get('sources', []):
                    sources.add(source['file'].replace('La_Garma_Unreal_Project', 'LaGarma_UE58_Android', 1))
missing = [p for p in sorted(sources) if not (root / p).is_file()]
legacy_plugin = root / 'La_Garma_Unreal_Project/Plugins/Wwise'
old_plugin = json.loads((legacy_plugin / 'Wwise.uplugin').read_text(encoding='utf-8-sig'))
report = {
    'status': 'Awaiting Wwise Authoring, SDK Windows x64, SDK Android ARM64 and matching UE 5.8 integration',
    'original_wwise_version': document.attrib['WwiseVersion'],
    'original_wwise_build': document.attrib['WwiseBuild'],
    'preserved_project': str(wwise / 'LaGarma_WwiseProject.wproj'),
    'original_platforms': [p.attrib['Name'] for p in document.findall('.//Platforms/Platform')],
    'existing_plugin': {'version': old_plugin['VersionName'], 'engine': old_plugin['EngineVersion'],
                        'platform_libraries': [p.name for p in (legacy_plugin / 'ThirdParty').iterdir() if p.is_dir()]},
    'event_count': len(events), 'events_with_unreal_asset': sum(bool(e['unreal_asset']) for e in events), 'events': events,
    'referenced_wav_sources': len(sources), 'missing_wav_sources': missing,
    'generated_android_banks': [str(p.relative_to(project)) for p in wwise.glob('GeneratedSoundBanks/Android/**/*.bnk')],
    'target_wwise_version': '2025.1.11 (UE 5.8 integration announced by Audiokinetic)',
    'restoration_work': [
        'Upgrade a copy of the preserved Wwise project and add the Android platform.',
        'Install matching Unreal integration and Windows/Android sound engine libraries.',
        'Replace native audio event/component/Sequencer references and conflicting AkAudio redirects.',
        'Restore language, RTPC, callback, attenuation and mixer routing through Wwise.',
        'Generate Android SoundBanks, compile, validate Blueprints and package a new APK.'
    ],
    'working_project_audio_backend_changed': False,
    'current_working_apk': str(root / 'Builds/Quest3-r3/Android_ASTC/LaGarma-arm64.apk'),
    'sources': [
        'https://www.linkedin.com/posts/audiokinetic_wwise-unity-unreal-activity-7506031040885354496-pbvp',
        'https://www.audiokinetic.com/en/library/Launcher_2023.2.4.3909/?id=getting_started&source=InstallGuide'
    ]
}
assert not missing, 'Missing original WAV sources'
output = root / 'Diagnostics/Wwise-restoration-audit.json'
output.write_text(json.dumps(report, indent=2, ensure_ascii=False), encoding='utf-8')
print(json.dumps({k: report[k] for k in ['event_count', 'events_with_unreal_asset', 'referenced_wav_sources', 'missing_wav_sources', 'original_platforms', 'generated_android_banks', 'status']}, indent=2))
