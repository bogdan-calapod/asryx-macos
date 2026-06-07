#include "engine/engine.hpp"

#ifdef ASRYX_TESTING

namespace engine::testing {

namespace {

struct Hooks
{
  StartRecordingHook start_recording = nullptr;
  StopRecordingHook stop_recording = nullptr;
  TranscribeHook transcribe = nullptr;
  CopyToClipboardHook copy_to_clipboard = nullptr;
  NotificationHook notification = nullptr;
};

Hooks& hooks()
{
  static Hooks value;
  return value;
}

} // namespace

void set_start_recording_hook(StartRecordingHook hook)
{
  hooks().start_recording = hook;
}

void set_stop_recording_hook(StopRecordingHook hook)
{
  hooks().stop_recording = hook;
}

void set_transcribe_hook(TranscribeHook hook)
{
  hooks().transcribe = hook;
}

void set_copy_to_clipboard_hook(CopyToClipboardHook hook)
{
  hooks().copy_to_clipboard = hook;
}

void set_notification_hook(NotificationHook hook)
{
  hooks().notification = hook;
}

void reset_hooks()
{
  hooks() = Hooks{};
}

StartRecordingHook start_recording_hook()
{
  return hooks().start_recording;
}

StopRecordingHook stop_recording_hook()
{
  return hooks().stop_recording;
}

TranscribeHook transcribe_hook()
{
  return hooks().transcribe;
}

CopyToClipboardHook copy_to_clipboard_hook()
{
  return hooks().copy_to_clipboard;
}

NotificationHook notification_hook()
{
  return hooks().notification;
}

} // namespace engine::testing

#endif
