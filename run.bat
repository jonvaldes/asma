@echo off
compiler.exe program.txt program.bin || goto err
test.exe program.bin
exit /B 0

:err
	echo ====== FAIL ======
	exit /B 1
