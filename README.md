<div align="center">

# asryx

Effortlessly turn your **voice** into **text**, **offline**, on Linux.

You get **one toggle** command and you **wire** it to **whatever** control **surface** you want.

</div>

<img width="1920" height="1080" alt="image" src="https://github.com/user-attachments/assets/5611ee6f-83f1-4640-b5cc-3d1716ec9586" />

## How it works

You simply **run** the installer, **confirm** the **plan**, and it **just works**. All you have to do after is:

You **run** it/keybind **once**: it records.

You **run** it/keybind **again**: it stops, **transcribes**, copies to **clipboard**, and **notifies**.

**Same** **toggle**, never have to hold it. Tap once, **Talk** for **hours** if you want and tap again. It **won’t fail** midway.

> [!NOTE]
> No daemons. No services. No dotfile edits. No compositor edits. No bloated GUIs, and no runtimes or Docker. Linux is all you need.

<details>
<summary><strong>What it does under the hood</strong></summary>

<br/>

It builds [whisper.cpp](https://github.com/ggml-org/whisper.cpp) from source on your machine using plain C++ and your system compiler. No Python, no CUDA stack, no runtimes, no virtual environments, no Rust, and no Node.

The engine is a single native binary that runs directly on your CPU and uses SIMD vector instructions (AVX, AVX2, FMA, etc. when available) for fast matrix math: tight, low-level numerical code compiled specifically for your machine, not a framework sitting on top of an interpreter.

And since whisper.cpp is dependency-minimal by design, if you can compile standard C++ code, you can build it. The installer handles the entire process: cloning a pinned commit, compiling a release build, and installing the resulting binaries with an atomic swap so you never end up with a half-written engine binary.

You never touch build flags, copy commands from GitHub issues, or debug toolchains. The complexity exists, but it’s sealed behind the installer so you don’t have to think about it.

The toggle is stateful, not timing-based.

On the first invocation, `asryx` starts the recorder and writes its process ID to a PID file. This is the single source of truth for what’s currently recording.

On the second invocation, it reads that PID and stops *that exact process*. No heuristics. No race with your fingers. No reliance on key-up / key-down semantics.

A lock directory guards the entire transition. If you press the key multiple times in rapid succession, only one invocation is allowed to enter the critical section. The rest exit immediately. This prevents duplicate recorders, partial stops, and corrupted state. The result is a **true toggle** that just works reliably.

Runtime payload artifacts are intentionally short-lived. Audio and transcripts are written to `$XDG_RUNTIME_DIR` when available (otherwise `/tmp`) and are deleted after transcription completes.

The persistent install boundary is the contract + manifest under `~/.local/share/asryx/`. There is no history, cache, or recovery layer for past transcripts. The clipboard is the persistence boundary by design: once the text is copied, the payload state is clean again.

Today it ships with whisper.cpp because it’s fast and local.

The point is the shape: a stable toggle surface plus an engine behind it. The engine is architecturally replaceable. Your workflow does not change. Your bind does not change.

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

<img width="1920" height="1080" alt="image" src="https://github.com/user-attachments/assets/59049de5-628d-483a-b7d5-947a8430904d" />

</details>

If it looks right, confirm and proceed. If not, abort.

That’s it.

Pass `--yes` / `-y` if you want to bypass the confirmation prompt.

> [!IMPORTANT]
> `asryx` installs into `~/.local/bin`. If `asryx` is “command not found”, ensure `~/.local/bin` is on your `PATH`, or run it as `~/.local/bin/asryx`.

<details>
<summary><strong>Model choice</strong></summary>

<br/>

Pick a different model at install time:

```sh
bash ./package/install --model tiny.en
```

* **tiny.en / tiny**: ~78 MB. Fastest, lowest resource usage. Great for clear English audio.
* **base.en / base**: ~148 MB. Good balance of speed and accuracy for basic tasks.
* **small.en / small**: ~488 MB. Noticeable accuracy jump; higher memory use.
* **medium.en / medium**: ~1.53 GB. High accuracy, significantly slower. Best for complex vocabulary.
* **large-v1 / large-v2 / large-v3**: ~2.9 to 3.1 GB. State-of-the-art accuracy. Heavy CPU and memory demand.
* **large-v3-turbo**: ~1.5 GB. Distilled Large-v3. Near-large accuracy with substantially faster inference.

> Models ending in `.en` are optimized for **English only**. Standard names (e.g., `small`) are multilingual.

</details>

<details>
<summary><strong>What the installer actually does</strong></summary>

<br/>

It builds the engine from source on your machine as discussed.

Engine binary install is atomic: it stages into a temporary directory, then swaps into place so you never end up with a half-written binary if something goes wrong mid-copy.

It then downloads the ggml model, installs the toggle script, writes an env contract file, and writes an uninstall manifest containing file paths and SHA256 hashes.

It installs only what it needs through your system package manager, and it will skip optional runtime helpers if your distro doesn’t have them.

Nothing in your shell config is modified. No compositor config is modified. No keybinds are created. No background services are installed.

</details>

## Supported distros

Supported (and [CI](./.github/workflows/) tested) on **Debian** based systems (Debian, Ubuntu, Mint and Pop! OS), **Arch**, and **Fedora**.

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

> [!NOTE]
> It **refuses** to run if the installer **contract** is **missing** or **broken**.


<details>
<summary><strong>Switching models</strong></summary>

<br/>

First install the model.

```bash
bash ./package/install --model large-v3-turbo --yes
```

Then update the model inside `~/.asryx.env`.

```bash
ASRYX_MODEL=base.en
```

If you see an error like:

```text
[err] model missing: ~/.local/share/asryx/models/ggml-large-v3-turbo.bin
```

It means your active `ASRYX_MODEL` is pointing to a model that is not installed at `~/.local/share/asryx/models/`.

Fix it but letting it know.

```sh
printf 'ASRYX_MODEL=base.en\n' > ~/.asryx.env
asryx
```

</details>

## Uninstall

```sh
bash ./package/uninstall --yes
```

Uninstall is manifest-driven. It removes only SHA-verified install artifacts and will not delete anything it didn’t install unless you force it.

> [!TIP]
> Using `--force` disables verification and removes known install paths even if files were modified or the manifest is incomplete.

<details>
<summary><strong>See a run</strong></summary>

<br/>

<img width="1920" height="1080" alt="image" src="https://github.com/user-attachments/assets/c3b3debc-1533-45ac-bc69-8a941d814c8a" />

</details>

## Surface

<details>
<summary><strong>touch surface</strong></summary>

<br/>

By default, everything goes under `$HOME/.local`.

```text
~/.local/bin/whisper-cli              engine
~/.local/bin/whisper-stream           optional engine helper (if built)
~/.local/bin/asryx                    toggle command
~/.local/share/asryx/models/*.bin     ggml model(s)
~/.local/share/asryx/asryx.env        installer-written contract
~/.local/share/asryx/install.manifest uninstall proof
~/.local/opt/whisper.cpp              build workspace (pinned checkout)
```

Runtime (ephemeral):

```text
$XDG_RUNTIME_DIR/asryx-$UID/          state dir (pid/lock + temporary files)
# falls back to /run/user/$UID or /tmp
```

Repo paths:

```text
./package/asryx      toggle source
./package/install    installer
./package/uninstall  uninstaller
```

Logs:

```text
~/.cache/asryx/install-*.log
~/.cache/asryx/uninstall-*.log
```

Layout:

```text
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
