<div align="center">

# asryx

Effortlessly turn your **voice** into **text**, **offline**, on Linux.

</div>

<img width="1920" height="1080" alt="image" src="https://github.com/user-attachments/assets/a12d2677-7307-4042-934b-c5f67011c335" />

## How it works

It will auto-install and auto-compile a [Whisper](https://github.com/ggml-org/whisper.cpp) binary on your machine that talks directly to your CPU’s hardware-level math accelerators. You don’t have to do anything, just run the installer. It **just works** for **you**. All you have to do after is:

You **run** it/keybind **once**: it records.

You **run** it/keybind **again**: it stops, **transcribes**, copies to **clipboard**, and **notifies**.

No daemons. No services. No dotfile edits. No compositor edits. No bloated GUIs, and no runtimes or Docker. You get one toggle command and you wire it to whatever control surface you want.

<details>
<summary><strong>What it does under the hood</strong></summary>

<br/>

It uses a PID file to remember the recorder process between invocations, so the second press knows exactly what to stop. It also uses a lock directory so if you mash the key five times in a second, it won’t spawn five recorders or corrupt state. It becomes a clean, debounced switch.

Audio and transcript live in your runtime directory (`$XDG_RUNTIME_DIR` when available, otherwise `/tmp`) and are **ephemeral by design**. The clipboard is the persistence boundary.

</details>

## Install

Clone, then run:

```sh
bash ./package/install
```

Starts with a **plan** showing you **exactly** what **will** **happen**, where files go, and what **will not** be **touched**.

<details>
<summary><strong>Here’s what it looks like</strong></summary>

<br/>

<img width="958" height="1080" alt="image" src="https://github.com/user-attachments/assets/0c20a97d-e917-44ee-a17e-7380e4e4b361" />

</details>

If it looks right, confirm and proceed. If not, abort.

That’s it.

> [!TIP]
> Pass `--yes` if you want to bypass the plan.

<details>
<summary><strong>You should see something like this</strong></summary>

<br/>

<img width="963" height="1080" alt="image" src="https://github.com/user-attachments/assets/e4bbf937-687f-4ba1-bf7d-f862a1868744" />

</details>

<details>
<summary><strong>Model choice (the only install knob)</strong></summary>

<br/>

```sh
bash ./package/install --help
```

Pick a different ggml model slug:

```sh
bash ./package/install --model tiny.en
```

</details>

> [!NOTE]
> asryx as a standalone FOSS project is an extension of [OSyx](https://github.com/rccyx/osyx).

## Usage

After install:

```sh
asryx
```

Speak.
Run `asryx` again.
Your text is copied to the clipboard and you get a notification.

If you’re on Hyprland, bind it to a key:

```ini
bind = ALT, W, exec, asryx
```

Press once to start recording, press again to stop and transcribe.

If you’re not on Hyprland, bind it anywhere else. It’s just a command.

## Supported distros

Tested and supported on Debian-based systems (Debian, Ubuntu), Arch, and Fedora.

## Contract and runtime overrides

asryx does not have a “config directory”.

The installer writes a **contract** file once:

```text
~/.local/share/asryx/asryx.env
```

This contract contains resolved absolute paths so the toggle stays dumb and deterministic. It is not advertised as a preference surface.

Runtime overrides are intentionally limited to output controls:

* `ASRYX_MODEL`
* `ASRYX_LANG`
* `ASRYX_THREADS` (optional)

If you don’t want to export env vars everywhere, you can put them in a single home file:

```text
~/.asryx.env
```

Same keys, same meaning. No other keys are read.

## Filesystem semantics

asryx does not create or read a user config directory. `$XDG_CONFIG_HOME/asryx` must not exist.

All runtime artifacts are ephemeral:

* audio + transcript live in `$XDG_RUNTIME_DIR` when available
* `/tmp` is the fallback
* nothing is intended to survive reboot
* after copying + notifying, the runtime files are deleted

No history. No cache. No recovery.

## Uninstall

```sh
bash ./package/uninstall --yes
```

It’s manifest-driven and conservative. It removes what’s installed and won’t blindly delete random files unless you force it.

<details>
<summary><strong>See a run</strong></summary>

<br/>

<img width="962" height="1080" alt="image" src="https://github.com/user-attachments/assets/1846a3ea-411d-49c0-b9f8-87f00199f1a8" />

</details>

## Surface

<details>
<summary><strong>touch surface</strong></summary>

<br/>

By default, everything goes under `$HOME/.local`.

```text
~/.local/bin/whisper-cli              engine
~/.local/bin/asryx                    toggle command
~/.local/share/asryx/models/*.bin     ggml model(s)
~/.local/share/asryx/asryx.env        installer-written contract
~/.local/share/asryx/install.manifest uninstall proof
~/.local/opt/whisper.cpp              build workspace (pinned checkout)
```

Logs:

```text
~/.cache/asryx/install-*.log
~/.cache/asryx/uninstall-*.log
```

Layout:

```
~/.local/
├── bin/
│   ├── whisper-cli
│   ├── whisper-stream
│   └── asryx
├── share/
│   └── asryx/
│       ├── models/
│       │   └── ggml-*.bin
│       ├── asryx.env
│       └── install.manifest
└── opt/
    └── whisper.cpp/
```

</details>

<details>
<summary><strong>requirements and behavior</strong></summary>

<br/>

Recording uses `pw-record` (PipeWire) when available, otherwise `arecord` (ALSA).
Clipboard uses `wl-copy` (Wayland) when available, otherwise `xclip` (X11).

Notifications are mandatory. `notify-send` must exist. The installer hard-enforces this.

Threads default to `nproc` and can be overridden with `ASRYX_THREADS`.
Language can be set with `ASRYX_LANG`. If you use an `.en` model and don’t set a language, it defaults to English.

</details>

## License

Apache-2.0 © [@rccyx](https://rccyx.com)
