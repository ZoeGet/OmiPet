from pathlib import Path
import shutil
import subprocess
import sys

Import("env")

project_dir = Path(env.subst("$PROJECT_DIR"))
build_dir = Path(env.subst("$BUILD_DIR"))
environment_name = env.subst("$PIOENV")
model_source = project_dir / "components" / "esp-sr" / "model"
model_script = model_source / "movemodel.py"
spiffsgen_script = Path(env.subst("$PROJECT_PACKAGES_DIR")) / "framework-espidf" / "components" / "spiffs" / "spiffsgen.py"
generated_config = project_dir / f"sdkconfig.{environment_name}"
default_config = project_dir / "sdkconfig.defaults"
compatibility_config = project_dir / "sdkconfig"
model_root = project_dir / "target"
model_bin = build_dir / "model.bin"

config_source = generated_config if generated_config.exists() else default_config
shutil.copyfile(config_source, compatibility_config)

print("[MODEL] generating ESP-SR model image")
subprocess.run(
    [sys.executable, str(model_script), "-d1", str(project_dir), "-d2", str(project_dir / "components" / "esp-sr")],
    check=True,
)
subprocess.run(
    [
        sys.executable,
        str(spiffsgen_script),
        "0x140000",
        str(model_root),
        str(model_bin),
        "--page-size=256",
        "--obj-name-len=32",
        "--meta-len=4",
        "--use-magic",
        "--use-magic-len",
    ],
    check=True,
)
print(f"[MODEL] generated {model_bin} ({model_bin.stat().st_size} bytes)")

#  将模型镜像加入 PlatformIO 烧录列表 / Add the model image to the PlatformIO flash image list
env.Append(
    FLASH_EXTRA_IMAGES=[
        ("0xc10000", str(model_bin)),
    ]
)
