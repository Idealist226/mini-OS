TI_GDT  equ     0
RPL0    equ     0
 
SELECTOR_VIDEO  equ     (0x0003 << 3) + TI_GDT + RPL0
 
[bits 32]
section .text
;---------------------------------------------------
; Function: put_str
;
; Description: 通过put_char来打印以0字符结尾的字符串
;---------------------------------------------------
global put_str
put_str:
        ;只用到了ebx和ecx，只备份这两个寄存器
        push ebx
        push ecx
        xor ecx, ecx
        mov ebx, [esp + 12]     ;获取待打印的地址
.goon:
        mov cl, [ebx]
        cmp cl, 0
        jz .str_over
        push ecx                ;put_char 参数
        call put_char
        add esp, 4
        inc ebx
        jmp .goon
.str_over:
        pop ecx
        pop ebx
        ret

;-------------------------------------------
; Function: put_char
;
; Description: 把栈中的1个字符写入光标所在处
;-------------------------------------------
global put_char
put_char:
        pushad  ;备份32位寄存器环境
 
        ;保险起见，每次打印都给 gs 寄存器赋值，因为要操作硬件，这里初始化gs的时候手动把DPL置零来执行内核代码
        mov ax, SELECTOR_VIDEO
        mov gs, ax
 
; 获取当前光标位置
;--------------------
        ;先设置索引寄存器的值选中要用的数据寄存器，然后从相应的数据寄存器进行读写操作
        ;高8位
        mov dx, 0x03d4  ;索引寄存器
        mov al, 0x0e    ;提供索引光标位置的高8位
        out dx, al
        mov dx, 0x03d5  ;通过读写数据端口获取/设置光标位置
        in  al, dx      ;得到光标位置高8位
        mov ah, al
 
        ;低8位
        mov dx, 0x03d4
        mov al, 0x0f
        out dx, al
        mov dx, 0x03d5
        in  al, dx
 
        ;将光标存入 bx
        mov bx, ax
 
        ;在栈中获取待打印字符
        mov ecx, [esp + 36]     ;pushad 压入 4*8=32 字节，主调函数返回地址 4 字节，所以 +36
        
        ;判断是不是控制字符
        cmp cl, 0x0d            ;CR 是 0x0d 回车符
        jz .is_carriage_return
        cmp cl, 0x0a            ;LF 是 0x0a 换行符
        jz .is_line_feed
        cmp cl, 0x8             ;BS 是 0x08 退格符
        jz .is_backspace
 
        jmp .put_other
;-------------------

; 退格键处理
.is_backspace:
        dec bx          ;bx 是下一个可字符的位置，减 1 则指向当前字符
        shl bx, 1       ;光标值 *2 就是光标在显存中的相对地址
 
        mov byte [gs:bx], 0x20  ;将待删除的字节补为0，低字节是 ascii 码
        inc bx
        mov byte [gs:bx], 0x07  ;将待删除的字节属性设置为0x07，高字节是属性
        shr bx, 1               ;还原光标值，删除掉的字符本身就是下一个可打印字符的光标位置
        jmp .set_cursor

;输入字符处理
.put_other:
        shl bx, 1
        
        mov [gs:bx], cl         ;cl里存放的是待打印的 ascii 码
        inc bx
        mov byte [gs:bx], 0x07  ;字符属性
        shr bx, 1
        inc bx
        
        cmp bx, 2000
        jl .set_cursor          ;若光标值小于2000，则没有写满，则设置新的光标值，反之则换行

; 换行/回车处理
.is_line_feed:
.is_carriage_return:
; \r \n 都按 \n处理，光标切换到下一行的行首
; 这里的处理是：将光标移动到当前行首
        xor dx, dx              ;dx是被除数的高 16 位
        mov ax, bx              ;ax是被除数的低 16 位
        mov si, 80
        div si                  ;光标位置除 80 的余数便是取整
        sub bx, dx              ;dx里存放的是余数
 
.is_carriage_return_end:
; 将光标移动到下一行的同位置
        add bx, 80
        cmp bx, 2000
 
.is_line_feed_end:
        jl .set_cursor

;滚屏处理
.roll_screen:
        ;先将1-24行搬运到0-23行里
        cld
        mov ecx, 960            ;2000-80=1920，1920*2=3840 个字节要搬运，一次搬运4字节，搬运 3840/4=960 次
        mov esi, 0xc00b80a0     ;第1行行首
        mov edi, 0xc00b8000     ;第0行行首
        rep movsd
 
        ;将第24行填充为空白
        mov ebx, 3840
        mov ecx, 80
 
.cls:
        mov word [gs:ebx], 0x0720       ;0x0720是黑底白字的空格
        add ebx, 2
        loop .cls
        mov bx, 1920                    ;将光标重置到24行行首
 
;设置光标
.set_cursor:
        ;先设置高8位
        mov dx, 0x03d4                  ;索引寄存器
        mov al, 0x0e                    ;光标高8位
        out dx, al

        mov dx, 0x03d5
        mov al, bh
        out dx, al

        ;再设置低8位
        mov dx, 0x03d4
        mov al, 0x0f
        out dx, al

        mov dx, 0x03d5
        mov al, bl
        out dx, al

.put_char_done:
        popad
        ret
