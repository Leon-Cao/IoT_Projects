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
	"candy-lock.c" 
	"blecent.c")
	
set(public_include_dirs
	"." 
	"sesame" 
	"utils"
)
	
idf_component_register(SRCS "${srcs}"
                    INCLUDE_DIRS "${public_include_dirs}"
					)
