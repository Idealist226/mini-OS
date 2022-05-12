;---------------loader and kernel---------------
LOADER_BASE_ADDR equ 0x900
LOADER_START_SECTOR equ 0x2
 
;---------------GDT 描述符属性------------------
DESC_G_4K       equ     1_00000000000000000000000b      ;颗粒度：4K
DESC_D_32       equ     1_0000000000000000000000b       ;操作数和地址大小：32位
DESC_L          equ     0_000000000000000000000b        ;是否是64位代码段：否
DESC_AVL        equ     0_00000000000000000000b         ;不用此位，暂设置为：0
DESC_LIMIT_CODE2 equ    1111_0000000000000000b          ;代码段界限19-16位
DESC_LIMIT_DATA2 equ    DESC_LIMIT_CODE2                ;数据段界限19-16位
DESC_LIMIT_VIDEO2 equ   0000_000000000000000b           ;显存段界限19-16位
DESC_P          equ     1_000000000000000b              ;表示段存在
DESC_DPL_0      equ     00_0000000000000b               ;特权级：0
DESC_DPL_1      equ     01_0000000000000b               ;特权级：1
DESC_DPL_2      equ     10_0000000000000b               ;特权级：2
DESC_DPL_3      equ     11_0000000000000b               ;特权级：3
DESC_S_CODE     equ     1_000000000000b                 ;表示非系统段
DESC_S_DATA     equ     DESC_S_CODE                     ;同上
DESC_S_SYS      equ     0_000000000000b                 ;表示系统段
DESC_TYPE_CODE  equ     1000_00000000b                  ;Type字段-代码段：x=1,c=0,r=0,a=0
DESC_TYPE_DATA  equ     0010_00000000b                  ;Type字段-数据段：x=0,e=0,w=1,a=0
 
DESC_CODE_HIGH4 equ     (0x00<<24) + DESC_G_4K + DESC_D_32 + DESC_L + DESC_AVL + DESC_LIMIT_CODE2 + DESC_P + DESC_DPL_0 + DESC_S_CODE + DESC_TYPE_CODE + 0x00
DESC_DATA_HIGH4 equ     (0x00<<24) + DESC_G_4K + DESC_D_32 + DESC_L + DESC_AVL + DESC_LIMIT_DATA2 + DESC_P + DESC_DPL_0 + DESC_S_DATA + DESC_TYPE_DATA + 0x00
DESC_VIDEO_HIGH4 equ   (0x00<<24) + DESC_G_4K + DESC_D_32 + DESC_L + DESC_AVL + DESC_LIMIT_VIDEO2 + DESC_P + DESC_DPL_0 + DESC_S_DATA + DESC_TYPE_DATA + 0x0B
 
;--------------选择子 属性-------------------
RPL0    equ     00b
RPL1    equ     01b
RPL2    equ     10b
RPL3    equ     11b
TI_GDT  equ     0_00b
TI_LDT  equ     1_00b
