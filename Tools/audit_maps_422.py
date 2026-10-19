from __future__ import print_function
import unreal
import json
import os

root = os.path.abspath(os.path.join(unreal.Paths.project_dir(), '..', 'Diagnostics'))
graph_dir = os.path.join(root, 'BlueprintGraphs422')
registry = unreal.AssetRegistryHelpers.get_asset_registry()
registry.search_all_assets(True)
assets = registry.get_assets_by_path('/Game/Assets/Scenes', recursive=True)
report = []
for item in assets:
    if unicode(item.asset_class) != 'World':
        continue
    world = unreal.EditorLoadingAndSavingUtils.load_map(unicode(item.package_name))
    unreal.MemoriaMigrationLibrary.wait_for_niagara()
    if not world:
        raise RuntimeError('Map failed to load: ' + unicode(item.package_name))
    bp = unreal.MemoriaMigrationLibrary.get_level_blueprint(world)
    ok = True
    if bp:
        ok = unreal.MemoriaMigrationLibrary.compile_and_export_blueprint(
            bp, os.path.join(graph_dir, 'Level_' + unicode(item.asset_name) + '.txt'))
    row = {'map': unicode(item.package_name), 'blueprint_ok': ok, 'audio_actors': []}
    for actor in unreal.EditorLevelLibrary.get_all_level_actors():
        components = actor.get_components_by_class(unreal.ActorComponent)
        for component in components:
            cls = unicode(component.get_class().get_name())
            if cls.startswith('Ak'):
                audio = {'actor': unicode(actor.get_name()), 'class': unicode(actor.get_class().get_name()),
                         'component': unicode(component.get_name()), 'component_class': cls}
                for prop in ['ak_audio_event', 'event_name', 'attenuation_scaling_factor',
                             'stop_when_owner_destroyed', 'auto_post']:
                    try:
                        audio[prop] = unicode(component.get_editor_property(prop))
                    except Exception:
                        try:
                            audio[prop] = unicode(actor.get_editor_property(prop))
                        except Exception:
                            pass
                row['audio_actors'].append(audio)
    report.append(row)
    unreal.log('MEMORIA_MAP %s blueprint_ok=%s audio_components=%d' %
               (item.package_name, ok, len(row['audio_actors'])))
    unreal.MemoriaMigrationLibrary.wait_for_niagara()
with open(os.path.join(root, 'UE422-map-audit.json'), 'w') as out:
    json.dump(report, out, indent=2)
if any(not row['blueprint_ok'] for row in report):
    raise RuntimeError('Map Blueprint compilation failed')
