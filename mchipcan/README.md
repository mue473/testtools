# mchipcan

Show Live Traffic:
```
Usage: mchipcan [filename]
If a filename is given, traffic is recorded in candump format in this file.

13:53:17.249  0 0x000B1F18  [5] 00 00 00 4A 01
13:53:17.250  1 0x00091F18  [6] 00 00 00 4A 00 00
13:53:17.251  0 0x000D1F18  [6] 00 00 00 4A 00 01
13:53:35.711  0 0x00301F18  [0]
13:53:35.954  0 <S>  0x280  [5] <RTR>
13:53:35.955  1 <S>  0x280  [5] 00 70 00 00 00
13:53:36.957  0 0x00301F18  [0]
```


Generate different types of CAN-Frames
```
rainer@aspire:~$ echo 00a1F18#00000017 > /tmp/cantx 
rainer@aspire:~$ echo 00a1F18#r4 > /tmp/cantx 
rainer@aspire:~$ echo 118#00000017 > /tmp/cantx 
rainer@aspire:~$ echo 118#r4 > /tmp/cantx
```
Read MCP2510 internal register 
(example: 1C = transmit error counter, 1D = receive error counter)
``` 
rainer@aspire:~$ echo \#1c > /tmp/cantx 
rainer@aspire:~$ echo \#1d > /tmp/cantx 
```
