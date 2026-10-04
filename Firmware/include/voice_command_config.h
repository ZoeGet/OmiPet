#pragma once

#include <cstddef>

namespace OmiPetAudio {

struct VoiceCommandPhrase {
  int commandId;
  const char* phonemes;
};

//  个人语音命令配置入口；修改或新增命令后直接编译即可自动同步词表并打包模型 / Personal voice-command configuration entry; edit or add commands here, then build to synchronize the phrase list and package the model automatically
constexpr VoiceCommandPhrase kVoiceCommandPhrases[] = {
    {1, "lao shu fei fei"},  //  自定义唤醒词 / Custom wake phrase
    {2, "liang yi dian"},  //  增加亮度 / Increase brightness
    {2, "diao liang yi dian"},  //  增加亮度同义表达 / Increase-brightness synonym
    {2, "zeng jia liang du"},  //  增加亮度同义表达 / Increase-brightness synonym
    {2, "ba deng tiao liang"},  //  增加亮度同义表达 / Increase-brightness synonym
    {2, "deng tai an le diao liang"},  //  增加亮度同义表达 / Increase-brightness synonym
    {3, "an yi dian"},  //  降低亮度 / Decrease brightness
    {3, "diao an yi dian"},  //  降低亮度同义表达 / Decrease-brightness synonym
    {3, "jiang di liang du"},  //  降低亮度同义表达 / Decrease-brightness synonym
    {3, "ba deng tiao an"},  //  降低亮度同义表达 / Decrease-brightness synonym
    {3, "deng tai liang le diao an"},  //  降低亮度同义表达 / Decrease-brightness synonym
    {4, "hong se"},  //  红色 / Red
    {4, "ba deng bian hong"},  //  红色同义表达 / Red synonym
    {5, "lv se"},  //  绿色 / Green
    {5, "ba deng bian lv"},  //  绿色同义表达 / Green synonym
    {6, "lan se"},  //  蓝色 / Blue
    {6, "ba deng bian lan"},  //  蓝色同义表达 / Blue synonym
    {7, "huang se"},  //  黄色 / Yellow
    {8, "zi se"},  //  紫色 / Purple
    {9, "qing se"},  //  青色 / Cyan
    {10, "bai se"},  //  白色 / White
    {11, "cai hong"},  //  彩虹动效 / Rainbow effect
    {11, "cai hong deng"},  //  彩虹动效同义表达 / Rainbow-effect synonym
    {12, "hu xi deng"},  //  呼吸动效 / Breathing effect
    {12, "hu xi deng guang"},  //  呼吸动效同义表达 / Breathing-effect synonym
    {13, "zhui zhu deng"},  //  左右往返扫描动效 / Left-to-right sweep effect
    {13, "pao ma deng"},  //  左右往返扫描同义表达 / Left-to-right sweep synonym
    {14, "chang liang"},  //  常亮模式 / Solid mode
    {14, "jing tai deng"},  //  常亮模式同义表达 / Solid-mode synonym
    {15, "guan bi deng"},  //  关闭灯带 / Turn off the strip
    {15, "guan deng"},  //  关闭灯带同义表达 / Turn-off synonym
    {16, "zhong xin kuo san deng"},  //  中心扩散动效 / Center-expand effect
    {16, "cong zhong jian xiang wai"},  //  中心向外扩散同义表达 / Center-outward expansion synonym
};

constexpr size_t kVoiceCommandPhraseCount =
    sizeof(kVoiceCommandPhrases) / sizeof(kVoiceCommandPhrases[0]);

}  //  OmiPetAudio 命名空间 / OmiPetAudio namespace
