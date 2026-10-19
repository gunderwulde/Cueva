import unreal
import json
import os

report = {'profiles': {}, 'input': {}, 'spawn': {}}
context = unreal.load_asset('/Game/NativeVR/Input/IMC_Quest')
report['input']['mappings'] = []
for mapping in context.get_editor_property('default_key_mappings').get_editor_property('mappings'):
    action = mapping.action
    report['input']['mappings'].append({'action': action.get_name(), 'key': str(mapping.key),
        'value_type': str(action.value_type), 'thresholds': [trigger.actuation_threshold for trigger in action.triggers]})
assert len(report['input']['mappings']) == 2
assert {row['action'] for row in report['input']['mappings']} == {'IA_SelectLeft', 'IA_SelectRight'}
assert all(row['thresholds'] == [0.5] for row in report['input']['mappings'])
report['input']['native_configuration_valid'] = unreal.MemoriaAudioEditorLibrary.validate_quest_configuration(unreal.load_asset('/Game/Blueprints/MotionControllerPawn'))
assert report['input']['native_configuration_valid']
world = unreal.EditorLoadingAndSavingUtils.load_map('/Game/Assets/Scenes/Start')
for actor in unreal.get_editor_subsystem(unreal.EditorActorSubsystem).get_all_level_actors():
    if not isinstance(actor, unreal.Pawn):
        continue
    report['spawn']['location'] = str(actor.get_actor_location())
    report['spawn']['auto_possess'] = str(actor.get_editor_property('auto_possess_player'))
    assert actor.get_components_by_class(unreal.MemoriaQuestInteraction)
    start = actor.get_actor_location() + unreal.Vector(0, 0, 20)
    end = start - unreal.Vector(0, 0, 600)
    trace_result = unreal.SystemLibrary.line_trace_single(world, start, end, unreal.TraceTypeQuery.TRACE_TYPE_QUERY1,
        True, [actor], unreal.DrawDebugTrace.NONE, True)
    report['spawn']['floor_trace'] = str(trace_result)
root = os.path.abspath(os.path.join(unreal.Paths.project_dir(), '..', 'Diagnostics'))
with open(os.path.join(root, 'Quest-r2-validation.json'), 'w', encoding='utf-8') as out:
    json.dump(report, out, indent=2)
unreal.log('MEMORIA_QUEST_R2_VALIDATION_COMPLETE')
