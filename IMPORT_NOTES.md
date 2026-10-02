# Import and validation notes

Recovered on 2026-10-02 as a new snapshot repository. The original repositories and local firmware folder were not edited. Historical commits were not merged, preventing old credentials from entering the new Git history.

## Sources

| Component | Original source | Imported revision |
| --- | --- | --- |
| Android | https://github.com/Leonerdo15/Final_Ulide_Cap_App | `5c422fddb3bdfea7a9b654139f0a474d5991e1cf` |
| Backend | https://github.com/Leonerdo15/NodeJS_esp32 | `843d7ff0bb55df511d5206433823b6fdd2531848` |
| Firmware | `D:\Documentos\Arduino\helmet` | Local snapshot of all four sketch folders and their `.ino`, `.h`, and `.cpp` files |

## Deliberate exclusions

- Original `.git/` metadata/history and per-project `.gitignore` files: replaced with one clean repository and a root ignore file.
- Tracked `.idea/` files: IDE-specific state, including database datasource metadata.
- Backend `bin/ca.key`, `bin/key.pem`, `bin/ca.crt`, and `bin/cert.pem`: private keys and associated historical TLS artifacts. The supplied entry point uses HTTP and does not load these files.
- Firmware `Relatorio.docx`: a separate historical report, not source/build input. Its contents and embedded material were not inspected or cleared for public import; it remains in the original folder.

No dependency directories or generated build outputs were imported. Gradle wrapper files, Android resources/tests, package metadata/lockfile, backend public assets, and the original backend Dockerfile were retained. No Docker/CI/infrastructure was added or run.

## Security/configuration substitutions

Exactly these five original files differ from their source snapshots:

1. `backend/database/connection.js`: remove credential-bearing connection strings, including comments; read `DATABASE_URL` and report a missing variable clearly.
2. `android-app/app/src/main/res/values/google_maps_api.xml`: replace the Maps API key with `YOUR_GOOGLE_MAPS_API_KEY`.
3. `esp32-firmware/detecaodepiscasbrake/detecaodepiscasbrake.ino`: read Wi-Fi, IFTTT, and personal alert values from an ignored local `helmet_config.h`; remove credential/personal-data comments.
4. `esp32-firmware/camera_server/camera_server.ino`: read Wi-Fi and private upload-host configuration from an ignored local `helmet_config.h`.
5. `esp32-firmware/Sound over BT/Sketch_29.1_BluetoothByPCM5102A_V2.0.0_or_Later.ino`: remove a commented numeric EEPROM sample that matched an original sensitive value. Runtime logic is unchanged.

Two safe example headers and `backend/.env.example` were added. The original public backend hostname, component ports, classes, filenames, API routes, logic, dependency versions, and sensor thresholds remain unchanged. No additional project license was inferred; existing notices were preserved.

The targeted scan covered Wi-Fi configuration, Maps/IFTTT keys, credential-bearing PostgreSQL URLs, personal alert values, private host values, token/private-key patterns, and IDE/TLS artifacts. Exact recovered sensitive values were checked against the imported text files and are absent. This was a targeted publication check, not a full security audit. Original copies still contain their historical values; any still-active recovered credentials should be replaced by their owners.

## Completed checks

- Compared 125 imported original files with their sources: 120 byte-identical; exactly the five sanitization/configuration changes above differ.
- Parsed 37 Android XML files successfully.
- Checked JavaScript syntax for 20 files, including backend modules, public JavaScript, and `bin/www`, using the installed Node.js.
- Verified all relative JavaScript `require()` targets resolve within the preserved backend layout.
- Parsed `package.json` and `package-lock.json`; dependency declarations match and the original lockfile is byte-identical.
- Inspected Android Gradle settings, manifest/launcher, and wrapper configuration; project-relative structure is retained.
- Inspected firmware includes, local helper files, API discovery, sensor JSON, MPU6050 reads, impact thresholds, and Android receiver/upload references. Example headers provide the new required local configuration values.
- Checked root ignore rules for local settings, dependencies, credentials, and generated outputs while retaining wrapper files and example configuration.

## Not verified

No Java/Android SDK or Arduino CLI was available on the command path, so no Android/firmware build was attempted and no toolchain was installed. No dependencies were installed for runtime testing. Database schema/data, actual database access, old hostname availability, Maps/IFTTT service operation, camera PHP receiver, physical hardware behavior, and end-to-end communication remain unverified. Historical firmware core/library versions and board selections are unknown.

The README documents the concrete existing telemetry mismatches and firmware caveats found during inspection. The confirmed links describe what the code attempts to do; they do not certify that the historical system executes successfully.
