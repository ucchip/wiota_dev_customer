# elec_dev



## 1、介绍

elec_dev 是基于使用DL/T645 2007和Q/GDW 1376.2-2013协议的电力设备的组件功能封装。
通信接口是uart，对外提供组件api接口。
	
	版本：v1.0	2021-6-17
	支持模式：	CCO集中器模式---已支持
				meter模式---待完善
				ctrler模式---待完善

### 1.1 特性

- 调用组件API接口，开发具体应用，比如表模块相位统计功能。
- 支持选择集中器模式，电表模式（单灯控制器），抄控器模式其中一个模式。

### 1.2 目录结构

| 名称     | 说明       |
| -------- | ---------- |
| elec_drv | 驱动层目录，提供通信读写接口   |
| elec_protocol      | 协议层目录，提供协议编解码，组帧，解析数据接口 |
| elec_thread      | 执行层目录，提供任务执行接口，给main调用 |
| elec_cco_api      | 组件层-cco模式目录，提供CCO模式组件接口 |
| elec_meter_api      | 组件层-meter模式目录，提供meter模式组件接口,待完善 |
| elec_ctrler_api      | 组件层-ctrler模式目录，提供ctrler模式组件接口，待完善 |

### 1.3 依赖

- RT-Thread 4.0+

		Hardware Drivers Config  --->
			On-chip Peripheral Drivers  --->
 				[*] Enable GPIO
  				[*] Enable UART  --->

## 2、使用 elec_dev 组件
### 2.1 配置选项
    	RT-Thread Components  --->

        	 electricity_device  --->

        	[*]   USING_ELEC_DEV  (please click y or n)

        		select electricity device mode (ELEC_DEV_CCO_MODE)--->
					 (X) ELEC_DEV_CCO_MODE                                     
                     ( ) ELEC_DEV_METER_MODE                                   
                     ( ) ELEC_DEV_CTRLER_MODE                               
                         *** ELEC_DEV_CCO_MODE-->using as a concentrator ***
                         *** ELEC_DEV_METER_MODE-->using as a meter ***     
                         *** ELEC_DEV_CTRLER_MODE-->using as a controller ***
				(uart1) com name for elec dev (NEW)
				(256) com transfer max len (NEW)
				elec dev uart com baudrate (ELEC_DEV_BAUD_2400)  --->
					(X) ELEC_DEV_BAUD_2400        
                    ( ) ELEC_DEV_BAUD_4800        
                    ( ) ELEC_DEV_BAUD_9600 
                    ( ) ELEC_DEV_BAUD_115200
 				elec dev uart com parity (ELEC_DEV_PARITY_EVEN)  --->
					(X) ELEC_DEV_PARITY_EVEN 
                    ( ) ELEC_DEV_PARITY_NONE
                    ( ) ELEC_DEV_PARITY_ODD



## 3、API 说明

### 3.1 模组初始化

```c
elec_drv.h
	elec_dev_init();	             /* 模块初始化 */
	elec_uart_read();				/*模块读接口*/
	elec_uart_write();				/*模块写接口*/
	elec_eventup_on();				/*事件触发接口---meter模式使用*/
	elec_reset();					/*复位接口*/

elec_dlt645.h
	dlt_645_encode();	             /* 645协议编码 */
	elec_dev_pack_645_frame();	/*将从串口接收到的数据，组包成完整的645数据帧*/
	elec_dev_parse_645_frame();				/*解析645数据帧*/

elec_1376_2.h
	elec_dev_pack_1376_2_frame();/* 将从串口接收到的数据组包成完整的1376.2数据帧 */
	elec_dev_parse_1376_2_frame();	/*解析1376.2数据帧*/
	elec_1376_2_send();				/*发送1376.2数据帧*/

elec_cco_api.h
	elec_cco_init();	/* 集中器模式初始化 */
	elec_cco_recv();	/*集中器模块式接收数据*/
	elec_cco_task();	/*集中器模式任务*/

elec_thread.h
	elec_dev_thread();	/* 电力设备线程 */

```

### 3.2 电力设备注册

```c
elec_dev_thread();			/*注册对应模式的类型，接收，发送接口*/
	#ifdef ELEC_DEV_METER_MODE
    elec_dev_register(&elec_dev, ELEC_DEV_METER, elec_meter_exec_recv, elec_meter_exec_send);
	#endif
	#ifdef ELEC_DEV_CTRLER_MODE
    elec_dev_register(&elec_dev, ELEC_DEV_CTRLER, elec_ctrler_exec_recv, elec_ctrler_exec_send);
	#endif
	#ifdef ELEC_DEV_CCO_MODE
    elec_dev_register(&elec_dev, ELEC_DEV_CCO, elec_cco_init, elec_cco_recv, elec_cco_task);
	#endif

```
	根据需求，完善相应命令字功能，并在对应elec_xxx_api.c的elec_cco_recvfn{}中枚举这些功能,
	比如CCO模式：
```c	
	elec_cco_api.c

	static s_cco_fn elec_cco_recvfn[FN_MAX_NUM] =
	{
    	{GDW1376_2_AFN_CONFIRM_0x00,        elec_cco_recv_confirm},

    	{GDW1376_2_AFN_TRANSFER_0x02,       elec_cco_recv_transfer},

    	{GDW1376_2_AFN_QUERY_0x03,          elec_cco_recv_query},

    	{GDW1376_2_AFN_LINK_CHK_0x04,       RT_NULL},

    	{GDW13766_2_AFN_CTRL_CMD_0x05,      RT_NULL},

    	{GDW1376_2_AFN_ACTIVE_REPROT_0x06,  RT_NULL},

    	{GDW1376_2_AFN_ROUTER_QUERY_0x10,   RT_NULL},

    	{GDW1376_2_AFN_TEST_0xF0,           elec_cco_recv_test},

	};
```
### 3.3 参考示例
	 components/elec_dev_elec_cco_app/wg_phase.c	/*相位读取与上报*/



## 4、联系方式

- 维护：chuanjun.luo@ucchip.com



