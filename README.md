# asr

A single toggle that turns your voice into text.

Run it once: it records.
Run it again: it stops, transcribes, copies to clipboard, and notifies.

This repo ships the whole surface: engine install, models, and the toggle script. Hyprland is assumed to already exist if you want a keybind, but nothing here edits your dotfiles or compositor config.

## install

Clone, then run the installer.

```sh
git clone https://github.com/rccyx/asr.git
cd asr
bash ./install --yes
````

By default it installs under `$HOME/.local`.

After install you should have:

`$HOME/.local/bin/asr-toggle`
`$HOME/.local/bin/whisper-cli`
`$HOME/.local/share/asr/models/ggml-<model>.bin`
`$HOME/.local/share/asr/asr.env`
`$HOME/.local/share/asr/install.manifest`

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
