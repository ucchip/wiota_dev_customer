# 1 文档范围

&emsp;&emsp;本手册详细介绍了UC8288 WIOTA终端和基站侧的AT指令使用示例。





# 2 调试工具介绍

* 串口调试工具用SSCOM V5.13.1

​		首先打开COM3打开，然后一定要把DTR和加回车换行勾上

![图标](images\serialtool.PNG)

​	波特率要选择115200，然后就可以发送AT命令了。





# 3 使用示例




| 功能           | 终端                                                         | 基站                                                         | 备注                                                         |
| -------------- | :----------------------------------------------------------- | :----------------------------------------------------------- | ------------------------------------------------------------ |
| 初始化         | at+wiotainit<br/>OK<br>at+wiotafreq=135<br/>OK<br>at+wiotauserid=99B980F2,2<br/>OK<br>at+wiotadcxo=2f000<br/>OK<br>at+wiotarun=1<br/>OK<br> | at+wiotainit<br/>OK<br>at+wiotafreq=135<br/>OK<br>at+wiotadcxo=14000<br/>OK<br>at+wiotarun=1<br/>OK | 频点设置必须一致<br>每个板子都有不同的频偏，需要用仪器测出<br>基站启动成功以后，终端才能连接并发送数据 |
| 连接并发送数据 | at+wiotaconnect=1,1<br/>OK<br/>at+wiotasend<br/>><br/>A123456789012345678<br/><br/>SEND OK<br/>OK | +WIOTARECV,0x99b980f2,21,A123456789012345678                 | SEND OK 代表已经发送成功，基站成功接收到终端发送的信息并打印处理 |
| 关闭           | at+wiotaconnect=0,0<br>OK<br>at+wiotarun=0<br/>OK            | at+wiotaconnect=0,0<br/>OK<br>at+wiotarun=0<br/>OK           |                                                              |

