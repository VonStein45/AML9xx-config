@pushd .
@cd /d "%~dp0"

PATH=%PATH%;C:\MinGW\bin
@rem unoptimized compile, at time of write approx 45kb exe
@rem C:\MinGW\bin\gcc aml9xx-config.c -o aml9xx-config-mingw-win-x86.exe -static

@rem optimized compile, at time of write approx 22kb exe
C:\MinGW\bin\gcc -Os -s -static -ffunction-sections -fdata-sections -fno-ident -fno-asynchronous-unwind-tables -Wl,--gc-sections aml9xx-config.c -o aml9xx-config-mingw-win-x86.exe

@popd
@pause