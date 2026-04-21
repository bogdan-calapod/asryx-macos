<div align="center">

# **_Asryx_**

<br/>

### **_Talk to your machine._**

<br/>

</div>

## What is it?

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

That’s it,

```text
record -> stop -> transcribe -> copy -> notify -> clean
```

You've seen it in the demo.

Repeated key presses are safe. If a compositor double-fires the keybind or the key repeats, it will not spawn five recorders or corrupt the active transcription.

No cloud API. No GUI. No Python environment. No background dashboard. No provider menu. No subscriptions, no key presistent pressing, no choose from 965 models, no do these 17 steps first and maybe it works. No Node, no Python, no Node again.

## Why?

Sometimes I speak for five seconds. Sometimes I pace around and talk for fifteen minutes straight while doing something else, or whatever thought needs to leave my head before it's gone forever, my fingers couldn't catch up.

Opening a transcription app or holding a key for that, or sending raw speech to someone else’s API just to talk to your own computer is unreliable, aside from privacy concerns, what if you get an error back, or maybe the net shut off? See?

Asryx does one thing:

```text
press keybind -> talk -> press keybind -> paste text
```

On Hyprland, you can bind it like `Alt+W`.

```ini
bind = ALT, W, exec, asryx
```

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
- installs missing build dependencies on supported apt-based systems
- builds the native Asryx binary locally
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

Runtime files live under the user runtime directory, usually:

```text
$XDG_RUNTIME_DIR/asryx
```

This tool does NOT keep transcript history. Does NOT keep audio history.

Once the transcript reaches your clipboard, the temporary recording payload is cleaned.

</details>

After installation:

```bash
asryx status
```

You should see the current runtime state.

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

It does not remove shared system packages.

It refuses dangerous paths such as `/`, `$HOME`, empty paths, and non-absolute paths.

</details>

## CLI

The surface area is tiny:

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

```bash
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
large
```

Installed models live in:

```bash
~/.local/share/asryx/
```

For example:

```bash
~/.local/share/asryx/ggml-base.en.bin
```

The active model is stored in:

```bash
~/.asryx.conf
```

Default config:

```bash
model=base.en
```

## Config

This binary uses one small config file.

Switching models through the CLI updates this file.

```bash
asryx --model use small.en
```

You can also edit it manually.

```text
model=small.en
```

## Audio

Asryx prefers PipeWire.

```text
pw-record
```

If unavailable, it falls back to ALSA.

```text
arecord
```

### Clipboard

Asryx prefers Wayland clipboard tooling.

```text
wl-copy
```

If unavailable, it falls back to X11.

```text
xclip
```

### Notifications

Asryx uses:

```text
notify-send
```

> [!IMPORTANT]
> You still need a notification daemon such as Mako, Dunst, or your desktop environment’s default notification service to see popups.

## Troubleshooting

Most issues are usually one of these:

- no microphone recorder available
- the microphone is off, and the audio is piped to it. Transcribes nothing.
- PipeWire or ALSA device setup issue
- missing clipboard provider
- missing notification daemon
- model not installed
- `~/.local/bin` not in `PATH`

Check the installed command first.

```bash
asryx status
```

Check models.

```bash
asryx --model list
```

Install the default model again if needed.

```bash
asryx --model install base.en
asryx --model use base.en
```

If audio behaves weirdly, it is usually the local Linux audio stack, interface, microphone, PipeWire, or ALSA path.

Open an issue with your distro, session type, recorder availability, model name, and the exact terminal output.

## License

Apache-2.0 © @rccyx
