#pragma once

#include <Arduino.h>

namespace OmiPetVoice {

//  唤醒词名称和中文音译 / Wake word name and Chinese pronunciation
constexpr char kWakeWordPhrase[] = "Hey Omi";
constexpr char kWakeWordPronunciation[] = "嘿！欧咪";

//  唤醒确认音配置 / Wake acknowledgement tone configuration
constexpr uint32_t kWakeAcknowledgementHighFrequencyHz = 2600;
constexpr uint32_t kWakeAcknowledgementLowFrequencyHz = 1800;
constexpr uint32_t kWakeAcknowledgementHighDurationMs = 110;
constexpr uint32_t kWakeAcknowledgementLowDurationMs = 70;
constexpr uint32_t kWakeAcknowledgementSilenceDurationMs = 25;
constexpr uint32_t kCommandListenTimeoutMs = 5000;

enum class VoiceState : uint8_t {
  Idle,
  ListeningForCommand,
};

enum class AcknowledgementPhase : uint8_t {
  None,
  LowTone,
  SilenceGap,
  HighTone,
};

//  管理唤醒后的语音交互状态 / Manage the voice interaction state after wake-up
class VoiceController {
 public:
  //  初始化语音交互状态机 / Initialize the voice interaction state machine
  bool begin();

  //  更新确认音序列和唤醒后的语音超时 / Update the acknowledgement sequence and post-wake timeout
  void update(bool speechActive);

  //  通知状态机检测到唤醒词 / Notify the state machine that a wake word was detected
  bool notifyWakeWordDetected();

  //  通知状态机命令处理完成 / Notify the state machine that command processing is complete
  bool notifyCommandCompleted();

  //  取消当前语音交互 / Cancel the current voice interaction
  void cancel();

  //  获取当前语音交互状态 / Get the current voice interaction state
  VoiceState state() const;

  //  查询是否正在等待用户指令 / Check whether a user command is expected
  bool listeningForCommand() const;

 private:
  void updateAcknowledgement(uint32_t nowMs);
  void enterIdle();

  VoiceState state_ = VoiceState::Idle;
  AcknowledgementPhase acknowledgementPhase_ = AcknowledgementPhase::None;
  uint32_t acknowledgementPhaseStartedAtMs_ = 0;
  uint32_t lastActivityAtMs_ = 0;
  bool initialized_ = false;
};

extern VoiceController voice;

}  //  OmiPetVoice 命名空间 / OmiPetVoice namespace
