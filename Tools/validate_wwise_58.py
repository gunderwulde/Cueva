import unreal
import json
import os

root = os.path.abspath(os.path.join(unreal.Paths.project_dir(), '..', 'Diagnostics'))
with open(os.path.join(root, 'Wwise-restoration-audit.json'), encoding='utf-8') as source:
    audit = json.load(source)
with open(os.path.join(root, 'UE422-asset-audit.json'), encoding='utf-8-sig') as source:
    legacy = json.load(source)
with open(os.path.join(root, 'Wwise-audio-before.json'), encoding='utf-8') as source:
    before = json.load(source)
original_sequences = {r['path']: r for r in legacy['audio_and_sequences'] if r.get('class') == 'LevelSequence'}
report = {'wwise_version': '2025.1.11.9262.4509', 'events': [], 'curves': [], 'maps': []}
for item in audit['events']:
    if not item['unreal_asset']:
        continue
    event = unreal.load_asset(item['unreal_asset'])
    assert event and event.get_class().get_name() == 'AkAudioEvent'
    info = event.get_editor_property('event_info')
    assert str(info.get_editor_property('WwiseName')) == item['name'] and info.get_editor_property('WwiseShortId') != 0
    report['events'].append({'event': event.get_path_name(), 'short_id': info.get_editor_property('WwiseShortId')})
event_sections = 0
for previous in before['sequences']:
    sequence = unreal.load_asset(previous['asset'])
    tracks = list(sequence.get_tracks())
    for binding in sequence.get_bindings():
        tracks.extend(binding.get_tracks())
    by_name = {t.get_name(): t for t in tracks}
    original_tracks = {t['name']: t for t in original_sequences[sequence.get_path_name()]['tracks']}
    for old_track in previous['tracks']:
        track = by_name[old_track['name']]
        sections = list(track.get_sections())
        assert len(sections) == len(old_track['sections'])
        for index, (section, old) in enumerate(zip(sections, old_track['sections'])):
            assert (section.get_start_frame() if section.has_start_frame() else None) == old['start']
            assert (section.get_end_frame() if section.has_end_frame() else None) == old['end']
            if old['class'] == 'MemoriaAudioEventSection':
                assert section.get_class().get_name() == 'MovieSceneAkAudioEventSection'
                event_sections += 1
            else:
                assert section.get_class().get_name() == 'MovieSceneWwiseGameParameterSection'
                expected = len(original_tracks[old_track['name']]['sections'][index]['channels'][0]['keys'])
                actual = unreal.MemoriaAudioEditorLibrary.get_wwise_curve_key_count(section)
                assert actual == expected and actual > 0, 'Volume automation lost: ' + sequence.get_name()
                report['curves'].append({'sequence': sequence.get_name(), 'keys': actual})
report['event_sections'] = event_sections
assert event_sections == 43 and len(report['curves']) == 3
report['quest_configuration_valid'] = unreal.MemoriaAudioEditorLibrary.validate_quest_configuration(unreal.load_asset('/Game/Blueprints/MotionControllerPawn'))
assert report['quest_configuration_valid']
for name in ['Start', 'Menu', 'Valley', 'Zona_I', 'Zona_IV', 'Zona_IX', 'Zona_Test']:
    world = unreal.EditorLoadingAndSavingUtils.load_map('/Game/Assets/Scenes/' + name)
    assert world
    actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem).get_all_level_actors()
    native = [c.get_path_name() for a in actors for c in a.get_components_by_class(unreal.MemoriaAudioComponent)]
    assert not native, 'Native emitters remain in ' + name
    for actor in actors:
        if isinstance(actor, unreal.LevelSequenceActor):
            assert actor.get_sequence(), 'Unassigned sequencer: ' + actor.get_actor_label()
    report['maps'].append({'map': name, 'wwise_components': sum(len(a.get_components_by_class(unreal.AkComponent)) for a in actors)})
assert len(report['events']) == 63
with open(os.path.join(root, 'Wwise-validation.json'), 'w', encoding='utf-8') as output:
    json.dump(report, output, indent=2)
unreal.log('MEMORIA_WWISE_VALIDATION_OK')
