#include "platform/audio.hpp"

#include "platform/process.hpp"

#import <AVFoundation/AVFoundation.h>
#import <CoreGraphics/CoreGraphics.h>
#import <CoreMedia/CoreMedia.h>
#import <Foundation/Foundation.h>
#import <ScreenCaptureKit/ScreenCaptureKit.h>

#include <algorithm>
#include <array>
#include <atomic>
#include <chrono>
#include <csignal>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <iostream>
#include <mach-o/dyld.h>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

namespace platform::audio {

namespace {

constexpr uint32_t kOutputSampleRate = 16000;
constexpr uint16_t kOutputChannels = 1;
constexpr uint16_t kOutputBitsPerSample = 16;
constexpr size_t kRingCapacityFrames = 1U << 18; // ~16 s of mono 16 kHz audio

// Lock-light SPSC ring of int16_t samples sized to a power of two.
class SampleRing
{
public:
  explicit SampleRing(size_t capacity) : buffer_(capacity), mask_(capacity - 1) {}

  void push(const int16_t* samples, size_t count)
  {
    std::lock_guard<std::mutex> lock(mutex_);
    for (size_t i = 0; i < count; ++i) {
      buffer_[write_ & mask_] = samples[i];
      ++write_;
      if (write_ - read_ > buffer_.size()) {
        ++read_; // drop oldest on overflow
      }
    }
  }

  size_t available() const
  {
    std::lock_guard<std::mutex> lock(mutex_);
    return write_ - read_;
  }

  size_t pop(int16_t* out, size_t count)
  {
    std::lock_guard<std::mutex> lock(mutex_);
    size_t have = write_ - read_;
    size_t take = std::min(have, count);
    for (size_t i = 0; i < take; ++i) {
      out[i] = buffer_[read_ & mask_];
      ++read_;
    }
    return take;
  }

  size_t drain(int16_t* out, size_t count) { return pop(out, count); }

private:
  std::vector<int16_t> buffer_;
  size_t mask_;
  size_t read_ = 0;
  size_t write_ = 0;
  mutable std::mutex mutex_;
};

// 16 kHz mono signed-16 PCM WAV writer. Patches header sizes on close.
class WavWriter
{
public:
  bool open(const std::string& path)
  {
    file_.open(path, std::ios::binary | std::ios::out | std::ios::trunc);
    if (!file_.is_open()) {
      return false;
    }
    constexpr size_t header_size = 44;
    static const std::array<char, header_size> placeholder{};
    file_.write(placeholder.data(), header_size);
    return true;
  }

  void write_samples(const int16_t* samples, size_t count)
  {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!file_.is_open() || count == 0) {
      return;
    }
    file_.write(reinterpret_cast<const char*>(samples),
                static_cast<std::streamsize>(count * sizeof(int16_t)));
    samples_written_ += count;
  }

  void close()
  {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!file_.is_open()) {
      return;
    }
    finalize_header();
    file_.close();
  }

private:
  static void put_u32_le(uint8_t* dst, uint32_t value)
  {
    dst[0] = static_cast<uint8_t>(value & 0xFFU);
    dst[1] = static_cast<uint8_t>((value >> 8U) & 0xFFU);
    dst[2] = static_cast<uint8_t>((value >> 16U) & 0xFFU);
    dst[3] = static_cast<uint8_t>((value >> 24U) & 0xFFU);
  }

  static void put_u16_le(uint8_t* dst, uint16_t value)
  {
    dst[0] = static_cast<uint8_t>(value & 0xFFU);
    dst[1] = static_cast<uint8_t>((value >> 8U) & 0xFFU);
  }

  void finalize_header()
  {
    const auto data_bytes = static_cast<uint32_t>(samples_written_ * sizeof(int16_t));
    const uint32_t riff_size = 36U + data_bytes;
    const uint32_t byte_rate = kOutputSampleRate * kOutputChannels * (kOutputBitsPerSample / 8U);
    const uint16_t block_align = kOutputChannels * (kOutputBitsPerSample / 8U);

    std::array<uint8_t, 44> header{};
    std::memcpy(header.data() + 0, "RIFF", 4);
    put_u32_le(header.data() + 4, riff_size);
    std::memcpy(header.data() + 8, "WAVE", 4);
    std::memcpy(header.data() + 12, "fmt ", 4);
    put_u32_le(header.data() + 16, 16U); // fmt chunk size for PCM
    put_u16_le(header.data() + 20, 1U);  // audio format = PCM
    put_u16_le(header.data() + 22, kOutputChannels);
    put_u32_le(header.data() + 24, kOutputSampleRate);
    put_u32_le(header.data() + 28, byte_rate);
    put_u16_le(header.data() + 32, block_align);
    put_u16_le(header.data() + 34, kOutputBitsPerSample);
    std::memcpy(header.data() + 36, "data", 4);
    put_u32_le(header.data() + 40, data_bytes);

    file_.seekp(0, std::ios::beg);
    file_.write(reinterpret_cast<const char*>(header.data()),
                static_cast<std::streamsize>(header.size()));
    file_.flush();
  }

  std::ofstream file_;
  size_t samples_written_ = 0;
  std::mutex mutex_;
};

// AVAudioPCMBuffer (Float32, any channel count) -> 16 kHz mono Int16 via
// AVAudioConverter. The returned vector is in interleaved mono Int16.
std::vector<int16_t> convert_to_mono_int16(AVAudioConverter* converter, AVAudioPCMBuffer* input,
                                           AVAudioFormat* output_format)
{
  if (input == nil || converter == nil) {
    return {};
  }
  const double input_rate = input.format.sampleRate;
  const double ratio = output_format.sampleRate / (input_rate > 0 ? input_rate : 1.0);
  AVAudioFrameCount estimated = static_cast<AVAudioFrameCount>(
      static_cast<double>(input.frameLength) * ratio + 32.0);
  if (estimated == 0) {
    estimated = 1;
  }

  AVAudioPCMBuffer* output = [[AVAudioPCMBuffer alloc] initWithPCMFormat:output_format
                                                           frameCapacity:estimated];
  if (output == nil) {
    return {};
  }

  __block BOOL consumed = NO;
  NSError* error = nil;
  AVAudioConverterOutputStatus status =
      [converter convertToBuffer:output
                           error:&error
              withInputFromBlock:^AVAudioBuffer*(AVAudioPacketCount packets,
                                                 AVAudioConverterInputStatus* outStatus) {
                (void)packets;
                if (consumed) {
                  *outStatus = AVAudioConverterInputStatus_NoDataNow;
                  return nil;
                }
                consumed = YES;
                *outStatus = AVAudioConverterInputStatus_HaveData;
                return input;
              }];

  if (status == AVAudioConverterOutputStatus_Error || error != nil) {
    return {};
  }

  const int16_t* data = output.int16ChannelData[0];
  const size_t frames = output.frameLength;
  return std::vector<int16_t>(data, data + frames);
}

class CaptureSession
{
public:
  CaptureSession(CaptureMode mode, const std::string& wav_path)
      : mode_(mode), wav_path_(wav_path), mic_ring_(kRingCapacityFrames),
        sys_ring_(kRingCapacityFrames)
  {
  }

  bool start();
  void stop();

  // pushed from background callbacks
  void on_mic_samples(const int16_t* samples, size_t count) { mic_ring_.push(samples, count); }
  void on_sys_samples(const int16_t* samples, size_t count) { sys_ring_.push(samples, count); }

  bool include_system() const { return mode_ == CaptureMode::All; }

private:
  void mixer_loop();

  CaptureMode mode_;
  std::string wav_path_;
  WavWriter writer_;

  SampleRing mic_ring_;
  SampleRing sys_ring_;

  std::atomic<bool> running_{false};
  std::thread mixer_;

  AVAudioEngine* engine_{nil};
  AVAudioFormat* output_format_{nil};
  AVAudioConverter* mic_converter_{nil};
  AVAudioConverter* sys_converter_{nil};
  SCStream* stream_{nil};
  id sc_delegate_{nil};
};

} // namespace

} // namespace platform::audio

// Objective-C delegate that adapts SCStream audio output into the C++ session.
@interface AsryxSCStreamOutput : NSObject <SCStreamDelegate, SCStreamOutput>
@property(nonatomic, assign) platform::audio::CaptureSession* session;
@property(nonatomic, strong) AVAudioConverter* converter;
@property(nonatomic, strong) AVAudioFormat* outputFormat;
@end

@implementation AsryxSCStreamOutput

- (void)stream:(SCStream*)stream didOutputSampleBuffer:(CMSampleBufferRef)sampleBuffer
                                              ofType:(SCStreamOutputType)type
{
  (void)stream;
  if (type != SCStreamOutputTypeAudio || self.session == nullptr) {
    return;
  }
  if (!CMSampleBufferDataIsReady(sampleBuffer)) {
    return;
  }

  CMFormatDescriptionRef format = CMSampleBufferGetFormatDescription(sampleBuffer);
  if (format == nullptr) {
    return;
  }

  AVAudioFormat* inputFormat =
      [[AVAudioFormat alloc] initWithCMAudioFormatDescription:format];
  if (inputFormat == nil) {
    return;
  }

  const CMItemCount frameCount = CMSampleBufferGetNumSamples(sampleBuffer);
  if (frameCount <= 0) {
    return;
  }

  AVAudioPCMBuffer* inputBuffer =
      [[AVAudioPCMBuffer alloc] initWithPCMFormat:inputFormat
                                     frameCapacity:static_cast<AVAudioFrameCount>(frameCount)];
  if (inputBuffer == nil) {
    return;
  }
  inputBuffer.frameLength = static_cast<AVAudioFrameCount>(frameCount);

  OSStatus copyStatus = CMSampleBufferCopyPCMDataIntoAudioBufferList(
      sampleBuffer, 0, static_cast<int32_t>(frameCount), inputBuffer.mutableAudioBufferList);
  if (copyStatus != noErr) {
    return;
  }

  std::vector<int16_t> samples =
      platform::audio::convert_to_mono_int16(self.converter, inputBuffer, self.outputFormat);
  if (!samples.empty()) {
    self.session->on_sys_samples(samples.data(), samples.size());
  }
}

- (void)stream:(SCStream*)stream didStopWithError:(NSError*)error
{
  (void)stream;
  if (error != nil) {
    std::cerr << "asryx: SCStream stopped with error: "
              << [[error localizedDescription] UTF8String] << "\n";
  }
}

@end

namespace platform::audio {

namespace {

bool CaptureSession::start()
{
  @autoreleasepool {
    if (!writer_.open(wav_path_)) {
      std::cerr << "asryx: failed to open wav for writing: " << wav_path_ << "\n";
      return false;
    }

    output_format_ =
        [[AVAudioFormat alloc] initWithCommonFormat:AVAudioPCMFormatInt16
                                          sampleRate:static_cast<double>(kOutputSampleRate)
                                            channels:kOutputChannels
                                         interleaved:YES];
    if (output_format_ == nil) {
      std::cerr << "asryx: failed to construct output audio format\n";
      return false;
    }

    // Microphone via AVAudioEngine input tap.
    engine_ = [[AVAudioEngine alloc] init];
    AVAudioInputNode* input = engine_.inputNode;
    AVAudioFormat* micFormat = [input outputFormatForBus:0];
    if (micFormat == nil || micFormat.sampleRate <= 0) {
      std::cerr << "asryx: microphone input format unavailable\n";
      return false;
    }

    mic_converter_ = [[AVAudioConverter alloc] initFromFormat:micFormat toFormat:output_format_];
    if (mic_converter_ == nil) {
      std::cerr << "asryx: failed to create mic converter\n";
      return false;
    }

    CaptureSession* self_ptr = this;
    AVAudioConverter* converter = mic_converter_;
    AVAudioFormat* outFormat = output_format_;
    AVAudioNode* input_node = input;
    [input_node installTapOnBus:0
                     bufferSize:1024
                         format:micFormat
                          block:^(AVAudioPCMBuffer* buffer, AVAudioTime* when) {
                            (void)when;
                            std::vector<int16_t> samples =
                                convert_to_mono_int16(converter, buffer, outFormat);
                            if (!samples.empty()) {
                              self_ptr->on_mic_samples(samples.data(), samples.size());
                            }
                          }];

    NSError* startErr = nil;
    if (![engine_ startAndReturnError:&startErr]) {
      std::cerr << "asryx: AVAudioEngine failed to start: "
                << (startErr != nil ? [[startErr localizedDescription] UTF8String] : "unknown")
                << "\n";
      return false;
    }

    // System audio via ScreenCaptureKit (optional).
    if (include_system()) {
      std::cerr << "asryx: fetching ScreenCaptureKit shareable content...\n";
      std::cerr.flush();
      __block NSArray<SCDisplay*>* displays = nil;
      __block NSError* shareErr = nil;
      dispatch_semaphore_t sem = dispatch_semaphore_create(0);
      // Reading sharedContent.displays after the completion fires has been
      // observed to crash on macOS 26+ for ad-hoc signed binaries (pointer
      // authentication failure on the XPC-delivered display array). Copy the
      // displays out of the SCShareableContent into our own retained array
      // inside the completion handler, while the XPC connection is still
      // alive and the autorelease pool that owns the reply data is still
      // valid.
      [SCShareableContent
          getShareableContentExcludingDesktopWindows:NO
                                 onScreenWindowsOnly:NO
                                   completionHandler:^(SCShareableContent* content, NSError* err) {
                                     shareErr = err;
                                     if (content != nil) {
                                       @try {
                                         displays = [content.displays copy];
                                       }
                                       @catch (NSException* ex) {
                                         // leave displays nil; outer checks bail
                                       }
                                     }
                                     dispatch_semaphore_signal(sem);
                                   }];
      long waitResult =
          dispatch_semaphore_wait(sem, dispatch_time(DISPATCH_TIME_NOW, 5LL * NSEC_PER_SEC));
      std::cerr << "asryx: shareable content fetch returned (wait=" << waitResult << ")\n";
      std::cerr.flush();

      if (waitResult != 0) {
        std::cerr << "asryx: ScreenCaptureKit shareable-content fetch timed out (5s)\n";
        return false;
      }

      if (shareErr != nil) {
        std::cerr << "asryx: ScreenCaptureKit fetch failed: "
                  << [[shareErr localizedDescription] UTF8String] << "\n";
        return false;
      }

      if (displays == nil || displays.count == 0) {
        std::cerr << "asryx: ScreenCaptureKit returned no displays (permission likely "
                     "missing; grant Screen Recording for asryx and rerun)\n";
        return false;
      }

      std::cerr << "asryx: got " << displays.count << " display(s)\n";
      std::cerr.flush();
      SCDisplay* display = displays.firstObject;
      if (display == nil) {
        std::cerr << "asryx: ScreenCaptureKit returned nil display\n";
        return false;
      }

      std::cerr << "asryx: constructing content filter...\n";
      std::cerr.flush();
      SCContentFilter* filter =
          [[SCContentFilter alloc] initWithDisplay:display excludingWindows:@[]];

      SCStreamConfiguration* config = [[SCStreamConfiguration alloc] init];
      config.capturesAudio = YES;
      config.excludesCurrentProcessAudio = YES;
      config.sampleRate = 48000;
      config.channelCount = 2;
      // We want audio only; keep video minimal to satisfy the API.
      config.width = 2;
      config.height = 2;
      config.minimumFrameInterval = CMTimeMake(1, 1);
      config.showsCursor = NO;

      AsryxSCStreamOutput* delegate = [[AsryxSCStreamOutput alloc] init];
      delegate.session = this;

      AVAudioFormat* sysFormat =
          [[AVAudioFormat alloc] initWithCommonFormat:AVAudioPCMFormatFloat32
                                            sampleRate:48000
                                              channels:2
                                           interleaved:NO];
      sys_converter_ = [[AVAudioConverter alloc] initFromFormat:sysFormat toFormat:output_format_];
      if (sys_converter_ == nil) {
        std::cerr << "asryx: failed to create system audio converter\n";
        return false;
      }
      delegate.converter = sys_converter_;
      delegate.outputFormat = output_format_;
      sc_delegate_ = delegate;

      stream_ = [[SCStream alloc] initWithFilter:filter configuration:config delegate:delegate];
      NSError* addErr = nil;
      if (![stream_ addStreamOutput:delegate
                                type:SCStreamOutputTypeAudio
                  sampleHandlerQueue:dispatch_queue_create("asryx.sc.audio",
                                                            DISPATCH_QUEUE_SERIAL)
                              error:&addErr])
      {
        std::cerr << "asryx: SCStream addStreamOutput failed: "
                  << (addErr != nil ? [[addErr localizedDescription] UTF8String] : "unknown")
                  << "\n";
        return false;
      }

      __block NSError* startScErr = nil;
      __block BOOL startScOk = NO;
      dispatch_semaphore_t startSem = dispatch_semaphore_create(0);
      [stream_ startCaptureWithCompletionHandler:^(NSError* err) {
        startScErr = err;
        startScOk = (err == nil);
        dispatch_semaphore_signal(startSem);
      }];
      dispatch_semaphore_wait(startSem, dispatch_time(DISPATCH_TIME_NOW, 5LL * NSEC_PER_SEC));

      if (!startScOk) {
        std::cerr << "asryx: SCStream failed to start"
                  << (startScErr != nil ? std::string(": ") +
                                              [[startScErr localizedDescription] UTF8String]
                                        : std::string())
                  << "\n";
        return false;
      }
    }

    running_ = true;
    mixer_ = std::thread([this] { mixer_loop(); });
    return true;
  }
}

void CaptureSession::mixer_loop()
{
  std::vector<int16_t> mic_chunk(2048);
  std::vector<int16_t> sys_chunk(2048);
  std::vector<int16_t> mix_chunk(2048);

  while (running_.load(std::memory_order_acquire)) {
    if (include_system()) {
      size_t available = std::min(mic_ring_.available(), sys_ring_.available());
      if (available == 0) {
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
        continue;
      }
      size_t take = std::min(available, mic_chunk.size());
      size_t mic_got = mic_ring_.pop(mic_chunk.data(), take);
      size_t sys_got = sys_ring_.pop(sys_chunk.data(), take);
      size_t n = std::min(mic_got, sys_got);
      for (size_t i = 0; i < n; ++i) {
        int32_t sum = static_cast<int32_t>(mic_chunk[i]) + static_cast<int32_t>(sys_chunk[i]);
        sum = std::max<int32_t>(INT16_MIN, std::min<int32_t>(INT16_MAX, sum));
        mix_chunk[i] = static_cast<int16_t>(sum);
      }
      writer_.write_samples(mix_chunk.data(), n);
    }
    else {
      size_t available = mic_ring_.available();
      if (available == 0) {
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
        continue;
      }
      size_t take = std::min(available, mic_chunk.size());
      size_t got = mic_ring_.pop(mic_chunk.data(), take);
      writer_.write_samples(mic_chunk.data(), got);
    }
  }

  // Drain whatever's left.
  std::vector<int16_t> tail(4096);
  if (include_system()) {
    while (mic_ring_.available() > 0 && sys_ring_.available() > 0) {
      size_t take = std::min({mic_ring_.available(), sys_ring_.available(), tail.size()});
      std::vector<int16_t> m(take);
      std::vector<int16_t> s(take);
      size_t mg = mic_ring_.pop(m.data(), take);
      size_t sg = sys_ring_.pop(s.data(), take);
      size_t n = std::min(mg, sg);
      for (size_t i = 0; i < n; ++i) {
        int32_t sum = static_cast<int32_t>(m[i]) + static_cast<int32_t>(s[i]);
        sum = std::max<int32_t>(INT16_MIN, std::min<int32_t>(INT16_MAX, sum));
        tail[i] = static_cast<int16_t>(sum);
      }
      writer_.write_samples(tail.data(), n);
    }
  }
  else {
    while (mic_ring_.available() > 0) {
      size_t got = mic_ring_.pop(tail.data(), tail.size());
      writer_.write_samples(tail.data(), got);
    }
  }
}

void CaptureSession::stop()
{
  @autoreleasepool {
    if (!running_.exchange(false)) {
      return;
    }

    if (stream_ != nil) {
      dispatch_semaphore_t sem = dispatch_semaphore_create(0);
      [stream_ stopCaptureWithCompletionHandler:^(NSError* err) {
        (void)err;
        dispatch_semaphore_signal(sem);
      }];
      dispatch_semaphore_wait(sem, dispatch_time(DISPATCH_TIME_NOW, 2LL * NSEC_PER_SEC));
      stream_ = nil;
    }

    if (engine_ != nil) {
      AVAudioNode* input_node = engine_.inputNode;
      [input_node removeTapOnBus:0];
      [engine_ stop];
      engine_ = nil;
    }

    if (mixer_.joinable()) {
      mixer_.join();
    }

    writer_.close();
  }
}

CaptureSession* g_session = nullptr;
std::mutex g_session_mutex;
std::atomic<bool> g_stop_requested{false};

void handle_signal(int) { g_stop_requested.store(true, std::memory_order_release); }

std::string resolve_self_executable()
{
  uint32_t size = 0;
  _NSGetExecutablePath(nullptr, &size);
  if (size == 0) {
    return "";
  }
  std::vector<char> buf(size);
  if (_NSGetExecutablePath(buf.data(), &size) != 0) {
    return "";
  }
  return std::string(buf.data());
}

} // namespace

bool has_screen_recording_permission()
{
  return CGPreflightScreenCaptureAccess() == TRUE;
}

bool request_screen_recording_permission()
{
  return CGRequestScreenCaptureAccess() == TRUE;
}

void open_screen_recording_settings()
{
  (void)platform::run_process_blocking(
      {"open", "x-apple.systempreferences:com.apple.preference.security?Privacy_ScreenCapture"});
}

pid_t spawn_capture(CaptureMode mode, const std::string& wav_path, const std::string& err_path)
{
  std::string self_exe = resolve_self_executable();
  if (self_exe.empty()) {
    return -1;
  }

  std::vector<std::string> argv;
  argv.push_back(self_exe);
  argv.emplace_back("--internal-capture");
  argv.emplace_back(mode == CaptureMode::All ? "all" : "mic");
  argv.push_back(wav_path);

  return platform::spawn_process_background(argv, err_path);
}

int run_capture_child(CaptureMode mode, const std::string& wav_path)
{
  std::cerr << "asryx: capture child started (mode="
            << (mode == CaptureMode::All ? "all" : "mic") << ", wav=" << wav_path << ")\n";
  std::cerr.flush();

  struct sigaction sa{};
  sa.sa_handler = handle_signal;
  sigemptyset(&sa.sa_mask);
  sa.sa_flags = 0;
  ::sigaction(SIGINT, &sa, nullptr);
  ::sigaction(SIGTERM, &sa, nullptr);

  if (mode == CaptureMode::All && !has_screen_recording_permission()) {
    std::cerr << "asryx: Screen Recording permission required for system audio capture.\n";
    open_screen_recording_settings();
    return 1;
  }

  std::cerr << "asryx: starting capture session...\n";
  std::cerr.flush();

  CaptureSession session(mode, wav_path);
  if (!session.start()) {
    std::cerr << "asryx: failed to start capture session\n";
    return 1;
  }

  std::cerr << "asryx: capture session started, awaiting stop signal\n";
  std::cerr.flush();

  {
    std::lock_guard<std::mutex> lock(g_session_mutex);
    g_session = &session;
  }

  while (!g_stop_requested.load(std::memory_order_acquire)) {
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
  }

  session.stop();

  {
    std::lock_guard<std::mutex> lock(g_session_mutex);
    g_session = nullptr;
  }
  return 0;
}

} // namespace platform::audio
