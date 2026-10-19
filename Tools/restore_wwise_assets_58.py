"""Reconnect the migrated project to real Wwise events, components and Sequencer tracks."""
import unreal
import json
import os
import xml.etree.ElementTree as ET

root = os.path.abspath(os.path.join(unreal.Paths.project_dir(), '..'))
with open(os.path.join(root, 'Diagnostics/Wwise-restoration-audit.json'), encoding='utf-8') as source:
    audit = json.load(source)
with open(os.path.join(root, 'Diagnostics/UE422-asset-audit.json'), encoding='utf-8-sig') as source:
    legacy = json.load(source)
with open(os.path.join(root, 'Diagnostics/Wwise-audio-before.json'), encoding='utf-8') as source:
    before = json.load(source)
report = {'events': [], 'sequences': [], 'blueprints': [], 'maps': []}

init = unreal.MemoriaAudioEditorLibrary.create_wwise_asset('/Game/Audio/InitBank', '/Script/AkAudio.AkInitBank')
assert init and unreal.MemoriaAudioEditorLibrary.initialize_wwise_asset(init, '', 'Init')
assert unreal.EditorAssetLibrary.save_loaded_asset(init, False)
for event in audit['events']:
    if not event['unreal_asset']:
        continue
    asset = unreal.load_asset(event['unreal_asset'])
    assert asset and asset.get_class().get_name() == 'AkAudioEvent', 'Event class not restored: ' + event['name']
    assert unreal.MemoriaAudioEditorLibrary.initialize_wwise_asset(asset, event['wwise_guid'], event['name']), 'Event not in banks: ' + event['name']
    assert unreal.EditorAssetLibrary.save_loaded_asset(asset, False)
    report['events'].append(asset.get_path_name())

parameter_file = os.path.join(unreal.Paths.project_dir(), 'LaGarma_WwiseProject/Game Parameters/Default Work Unit.wwu')
parameter = next(p for p in ET.parse(parameter_file).iter('GameParameter') if p.get('Name') == 'FADES')
fades = unreal.MemoriaAudioEditorLibrary.create_wwise_asset('/Game/Audio/FADES', '/Script/AkAudio.AkRtpc')
assert fades and unreal.MemoriaAudioEditorLibrary.initialize_wwise_asset(fades, parameter.get('ID'), 'FADES')
assert unreal.EditorAssetLibrary.save_loaded_asset(fades, False)

old_sequences = {r['path']: r for r in legacy['audio_and_sequences'] if r.get('class') == 'LevelSequence'}
for previous in before['sequences']:
    sequence = unreal.load_asset(previous['asset'])
    tracks = [(None, t) for t in sequence.get_tracks()]
    for binding in sequence.get_bindings():
        tracks.extend((binding, t) for t in binding.get_tracks())
    by_name = {t.get_name(): (b, t) for b, t in tracks}
    old_row = old_sequences.get(sequence.get_path_name())
    assert old_row, 'Legacy timeline audit missing: ' + sequence.get_path_name()
    originals = {t['name']: t for t in old_row.get('tracks', [])}
    row = {'asset': sequence.get_path_name(), 'event_sections': 0, 'volume_sections': 0}
    for old_track in previous['tracks']:
        binding, track = by_name[old_track['name']]
        assert unreal.MemoriaAudioEditorLibrary.repair_wwise_track(track, binding is None)
        sections = list(track.get_sections())
        assert len(sections) == len(old_track['sections'])
        for index, (section, old) in enumerate(zip(sections, old_track['sections'])):
            if old['class'] == 'MemoriaAudioEventSection':
                assert section.get_class().get_name() == 'MovieSceneAkAudioEventSection'
                event = unreal.load_asset(old['event'])
                assert unreal.MemoriaAudioEditorLibrary.set_wwise_audio_section(section, event, old['stop_at_section_end'], old['retrigger_event'])
                row['event_sections'] += 1
            else:
                assert section.get_class().get_name() == 'MovieSceneWwiseGameParameterSection'
                assert unreal.MemoriaAudioEditorLibrary.set_wwise_audio_section(section, fades, False, False)
                curve = originals[old_track['name']]['sections'][index]['channels'][0]
                assert unreal.MemoriaAudioEditorLibrary.restore_volume_curve(section, json.dumps(curve))
                row['volume_sections'] += 1
            # Wwise SetEvent matches its duration; retain the authored timeline range.
            if old['start'] is not None:
                section.set_start_frame(old['start'])
            else:
                section.set_start_frame_bounded(False)
            if old['end'] is not None:
                section.set_end_frame(old['end'])
            else:
                section.set_end_frame_bounded(False)
            assert (section.get_start_frame() if section.has_start_frame() else None) == old['start']
            assert (section.get_end_frame() if section.has_end_frame() else None) == old['end']
    assert unreal.EditorAssetLibrary.save_loaded_asset(sequence, False)
    report['sequences'].append(row)

for item in legacy['inventory']:
    if item['class'] != 'Blueprint':
        continue
    bp = unreal.load_asset(item['path'])
    ok = bool(bp and unreal.MemoriaAudioEditorLibrary.compile_native_blueprint(bp))
    if ok:
        ok = bool(unreal.EditorAssetLibrary.save_loaded_asset(bp, False))
    report['blueprints'].append({'asset': item['path'], 'ok': ok})
for name in ['Start', 'Menu', 'Valley', 'Zona_I', 'Zona_IV', 'Zona_IX', 'Zona_Test']:
    world = unreal.EditorLoadingAndSavingUtils.load_map('/Game/Assets/Scenes/' + name)
    assert world
    unreal.SystemLibrary.execute_console_command(world, 'save.FixupStandaloneFlags 1')
    bp = unreal.MemoriaAudioEditorLibrary.get_level_blueprint(world)
    ok = not bp or unreal.MemoriaAudioEditorLibrary.compile_native_blueprint(bp)
    native_components = []
    wwise_components = 0
    for actor in unreal.get_editor_subsystem(unreal.EditorActorSubsystem).get_all_level_actors():
        wwise_components += len(actor.get_components_by_class(unreal.AkComponent))
        native_components += [c.get_path_name() for c in actor.get_components_by_class(unreal.MemoriaAudioComponent)]
    if native_components:
        ok = False
    if ok:
        ok = bool(unreal.EditorLoadingAndSavingUtils.save_map(world, '/Game/Assets/Scenes/' + name))
    report['maps'].append({'map': name, 'ok': bool(ok), 'wwise_components': wwise_components,
                           'remaining_native_audio_components': native_components})
path = os.path.join(root, 'Diagnostics/Wwise-assets-restored.json')
with open(path, 'w', encoding='utf-8') as output:
    json.dump(report, output, indent=2)
assert len(report['events']) == 63
assert sum(s['event_sections'] for s in report['sequences']) == 43
assert sum(s['volume_sections'] for s in report['sequences']) == 3
assert all(row['ok'] for row in report['blueprints'] + report['maps']), 'Blueprint/map errors; see restoration report'
unreal.log('MEMORIA_WWISE_ASSETS_RESTORED')
