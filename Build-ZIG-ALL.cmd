@pushd .
@cd /d "%~dp0"

@PATH=%PATH%;C:\zig

@set SRC=AML9xx-config.c
@set OPTS=-O ReleaseSmall -fstrip -fsingle-threaded -lc --color off

@echo Building Windows x86... (32-bit x86) - static and small
@zig build-exe %SRC% -target x86-windows-gnu %OPTS% --name AML9xx-config-win-x86

@echo Building Linux x64 (Static musl)...
@zig build-exe %SRC% -target x86_64-linux-musl %OPTS% --name AML9xx-config-linux-x64

@echo Building ARM64 (Static musl)...
@zig build-exe %SRC% -target aarch64-linux-musl %OPTS% --name AML9xx-config-arm64

@echo Done!
@popd
@pause