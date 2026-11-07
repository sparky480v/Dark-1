# Building DARK-1 1.0.5 (your computer, WSL Ubuntu)

The firmware file (.fwsc) needs JieLi's compiler and SDK, which can only be downloaded from your own machine. Everything else is ready.

1. Open WSL Ubuntu and go to the project: `cd ~/DARK-1-102/DARK-1`
2. Copy in the files from this package (they replace the old ones).
3. Install the tools once:
   - `pip3 install Pillow`
   - `tools/get_toolchain.sh`
   - `git clone --depth 1 --branch AC79NN_SDK_V1.2.1_2023-12-13 https://gitee.com/Jieli-Tech/fw-AC79_AIoT_SDK.git ~/fw-AC79_AIoT_SDK`
4. Build the package: `tools/release.sh 1.0.5 --no-tests`
   (leave out `--no-tests` to run the tests first; a few older tests, fxedit, genre and pluckmig and three in amb_test, were already failing before this version and look out of date.)
5. The file to flash is `build/DARK-1-1.0.5.fwsc`. Flash it with the hub's installer.
6. Try it first in the emulator: open `DARK-1-1.0.5-emulator.html` in Chrome or Edge.

What is new is listed in `docs/CHANGELOG.md` under 1.0.5.
