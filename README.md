<div align="center">

# asryx

Turns your **voice** into **text**, **offline**, on Linux.

</div>

<img width="1920" height="1080" alt="image" src="https://github.com/user-attachments/assets/a12d2677-7307-4042-934b-c5f67011c335" />

## How it works

Run it once: it records.  
Run it again: it stops, transcribes, copies to clipboard, and notifies.

No daemons. No services. No dotfile edits. No compositor edits. You get one toggle command and you wire it to whatever control surface you want.

> [!NOTE]
> Thyx as a standalone FOSS is an extension of the [OSyx](https://github.com/rccyx/osyx) ecosystem.


## Install

Clone, then run:

```sh
bash ./install 
```

Stats with a **plan** showing you **exactly** what **will** **happen**, where files go, and what **will not** be **touched**.

<details>
<summary><strong>Here's how it looks like</strong></summary>

<br/>

<img width="958" height="1080" alt="image" src="https://github.com/user-attachments/assets/0c20a97d-e917-44ee-a17e-7380e4e4b361" />


</details>

If it looks right, confirm and proceed. If not, abort.

That’s it.

> [!TIP]
> pass `--yes` if you want to bypass the plan.

<details>
<summary><strong>You should see something like this</strong></summary>

<br/>

<img width="963" height="1080" alt="image" src="https://github.com/user-attachments/assets/e4bbf937-687f-4ba1-bf7d-f862a1868744" />


</details>

After install, run the toggle:

```sh
asryx
```

Speak. Run `asryx` again. Your text is copied to clipboard and you get a notification.


<details>
<summary><strong>advanced install knobs</strong></summary>

```sh
./install --help
```

base.en

Useful flags:

`--prefix <path>` install somewhere else.
`--no-model` skip model download.
`--model <slug>` pick a different ggml model slug, default: e.g: `tiny.en`, `small.en`;`medium.en`, (default is `base.en`).
`--sha <commit>` change the pinned whisper.cpp commit (default is pinned for reproducibility).
`--clean-build` rebuild from scratch.

</details>

## Backend

This uses Whisper today (`whisper.cpp`), but the shape is a stable toggle surface + an engine behind it. The engine is swappable. Your keybind and workflow don’t change.

## Usage

If you’re on Hyprland, bind it to a key:

```ini
bind = ALT, W, exec, asryx
```

Press once to start recording, press again to stop and transcribe.

You can also run `asryx` from a terminal, a script, a launcher, a bar, anything. It doesn’t care. It’s just a toggle.

## Uninstall

```sh
bash ./uninstall --yes
```

It’s manifest-driven and conservative, it removes what's installed, and won’t blindly delete random files unless you force it.

<details>
<summary><strong>See run</strong></summary>

<br/>

<img width="962" height="1080" alt="image" src="https://github.com/user-attachments/assets/1846a3ea-411d-49c0-b9f8-87f00199f1a8" />


</details>

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

## License

Apache-2.0 © [@rccyx](https://rccyx.com)
