"""Import authored WAVs and rebuild legacy events using Unreal's audio engine."""
import copy
import hashlib
import json
import os
import re
import xml.etree.ElementTree as ET
import unreal

project = os.path.abspath(unreal.Paths.project_dir())
root = os.path.dirname(project.rstrip('/\\'))
diagnostics = os.path.join(root, 'Diagnostics')
with open(os.path.join(diagnostics, 'native-audio-manifest.json'), encoding='utf-8') as f:
    manifest = json.load(f)
with open(os.path.join(diagnostics, 'UE422-asset-audit.json'), encoding='utf-8') as f:
    legacy_audit = json.load(f)

def walk(node):
    yield node
    for child in node.get('children', []):
        yield from walk(child)

files = {}
for event in manifest['events']:
    for action in event['actions']:
        if 'target' not in action:
            continue
        for node in walk(action['target']):
            for source in node.get('sources', []):
                relative = source['file']
                filename = os.path.join(root, relative.replace('La_Garma_Unreal_Project', 'LaGarma_UE58_Android', 1))
                if not os.path.isfile(filename):
                    raise RuntimeError('Missing source audio: ' + filename)
                digest = hashlib.sha1(relative.encode('utf-8')).hexdigest()[:10]
                stem = re.sub('[^A-Za-z0-9_]', '_', os.path.splitext(os.path.basename(filename))[0])
                files[relative] = {'filename': filename, 'asset_name': stem + '_' + digest}

tasks = []
for source in files.values():
    asset_path = '/Game/NativeAudio/Waves/' + source['asset_name']
    source['wave_asset'] = asset_path
    if unreal.EditorAssetLibrary.does_asset_exist(asset_path):
        continue
    task = unreal.AssetImportTask()
    task.filename = source['filename']
    task.destination_path = '/Game/NativeAudio/Waves'
    task.destination_name = source['asset_name']
    task.automated = True
    task.save = True
    task.replace_existing = True
    tasks.append(task)
if tasks:
    unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks(tasks)
for source in files.values():
    if not unreal.EditorAssetLibrary.does_asset_exist(source['wave_asset']):
        raise RuntimeError('WAV import failed: ' + source['filename'])

def prepare_tree(original):
    tree = copy.deepcopy(original)
    for node in walk(tree):
        for source in node.get('sources', []):
            source['wave_asset'] = files[source['file']]['wave_asset']
        if node.get('kind') == 'MusicPlaylistContainer' and node.get('Playlist'):
            playlist = ET.fromstring(node['Playlist'][0])
            ids = [r.get('ID') for r in playlist.iter('SegmentRef')]
            children = {c['id']: c for c in node.get('children', [])}
            node['children'] = [children[key] for key in ids if key in children]
        if node.get('kind') == 'RandomSequenceContainer' and node.get('properties', {}).get('PlayMechanismStepOrContinuous') == '0':
            node['properties']['IsContinuousPlayback'] = 'True'
    return tree

cue_cache = {}
def cue_for(target, language):
    tree = prepare_tree(target)
    serialized = json.dumps(tree, ensure_ascii=False, sort_keys=True)
    digest = hashlib.sha1((serialized + language).encode('utf-8')).hexdigest()[:16]
    path = '/Game/NativeAudio/Cues/Cue_' + digest
    if path not in cue_cache:
        cue = unreal.MemoriaAudioEditorLibrary.build_native_sound_cue(path, serialized, language)
        if not cue:
            raise RuntimeError('Native sound cue generation failed: ' + target['name'])
        cue_cache[path] = cue
    return cue_cache[path]

event_paths = {row['path'].split('.')[0]: row for row in legacy_audit['inventory'] if row['class'] == 'AkAudioEvent'}
events_by_name = {event['name']: event for event in manifest['events']}
converted = []
for path in sorted(event_paths):
    asset = unreal.load_asset(path)
    event = events_by_name.get(path.rsplit('/', 1)[-1])
    if not asset or not event:
        raise RuntimeError('Missing legacy event or source definition: ' + path)
    if asset.get_class().get_name() != 'MemoriaAudioEvent':
        raise RuntimeError('Unexpected audio class: ' + asset.get_class().get_path_name())
    actions = []
    for old_action in event['actions']:
        props = old_action['properties']
        if props.get('Inclusion') == 'False':
            continue
        action_type = props.get('ActionType', '1')
        if action_type not in ('1', '2', '3'):
            raise RuntimeError('Unsupported audio action ' + action_type + ' in ' + event['name'])
        action = unreal.MemoriaAudioAction()
        action.target_id = old_action.get('target_id', '')
        action.delay = float(props.get('Delay', 0))
        action.fade_time = float(props.get('FadeTime', 0))
        action.stop = action_type in ('2', '3')
        action.global_scope = props.get('Scope') == '1' or action_type == '3'
        if not action.stop:
            target = old_action.get('target')
            if not target:
                raise RuntimeError('Audio target is missing: ' + event['name'])
            if target.get('properties', {}).get('Inclusion') == 'False':
                continue
            action.english_sound = cue_for(target, 'English(US)')
            has_spanish = any(s['language'] == 'Spanish' for n in walk(target) for s in n.get('sources', []))
            action.spanish_sound = cue_for(target, 'Spanish') if has_spanish else action.english_sound
            action.use_fades_parameter = any(r['name'] == 'FADES' for n in walk(target) for r in n.get('rtpcs', []))
            effective = {}
            for ancestor in target.get('ancestors', []):
                effective.update(ancestor.get('properties', {}))
            effective.update(target.get('properties', {}))
            action.spatialized = effective.get('3DSpatialization', '0') != '0'
        actions.append(action)
    asset.native_actions = actions
    asset.set_editor_property('required_bank', None)
    if not unreal.EditorAssetLibrary.save_loaded_asset(asset, False):
        raise RuntimeError('Could not save native event: ' + path)
    converted.append({'event': path, 'actions': len(actions)})
    unreal.log('MEMORIA_AUDIO_CONVERTED: ' + path)

restored_curves = 0
sequences = []
for row in legacy_audit['audio_and_sequences']:
    if row['class'] != 'LevelSequence':
        continue
    sequence = unreal.load_asset(row['path'])
    tracks = list(sequence.get_tracks())
    for binding in sequence.get_bindings():
        tracks.extend(binding.get_tracks())
    tracks_by_name = {track.get_name(): track for track in tracks}
    event_sections = 0
    for old_track in row.get('tracks', []):
        if old_track['class'] not in ('MovieSceneAkAudioEventTrack', 'MovieSceneAkAudioRTPCTrack'):
            continue
        track = tracks_by_name.get(old_track['name'])
        if not track:
            raise RuntimeError('Migrated audio track missing: ' + row['path'] + ' / ' + old_track['name'])
        sections = list(track.get_sections())
        if len(sections) != len(old_track['sections']):
            raise RuntimeError('Audio section count changed: ' + row['path'])
        for section, old_section in zip(sections, old_track['sections']):
            if old_track['class'] == 'MovieSceneAkAudioRTPCTrack':
                section.set_editor_property('name', old_section.get('name', 'FADES'))
                curves = old_section.get('channels', [])
                if len(curves) != 1:
                    raise RuntimeError('Expected one authored volume curve')
                if not unreal.MemoriaAudioEditorLibrary.restore_volume_curve(section, json.dumps(curves[0])):
                    raise RuntimeError('Could not restore a volume curve')
                restored_curves += 1
            else:
                event_sections += 1
                if not section.get_editor_property('event'):
                    raise RuntimeError('Event reference lost in ' + row['path'])
    if not unreal.EditorAssetLibrary.save_loaded_asset(sequence, False):
        raise RuntimeError('Could not save sequence: ' + row['path'])
    sequences.append({'sequence': row['path'], 'native_event_sections': event_sections})

with open(os.path.join(diagnostics, 'UE58-native-audio-report.json'), 'w', encoding='utf-8') as out:
    json.dump({'source_waves': len(files), 'native_cues': len(cue_cache), 'converted_events': converted,
               'restored_volume_curves': restored_curves, 'sequences': sequences}, out, indent=2)
unreal.log('MEMORIA_NATIVE_AUDIO_IMPORT_COMPLETE')


