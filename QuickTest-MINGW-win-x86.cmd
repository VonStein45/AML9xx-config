@pushd .
@cd /d "%~dp0"

copy AML9xx-config-win-x86.exe QuickTest

cd QuickTest
copy .\extlinux\extlinux.conf.testtemplate .\extlinux\extlinux.conf
del .\extlinux\extlinux.conf.bak

AML9xx-config-win-x86

@popd
@pause