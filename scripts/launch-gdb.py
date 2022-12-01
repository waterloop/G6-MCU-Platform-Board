import common
from common import *

devices = get_devices()

for device in devices:
    build_device(device)

targets = common.get_targets()

print("Please choose your target")
for index, target in enumerate(targets):
    print(f"\t{index}: {target}")

selection = int(input())
common.run_gdb(targets[selection])
