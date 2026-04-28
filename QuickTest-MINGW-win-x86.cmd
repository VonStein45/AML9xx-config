@pushd .
@cd /d "%~dp0"

copy aml9xx-config-mingw-win-x86.exe QuickTest

cd QuickTest
copy .\extlinux\extlinux.conf.testtemplate .\extlinux\extlinux.conf
@rem del .\extlinux\extlinux.conf.bak

aml9xx-config-mingw-win-x86

@popd
@pause