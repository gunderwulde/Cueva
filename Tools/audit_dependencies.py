"""Read-only audit of legacy package strings and Wwise work units."""
from pathlib import Path
import hashlib
import json
import re
import xml.etree.ElementTree as ET

ROOT = Path(__file__).resolve().parents[1]
PROJECT = ROOT / 'La_Garma_Unreal_Project'
packages = []
for file in sorted((PROJECT / 'Content').rglob('*')):
    if file.suffix not in ('.uasset', '.umap'):
        continue
    # Package names and import tables precede bulk mesh/texture data.
    with file.open('rb') as stream:
        data = stream.read(4 * 1024 * 1024)
    strings = {m.decode('ascii') for m in re.findall(rb'[\x20-\x7e]{5,}', data)}
    strings.update(m.decode('utf-16-le') for m in re.findall(rb'(?:[\x20-\x7e]\x00){5,}', data))
    dependencies = sorted(s for s in strings if any(k in s for k in
        ('/Script/AkAudio', '/Script/SteamVR', '/SteamVR/', 'MotionController_',
         'PostEvent', 'SetState', 'SetSwitch', 'LoadBank', 'SetRTPC', 'AkAudioEvent',
         'MovieSceneAk', 'GetTrackedDevice', 'SteamVRChaperone')) and len(s) < 500)
    if dependencies:
        packages.append({'file': str(file.relative_to(PROJECT)), 'strings': dependencies})

wwise = []
for file in sorted((PROJECT / 'LaGarma_WwiseProject').rglob('*.wwu')):
    if '.cache' in file.parts:
        continue
    tree = ET.parse(file)
    for obj in tree.iter():
        if obj.tag in ('Event', 'Action', 'Sound', 'AudioFileSource', 'RandomSequenceContainer',
                       'SwitchContainer', 'MusicTrack', 'StateGroup', 'SwitchGroup'):
            wwise.append({'file': str(file.relative_to(PROJECT)), 'kind': obj.tag,
                          'attributes': obj.attrib,
                          'xml': ET.tostring(obj, encoding='unicode') if obj.tag == 'Event' else None})

out = ROOT / 'Diagnostics' / 'static-dependencies.json'
out.write_text(json.dumps({'packages': packages, 'wwise': wwise}, indent=2), encoding='utf-8')
print('Packages with legacy dependencies:', len(packages))
print('Wwise objects:', len(wwise))
print('SteamVR packages:', [p['file'] for p in packages if any('SteamVR' in s for s in p['strings'])])
print('Audio functions:', sorted({s for p in packages for s in p['strings']
                                 if any(k in s for k in ('PostEvent', 'SetState', 'SetSwitch', 'LoadBank', 'SetRTPC'))}))
