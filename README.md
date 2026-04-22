<div align="center">

# A S R Y X

<br/>

<p align="center">
  <a href="https://github.com/rccyx/asryx/actions"><img src="https://img.shields.io/github/actions/workflow/status/rccyx/asryx/ci.yml?style=for-the-badge&color=black&labelColor=111111&logo=githubactions&logoColor=white" alt="CI Status"/></a>
  <a href="#installation"><img src="https://img.shields.io/badge/Platform-Linux-black?style=for-the-badge&color=black&labelColor=111111&logo=linux&logoColor=white" alt="Platform: Linux"/></a>
  <a href="#installation"><img src="https://img.shields.io/badge/Offline-No_Cloud-black?style=for-the-badge&color=black&labelColor=111111" alt="Offline"/></a>
  <a href="https://github.com/rccyx/asryx/blob/main/LICENSE"><img src="https://img.shields.io/github/license/rccyx/asryx?style=for-the-badge&color=black&labelColor=111111&logo=apache&logoColor=white" alt="License"/></a>
</p>

</div>

## Overview

<p align="center">
  <a href="https://www.youtube.com/watch?v=lVDFpoCkvh8">
    <img src="./assets/demo.gif" alt="asryx demo" width="100%">
  </a>
</p>

<details>
<summary><strong>Why?</strong></summary>

<br/>

Voice is always faster than typing. The problem is everything built around it: hold a key (pessimal), open a (bloated) app, pick a provider (decision fatigue), send audio to a server (privacy), wait for a response, hope the network holds (unreliable).

This is supposed to be instant, not a dependency chain with a mic attached.

</details>

<details>
<summary><strong>What?</strong></summary>

<br/>

Asryx is a native Linux voice-to-text toggle.

Press once to record. Talk as long as you want. Press again to stop, it transcribes locally, copies the text to your clipboard, notifies you, and cleans up.

```bash
asryx
```

Speak.

```bash
asryx
```

Paste.

That's it.

```text
record -> stop -> transcribe -> copy -> notify -> clean
```

Repeated key presses are safe. If a compositor double-fires the keybind or the key repeats, it will not spawn five recorders or corrupt the active transcription.

You might keybind it, for example, on Hyprland:

```ini
bind = ALT, W, exec, asryx
```

But most importantly:

No cloud APIs. No GUI. No Python envs. No dashboard. No provider menu. No subscriptions, no persistent key pressing, no choose from 965 models, no do these 17 steps first and maybe it works. No Node, no Python again.

</details>

<details>
<summary><strong>How?</strong></summary>

<br/>

The binary wraps `whisper.cpp` with a native Linux runtime. The model inference is `whisper-cli`.

Asryx owns everything around it: recording lifecycle, toggle state, locking, model lookup, clipboard, notifications, cleanup.

The installer builds [whisper.cpp](https://github.com/ggml-org/whisper.cpp) from source on your machine using your system compiler, you get a single native binary that runs directly on your CPU.

```text
press
  -> start local recorder
  -> write recorder pid
  -> mark state as recording

press again
  -> stop recorder
  -> run whisper-cli on the wav
  -> read transcript
  -> copy to clipboard
  -> notify
  -> clean runtime files
```

The first press starts the recorder. PipeWire through `pw-record` is preferred, falls back to ALSA through `arecord`. Audio is captured as a temporary WAV file: mono, 16 kHz, signed 16-bit.

The second press stops the recorder by signal, waits for the WAV to finalize, trims the edges, and pushes it into the clipboard.

Clipboard output:

```text
wl-copy                     # Wayland
xclip -selection clipboard  # X11 fallback
```

Notifications:

```text
notify-send
```

The binary emits the event. Mako, Dunst, or your desktop environment displays it.

Runtime state lives under:

```text
$XDG_RUNTIME_DIR/asryx
```

Falls back to `/tmp/asryx-$UID` if `$XDG_RUNTIME_DIR` is unavailable.

The directory holds only temporary payloads:

```text
lock/
rec.pid
rec.wav
rec.err
state
out.txt
```

After a successful transcription, all of it is deleted. The transcript survives only through your clipboard.

</details>

## Installation

```bash
bash <(curl -fsSL http://raw.githubusercontent.com/rccyx/asryx/refs/heads/main/bootstrap)
```

That's it.

<details>
<summary><strong>What the installer does</strong></summary>

<br/>

It runs in this order.

- validates your home directory
- installs missing build dependencies on supported distros
- builds the native binary locally
- installs `asryx` into `~/.local/bin/asryx`
- clones the pinned `whisper.cpp` source into `~/.local/opt/whisper.cpp`
- builds `whisper-cli`
- installs `whisper-cli` into `~/.local/bin/whisper-cli`
- writes the default config if missing
- installs the default model
- selects the default model
- prints a PATH note if `~/.local/bin` is not already available

The default model is:

```text
base.en
```

Models are downloaded through the official `whisper.cpp` model downloader.

The install uses:

```text
~/.local/bin/asryx
~/.local/bin/whisper-cli
~/.local/opt/whisper.cpp
~/.local/share/asryx
~/.local/share/asryx/versions/whisper-cpp-sha
~/.asryx.conf
```

</details>

After installation. You should see the current runtime state.

```text
$ asryx status
idle
```

## First use

Start recording.

```bash
asryx
```

Stop and transcribe.

```bash
asryx
```

Then paste normally.

The transcript is copied to your clipboard. The temporary audio and transcript files are cleaned after the run.

> [!TIP]
> Use a clipboard manager. Asryx gives you the transcript through the clipboard and then cleans its runtime files. A clipboard manager keeps the transcript recoverable if you accidentally copy something else after a long recording.

## Uninstallation

Comes off clean as it got in.

```bash
./scripts/uninstall
```

<details>
<summary><strong>What the uninstaller removes</strong></summary>

<br/>

It removes Asryx-owned files.

```text
~/.local/bin/asryx
~/.local/bin/whisper-cli
~/.local/opt/asryx
~/.local/opt/whisper.cpp
~/.local/share/asryx
~/.cache/asryx
~/.asryx.conf
$XDG_RUNTIME_DIR/asryx
```

It doesn't remove shared system packages.

It refuses dangerous paths such as `/`, `$HOME`, empty paths, and non-absolute paths.

</details>

## CLI

Surface area:

```
asryx
asryx status
asryx --model list
asryx --model install <MODEL>
asryx --model use <MODEL>
asryx --model uninstall <MODEL>
```

## Models

List models.

```bash
asryx --model list
```

Install a model.

```bash
asryx --model install small.en
```

Use a model.

```bash
asryx --model use small.en
```

Remove a model.

```bash
asryx --model uninstall small.en
```

Supported models:

```
tiny.en
tiny
base.en
base
small.en
small
medium.en
medium
large-v1
large-v2
large-v3
large-v3-turbo
large
```

Models ending in `.en` are English-only. All others are multilingual.

| Model              | Disk    | RAM     | Speed vs large |
| ------------------ | ------- | ------- | -------------- |
| tiny / tiny.en     | 75 MiB  | ~273 MB | ~10x           |
| base / base.en     | 142 MiB | ~388 MB | ~7x            |
| small / small.en   | 466 MiB | ~852 MB | ~4x            |
| medium / medium.en | 1.5 GiB | ~2.1 GB | ~2x            |
| large-v3-turbo     | 1.5 GiB | ~2.3 GB | ~8x            |
| large-v1 / v2 / v3 | 2.9 GiB | ~3.9 GB | 1x             |

Speed is relative to large on CPU. RAM figures are whisper.cpp runtime.

`base.en` is the default. It covers most use cases and starts fast.

> [!TIP]
> `large-v3-turbo` is the best upgrade: near-large accuracy at roughly small-level speed, for about the same disk cost as `medium`.

Installed models live in:

```bash
~/.local/share/asryx/
```

For example:

```bash
~/.local/share/asryx/ggml-base.en.bin
```

The active model config is stored in:

```bash
~/.asryx.conf
```

Default config:

```bash
model=base.en
```

## Config

Switching models through the CLI updates this file.

```bash
asryx --model use small.en
```

You can also edit it manually.

```bash
model=small.en
```

## Audio

Prefers PipeWire.

```text
pw-record
```

If unavailable, it falls back to ALSA.

```text
arecord
```

### Clipboard

Prefers Wayland clipboard tooling.

```text
wl-copy
```

If unavailable, it falls back to X11.

```text
xclip
```

### Notifications

Uses:

```text
notify-send
```

> [!IMPORTANT]
> You still need a notification daemon such as Mako, Dunst, or your desktop environment's default notification service to see popups.

## Troubleshooting

If it compiles successfully, most trouble comes from these:

- no microphone recorder available
- microphone is muted or disconnected, transcribes nothing
- PipeWire or ALSA device setup issue
- missing clipboard provider
- missing notification daemon
- `~/.local/bin` not in `PATH`

Otherwise, open an issue.

## License

Apache-2.0 © @rccyx
