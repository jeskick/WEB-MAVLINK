Import("env")
import os
import shutil

def copy_ota_bin(source, target, env):
    firmware = os.path.join(env.subst("$BUILD_DIR"), "firmware.bin")
    out_dir = os.path.join(env.subst("$PROJECT_DIR"), "build")
    out_bin = os.path.join(out_dir, "WEB-Mavlink.bin")
    os.makedirs(out_dir, exist_ok=True)
    shutil.copy2(firmware, out_bin)
    print("OTA bin synced -> build/WEB-Mavlink.bin")

env.AddPostAction("$BUILD_DIR/firmware.bin", copy_ota_bin)
