from __future__ import annotations

import json
import re
from pathlib import Path


EXPECTED_COMMAND_IDS = {
    "wake": 1,
    "increase": 2,
    "decrease": 3,
}


def parse_command_file(command_file: Path) -> list[tuple[int, str]]:
    commands: list[tuple[int, str]] = []
    seen_phrases: set[str] = set()
    for line_number, raw_line in enumerate(
        command_file.read_text(encoding="utf-8").splitlines(), start=1
    ):
        line = raw_line.strip()
        if not line:
            continue
        fields = line.split(maxsplit=1)
        if len(fields) != 2 or not fields[0].isdigit():
            raise ValueError(
                f"{command_file}:{line_number}: expected '<id> <pinyin phrase>'"
            )
        command_id = int(fields[0])
        phrase = fields[1].strip()
        if command_id not in EXPECTED_COMMAND_IDS.values():
            raise ValueError(
                f"{command_file}:{line_number}: unsupported command id {command_id}"
            )
        if not phrase:
            raise ValueError(f"{command_file}:{line_number}: empty phrase")
        if phrase in seen_phrases:
            raise ValueError(f"{command_file}:{line_number}: duplicate phrase '{phrase}'")
        seen_phrases.add(phrase)
        commands.append((command_id, phrase))

    command_ids = {command_id for command_id, _ in commands}
    missing_ids = set(EXPECTED_COMMAND_IDS.values()) - command_ids
    if missing_ids:
        raise ValueError(f"{command_file}: missing command ids {sorted(missing_ids)}")
    wake_phrase_count = sum(current_id == EXPECTED_COMMAND_IDS["wake"]
                            for current_id, _ in commands)
    if wake_phrase_count != 1:
        raise ValueError(
            f"{command_file}: expected exactly one wake phrase for id 1, "
            f"found {wake_phrase_count}"
        )
    return commands


def parse_runtime_ids(runtime_header: Path) -> dict[str, int]:
    source = runtime_header.read_text(encoding="utf-8")
    result: dict[str, int] = {}
    for name, symbol in (
        ("wake", "kWakePhraseCommandId"),
        ("increase", "kIncreaseBrightnessCommandId"),
        ("decrease", "kDecreaseBrightnessCommandId"),
    ):
        match = re.search(
            rf"constexpr\s+int\s+{symbol}\s*=\s*(\d+)\s*;", source
        )
        if match is None:
            raise ValueError(f"{runtime_header}: missing {symbol}")
        result[name] = int(match.group(1))
    return result


def validate_commands(command_file: Path, runtime_header: Path) -> list[tuple[int, str]]:
    commands = parse_command_file(command_file)
    runtime_ids = parse_runtime_ids(runtime_header)
    if runtime_ids != EXPECTED_COMMAND_IDS:
        raise ValueError(
            f"{runtime_header}: command ids {runtime_ids} do not match "
            f"expected {EXPECTED_COMMAND_IDS}"
        )
    if len(set(runtime_ids.values())) != len(runtime_ids):
        raise ValueError(f"{runtime_header}: command ids must be unique")
    return commands


def write_generated_header(
    commands: list[tuple[int, str]], generated_header: Path
) -> None:
    wake_phrases = [phrase for command_id, phrase in commands if command_id == 1]
    lines = [
        "#pragma once",
        "",
        "#include <cstddef>",
        "",
        "namespace OmiPetAudio {",
        "",
        "struct GeneratedCommandPhrase {",
        "  int commandId;",
        "  const char* phonemes;",
        "};",
        "",
        "constexpr GeneratedCommandPhrase kGeneratedCommandPhrases[] = {",
    ]
    lines.extend(
        f"    {{{command_id}, {json.dumps(phrase, ensure_ascii=False)}}},"
        for command_id, phrase in commands
    )
    lines.extend(
        [
            "};",
            "",
            "constexpr size_t kGeneratedCommandPhraseCount =",
            "    sizeof(kGeneratedCommandPhrases) / sizeof(kGeneratedCommandPhrases[0]);",
            f"constexpr char kGeneratedWakePhrasePinyin[] = {json.dumps(wake_phrases[0], ensure_ascii=False)};",
            "",
            "}  //  OmiPetAudio 命名空间 / OmiPetAudio namespace",
            "",
        ]
    )
    generated_header.write_text("\n".join(lines), encoding="utf-8", newline="\n")


def check_and_generate(
    command_file: Path, runtime_header: Path, generated_header: Path
) -> None:
    commands = validate_commands(command_file, runtime_header)
    generated_header.parent.mkdir(parents=True, exist_ok=True)
    write_generated_header(commands, generated_header)
    wake_phrase = next(phrase for command_id, phrase in commands if command_id == 1)
    print(
        f"[MODEL] command consistency ok ids={sorted({command_id for command_id, _ in commands})} "
        f"phrases={len(commands)} wake='{wake_phrase}'"
    )


if __name__ == "__main__":
    project_dir = Path(__file__).resolve().parents[1]
    check_and_generate(
        project_dir / "scripts" / "multinet_commands_cn.txt",
        project_dir / "include" / "multinet_command_recognizer.h",
        project_dir / "include" / "generated_multinet_commands.h",
    )
