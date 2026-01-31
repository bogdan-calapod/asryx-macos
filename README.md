<div align="center">

# asryx

Turns your **voice** into **text**, **offline**, on Linux.

</div>


<img width="1920" height="1080" alt="image" src="https://github.com/user-attachments/assets/a12d2677-7307-4042-934b-c5f67011c335" />

## How it works

Run it once: it records.
Run it again: it stops, transcribes, copies to clipboard, and notifies.

This repo ships the whole surface: engine install, models, and the toggle script. You're free to hookup keybinds, scripts whatever, this provides you eveyrthing you need and more, but nothing here edits your dotfiles or compositor config.

## install

Clone, then run the installer.

```sh
bash ./install --yes
```

That's it, run `asr-toggle` once, you'll get a notification talk, run it again, you'll get text copied to your clipboard, you'll get a notification.

Read more below to learn how it works & how you can hook it up.

## Setup
By default it installs under `$HOME/.local`.

After install you should have:

```bash
~/.local/bin/whisper-cli        ← engine
~/.local/bin/asr-toggle         ← toggle
~/.local/share/asr/models/...   ← model
~/.local/share/asr/asr.env      ← contract
~/.local/share/asr/install.manifest
~/.local/opt/whisper.cpp        ← build workspace
```

> [!NOTE]
> It uses Whisper, but the backend is swappable, whisper today, somthing else tmorrow, the thing is, it will always work.

## use

If you are on Hyprland, bind it to a key. Example:

```ini
bind = ALT, W, exec, asr-toggle
```

Now press `ALT+W` to start recording, press again to stop and transcribe.

You can also just run `asr-toggle` from a terminal, from a script, or from whatever control surface you prefer. It does not care, it only exposes a single toggle.

## uninstall

```sh
bash ./uninstall --yes
```

<details>
<summary><strong>what it uses under the hood</strong></summary>

It installs a pinned build of `whisper.cpp` for offline speech to text, installs `whisper-cli` into your prefix, and downloads a ggml model into your prefix.

The toggle script is installed as `asr-toggle` and is prefix-relative, so it does not depend on your repo checkout after install.

</details>

<details>
<summary><strong>requirements and behavior</strong></summary>

This targets Debian/Ubuntu style systems (apt). It installs what it needs via apt.

Recording: prefers `pw-record` if present, otherwise uses `arecord`.
Clipboard: uses `wl-copy` if present, otherwise `xclip`.
Notifications: uses `notify-send` if present.

Nothing runs as a daemon. No services. No background installs after the toggle exits.

</details>

<details>
<summary><strong>configuration (optional)</strong></summary>

You are not supposed to need options. But, if you insist, the toggle supports environment overrides:

`ASR_LANG` sets the transcription language (example: `en`, `fr`, `ar`).
`ASR_THREADS` sets thread count.
`ASR_MODEL` overrides the model path.
`ASR_ENGINE` overrides the engine path.

Everything else is opinionated on purpose.

</details>

## license

Apache-2.0
