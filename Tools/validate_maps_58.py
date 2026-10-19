import unreal
import json
import os

root = os.path.abspath(os.path.join(unreal.Paths.project_dir(), '..', 'Diagnostics'))
registry = unreal.AssetRegistryHelpers.get_asset_registry()
registry.search_all_assets(True)
report = []
for item in registry.get_assets_by_path('/Game/Assets/Scenes', recursive=True):
    if str(item.asset_class_path.asset_name) != 'World':
        continue
    package = str(item.package_name)
    if os.environ.get('MEMORIA_MAP_FILTER') and not package.endswith('/' + os.environ['MEMORIA_MAP_FILTER']):
        continue
    world = unreal.EditorLoadingAndSavingUtils.load_map(package)
    if not world:
        raise RuntimeError('Failed to load map: ' + package)
    bp = unreal.MemoriaAudioEditorLibrary.get_level_blueprint(world)
    ok = not bp or unreal.MemoriaAudioEditorLibrary.compile_native_blueprint(bp)
    audio = []
    for actor in unreal.get_editor_subsystem(unreal.EditorActorSubsystem).get_all_level_actors():
        for component in actor.get_components_by_class(unreal.SceneComponent):
            if isinstance(component, unreal.MemoriaAudioComponent):
                event = component.ak_audio_event
                audio.append({'actor': actor.get_name(), 'component': component.get_name(),
                              'event': event.get_path_name() if event else None,
                              'native_actions': len(event.native_actions) if event else 0})
    if ok:
        ok = unreal.EditorLoadingAndSavingUtils.save_map(world, package)
    report.append({'map': package, 'ok': bool(ok), 'audio_components': audio})
    unreal.log('MEMORIA_NATIVE_MAP: %s ok=%s audio_components=%d' % (package, ok, len(audio)))
if os.environ.get('MEMORIA_MAP_FILTER'):
    with open(os.path.join(root, 'UE58-map-validation.json'), encoding='utf-8-sig') as prior:
        merged = {row['map']: row for row in json.load(prior)}
    merged.update({row['map']: row for row in report})
    report = list(merged.values())
with open(os.path.join(root, 'UE58-map-validation.json'), 'w', encoding='utf-8') as out:
    json.dump(report, out, indent=2)
if any(not row['ok'] for row in report):
    raise RuntimeError('One or more maps failed validation')
unreal.log('MEMORIA_ALL_NATIVE_MAPS_OK: ' + str(len(report)))

