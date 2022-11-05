import common

targets = common.get_targets()

print("Please choose your target")
for index, target in enumerate(targets):
    print(f"\t{index}: {target}")

selection = int(input())
common.run_gdb(targets[selection])
