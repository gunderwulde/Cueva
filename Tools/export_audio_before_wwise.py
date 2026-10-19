"""Record the current audio references and Quest input before restoring Wwise."""
import unreal
import json
import os

report = {'sequences': [], 'maps': [], 'quest_input_valid': False}
registry = unreal.AssetRegistryHelpers.get_asset_registry()
registry.search_all_assets(True)
for data in registry.get_assets_by_path('/Game/Assets/Sequences', recursive=True):
    if str(data.asset_class_path.asset_name) != 'LevelSequence':
        continue
    sequence = data.get_asset()
    tracks = [(None, t) for t in sequence.get_tracks()]
    for binding in sequence.get_bindings():
        tracks.extend((str(binding.get_id()), t) for t in binding.get_tracks())
    rows = []
    for binding, track in tracks:
        if not track.get_class().get_name().startswith('MemoriaAudio'):
            continue
        sections = []
        for section in track.get_sections():
            row = {'name': section.get_name(), 'class': section.get_class().get_name(),
                   'start': section.get_start_frame() if section.has_start_frame() else None,
                   'end': section.get_end_frame() if section.has_end_frame() else None}
            if isinstance(section, unreal.MemoriaAudioEventSection):
                event = section.get_editor_property('event')
                row.update({'event': event.get_path_name() if event else None,
                            'event_name': section.get_editor_property('event_name'),
                            'stop_at_section_end': section.get_editor_property('stop_at_section_end'),
                            'retrigger_event': section.get_editor_property('retrigger_event')})
            sections.append(row)
        rows.append({'name': track.get_name(), 'class': track.get_class().get_name(), 'binding': binding, 'sections': sections})
    report['sequences'].append({'asset': sequence.get_path_name(), 'tracks': rows})
for name in ['Start', 'Menu', 'Valley', 'Zona_I', 'Zona_IV', 'Zona_IX', 'Zona_Test']:
    world = unreal.EditorLoadingAndSavingUtils.load_map('/Game/Assets/Scenes/' + name)
    assert world
    rows = []
    for actor in unreal.get_editor_subsystem(unreal.EditorActorSubsystem).get_all_level_actors():
        for component in actor.get_components_by_class(unreal.MemoriaAudioComponent):
            event = component.get_editor_property('ak_audio_event')
            rows.append({'actor': actor.get_path_name(), 'component': component.get_name(),
                         'event': event.get_path_name() if event else None,
                         'event_name': component.get_editor_property('event_name'),
                         'attenuation_scaling_factor': component.get_editor_property('attenuation_scaling_factor')})
    report['maps'].append({'map': name, 'audio_components': rows})
report['quest_input_valid'] = unreal.MemoriaAudioEditorLibrary.validate_quest_configuration(unreal.load_asset('/Game/Blueprints/MotionControllerPawn'))
assert report['quest_input_valid'], 'Quest configuration changed before Wwise restoration'
root = os.path.abspath(os.path.join(unreal.Paths.project_dir(), '..', 'Diagnostics'))
with open(os.path.join(root, 'Wwise-audio-before.json'), 'w', encoding='utf-8') as out:
    json.dump(report, out, indent=2)
unreal.log('MEMORIA_AUDIO_BEFORE_WWISE_EXPORTED')
