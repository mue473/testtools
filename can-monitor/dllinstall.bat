REM Kopiert die benötigten Npcap-DLLs zu den EXEs

IF EXIST %windir%\SysWow64 (
	echo 64-Bit-System
	copy %windir%\System32\Npcap\*.dll x64\Debug
	copy %windir%\SysWow64\Npcap\*.dll Debug
	copy x64\Debug\*.dll x64\Release
	copy Debug\*.dll Release
) ELSE (
	echo 32-Bit-System
	copy %windir%\System32\Npcap\*.dll Debug
	copy Debug\*.dll Release
)
