# Memoria - Stories of La Garma

El proyecto actual para **Unreal Engine 5.8.3 y Meta Quest 3 / 3S** está en:

**`LaGarma_UE58_Android/LaGarma.uproject`**

El `Cueva.uproject`, `Config`, `Content` y `Source` de la raíz pertenecen a la versión antigua que ya existía en este repositorio. Se conservan junto con su historial.

## Descargar y abrir

1. Clona este repositorio con GitHub Desktop. Los assets y las fuentes de audio usan **Git LFS**. Si clonas con Git, ejecuta `git lfs install` y `git lfs pull`.
2. Instala Unreal Engine **5.8.3** y las herramientas de C++ de Visual Studio. El proyecto contiene plugins con código C++ y debe recompilarlos en el primer arranque.
3. Instala **Wwise 2025.1.11.9262** desde Audiokinetic Launcher, con Authoring, SDK Windows x64 y SDK Android ARM64. La integración de Unreal debe ser **2025.1.11.9262.4509**. Desde Launcher integra esa versión en `LaGarma_UE58_Android/LaGarma.uproject` para completar los SDK en `Plugins/WwiseSoundEngine/ThirdParty`. El código de los plugins está versionado; las bibliotecas del SDK y los binarios compilados se instalan localmente.
4. Abre `LaGarma_UE58_Android/LaGarma_WwiseProject/LaGarma_WwiseProject.wproj` en Wwise y genera SoundBanks para **Windows** y **Android**, con **English(US)** y **Spanish**. También puedes usar WwiseConsole:

   ```powershell
   & 'C:\Audiokinetic\Wwise_2025.1.11.9262\Authoring\x64\Release\bin\WwiseConsole.exe' generate-soundbank "$PWD\LaGarma_UE58_Android\LaGarma_WwiseProject\LaGarma_WwiseProject.wproj" --platform Windows Android --language 'English(US)' --language Spanish --no-source-control --save
   ```

5. Abre el `.uproject` actual y acepta recompilar los módulos si Unreal lo solicita. El mapa de entrada es `/Game/Assets/Scenes/Start`.

Los SoundBanks generados y las cachés no se guardan en Git. Las fuentes WAV originales, los Work Units de Wwise, los assets de Unreal, la configuración y el código sí están incluidos. Cada equipo mantiene sus credenciales locales de Android File Server en `Config/UserEngine.ini`, que se ignora.

## Generar el APK de Quest

Instala Android SDK 36, NDK **27.2.12479018** y JDK 21. Configura sus rutas en Unreal y ejecuta desde la raíz del repositorio:

```powershell
powershell -ExecutionPolicy Bypass -File .\Tools\BuildQuest3.ps1 -EngineRoot 'E:\Epic\UE_5.8' -ArchiveDirectory "$PWD\Builds\Quest3-Wwise"
```

Ajusta `-EngineRoot` a la instalación de Unreal de tu equipo. El APK completo queda en `Builds/Quest3-Wwise/Android_ASTC/LaGarma-arm64.apk`. Los APK y sus símbolos se mantienen fuera de Git.

## Estado de la versión 1.1.0

- Android ARM64, OpenXR, ASTC y MSAA 4x para Quest 3 / 3S.
- Selección mediante gatillo y rayo visible desde el mando; referencia de suelo Stage.
- Wwise restaurado: 63 eventos de Unreal, voces en español e inglés, 43 secciones de eventos y 3 curvas FADES en Sequencer.
- Asignaciones de secuencias reparadas en Menu y los otros cinco niveles.
- Compilación y cook de Android completados sin errores. Firma, manifiesto VR, mapas, secuencias y bancos del APK verificados.
- La prueba de esta versión en visor y el perfilado de rendimiento siguen pendientes.

Consulta `MIGRATION.txt` y los informes de `Diagnostics/` para los detalles de la migración. `Tools/` incluye scripts de reconstrucción, validación y las reparaciones realizadas. Los scripts antiguos de conversión a audio nativo son históricos: **no deben ejecutarse otra vez sobre el proyecto Wwise actual**.

Las copias de seguridad, el proyecto separado de Unreal 4.22, los archivos temporales y los instaladores se conservan localmente y no forman parte de esta subida.
