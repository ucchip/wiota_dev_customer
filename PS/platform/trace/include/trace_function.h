#ifndef TRACE_FUNCTION_H_
#define TRACE_FUNCTION_H_
 
//hight 16 bit + low 16 bit => module id << 16  + log macro id.

 

enum trace_function_ucchip
{
  TRACE_FUNCTION_UCCHIP_ID = 0x16,
  VSENDDESCTOIP = 0x160001,
  IP_SN_RDY_IND = 0x160002,
};

enum trace_function_network
{
  TRACE_FUNCTION_NETWORK_ID = 0x17,
  VDHCPPROCESS_DISCOVER = 0x170001,//vDHCPProcess: discover
  PRVIPTASK_STARTED = 0x170002,//prvIPTask started
  ARP_IS_DISABLED = 0x170003,//ARP is disabled!
  FREERTOS_IPINIT_XNETWORKBUFFERINIT = 0x170004,//FreeRTOS_IPInit: xNetworkBuffersInitialise() failed
  FREERTOS_IPINIT_XNETWORK_EVENT_QUEUE = 0x170005, //"FreeRTOS_IPInit:Network event queue could not be created"
  FREERTOS_SELECT_INTERRUPTE = 0x170006,//FreeRTOS_select: interrupted
  PRVFINDSELECTEDSOCKET_FAIL = 0x170007,//prvFindSelectedSocket: failed
  FREERTOS_BIND_SENDEVENT_FIAL = 0x170008,//FreeRTOS_bind: send event failed
  VSOCKET_NO_ADDR = 0x170009,//vSocketBind: Socket no addr
  FREERTOS_CLOSESOCKET_FIAL = 0x17000a,//FreeRTOS_closesocket: failed
  SET_SO_WIN_PROP_WRONG_SOCKET = 0x17000b,//Set SO_WIN_PROP: wrong socket type
  SET_SO_WIN_PROP_BUFFER_ALREADY_CREATE = 0x17000c,//Set SO_WIN_PROP: buffer already created
  PRVTCPCREATESTREAM_MALLOC_FAIL = 0x17000d, //prvTCPCreateStream: malloc failed
  LUS_TCP_NOT_INITIALIZED = 0x17000e, //PLUS-TCP not initialized
  PROT_PORT_IP_REMOTE_PORT_RT_STATE = 0x17000f,//Prot Port IP-Remote:Port  R/T Status Alive tmout Child
  PRVTCPRETURNPACKET_DUPLICATE_FAIL = 0x170010,//prvTCPReturnPacket:duplicate failed
  VTCPSTATECHANGE_CLOSING_SOCKET = 0x170011, //vTCPStateChange: Closing socket
  ESYN_RECEIVED_ACK_EXPECTED_NO_SYN_PEER_MISS = 0x170012, //eSYN_RECEIVED: ACK expected, not SYN: peer missed our SYN+ACK
  TCP_LISTEN_NEW_SOCKET_FAIL = 0x170013, //TCP: Listen: new socket failed
  TCP_LISTEN_NEW_SOCKET_BIND_ERROR = 0x170014, //TCP: Listen: new socket bind error
  
};

 
#endif
