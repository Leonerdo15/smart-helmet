# Smart Helmet

Recovered monorepo for a Smart Helmet prototype: an Android application, a Node.js/Express API, and ESP32/Arduino sketches. This import preserves the existing code and dependency versions; it does not claim that the old prototype is ready for deployment or that every feature works end to end.

## Repository layout

```text
smart-helmet/
├── android-app/       Android project, resources, tests, and Gradle wrapper
├── backend/           Express API, PostgreSQL models, and original public assets
├── esp32-firmware/
│   ├── detecaodepiscasbrake/   Main IMU / helmet-control sketch
│   ├── camera_server/         ESP32 camera-upload sketch
│   ├── bluethoot_send_try/    Bluetooth serial experiment
│   └── Sound over BT/         Bluetooth A2DP / I2S audio sketch and helper sources
├── README.md
├── IMPORT_NOTES.md
└── .gitignore
```

The firmware sketches are separate programs, not four tabs to combine into one sketch. Their original names and supporting files are retained.

## Architecture confirmed from the source

```mermaid
flowchart LR
    Android[Android app] -->|Login, register phone IP, post point data| API[Node.js API :8080]
    API --> DB[(PostgreSQL / PostGIS)]
    ESP[ESP32 IMU sketch] -->|GET phone IP for user 1| API
    ESP -->|POST /test sensor JSON :8080| Android
    ESP -->|Impact event webhook| IFTTT[IFTTT]
    Android -.->|Bluetooth serial experiment| BT[ESP32test]
    Camera[Camera sketch] -->|Image upload :81/upload.php| PHP[Separate PHP server - not supplied]
```

The API is used for phone discovery and persistence. The IMU sketch sends telemetry **directly to the Android phone**, rather than posting it to the Node.js API.

| Relationship | Source evidence |
| --- | --- |
| Android logs in through `GET /api/user/login/email/:email/pass/:pass` | `android-app/app/src/main/java/com/example/finalulidecap/data/LoginDataSource.java`, `backend/routes/userRouter.js` |
| Android registers its Wi-Fi IP and starts a local HTTP server on port 8080 | `android-app/app/src/main/java/com/example/finalulidecap/ui/Main/MainActivity.java` |
| Firmware retrieves the phone IP through `GET /api/user/1/ip`, then posts acceleration, gyro, temperature, and distance JSON to the phone at `/test` | `esp32-firmware/detecaodepiscasbrake/detecaodepiscasbrake.ino`, functions `getIp()` and `sendData()` |
| Android captures POST bodies and attempts to combine them with phone location/speed for `POST /api/point` | `android-app/app/src/main/java/com/example/finalulidecap/server/TinyWebServer.java`, `ui/Main/ui/slideshow/SlideshowFragment.java`, `downloaders/PostData.java` |
| API stores/query data through PostgreSQL | `backend/app.js`, `backend/routes/pointRouter.js`, `backend/models/pointModels.js`, `backend/database/connection.js` |
| Android Bluetooth serial code targets `ESP32test` | `ui/Main/ui/esp32/Esp32Fragment.java`, `esp32-firmware/bluethoot_send_try/bluethoot_send_try.ino` |

Android paths abbreviated in the last two rows are relative to `android-app/app/src/main/java/com/example/finalulidecap/`.

## Functionality present in the code

- MPU6050 acceleration and gyroscope measurements; gyroscope thresholds control left/right indicators, and acceleration controls a brake LED.
- An impact-alert attempt: `gforce > 30` activates a buzzer and calls an IFTTT `sms` event. This is rudimentary acceleration-threshold logic, **not verified fall detection**. The sketch also contains a TODO for posting impact information to the API; that upload is not implemented there.
- Ultrasonic proximity indication, a hall-sensor gate, temperature readings, and temperature-driven servo control.
- Android login, Google Maps/location route display, speed display, telemetry forwarding code, and Bluetooth serial UI.
- Backend route groups for users, routes, points, impacts, LEDs, joystick data, and IP data, mounted in `backend/app.js`.
- Separate camera image upload and Bluetooth A2DP audio/I2S output experiments. The camera's PHP receiver is not part of the supplied backend.

## Basic setup

### Backend

The original `package.json` uses Express 4, `pg`, and other pinned dependencies; the lockfile is preserved. Run commands from `backend/`:

```powershell
npm ci
$env:DATABASE_URL = 'postgresql://YOUR_DB_USER:YOUR_DB_PASSWORD@localhost:5432/esp32'
$env:PORT = '8080'
npm start
```

`bin/www` starts an HTTP server. `backend/.env.example` documents the variables; `.env` is not automatically loaded. The connection pool now reads `DATABASE_URL` instead of the original embedded password.

A compatible database is required. Models reference `users`, `routes`, `points`, `impacts`, `leo_leds`, `joystick`, and `ip_esp32`. Point queries use `st_x`/`st_y`, indicating PostGIS. **No database schema, migration, or data export was supplied**, so a working database cannot be reconstructed confidently from this import alone.

### Android

Open `android-app/` as the Android Studio project. The original configuration uses Gradle 8.0, Android Gradle Plugin 8.0.1, Kotlin 1.7.20, compile/target SDK 33, and minimum SDK 28. Use a JDK compatible with that Gradle/Android plugin combination (JDK 17 for AGP 8), and configure the SDK locally through Android Studio / ignored `local.properties`.

Replace `YOUR_GOOGLE_MAPS_API_KEY` in `app/src/main/res/values/google_maps_api.xml` with your own Maps key for local use. Do not commit that replacement. The launcher activity is `ui/login/LoginActivity.java`.

Once the toolchain is configured, the existing wrapper can be used from `android-app/`:

```powershell
.\gradlew.bat assembleDebug
```

### ESP32 firmware

For the main helmet and camera sketches, copy the adjacent `helmet_config.example.h` to `helmet_config.h` and fill in local values. These local headers are ignored by Git. The main sketch's header contains Wi-Fi, IFTTT, and alert-recipient configuration; the camera header also contains its upload host.

Use the Arduino ESP32 board support appropriate to your hardware. The IMU sketch includes `MPU6050_tockn`, `ESP32Servo`, and `ArduinoJson` (the source uses the `StaticJsonDocument` API), plus ESP32 core headers for Wi-Fi, Bluetooth, EEPROM, and ADC calibration. Exact historical library/core versions and board selections were not recorded. Sensor pins are defined in the sketch (including I2C SDA 13/SCL 14).

Keep each sketch with its companion headers/sources. The audio sketch depends on the bundled `bt_app_core.*` and `bt_app_av.*` files and ESP32 Bluetooth/I2S APIs. Its original `Sound over BT` folder does not match the `.ino` basename; if the Arduino IDE requests a matching sketch folder, create a local working copy and copy all four companion files with it. No source rename was imposed by this recovery.

### Host and user configuration

The original public host `ulideparty.ddns.net:8080` is retained; its present availability was not tested. For another backend, update it consistently in Android `LoginDataSource.java`, `MainActivity.java`, `SlideshowFragment.java`, and firmware `getIp()`. No host change is required solely because of the monorepo folders.

The phone and IMU ESP32 need network reachability between them on port 8080. Firmware currently looks up **user 1**, while Android registers the logged-in user's IP; use a matching account or explicitly adjust that firmware ID for your installation. Android point uploads currently assume **route 1**.

## Recovery limits and validation

Source-level relationships are confirmed, but execution across the components is not. Existing inconsistencies are preserved:

- Firmware telemetry has no `po_velocity`, but Android reads that key before building the point upload. Android also sends `po_tempInside` / `po_distancia`, while the backend expects `po_TempInside` / `po_DistUltraSound`.
- The point insertion model does not insert the supplied location despite accepting it as an argument.
- The IMU sketch has a one-argument `send_event` forward declaration and a two-argument definition/call. Its high impact threshold has not been calibrated or validated.
