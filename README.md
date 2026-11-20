# SailGBA

A native Sailfish OS port of the [mGBA](https://mgba.io) emulator: plays Game Boy
Advance, Game Boy and Game Boy Color games with a Silica interface and on-screen
touch controls.

## Features

- **mGBA 0.10.5 core** compiled into the app (GBA + GB/GBC), with no extra libraries.
- **Touch gamepad**: one multi-touch surface for all buttons. Slide between
  buttons, roll around the 8-way D-pad, or press A and B together with one thumb.
  Haptic feedback, adjustable size and opacity. Portrait and landscape layouts.
- **Game library**: finds `.gba`, `.gbc` and `.gb` files in Documents, Downloads,
  Android storage, memory cards and any folder you add. Shows the last screen
  you played as a thumbnail. You can also open any file from the pulley menu.
- **Resume where you left off**: the game state is saved automatically when you
  quit (also on SIGTERM) and restored next time.
- **Six save state slots** with screenshots, plus normal in-game (battery) saves.
  A `.sav` file next to the ROM is adopted the first time the game starts.
- **Fast forward** (2×, 3×, 4×, 8× or unlimited).
- **Live cover** showing the game, with a pause/resume cover action.
- Game pauses automatically when the app goes to the background; the display is
  kept on while playing.
- Optional **GBA BIOS** (dumped from your own console) for the original start-up
  logo and better compatibility; mGBA's built-in replacement is used otherwise.
- Rumble cartridges vibrate the phone. Hardware keyboards work too
  (arrows, X = A, Z = B, A/S = L/R, Enter = Start, Backspace = Select, Esc = menu).
- English and Finnish.

No games or BIOS files are included.

### Known limitations

- Saves, states and thumbnails are named after the ROM file name without its
  extension (as mGBA does), so two games with the same name in different
  folders, or `Foo.gb` next to `Foo.gbc`, would share them.
- `.zip`/`.7z` archives are not opened (mGBA's archive support needs extra libraries).
- No link cable, cheats UI, rewind or tilt sensor yet.

## Building

Built with the Sailfish SDK (target `SailfishOS-5.1.0.11`), runs on SFOS 5.1+:

```sh
sfdev build phone      # aarch64 -> build-aarch64/RPMS/
sfdev build emu        # i486    -> build-i486/RPMS/
sfdev run emu -- tools/deploy.sh    # build, install and start on a device
```

Or plain sfdk: `sfdk -c target=SailfishOS-5.1.0.11-aarch64 build`.

## Layout

| Path | What |
|------|------|
| `src/emulator.*` | Runs the mGBA core on its own thread: loading, frame pacing, save states, auto-save. |
| `src/audiooutput.*` | QAudioOutput (PulseAudio) fed from a ring buffer; the emulator steers its resampling rate to keep latency low (dynamic rate control). |
| `src/gamescreen.*` | QQuickItem that draws the frame through a reused scene graph texture. |
| `src/romlibrary.*` | Game list model: background folder scan, cartridge header titles. |
| `src/settings.*` | Preferences (QSettings), BIOS import. |
| `qml/components/TouchControls.qml` | The on-screen gamepad and its hit testing. |
| `3rdparty/mgba/` | Unmodified mGBA 0.10.5 sources (only the parts needed). |
| `3rdparty/mgba.pri` | Builds the core with qmake; mirrors mGBA's CMake source list. |
| `tools/` | Device scripts: deploy, multi-touch injection (`sfinput`), unlock. |

Data lives in `~/.local/share/io.github.henrivsl/harbour-sailgba/`
(`saves/`, `states/`, `thumbs/`, `bios/`, `settings.ini`).

### Updating mGBA

Replace `3rdparty/mgba` with the new release's `include/`, `src/` (core, gba, gb,
arm, sm83, util, feature, platform/posix, third-party/inih, third-party/blip_buf),
`LICENSE`, `README.md` and `CHANGES`, then compare the source list in
`3rdparty/mgba.pri` against a CMake `LIBMGBA_ONLY` build with the optional
dependencies switched off, and bump `3rdparty/mgba-gen/version.c`.

## License

SailGBA is licensed under the Mozilla Public License 2.0, the same as mGBA.
mGBA is © Vicki Pfau and contributors; see `3rdparty/mgba/LICENSE`.
Game Boy and Game Boy Advance are trademarks of Nintendo; this project is not
affiliated with Nintendo.
