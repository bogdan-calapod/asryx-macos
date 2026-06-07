#include "engine/engine.hpp"

#ifdef ASRYX_TESTING

namespace engine::testing {

namespace {

StartRecordingHook start_recording_hook_value = nullptr;
StopRecordingHook stop_recording_hook_value = nullptr;
TranscribeHook transcribe_hook_value = nullptr;
CopyToClipboardHook copy_to_clipboard_hook_value = nullptr;
NotificationHook notification_hook_value = nullptr;

} // namespace

void set_start_recording_hook(StartRecordingHook hook)
{
  start_recording_hook_value = hook;
}

void set_stop_recording_hook(StopRecordingHook hook)
{
  stop_recording_hook_value = hook;
}

void set_transcribe_hook(TranscribeHook hook)
{
  transcribe_hook_value = hook;
}

void set_copy_to_clipboard_hook(CopyToClipboardHook hook)
{
  copy_to_clipboard_hook_value = hook;
}

void set_notification_hook(NotificationHook hook)
{
  notification_hook_value = hook;
}

void reset_hooks()
{
  start_recording_hook_value = nullptr;
  stop_recording_hook_value = nullptr;
  transcribe_hook_value = nullptr;
  copy_to_clipboard_hook_value = nullptr;
  notification_hook_value = nullptr;
}

StartRecordingHook start_recording_hook()
{
  return start_recording_hook_value;
}

StopRecordingHook stop_recording_hook()
{
  return stop_recording_hook_value;
}

TranscribeHook transcribe_hook()
{
  return transcribe_hook_value;
}

CopyToClipboardHook copy_to_clipboard_hook()
{
  return copy_to_clipboard_hook_value;
}

NotificationHook notification_hook()
{
  return notification_hook_value;
}

} // namespace engine::testing

#endif
