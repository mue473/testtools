# CAN-Monitor
is a can traffic analyzer version usable for Windows, based on Gerd's version at
https://github.com/GBert/railroad/tree/master/can-monitor
Files are identical as much as possible, only necessary changes are done.
Note that the "linuxinc" directory must be reachable for successful compilation.
Using Visual Studio 2022, it is possible to generate 32bit version as well as 
64bit version, both will run on 64bit systems.

The generated executable has to be put in a directory with some DLLs:
- packet.dll and wpcap.dll for network packet analysis, taken from the wireshark 
        project  (https://www.wireshark.org/)
- zlib1.dll for decompression, taken from zlib project (https://www.zlib.net/)
The DDLs have to fit the corresponding architecture.

Note that for live IP traffic analysation the npcap service must be active.
This is normally the case if wireshark is installed on the machine, otherwise
it is available from  https://npcap.com/ as npcap-x.xx.exe

