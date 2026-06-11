#ifndef ASRYX_ENGINE_ENGINE_HPP
#define ASRYX_ENGINE_ENGINE_HPP

#include <string>
#include <sys/types.h>
#include <vector>

namespace engine {

// A single transcribed text region with its time range in seconds.
struct TranscriptSegment
{
  double t0 = 0.0;
  double t1 = 0.0;
  std::string text;
};

pid_t start_recording(const std::string& mic_wav_path, const std::string& sys_wav_path,
                      const std::string& err_path);
bool stop_recording(pid_t pid);
std::string transcribe(const std::string& model_path, const std::string& wav_path,
                       const std::string& language);
std::vector<TranscriptSegment> transcribe_with_segments(const std::string& model_path,
                                                        const std::string& wav_path,
                                                        const std::string& language);
bool copy_to_clipboard(const std::string& text);
bool send_notification(const std::string& message);

#ifdef ASRYX_TESTING
namespace testing {
using StartRecordingHook = pid_t (*)(const std::string& mic_wav_path,
                                     const std::string& sys_wav_path, const std::string& err_path);
using StopRecordingHook = bool (*)(pid_t pid);
using TranscribeHook = std::string (*)(const std::string& model_path, const std::string& wav_path,
                                       const std::string& language);
using TranscribeWithSegmentsHook = std::vector<TranscriptSegment> (*)(const std::string& model_path,
                                                                      const std::string& wav_path,
                                                                      const std::string& language);
using CopyToClipboardHook = bool (*)(const std::string& text);
using NotificationHook = bool (*)(const std::string& message);

void set_start_recording_hook(StartRecordingHook hook);
void set_stop_recording_hook(StopRecordingHook hook);
void set_transcribe_hook(TranscribeHook hook);
void set_transcribe_with_segments_hook(TranscribeWithSegmentsHook hook);
void set_copy_to_clipboard_hook(CopyToClipboardHook hook);
void set_notification_hook(NotificationHook hook);
void reset_hooks();
StartRecordingHook start_recording_hook();
StopRecordingHook stop_recording_hook();
TranscribeHook transcribe_hook();
TranscribeWithSegmentsHook transcribe_with_segments_hook();
CopyToClipboardHook copy_to_clipboard_hook();
NotificationHook notification_hook();
} // namespace testing
#endif

} // namespace engine

#endif // ASRYX_ENGINE_ENGINE_HPP
