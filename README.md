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

Audio and transcript live in your runtime directory (`$XDG_RUNTIME_DIR` when available, otherwise `/tmp`). On most modern systems, `$XDG_RUNTIME_DIR` is memory-backed.

</details>

## Install

Clone, then run:

```sh
bash ./install
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
<summary><strong>Advanced install knobs</strong></summary>

<br/>

```sh
./install --help
```

Useful flags:

* `--prefix <path>` install somewhere else.
* `--no-model` skip model download.
* `--model <slug>` pick a different ggml model slug, e.g. `tiny.en`, `small.en`, `medium.en` (default is `base.en`).
* `--sha <commit>` change the pinned whisper.cpp commit (default is pinned for reproducibility).
* `--clean-build` rebuild from scratch.

</details>

<details>
<summary><strong>What the installer actually does</strong></summary>

<br/>

It basically builds the engine from source on your machine.

It clones `whisper.cpp`, hard-checks out a pinned commit (reproducible by default, overridable if you want), builds a Release engine, then installs the resulting binaries into your prefix. Engine install is atomic: it stages into a temporary directory, then swaps into place so you never end up with a half-written binary if something goes wrong mid-copy.

It then downloads the ggml model (unless you disable models), installs the toggle script, writes an env contract file you can override, and writes an uninstall manifest containing file paths and SHA256 hashes.

It installs only what it needs through your system package manager, and it will skip optional runtime helpers if your distro doesn’t have them.

Nothing in your shell config is modified. No compositor config is modified. No keybinds are created. No background services are installed.

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

If you’re not on Hyprland, bind it anywhere else. It’s just a command. Keyboard daemon, bar button, launcher, shell alias, Stream Deck, whatever, asryx doesn’t care.

## Supported distros

Tested and supported on Debian-based systems (Debian, Ubuntu, Mint, Pop!_OS, Zorin), Arch, and Fedora.

> [!TIP]
> Advanced users can fork or contribute to adapt it for other distributions.

## Backend

Today it ships with `whisper.cpp` because it’s fast and local.

The point is the shape: a stable toggle surface plus an engine behind it. The engine is a replaceable component. Your workflow does not change. Your bind does not change.

## Uninstall

```sh
bash ./uninstall --yes
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
~/.local/share/asryx/asryx.env        env contract (optional overrides)
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
│   ├── whisper-cli        # runtime binary
│   ├── whisper-stream
│   └── asryx              # toggle
├── share/
│   └── asryx/
│       ├── models/
│       │   └── ggml-*.bin
│       ├── asryx.env
│       └── install.manifest
└── opt/
    └── whisper.cpp/       # pinned build workspace
```

</details>

<details>
<summary><strong>requirements and behavior</strong></summary>

<br/>

Recording prefers `pw-record` (PipeWire) when available, otherwise, it uses `arecord` (ALSA).
Clipboard prefers `wl-copy` when available, otherwise, it uses `xclip`.
Notifications use `notify-send` when available.

Threads default to `nproc` and can be overridden with `ASRYX_THREADS` (or `ASR_THREADS`).
Language can be set with `ASRYX_LANG` (or `ASR_LANG`). If you use an `.en` model, it will default to English automatically.

</details>

<details>
<summary><strong>configuration (optional)</strong></summary>

<br/>

You’re not supposed to need options. But if you insist, set env vars in your shell, your keybind, or in the generated contract file:

`~/.local/share/asryx/asryx.env`

Supported overrides:

`ASRYX_LANG` sets transcription language (`en`, `fr`, `ar`, etc.).
`ASRYX_THREADS` sets thread count.
`ASRYX_MODEL` overrides the model path.
`ASRYX_ENGINE` overrides the engine path.

Compatibility aliases also work:

`ASR_LANG`, `ASR_THREADS`, `ASR_MODEL`, `ASR_ENGINE`.

</details>

## License

Apache-2.0 © [@rccyx](https://rccyx.com)
