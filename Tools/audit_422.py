from __future__ import print_function
import unreal
import json
import os

root = os.path.abspath(os.path.join(unreal.Paths.project_dir(), '..', 'Diagnostics'))
registry = unreal.AssetRegistryHelpers.get_asset_registry()
registry.search_all_assets(True)
assets = registry.get_assets_by_path('/Game', recursive=True)
inventory = []
for item in assets:
    inventory.append({'path': unicode(item.object_path), 'class': unicode(item.asset_class)})

details = []
for item in assets:
    cls = unicode(item.asset_class)
    if cls.startswith('Ak') or cls == 'LevelSequence':
        obj = item.get_asset()
        unreal.MemoriaMigrationLibrary.wait_for_niagara()
        row = {'path': unicode(item.object_path), 'class': cls}
        if obj:
            for prop in ['required_bank', 'max_attenuation_radius', 'is_infinite',
                         'minimum_duration', 'maximum_duration', 'auto_load',
                         'event', 'event_name', 'stop_at_section_end']:
                try:
                    row[prop] = unicode(obj.get_editor_property(prop))
                except Exception:
                    pass
            if cls == 'LevelSequence':
                try:
                    row['tracks'] = []
                    tracks = list(obj.get_master_tracks())
                    for binding in obj.get_bindings():
                        tracks += list(binding.get_tracks())
                    for track in tracks:
                        tr = {'class': track.get_class().get_name(),
                              'name': track.get_name(), 'sections': []}
                        for section in track.get_sections():
                            sec = {'class': section.get_class().get_name()}
                            for prop in ['event', 'event_name', 'stop_at_section_end', 'name', 'sound']:
                                try:
                                    sec[prop] = unicode(section.get_editor_property(prop))
                                except Exception:
                                    pass
                            if 'RTPC' in sec['class']:
                                sec['channels'] = []
                                for channel in section.get_channels():
                                    curve = {'keys': []}
                                    if channel.has_default():
                                        curve['default'] = channel.get_default()
                                    for key in channel.get_keys():
                                        time = key.get_time(time_unit=unreal.SequenceTimeUnit.TICK_RESOLUTION)
                                        curve['keys'].append({'frame': time.frame_number.value,
                                                              'sub_frame': time.sub_frame,
                                                              'value': key.get_value(),
                                                              'interpolation': unicode(key.get_interpolation_mode()),
                                                              'arrive_tangent': key.get_arrive_tangent(),
                                                              'leave_tangent': key.get_leave_tangent()})
                                    sec['channels'].append(curve)
                            tr['sections'].append(sec)
                        row['tracks'].append(tr)
                except Exception as exc:
                    row['inspection_error'] = unicode(exc)
        details.append(row)

with open(os.path.join(root, 'UE422-asset-audit.json'), 'w') as out:
    json.dump({'inventory': inventory, 'audio_and_sequences': details}, out, indent=2)
unreal.log('MEMORIA_AUDIT_COMPLETE: %d assets, %d audio/sequence records' % (len(inventory), len(details)))

graph_dir = os.path.join(root, 'BlueprintGraphs422')
if not os.path.isdir(graph_dir):
    os.makedirs(graph_dir)
failures = []
for item in assets:
    if unicode(item.asset_class).endswith('Blueprint'):
        obj = item.get_asset()
        unreal.MemoriaMigrationLibrary.wait_for_niagara()
        output = os.path.join(graph_dir, unicode(item.asset_name) + '.txt')
        if not unreal.MemoriaMigrationLibrary.compile_and_export_blueprint(obj, output):
            failures.append(unicode(item.object_path))
unreal.MemoriaMigrationLibrary.wait_for_niagara()
with open(os.path.join(root, 'UE422-blueprint-validation.json'), 'w') as out:
    json.dump({'failed_blueprints': failures}, out, indent=2)
if failures:
    raise RuntimeError('Blueprint validation failed: ' + repr(failures))

