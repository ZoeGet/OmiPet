#include "voice_controller.h"

#include "buzzer.h"
#include "omi_pet_ui.h"

namespace OmiPetVoice {

bool VoiceController::begin() {
  state_ = VoiceState::Idle;
  acknowledgementPhase_ = AcknowledgementPhase::None;
  acknowledgementPhaseStartedAtMs_ = 0;
  lastActivityAtMs_ = millis();
  initialized_ = true;
  OmiPetUi::setVoiceStatus("IDLE");
  return true;
}

void VoiceController::update(bool speechActive) {
  if (!initialized_) {
    return;
  }

  const uint32_t nowMs = millis();
  updateAcknowledgement(nowMs);

  if (state_ != VoiceState::ListeningForCommand) {
    return;
  }

  if (speechActive) {
    lastActivityAtMs_ = nowMs;
    return;
  }

  if (nowMs - lastActivityAtMs_ >= kCommandListenTimeoutMs) {
    Serial.println("[VOICE] command timeout");
    enterIdle();
  }
}

bool VoiceController::notifyWakeWordDetected() {
  if (!initialized_ || state_ != VoiceState::Idle) {
    return false;
  }

  const uint32_t nowMs = millis();
  state_ = VoiceState::ListeningForCommand;
  acknowledgementPhase_ = AcknowledgementPhase::HighTone;
  acknowledgementPhaseStartedAtMs_ = nowMs;
  lastActivityAtMs_ = nowMs;
  if (!OmiPetBuzzer::buzzer.startTone(
          kWakeAcknowledgementHighFrequencyHz,
          kWakeAcknowledgementHighDurationMs)) {
    Serial.println("[VOICE] high acknowledgement tone failed");
  }
  OmiPetUi::setVoiceStatus("LISTENING");
  Serial.println("[VOICE] wake accepted, acknowledgement=high, listening for command");
  return true;
}

bool VoiceController::notifyCommandCompleted() {
  if (!initialized_ || state_ != VoiceState::ListeningForCommand) {
    return false;
  }

  Serial.println("[VOICE] command completed");
  enterIdle();
  return true;
}

void VoiceController::cancel() {
  if (!initialized_ || state_ == VoiceState::Idle) {
    return;
  }

  Serial.println("[VOICE] interaction cancelled");
  enterIdle();
}

VoiceState VoiceController::state() const {
  return state_;
}

bool VoiceController::listeningForCommand() const {
  return state_ == VoiceState::ListeningForCommand;
}

void VoiceController::updateAcknowledgement(uint32_t nowMs) {
  if (acknowledgementPhase_ == AcknowledgementPhase::None) {
    return;
  }

  if (acknowledgementPhase_ == AcknowledgementPhase::HighTone &&
      nowMs - acknowledgementPhaseStartedAtMs_ >=
          kWakeAcknowledgementHighDurationMs) {
    acknowledgementPhase_ = AcknowledgementPhase::LowTone;
    acknowledgementPhaseStartedAtMs_ = nowMs;
    Serial.println("[VOICE] acknowledgement=low");
    if (!OmiPetBuzzer::buzzer.startTone(
            kWakeAcknowledgementLowFrequencyHz,
            kWakeAcknowledgementLowDurationMs)) {
      Serial.println("[VOICE] low acknowledgement tone failed");
      acknowledgementPhase_ = AcknowledgementPhase::None;
    }
    return;
  }

  if (acknowledgementPhase_ == AcknowledgementPhase::LowTone &&
      nowMs - acknowledgementPhaseStartedAtMs_ >=
          kWakeAcknowledgementLowDurationMs) {
    acknowledgementPhase_ = AcknowledgementPhase::None;
    Serial.println("[VOICE] acknowledgement=complete");
  }
}

void VoiceController::enterIdle() {
  OmiPetBuzzer::buzzer.stop();
  state_ = VoiceState::Idle;
  acknowledgementPhase_ = AcknowledgementPhase::None;
  acknowledgementPhaseStartedAtMs_ = 0;
  lastActivityAtMs_ = millis();
  OmiPetUi::setVoiceStatus("IDLE");
}

VoiceController voice;

}  //  OmiPetVoice 命名空间 / OmiPetVoice namespace
