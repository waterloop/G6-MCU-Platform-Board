# G6 MCU Platform Board

Code repository for the Goose VI general platform board for the electrical architecture.

## Installation

```bash
git clone https://github.com/waterloop/G6-MCU-Platform-Board.git
cd G6-MCU-Platform-Board
```

## Building

Each supported device has its own corresponding directory. To build for a particular device,
pass the device directory into `make` using the `DEV` variable.

For example:

```bash
make DEV=STM32G473CBTx
```

The build will generate the following:
1. Object files contained in `build/objects`
2. A standalone executable in `build/standalone`
3. A static library in `build/platform` to be linked with an application

## Developing for This Repository

### Intellisense and Autocomplete
Intellisense and autocomplete are available through the `clangd` language server. It is supported on
most modern editors.

If you use VSCode, there exists an
[extension](https://marketplace.visualstudio.com/items?itemName=llvm-vs-code-extensions.vscode-clangd)
for it.

`clangd` is already installed on your [vagrant VMs](https://github.com/waterloop/firmware-vagrant).
It also easily installed through apt and brew if you like.

`clangd` uses the auto-generated file `compile_commands.json` to provide intellisense and autocomplete.
This file is gitignored in this repository, so you will need to generate it yourself if you want
intellisense. This can be easily done using `bear`, which is pre-installed on your vagrant VMS.

```
# this is an example, can replace DEV with whatever device you need
bear -- make DEV=STM32G473CBTx
```

### Code

Write your code in the `platform` directory.

You can auto-generate HW initialization code using STM32CubeMX, but only code in the
`platform` directory will be put into the static library. This means that you will have
to wrap (copy and paste, lol) any auto-generated code that you need into the `platform`
directory.

The executable `main.elf` in the `build/standalone` directory - if flashed onto HW - will
run the platform layer by itself (with no application layer). This will prove useful for testing.

Some useful makefile targets for developers include:

```bash
make analyze    # provides an object dump of the standalone executable `main.elf`
make flash      # flash the standalone executable onto HW if an ST-Link programmer is connected
```

### Makefiles

The "main" makefile is in the project's root directory. It simply a wrapper to call other makefiles,
which are located in the device directories.

The `BUILD_DIR` variable is passed into the device makefiles, which do the actual building.

You can use the existing makefiles as a template of sorts if you ever need to write a makefile
to support a new device. 

