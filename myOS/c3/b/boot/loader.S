%include "boot.inc"
SECTION loader vstart=LOADER_BASE_ADDR

;输出背景色绿色,前景色红色,并且跳动的字符串"1 MBR"
mov byte [gs:0x00], 'F'
mov byte [gs:0x01], 0xA4

mov byte [gs:0x02], 'O'
mov byte [gs:0x03], 0xA4

mov byte [gs:0x04], 'R'
mov byte [gs:0x05], 0xA4

mov byte [gs:0x06], ' '
mov byte [gs:0x07], 0xA4

mov byte [gs:0x08], 'Z'
mov byte [gs:0x09], 0xA4

mov byte [gs:0x0a], 'G'
mov byte [gs:0x0b], 0xA4

mov byte [gs:0x0c], 'R'
mov byte [gs:0x0d], 0xA4

jmp $
