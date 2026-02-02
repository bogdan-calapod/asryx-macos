<div align="center">

# asryx

Effortlessly turn your **voice** into **text**, **offline**, on Linux.

</div>

<img width="1920" height="1080" alt="image" src="https://github.com/user-attachments/assets/a12d2677-7307-4042-934b-c5f67011c335" />

## How it works

It will auto-install and auto-compile a [Whisper](https://github.com/ggml-org/whisper.cpp) binary on your machine that talks directly to your CPU’s hardware-level math accelerators. You don’t have to do anything, waste your time debugging, copy pasting, or worry about edge cases. 

Just run the installer, confirm the plan, and it **just works**. All you have to do after is:

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
> Pass `--yes` if you want to bypass the confirmation prompt.

<details>
<summary><strong>You should see something like this</strong></summary>

<br/>

<img width="963" height="1080" alt="image" src="https://github.com/user-attachments/assets/e4bbf937-687f-4ba1-bf7d-f862a1868744" />

</details>

<details>
<summary><strong>Model choice</strong></summary>

<br/>

Pick a different model:

```sh
bash ./package/install --model tiny.en
```

* **tiny.en / tiny**: ~75MB. Fastest, lowest resource usage. Great for clear English audio.
* **base.en / base**: ~140MB. Good balance of speed and accuracy for basic tasks.
* **small.en / small**: ~460MB. Significant bump in accuracy; requires more memory.
* **medium.en / medium**: ~1.5GB. High accuracy, much slower. Best for complex vocabulary.
* **large-v1 / large-v2 / large-v3**: ~2.9GB. State-of-the-art accuracy. Heavy resource demand.
* **turbo**: ~1.5GB. The "Large-v3" distilled version. Near-large accuracy with significantly faster inference speeds.


> Models ending in `.en` are optimized for **English only**. Standard names (e.g., `small`) are multilingual.

</details>



<details>
<summary><strong>Whisper backend</strong></summary>

<br/>

Today it ships with `whisper.cpp` because it’s fast and local.

The point is the shape, a stable toggle surface plus an engine behind it. The engine is a architecturally replaceable. Your workflow does not change. Your bind does not change.
</details>



<details>
<summary><strong>What the installer actually does</strong></summary>

<br/>

It basically builds the engine from source on your machine.

It clones `whisper.cpp`, hard-checks out a pinned commit that actually works, builds a release engine, then installs the resulting binaries. Engine install is atomic: it stages into a temporary directory, then swaps into place so you never end up with a half-written binary if something goes wrong mid-copy.

It then downloads the ggml model, installs the toggle script, writes an env contract file you can override, and writes an uninstall manifest containing file paths and SHA256 hashes.

It installs only what it needs through your system package manager, and it will skip optional runtime helpers if your distro doesn’t have them.

Nothing in your shell config is modified. No compositor config is modified. No keybinds are created. No background services are installed.

</details>


## Supported distros

Supported (and [tested](/.github/workflows/ci.yml)) on **Debian** based systems (Debian, Ubuntu, Mint and Pop! Os), **Arch**, and **Fedora**.

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

Press **once** to **start** recording, press **again** to **stop** and transcribe.

If you’re not on Hyprland, bind it anywhere else. It’s just a command.


<details>
<summary><strong>What it actually does</strong></summary>

<br/>

All runtime artifacts are ephemeral:

* audio + transcript live in `$XDG_RUNTIME_DIR` when available
* `/tmp` is the fallback
* nothing is intended to survive reboot
* after copying + notifying, the runtime files are deleted

No history. No cache. No recovery.

Note that it **refuses** to run if the installer **contract** is **missing** or **broken**

</details>

## Uninstall

```sh
bash ./package/uninstall --yes
```

Uninstall is manifest-driven. It removes only verified install artifacts and will not delete anything it didn’t install unless you force it.

> Using `--force` disables verification and removes known install paths even if files were modified or the manifest is incomplete.


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
