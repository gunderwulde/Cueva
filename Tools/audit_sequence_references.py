import unreal
import json
import os

report = {}
for name in ['Start', 'Menu', 'Valley', 'Zona_I', 'Zona_IV', 'Zona_IX', 'Zona_Test']:
    world = unreal.EditorLoadingAndSavingUtils.load_map('/Game/Assets/Scenes/' + name)
    assert world
    rows = []
    for actor in unreal.get_editor_subsystem(unreal.EditorActorSubsystem).get_all_level_actors():
        if isinstance(actor, unreal.LevelSequenceActor):
            seq = actor.get_sequence()
            rows.append({'actor': actor.get_path_name(), 'label': actor.get_actor_label(), 'sequence': seq.get_path_name() if seq else None})
        if isinstance(actor, unreal.Pawn):
            try:
                current = actor.get_editor_property('currentSequence')
                rows.append({'pawn': actor.get_path_name(), 'currentSequence': current.get_path_name() if current else None})
            except Exception:
                pass
    report[name] = rows
seq = unreal.load_asset('/Game/Assets/Sequences/Menu')
report['MenuTimeline'] = {'start': seq.get_playback_start(), 'end': seq.get_playback_end(), 'tracks': []}
for track in list(seq.get_tracks()) + [t for binding in seq.get_bindings() for t in binding.get_tracks()]:
    report['MenuTimeline']['tracks'].append({'class': track.get_class().get_name(), 'sections': len(track.get_sections())})
root = os.path.abspath(os.path.join(unreal.Paths.project_dir(), '..', 'Diagnostics'))
with open(os.path.join(root, 'Sequence-references-audit.json'), 'w', encoding='utf-8') as out:
    json.dump(report, out, indent=2)
unreal.log('MEMORIA_SEQUENCE_REFERENCES_AUDIT_OK')
