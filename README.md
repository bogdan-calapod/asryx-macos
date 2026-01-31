<div align="center">

# asryx

Turns your **voice** into **text**, **offline**, on Linux.

</div>

<img width="1920" height="1080" alt="image" src="https://github.com/user-attachments/assets/a12d2677-7307-4042-934b-c5f67011c335" />

## How it works

Run it once: it records.  
Run it again: it stops, transcribes, copies to clipboard, and notifies.

No daemons. No services. No dotfile edits. No compositor edits. You get one toggle command and you wire it to whatever control surface you want.

## Install

Clone, then run:

```sh
bash ./install --yes
````

That’s it.

After install, run the toggle:

```sh
asryx
```

Speak. Run `asryx` again. Your text is copied to clipboard and you get a notification.

## Backend 

This uses Whisper today (`whisper.cpp`), but the shape is a stable toggle surface + an engine behind it. The engine is swappable. Your keybind and workflow don’t change.

## Usage

If you’re on Hyprland, bind it to a key:

```ini
bind = ALT, W, exec, asryx
```

Press once to start recording, press again to stop and transcribe.

You can also run `asryx` from a terminal function, a script, a launcher (GNOME, etc), a bar, anything. It doesn’t care. It’s just a toggle.

## Uninstall

```sh
bash ./uninstall --yes
```

It’s manifest-driven and conservative, it removes what's installed, and won’t blindly delete random files unless you force it.

## What gets installed

By default everything goes under `$HOME/.local`.

```text
~/.local/bin/whisper-cli              engine
~/.local/bin/asryx                    toggle command
~/.local/share/asryx/models/*.bin     ggml model(s)
~/.local/share/asryx/asryx.env        env contract (optional overrides)
~/.local/share/asryx/install.manifest uninstall proof
~/.local/opt/whisper.cpp              build workspace (pinned checkout)
```

Logs:

```text
~/.cache/asryx/install-*.log
~/.cache/asryx/uninstall-*.log
```

<details>
<summary><strong>requirements and behavior</strong></summary>

This targets Debian/Ubuntu-style systems (apt + dpkg). The installer pulls everything it needs via apt.

Recording: prefers `pw-record` (pipewire) if present, otherwise uses `arecord`.
Clipboard: prefers `wl-copy` if present, otherwise uses `xclip`.
Notifications: uses `notify-send` if present.

State is local and temporary: it records into `$XDG_RUNTIME_DIR` (or `/tmp`) and cleans up after it finishes. Nothing stays running after the toggle exits.

</details>

<details>
<summary><strong>configuration (optional)</strong></summary>

You’re not supposed to need options. But if you insist, set env vars in your shell, your keybind, or in the generated contract file:

`~/.local/share/asryx/asryx.env`

Supported overrides:

`ASRYX_LANG` sets transcription language (`en`, `fr`, `ar`, etc).
`ASRYX_THREADS` sets thread count.
`ASRYX_MODEL` overrides the model path.
`ASRYX_ENGINE` overrides the engine path.

Compatibility aliases also work:

`ASR_LANG`, `ASR_THREADS`, `ASR_MODEL`, `ASR_ENGINE`.

</details>

<details>
<summary><strong>advanced install knobs</strong></summary>

```sh
./install --help
```

Useful flags:

`--prefix <path>` install somewhere else.
`--no-model` skip model download.
`--model <slug>` pick a different ggml model slug (default: `base.en`).
`--sha <commit>` change the pinned whisper.cpp commit (default is pinned for reproducibility).
`--clean-build` rebuild from scratch.

</details>

## License

Apache-2.0 © [@rccyx](https://rccyx.com)

