if not exist .\build mkdir .\build
odin build ./src -debug -out:build/debug.exe
start build/debug.exe