%include "boot.inc"
section loader vstart=LOADER_BASE_ADDR
LOADER_STACK_TOP     equ     LOADER_BASE_ADDR

jmp loader_start

;构建 GDT 及其内部的描述符
GDT_BASE:
    dd      0x00000000
    dd      0x00000000

CODE_DESC:
    dd      0x0000FFFF
    dd      DESC_CODE_HIGH4

DATA_STACK_DESC:
    dd      0x0000FFFF
    dd      DESC_DATA_HIGH4

VIDEO_DESC:
    dd      0x80000007          ;limit=(0xbffff - 0xb8000)/4k = 7
    dd      DESC_VIDEO_HIGH4    ;此时dpl为0

GDT_SIZE        equ     $ - GDT_BASE    ;获取 GDT 大小
GDT_LIMIT       equ     GDT_SIZE - 1    ;获取 段界限
 
times   60      dq      0       ;预留60个空位，为以后填入中断描述符表和任务状态段TSS描述符留空间
                                ;times 60 表示后面的内容循环60次，是nasm提供的伪指令

SELECTOR_CODE   equ     (0x0001 << 3) + TI_GDT + RPL0   ;相当于(CODE_DESC - GDT_BASE) / 8 + TI_GDT + RPL
SELECTOR_DATA   equ     (0x0002 << 3) + TI_GDT + RPL0   ;相当于(DATA_STACK_DESC - GDT_BASE) / 8 + TI_GDT + RPL
SELECTOR_VIDEO  equ     (0x0003 << 3) + TI_GDT + RPL0   ;相当于(VIDEO_DESC - GDT_BASE) / 8 + TI_GDT + RPL

;以下是 gdt 指针，前2字节是gdt界限，后4字节是gdt起始地址
gdt_ptr dw      GDT_LIMIT
        dd      GDT_BASE

loadermsg db '2 loader in real.'

;---------------------------------------------------------
;INT 0x10       功能号:0x13     功能描述符:打印字符串
;---------------------------------------------------------
;输入:
;AH 子功能号=13H
;BH = 页码
;BL = 属性（若AL=00H或01H）
;CX = 字符串长度
;(DH,DL)=坐标(行，列)
;ES:BP=字符串地址
;AL=显示输出方式
;0——字符串中只含显示字符，其显示属性在BL中。显示后，光标位置不变
;1——字符串中只含显示字符，其显示属性在BL中。显示后，光标位置改变
;2——字符串中只含显示字符和显示属性。显示后，光标位置不变。
;3——字符串中只含显示字符和显示属性。显示后，光标位置改变。
;无返回值

loader_start:
    ;显示字符串,表示当前在实模式
    mov sp, LOADER_BASE_ADDR    
    mov bp, loadermsg       ;ES:BP 字符串地址
    mov cx, 17              ;字符串长度
    mov ax, 0x1301          ;AH=13h，AL=01h
    mov bx, 0x001f          ;页号为0（BH=0h），蓝底粉红字（BL=1fh）
    mov dx, 0x1800
    int 0x10                ;int 10 BIOS中断

    ;准备进入保护模式
    ;1.打开A20地址现
    in al, 0x92
    or al, 0000_0010B
    out 0x92, al

    ;2.加载GDT
    lgdt [gdt_ptr]

    ;3.cr0第0位置1
    mov eax, cr0
    or eax, 0x00000001
    mov cr0, eax

    jmp dword SELECTOR_CODE:p_mode_start    ;刷新流水线
                                            ;流水线是CPU的工作方式，会把当前指令和后面的几个指令同时放在流水线中重叠执行，由于之前的代码是16位，接下来的代码变成32位了，指令按照16位译码会出错，通过刷新流水线可以解决这个问题

[bits 32]
p_mode_start:
    mov ax, SELECTOR_DATA
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov esp, LOADER_STACK_TOP
    mov ax, SELECTOR_VIDEO
    mov gs, ax

    mov byte [gs:160], 'P'

    jmp $
