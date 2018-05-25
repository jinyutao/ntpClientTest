# ntpClientTest

./ntp_test_client [ntp server ip]</br>
Default ip is "182.92.12.11". It's ntp1.aliyun.com

For example:
```
$ ./ntp_test_client 182.92.12.11

send to ip: 182.92.12.11
local Transmit Timestamp: 2018-05-25 05:52:32.816965 2023B2DE-449E24D1

Remote NTP time
 Reference Timestamp -- : 2018-05-25 05:51:52.816965 F822B2DE-449E24D1
 Originate Timestamp(T1): 2018-05-25 05:52:32.816965 2023B2DE-449E24D1
 Receive Timestamp  (T2): 2018-05-25 05:52:32.828075 2023B2DE-A9B4FCD3
 Transmit Timestamp (T3): 2018-05-25 05:52:32.828095 2023B2DE-FD04FED3

Loacl receive time(T4): 2018-05-25 05:52:32.846380

Offset time form NTP-server to local ((T2-T1)+(T3-T4))/2: = -3587us
Synced time = 2018-05-25 05:52:32.842793

recvfrom() = 48. Data is [OK]
exit
```
