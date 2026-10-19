"""Place upgraded work units under the Wwise 2025 hierarchy names, retaining GUIDs."""
from pathlib import Path
import json
import shutil
import copy
import uuid
import xml.etree.ElementTree as ET

root = Path(__file__).resolve().parents[1]
project = root / 'LaGarma_UE58_Android/LaGarma_WwiseProject'
report_file = root / 'Diagnostics/Wwise-hierarchy-relocation.json'
if report_file.exists():
    raise SystemExit('Hierarchy relocation already completed; do not overwrite upgraded work units.')
rows = []
for old, new in [('Actor-Mixer Hierarchy', 'Containers'),
                 ('Interactive Music Hierarchy', 'Containers/Music (Migrated)'),
                 ('Master-Mixer Hierarchy', 'Busses'), ('Audio Devices', 'Devices')]:
    for source in (project / old).rglob('*.wwu'):
        target = project / new / source.relative_to(project / old)
        tree = ET.parse(source)
        expected_root = new.split('/')[0]
        assert tree.getroot()[0].tag == expected_root, 'Unmigrated work unit: ' + str(source)
        if target.exists():
            preserved = root / 'Diagnostics/Wwise-migration-generated-units' / new / target.name
            preserved.parent.mkdir(parents=True, exist_ok=True)
            shutil.copy2(target, preserved)
        target.parent.mkdir(parents=True, exist_ok=True)
        shutil.copy2(source, target)
        rows.append({'source': str(source.relative_to(project)), 'target': str(target.relative_to(project)),
                     'work_unit_guid': tree.getroot().get('ID')})

# Android inherits the authored Windows channel/rate/codec settings, instead of default PCM.
for path in (project / 'Conversion Settings').rglob('*.wwu'):
    tree = ET.parse(path)
    for values in tree.iter('ValueList'):
        windows = values.find('./Value[@Platform="Windows"]')
        android = values.find('./Value[@Platform="Android"]')
        if windows is not None:
            if android is not None:
                values.remove(android)
            node = copy.deepcopy(windows)
            node.set('Platform', 'Android')
            values.append(node)
    for plugins in tree.iter('ConversionPluginInfoList'):
        windows = plugins.find('./ConversionPluginInfo[@Platform="Windows"]')
        android = plugins.find('./ConversionPluginInfo[@Platform="Android"]')
        if windows is not None:
            if android is not None:
                plugins.remove(android)
            node = copy.deepcopy(windows)
            node.set('Platform', 'Android')
            for child in node.iter():
                if child.get('ID'):
                    child.set('ID', '{' + str(uuid.uuid4()).upper() + '}')
            plugins.append(node)
    ET.indent(tree, space='\t')
    tree.write(path, encoding='utf-8', xml_declaration=True)
report_file.write_text(json.dumps(rows, indent=2), encoding='utf-8')
print('Restored upgraded hierarchy work units:', len(rows))
