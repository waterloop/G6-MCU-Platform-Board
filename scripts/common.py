import subprocess
import os
import re
import platform

def get_devices():
    pattern = "./STM32[A-Z0-9]*x$"
    sub_directories = [x[0].split('/')[1] for x in os.walk(".") if re.match(pattern, x[0])]
    return sub_directories

def build_device(device):
    subprocess.run(f"make DEV={device}", shell=True, check=True)

def clean():
    subprocess.run("make clean", shell=True, check=True)

def get_targets():
    pattern = "./build/STM32[A-Z0-9]*x/standalone"
    targets = [[f"{x[0]}/{file}" for file in x[2] if re.match(".*\.elf", file)] for x in os.walk(".") if re.match(pattern, x[0])]
    output = []
    for target in targets:
        output = [*output, *target]
    return output

def run_gdb(target):
    gdb_exe = {
        "Linux": "gdb-multiarch",
        "Darwin": "arm-none-eabi-gdb",
        "Windows": "arm-none-eabi-gdb",
    }
    gdb_exe = gdb_exe[platform.system()]
    subprocess.run(f"{gdb_exe} -ex 'target extended-remote :3333' -ex 'file {target}' -ex 'y' -ex 'load' -ex 'b main' -ex 'c'", shell=True, check=True)
