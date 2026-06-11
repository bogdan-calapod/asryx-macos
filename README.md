<div align="center">

# asryx

<p align="center">
  <a href="https://github.com/bogdan-calapod/asryx-macos/actions/workflows/ci.yml">
    <img src="https://img.shields.io/github/actions/workflow/status/bogdan-calapod/asryx-macos/ci.yml?branch=main&style=for-the-badge&color=black&labelColor=111111&logo=githubactions&logoColor=white" alt="CI Status"/>
  </a>
  <a href="#installation">
    <img src="https://img.shields.io/badge/Platform-Linux-black?logo=linux&logoColor=white&style=for-the-badge&labelColor=111111" alt="Platform: Linux"/>
  </a>
  <a href="#macos">
    <img src="https://img.shields.io/badge/Platform-macOS-black?logo=apple&logoColor=white&style=for-the-badge&labelColor=111111" alt="Platform: macOS"/>
  </a>
  <a href="#mechanism">
    <img src="https://img.shields.io/badge/Offline-100%25-black?logo=shield&logoColor=white&style=for-the-badge&labelColor=111111" alt="Offline"/>
  </a>
  <a href="https://github.com/bogdan-calapod/asryx-macos/blob/main/LICENSE">
    <img src="https://img.shields.io/badge/License-Apache--2.0-black?logo=apache&logoColor=white&style=for-the-badge&labelColor=111111" alt="License: Apache-2.0"/>
  </a>
</p>

</div>

<p align="center">
  <a href="./assets/demo.gif">
    <img src="./assets/demo.gif" alt="asryx demo" width="100%">
  </a>
</p>

## Overview

This is a native C++ ASR binary toggle/CLI for Linux. macOS is also supported as a secondary target ([see below](#macos)).

Runs local transcription against [GGML Whisper](https://github.com/ggml-org/whisper.cpp) models through a [pinned](./versions/) native inference backend, linked in process through its public C compatible API.

The model supplies inference and all 99 language support options, while `asryx` owns the entire native Linux runtime around it.

Records audio through the active Linux audio stack, runs recognition in process, writes the transcript to the active clipboard backend, optionally pipes it to a user command, emits desktop notifications, and removes runtime artifacts after completion.

Easily [installed](#installation), and more easily [removed](#uninstallation).

Doesn't stay in memory between uses. 0MB idle RAM. Doesn't load the model unless invoked.

Boots instantly and exits instantly.

One command install. You compile the program on your own machine, no package manager or third parties.

It uses standard C++ and Linux [dependencies](#dependencies), and it's CPU only by default, so it works with any machine, regardless of distro or GPU model.

GGML Models are [downloaded locally](#uninstallation) and transcription runs against those weights on your machine.

[Reliable](#mechanism) is the word. Repeated invocations, key repeat, stale locks, interrupted sessions, and abandoned runtime artifacts are handled before a new recording begins.

There is no ASR server, background daemon(s), hosted API, Python runtime, Node runtime, container layer, resident daemon, GUI process, dashboard, subscription, or network dependency during transcription.

## Usage

The program is a toggle

```bash
asryx
```

The first invocation starts capture.

You talk for as long as you want.

```bash
asryx
```

The next invocation stops capture, transcribes locally, copies the transcript, notifies the session, and cleans the runtime directory.

> [!TIP]
> This toggle can be hooked to Sway, Hyprland, i3, GNOME, etc.

And a very simple [CLI](#cli)

```
asryx                         # Toggle record/transcribe
asryx status                  # Check idle/recording/transcribing
asryx --pipe-to '<COMMAND>'   # Set post copy pipe command
asryx --no-pipe               # Clear post copy pipe command
asryx --language <auto|CODE>  # Set language
asryx --model list            # List supported models
asryx --model install <MODEL> # Download model
asryx --model use <MODEL>     # Switch model
```

## Mechanism

My biggest problem with this ecosystem is relying on black box tools without knowing what they do to my system. So, here's exactly how this tool works:

Audio capture orchestration, runtime state, model management, clipboard delivery, notification dispatch, cleanup, and failure recovery are all owned by the binary.

The audio path is native code. Captured audio is validated as RIFF/WAVE, walked chunk by chunk, checked for 16 kHz mono signed 16-bit PCM, and decoded from PCM16 into float samples before inference.

Dependency free as in, not even a WAV library sits between the recorder output and the model input.

Dependencies are ones you already have, only needed to compile the program, pipe audio to the audio server, and emit notifications, that's it.

The recorder is launched as a native Linux process, tracked through its PID, stopped by signal, and verified to have exited before transcription begins.

An atomic lock directory guards the runtime. Directory creation is the mutex primitive, and ownership is tracked through live PIDs.

Stale sessions are recovered automatically, and repeated invocations coalesce into a single active operation, so key repeats, compositor double fires, interrupted sessions, and abandoned runtime artifacts resolve safely before a new recording begins.

**Runtime model:**

```text
press
  -> acquire lock
  -> start local recorder
  -> write recorder pid
  -> mark state as recording
  -> notify

talk...

press again
  -> acquire lock
  -> stop recorder
  -> mark state as transcribing
  -> parse RIFF/WAVE
  -> validate format chunk
  -> locate data chunk
  -> decode PCM16 into float samples
  -> run local inference in-process
  -> trim transcript
  -> write transcript to clipboard
  -> if pipe_to is configured, pipe transcript to command stdin
  -> notify
  -> remove runtime files
  -> release lock
```

Audio capture prefers PipeWire on Linux:

```text
pw-record
```

ALSA is used as fallback:

```text
arecord
```

On macOS, audio capture is native: `AVAudioEngine` taps the default microphone and `ScreenCaptureKit` captures system audio. Both streams are converted to 16 kHz mono signed-16 PCM in process and mixed into a single track before transcription. There is no external recorder binary on the macOS path.

```text
AVAudioEngine        # microphone
ScreenCaptureKit     # system audio (Zoom, Meet, Teams, browsers, etc.)
```

Clipboard backends:

```text
wl-copy                     # Wayland
xclip -selection clipboard  # X11 fallback
pbcopy                      # macOS
```

Notifications:

```text
notify-send   # Linux
osascript     # macOS
```

On Linux, `notify-send` pipes the event to Mako, Dunst, or any active desktop notification daemon to render it. On macOS, `osascript` posts a `display notification` event through the parent terminal application's bundle id.

**Runtime state:**

```text
$XDG_RUNTIME_DIR/asryx
```

If `$XDG_RUNTIME_DIR` is unavailable, falls back to:

```text
/tmp/asryx-$UID
```

**Runtime files:**

```text
lock/
rec.pid
rec.wav
rec.err
error.log
state
```

After a completed transcription, runtime files are completely removed. The transcript is always copied to the clipboard first. If `pipe_to` is configured, the same transcript is then piped into that command's stdin.

> [!NOTE]
> The clipboard is the permanent backup path. If the custom command exits non zero or otherwise fails at the process level, the transcript remains in the clipboard so the text isn't lost.

## Installation

### Linux

```bash
git clone https://github.com/bogdan-calapod/asryx-macos
cd asryx-macos && bash ./scripts/install
```

The installer validates the user environment, checks required tools, clones the pinned native inference source, builds the binary locally, installs the executable, writes the version pin, writes the default config, installs the default model, selects it, and prints a PATH note when `~/.local/bin` is unavailable from the current shell.

Installed paths:

```text
~/.local/bin/asryx
~/.local/opt/whisper.cpp
~/.local/share/asryx/
~/.local/share/asryx/versions/whisper-cpp-sha
~/.asryx.conf
```

Default model:

```text
base.en
```

Model downloads pull from [Hugging Face.](https://huggingface.co/ggerganov/whisper.cpp)

After installation:

```bash
asryx status
```

Expected output:

```text
idle
```

prints one of:

```text
idle
recording
transcribing
```

> [!TIP]
> This output can be used for status surfaces such as Waybar and Polybar.

### macOS

Recommended:

```bash
brew install bogdan-calapod/tap/asryx
```

This installs a Homebrew-managed `asryx` binary that links a bundled, pinned `whisper.cpp` source tree shipped inside the Homebrew Cellar. Models live under `~/.local/share/asryx/`, the config at `~/.asryx.conf`, and the runtime directory at `/tmp/asryx-$UID`.

After installation:

```bash
asryx --model install base.en
asryx --model use base.en
asryx status   # idle
```

From source:

```bash
git clone https://github.com/bogdan-calapod/asryx-macos
cd asryx-macos && bash ./scripts/install
```

The installer detects macOS, requires Homebrew and the Xcode Command Line Tools, and auto-installs missing build dependencies (`cmake`, `ninja`, `git`, `curl`) before building and installing the binary at `~/.local/bin/asryx`.

> [!NOTE]
> On macOS, asryx captures both your microphone and the system's audio output (so you can transcribe entire calls, not just your own voice). The first toggle triggers two permission prompts:
> - **Microphone**, against the parent terminal application (Terminal.app, iTerm, Ghostty, …)
> - **Screen Recording**, against `asryx` itself
>
> Grant both in **System Settings > Privacy & Security**. The "Screen Recording" permission is named that way by macOS because `ScreenCaptureKit` is the framework used; asryx never reads or stores any image, video, or window content.
>
> If you prefer to skip Screen Recording entirely and only capture your own microphone, set `mic_only_fallback=true` in `~/.asryx.conf`. asryx will then transcribe just your voice whenever the permission is missing.

### Releasing

Pushing a `vX.Y.Z` tag triggers `.github/workflows/release.yml`, which:

1. Publishes a GitHub Release with auto-generated notes.
2. Computes the sha256 of GitHub's source tarball, regenerates `bogdan-calapod/homebrew-tap/Formula/asryx.rb` with the new version and sha256, and pushes the formula update to the tap.

The tap update step requires a repository secret named `HOMEBREW_TAP_TOKEN`: a fine-grained Personal Access Token with `contents:write` permission on `bogdan-calapod/homebrew-tap`.

## Dependencies

You probably have most of these already, but check.

Build:

```text
bash
git
curl
cmake
ninja
g++ or clang++
```

Runtime depends on your machine. For audio, check what you have:

```bash
which pw-record || which arecord
```

PipeWire systems have `pw-record`, ALSA systems have `arecord`. If you have neither, install `pipewire` or `alsa-utils` through your package manager.

For clipboard, it depends on your session. Hyprland, Sway, and any other Wayland compositor need `wl-clipboard`. X11 needs `xclip`. If you're not sure which you're on:

```bash
echo "$XDG_SESSION_TYPE"
```

Desktop notifications require an active notification daemon such as Mako, Dunst, or the session's native notification service.

### macOS dependencies

Build (auto-installed by `./scripts/install` if Homebrew is present):

```text
xcode command line tools   # xcode-select --install
homebrew                   # https://brew.sh
cmake
ninja
git
curl
```

Runtime built-ins (always present on macOS):

```text
AVAudioEngine        # microphone capture (AVFoundation framework)
ScreenCaptureKit     # system audio capture (ScreenCaptureKit framework)
pbcopy               # clipboard
osascript            # notifications
```

Minimum macOS version: **13.0 (Ventura)** for ScreenCaptureKit audio capture.

## Keybind

The binary takes no arguments to toggle, so bind it to a key in the active compositor or desktop environment.

I personally use `Alt + W`, so the config for each DE/WM becomes:

Hyprland

```ini
bind = ALT, W, exec, asryx
```

Sway / i3

```ini
bindsym $mod+w exec asryx
```

GNOME

```text
Settings > Keyboard > Custom Shortcuts
command: asryx
```

KDE Plasma

```text
System Settings > Shortcuts > Custom Shortcuts
command: asryx
```

macOS — Karabiner-Elements (`~/.config/karabiner/karabiner.json` complex modification, or via the GUI):

```json
{
  "from": { "key_code": "w", "modifiers": { "mandatory": ["left_option"] } },
  "to":   [{ "shell_command": "/opt/homebrew/bin/asryx" }]
}
```

macOS — Hammerspoon (`~/.hammerspoon/init.lua`):

```lua
hs.hotkey.bind({"alt"}, "W", function()
  hs.execute("/opt/homebrew/bin/asryx", true)
end)
```

macOS — skhd (`~/.config/skhd/skhdrc`):

```text
alt - w : /opt/homebrew/bin/asryx
```

macOS — Raycast: add a script command pointing at `/opt/homebrew/bin/asryx` and assign it a hotkey from the Raycast extension preferences.

> [!TIP]
> A clipboard manager is highly recommended for long recordings. In case you copy something else by mistake after the transcription is emitted.

## CLI

The full surface area:

```text
asryx
asryx status
asryx --pipe-to '<COMMAND>'
asryx --no-pipe
asryx --language <auto|CODE>
asryx --model list
asryx --model install <MODEL>
asryx --model use <MODEL>
asryx --model uninstall <MODEL>
asryx --diarize list
asryx --diarize install <NAME>
asryx --diarize use <NAME>
asryx --diarize uninstall <NAME>
asryx --no-diarize
```

List supported models:

```bash
asryx --model list
```

Install a model:

```bash
asryx --model install small.en
```

Select a model:

```bash
asryx --model use small.en
```

Remove a model:

```bash
asryx --model uninstall small.en
```

Set transcription language:

```bash
asryx --language auto
asryx --language en
asryx --language de
```

Set the post copy pipe hook:

```bash
asryx --pipe-to 'tee -a ~/transcripts.txt'
```

`asryx --pipe-to '<COMMAND>'` simply updates `pipe_to` in `~/.asryx.conf` and exits, you can also do it manually of course.

`--pipe-to` is just a shell command string, so it can be a script path, a binary, or any command expression the shell can run.

> [!IMPORTANT]
> It doesn't verify that the command exists, resolves on `PATH`, or is executable. The command string can point at a shell script, binary, or any command expression. That's your responsibility to verify.

Clear the post copy pipe hook:

```bash
asryx --no-pipe
```

This simply clears `pipe_to` in `~/.asryx.conf` and exits.

Clipboard output is the default and always happens first. A completed clipboard only transcription sends this notification:

```text
transcription copied to clipboard.
```

`pipe_to` configures an optional post copy hook:

```text
pipe_to=tee -a /tmp/transcripts.txt
```

When `pipe_to` is non empty, bare `asryx` copies the transcript to the system clipboard first, then pipes the same text into `pipe_to` through stdin. The pipe path sends this notification:

```text
piped and copied to clipboard.
```

## Speaker diarization (macOS)

When asryx captures both your microphone and the system audio (the default macOS path with Screen Recording granted), the transcript is **speaker-labeled** instead of a single block of text.

- Your microphone is always tagged `You` — it's your mic, no inference needed.
- The system-audio stream is run through [`sherpa-onnx`](https://github.com/k2-fsa/sherpa-onnx) speaker diarization (pyannote segmentation 3.0 + a wespeaker embedding model) to assign `Speaker 1`, `Speaker 2`, `Speaker 3`, … labels.
- The two streams are merged by timestamp into a single dialogue.

### Models

Two ONNX models are required for diarization. Install them once:

```bash
asryx --diarize install pyannote-segmentation-3-0
asryx --diarize install wespeaker-voxceleb-resnet34
```

Combined download is ~55 MB into `~/.local/share/asryx/diarize/`.

Available embedding models:

| Name | Size | Strength |
|---|---|---|
| `wespeaker-voxceleb-resnet34` | ~25 MB | English-strong, recommended default |
| `3dspeaker-eres2net` | ~25 MB | Multilingual, slightly heavier inference |

Switch via:

```bash
asryx --diarize use 3dspeaker-eres2net
```

### Output formats

asryx renders the transcript in two places:

- **Clipboard** — human-readable dialogue (default), e.g.:
  ```text
  You: Morning team, quick standup. Andy, can you start?
  Speaker 1: Sure, I shipped the auth PR yesterday, waiting on review.
  You: Cool. Maria?
  Speaker 2: Thursday after lunch for the demo.
  ```
  Pure-dictation recordings (only the mic, no other speakers) drop the `You:` prefix and emit plain text — that's the same behavior asryx had pre-diarization.

- **`pipe_to`** — structured JSON (default), conforming to `asryx_schema_version: 1`:
  ```json
  {
    "asryx_schema_version": 1,
    "meta": {
      "language": "en",
      "started_at": "2026-06-11T08:42:13Z",
      "duration_seconds": 184.3,
      "num_speakers_detected": 3,
      "model": "large-v3-turbo",
      "diarization_segmentation_model": "pyannote-segmentation-3-0",
      "diarization_embedding_model": "wespeaker-voxceleb-resnet34"
    },
    "segments": [
      {"speaker": "you",       "t0": 0.00,  "t1":  4.21, "text": "Morning team..."},
      {"speaker": "speaker_1", "t0": 4.55,  "t1":  9.12, "text": "Sure, I shipped..."}
    ]
  }
  ```

Configure either format via `~/.asryx.conf`:

```text
clipboard_format=dialogue    # or json, plain
pipe_to_format=json          # or dialogue, plain
diarize=true                 # set false to skip diarization (single transcript)
diarize_threshold=0.7        # higher = merge speakers more aggressively
```

Or one-shot disable for an invocation with `asryx --no-diarize`.

### Hallucination filtering

Whisper sometimes invents text on quiet system audio (the classic "Thanks for watching!" problem). asryx uses sherpa's voice-activity output as a conservative gate: if sherpa reports no speech in a `[t0, t1]` range, the corresponding whisper text is dropped. Aggressive filtering is left to your downstream pipeline.

### Linux

Diarization is **macOS-only in v0.2.x**. Linux captures only the mic stream and emits a single unlabeled transcript, same as v0.1.x.

## Models

Supported models:

```text
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

| Model              | Disk    | RAM     | Speed vs large |
| ------------------ | ------- | ------- | -------------- |
| tiny / tiny.en     | 75 MiB  | ~273 MB | ~10x           |
| base / base.en     | 142 MiB | ~388 MB | ~7x            |
| small / small.en   | 466 MiB | ~852 MB | ~4x            |
| medium / medium.en | 1.5 GiB | ~2.1 GB | ~2x            |
| large-v3-turbo     | 1.5 GiB | ~2.3 GB | ~8x            |
| large-v1 / v2 / v3 | 2.9 GiB | ~3.9 GB | 1x             |

Speed is relative to large on CPU.

`base.en` is the default. It starts quickly and covers the default English offline transcription path.

Installed models live under:

```text
~/.local/share/asryx/
```

Example:

```text
~/.local/share/asryx/ggml-base.en.bin
```

## Configuration

Configuration is stored in `~/.asryx.conf`.

**Model**

`model` selects the active model. Switching via CLI updates the config instantly:

```bash
asryx --model use small.en
```

**Language**

`language` controls transcription language. `auto` detects the language first, which adds a small amount of unnecessary latency if you speak the same language all the time. Locking to a code skips detection entirely.

**mic_only_fallback** (macOS only)

`mic_only_fallback=true` makes asryx fall back to mic-only capture when Screen Recording permission is missing, instead of failing. Default is `false` (hard fail with an instructive error). Set it if you don't intend to transcribe call audio and want to skip the second permission prompt.

```bash
asryx --language es
asryx --language auto
```

English only models (`tiny.en`, `base.en`, `small.en`, `medium.en`) only accept `en` or `auto`. Multilingual models accept any of the 99 supported language codes.

> [!NOTE]
> Language support and transcription quality are a property of the [GGML model weights](https://github.com/ggerganov/whisper.cpp/blob/d682e150908e10caa4c15883c633d7902d685237/src/whisper.cpp#L248).

Invalid model and language values are rejected before recording starts.

<details>
<summary><strong>Supported language codes</strong></summary>

<br/>

| Code | Language       |
| ---- | -------------- |
| en   | english        |
| zh   | chinese        |
| de   | german         |
| es   | spanish        |
| ru   | russian        |
| ko   | korean         |
| fr   | french         |
| ja   | japanese       |
| pt   | portuguese     |
| tr   | turkish        |
| pl   | polish         |
| ca   | catalan        |
| nl   | dutch          |
| ar   | arabic         |
| sv   | swedish        |
| it   | italian        |
| id   | indonesian     |
| hi   | hindi          |
| fi   | finnish        |
| vi   | vietnamese     |
| he   | hebrew         |
| uk   | ukrainian      |
| el   | greek          |
| ms   | malay          |
| cs   | czech          |
| ro   | romanian       |
| da   | danish         |
| hu   | hungarian      |
| ta   | tamil          |
| no   | norwegian      |
| th   | thai           |
| ur   | urdu           |
| hr   | croatian       |
| bg   | bulgarian      |
| lt   | lithuanian     |
| la   | latin          |
| mi   | maori          |
| ml   | malayalam      |
| cy   | welsh          |
| sk   | slovak         |
| te   | telugu         |
| fa   | persian        |
| lv   | latvian        |
| bn   | bengali        |
| sr   | serbian        |
| az   | azerbaijani    |
| sl   | slovenian      |
| kn   | kannada        |
| et   | estonian       |
| mk   | macedonian     |
| br   | breton         |
| eu   | basque         |
| is   | icelandic      |
| hy   | armenian       |
| ne   | nepali         |
| mn   | mongolian      |
| bs   | bosnian        |
| kk   | kazakh         |
| sq   | albanian       |
| sw   | swahili        |
| gl   | galician       |
| mr   | marathi        |
| pa   | punjabi        |
| si   | sinhala        |
| km   | khmer          |
| sn   | shona          |
| yo   | yoruba         |
| so   | somali         |
| af   | afrikaans      |
| oc   | occitan        |
| ka   | georgian       |
| be   | belarusian     |
| tg   | tajik          |
| sd   | sindhi         |
| gu   | gujarati       |
| am   | amharic        |
| yi   | yiddish        |
| lo   | lao            |
| uz   | uzbek          |
| fo   | faroese        |
| ht   | haitian creole |
| ps   | pashto         |
| tk   | turkmen        |
| nn   | nynorsk        |
| mt   | maltese        |
| sa   | sanskrit       |
| lb   | luxembourgish  |
| my   | myanmar        |
| bo   | tibetan        |
| tl   | tagalog        |
| mg   | malagasy       |
| as   | assamese       |
| tt   | tatar          |
| haw  | hawaiian       |
| ln   | lingala        |
| ha   | hausa          |
| ba   | bashkir        |
| jw   | javanese       |
| su   | sundanese      |
| yue  | cantonese      |

</details>

## Uninstallation

```bash
./scripts/uninstall
```

Simply removes the owned files:

```text
~/.local/bin/asryx
~/.local/opt/whisper.cpp
~/.local/share/asryx
~/.cache/asryx
~/.asryx.conf
$XDG_RUNTIME_DIR/asryx     # Linux
/tmp/asryx-$UID            # macOS / Linux fallback
```

> [!NOTE]
> Deletion goes through owned [path validation](/src/platform/fs.cpp) before files or directories are even removed.

If you installed asryx via Homebrew on macOS, uninstall the binary with:

```bash
brew uninstall asryx
```

Then run `./scripts/uninstall` (from a checkout) to clear user-owned data and runtime artifacts; the script skips paths that do not exist.

## License

Apache-2.0 © @rccyx
