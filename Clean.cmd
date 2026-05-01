@pushd .
@cd /d "%~dp0"

@rem MINGW
del aml9xx-config-mingw-win-x86.exe
@ZIG
del AML9xx-config-arm64
del AML9xx-config-linux-x64
del AML9xx-config-win-x86.exe

del AML9xx-config-macos-arm64
del AML9xx-config-macos-x86_64

cd QuickTest
del AML9xx-config-win-x86.exe
del .\extlinux\extlinux.conf
del .\extlinux\extlinux.conf.bak
del u-boot.ext

@popd
@pause