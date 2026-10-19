"""Translate the authored Wwise hierarchy into a reviewable native-audio plan."""
from pathlib import Path
import json
import sys
sys.stdout.reconfigure(encoding="utf-8", errors="backslashreplace")
import xml.etree.ElementTree as ET

ROOT = Path(__file__).resolve().parents[1]
PROJECT = ROOT / 'La_Garma_Unreal_Project' / 'LaGarma_WwiseProject'
nodes = {}
parents = {}
for file in PROJECT.rglob('*.wwu'):
    if '.cache' in file.parts:
        continue
    tree = ET.parse(file)
    for node in tree.iter():
        if node.tag not in ('ObjectRef', 'ActiveSource') and node.get('ID'):
            nodes[node.get('ID')] = node
        for child in node:
            parents[child] = node

def properties(node):
    result = {}
    for prop in node.findall('./PropertyList/Property'):
        value = prop.get('Value')
        if value is None:
            values = prop.findall('./ValueList/Value')
            value = values[0].text if values else None
        if value is not None:
            result[prop.get('Name')] = value
    return result

def source_file(node):
    lang = node.findtext('Language', 'SFX')
    name = node.findtext('AudioFile', '')
    category = 'SFX' if lang == 'SFX' else 'Voices/' + lang
    path = PROJECT / 'Originals' / category / name
    if not path.is_file():
        candidates = list((PROJECT / 'Originals').rglob(name)) if name else []
        candidates = [p for p in candidates if lang in p.parts] or candidates
        if candidates:
            path = candidates[0]
    return {'language': lang, 'file': str(path.relative_to(ROOT)), 'exists': path.is_file(), 'properties': properties(node)}

def hierarchy(node, seen=None):
    seen = set() if seen is None else seen
    key = node.get('ID')
    if key in seen:
        return {'reference': key}
    seen = seen | {key}
    chain = []
    p = node
    while p is not None:
        if p.tag not in ('ChildrenList', 'AudioObjects', 'InteractiveMusic', 'WwiseDocument'):
            chain.insert(0, {'kind': p.tag, 'name': p.get('Name'), 'properties': properties(p)})
        p = parents.get(p)
    result = {'kind': node.tag, 'name': node.get('Name'), 'id': key,
              'properties': properties(node), 'ancestors': chain[:-1]}
    sources = [source_file(s) for s in node.findall('./ChildrenList/AudioFileSource')]
    if sources:
        result['sources'] = sources
    result['rtpcs'] = []
    for rtpc in node.findall('./PropertyList/Property/RTPCList/RTPC'):
        ref = rtpc.find('./ReferenceList/Reference/ObjectRef')
        result['rtpcs'].append({'name': ref.get('Name') if ref is not None else '',
                                'points': [{'x': p.findtext('XPos'), 'y': p.findtext('YPos')}
                                           for p in rtpc.findall('./Curve/PointList/Point')]})
    result['music_clips'] = [ET.tostring(p, encoding='unicode') for p in node.findall('./MusicTrackSequence/ClipList/MusicClip')]
    result['plugin_sources'] = [ET.tostring(p, encoding='unicode') for p in node.findall('./ChildrenList/PluginSource')]
    children = []
    for child in node.findall('./ChildrenList/*'):
        if child.tag != 'AudioFileSource':
            children.append(hierarchy(child, seen))
    if children:
        result['children'] = children
    for tag in ('Playlist', 'PlaylistItem', 'MusicTransNode', 'MusicTrack', 'Segment', 'Track', 'MusicClip', 'Source'):
        parts = node.findall('./' + tag)
        if parts:
            result[tag] = [ET.tostring(p, encoding='unicode') for p in parts]
    return result

events = []
for node in nodes.values():
    if node.tag != 'Event':
        continue
    event = {'name': node.get('Name'), 'id': node.get('ID'), 'actions': []}
    for action in node.findall('./ChildrenList/Action'):
        row = {'properties': properties(action)}
        target = action.find('./ReferenceList/Reference[@Name="Target"]/ObjectRef')
        if target is not None:
            row['target_name'] = target.get('Name')
            row['target_id'] = target.get('ID')
            if target.get('ID') in nodes:
                row['target'] = hierarchy(nodes[target.get('ID')])
        event['actions'].append(row)
    events.append(event)

out = ROOT / 'Diagnostics' / 'native-audio-manifest.json'
out.write_text(json.dumps({'events': sorted(events, key=lambda e: e['name'])}, indent=2, ensure_ascii=False), encoding='utf-8')
for event in sorted(events, key=lambda e: e['name']):
    print(event['name'] + ': ' + ', '.join(
        a.get('target_name', '?') + ' ' + repr(a['properties']) for a in event['actions']))
print('Manifest:', out)

