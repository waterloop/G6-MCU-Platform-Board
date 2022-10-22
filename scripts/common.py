import subprocess
import os
import re

def get_devices():
    pattern = "./STM32[A-Z0-9]*x$"
    sub_directories = [x[0] for x in os.walk(".") if re.match(pattern, x[0])]
    return sub_directories

def build_device(device):
    subprocess.run(f"make DEV={device}", shell=True, check=True)

def clean():
    subprocess.run("make clean", shell=True, check=True)
