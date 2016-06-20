@echo off
if not exist generated ( mkdir generated )
go run instrs.go || goto err
cl /nologo compiler.cpp || goto err
compiler.exe program.txt program.bin || goto err
cl /nologo test.cpp || goto err
test.exe program.bin
exit /B 0

:err
	echo ====== FAIL ======
	exit /B 1
