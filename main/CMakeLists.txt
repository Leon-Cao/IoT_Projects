set(srcs
	"sesame/ssm.c" 
	"sesame/ssm_cmd.c" 
	"utils/aes-cbc-cmac.c" 
	"utils/c_ccm.c" 
	"utils/TI_aes_128.c" 
	"utils/uECC.c" 
	"utils/utils.c"
	"utils/misc.c"
	"utils/peer.c"
	"iotGateway/cmd_iot_gateway.c"
	"iotGateway/blecent.c"
	"iotGateway/wifi_station.c"
	"iotGateway/iot_gateway_main.c"
	"iot_gateway.c")
	
set(public_include_dirs
	"." 
	"sesame" 
	"utils"
	"iotGateway"
)
	
idf_component_register(SRCS "${srcs}"
                    INCLUDE_DIRS "${public_include_dirs}"
					)
