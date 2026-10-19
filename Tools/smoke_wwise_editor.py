import unreal
import json
import os

root = os.path.abspath(os.path.join(unreal.Paths.project_dir(), '..', 'Diagnostics'))
try:
    report = json.loads(unreal.MemoriaAudioEditorLibrary.smoke_test_wwise_playback())
    report['passed'] = (report.get('initialized') and report.get('english_changed') and report.get('spanish_changed')
                        and all(report.get(key, 0) > 0 for key in ['english_effect_id', 'english_voice_id', 'spanish_effect_id', 'spanish_voice_id']))
    with open(os.path.join(root, 'Wwise-playback-smoke.json'), 'w', encoding='utf-8') as output:
        json.dump(report, output, indent=2)
    unreal.log('MEMORIA_WWISE_PLAYBACK_SMOKE: ' + json.dumps(report))
    assert report['passed'], 'Wwise playback smoke test failed'
finally:
    unreal.SystemLibrary.quit_editor()
