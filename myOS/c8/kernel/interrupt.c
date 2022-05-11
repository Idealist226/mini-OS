#include "interrupt.h"
#include "stdint.h"
#include "global.h"
#include "print.h"
#include "io.h"

#define PIC_M_CTRL      0X20    //主片的控制端口是0x20
#define PIC_M_DATA      0X21    //主片的数据端口是0x21
#define PIC_S_CTRL      0Xa0    //从片的控制端口是0xa0
#define PIC_S_DATA      0xa1    //从片的数据端口是0xa1

#define IDT_DESC_CNT    0X21    //目前总共支持的中断数量

#define EFLAGS_IF               0x00000200      //eflags寄存器中的if位为1
#define GET_EFLAGS(EFLAG_VAR)   asm volatile("pushfl; popl %0" : "=g"(EFLAG_VAR))

/*中断描述结构体*/
struct gate_desc {
    uint16_t    func_offset_low_word;
    uint16_t    selector;
    uint8_t     dcount;
    uint8_t     attribute;
    uint16_t    func_offset_high_word;
};

//静态函数声明
static void make_idt_desc(struct gate_desc* p_gdesc, uint8_t attr, intr_handler function);
static struct gate_desc idt[IDT_DESC_CNT];          //idt 本质上就是个中断门描述符数组

extern intr_handler intr_entry_table[IDT_DESC_CNT]; //intr_handler 实际上是 void* 在 interrupt.h 里定义的

char* intr_name[IDT_DESC_CNT];              //用于保存异常的名字
intr_handler idt_table[IDT_DESC_CNT];       //用于保存处理程序地址

/* 初始化可编程中断控制器 8259A */
static void pic_init(void) {
    //初始化主片
    outb(PIC_M_CTRL, 0x11);         //ICW1: 0001 0001 ,边沿触发，级联 8259，需要ICW4
    outb(PIC_M_DATA, 0x20);         //ICW2: 0010 0000 ,起始中断向量号为 0x20(0x20-0x27)
    outb(PIC_M_DATA, 0x04);         //ICW3: 0000 0100 ,IR2 接从片
    outb(PIC_M_DATA, 0x01);         //ICW4: 0000 0001 ,8086 模式，正常EOI

    //初始化从片
    outb(PIC_S_CTRL, 0x11);         //ICW1: 0001 0001 ,边沿触发，级联 8259，需要ICW4
    outb(PIC_S_DATA, 0x28);         //ICW2: 0010 1000 ,起始中断向量号为 0x28(0x28-0x2f)
    outb(PIC_S_DATA, 0x02);         //ICW3: 0000 0010 ,设置连接到主片的 IR2 引脚
    outb(PIC_S_DATA, 0x01);         //ICW4: 0000 0001 ,8086 模式，正常EOI

    //打开主片上IR0，也就是目前只接受时钟产生的中断
    //eflags 里的 IF 位对所有外部中断有效，但不能屏蔽某个外设的中断了
    outb(PIC_M_DATA, 0xfe);
    outb(PIC_S_DATA, 0xff);

    put_str("   pic_init done\n");
}

/* 创建中断门描述符 */
//参数：中断描述符，属性，中断处理函数地址
//功能：向中断描述符填充属性和地址
static void make_idt_desc(struct gate_desc* p_gdesc, uint8_t attr, intr_handler function) {
    p_gdesc->func_offset_low_word = (uint32_t)function & 0x0000FFFF;
    p_gdesc->selector = SELECTOR_K_CODE;
    p_gdesc->dcount = 0;
    p_gdesc->attribute = attr;
    p_gdesc->func_offset_high_word = ((uint32_t)function & 0xFFFF0000) >> 16;
}

/*初始化中断描述符表*/
static void idt_desc_init(void) {
    int i;
    for (i = 0; i < IDT_DESC_CNT; i++) {
        make_idt_desc(&idt[i], IDT_DESC_ATTR_DPL0, intr_entry_table[i]);
    }
    put_str("   idt_desc_init done\n");
}

/*通用的中断处理函数，一般用在异常出现时的处理*/
static void general_intr_handler(uint8_t vec_nr) {
    if (vec_nr == 0x27 || vec_nr == 0x2f) {
        // IRQ7 IRQ15 会产生伪中断，无需处理
        // 0x2f 是从片 8259A 上的最后一个 IRQ 引脚，保留项
        return ;
    }
    set_cursor(0);
    int cursor_pos = 0;
    while (cursor_pos < 320) {
        put_char(' ');
        ++cursor_pos;
    }

    set_cursor(0);      //重置光标为屏幕左上角
    put_str("---------------Exception message:\n");
    set_cursor(88);     //从第 2 行第 8 个字符开始打印
    put_str(intr_name[vec_nr]);

    if (vec_nr == 14) {
        // Pagefault，打印缺失的地址
        int page_fault_vaddr = 0;
        asm ("movl %%cr2, %0" : "=r" (page_fault_vaddr));
        put_str("\nPage fault address is: ");
        put_int(page_fault_vaddr);
    }

    put_str("\n---------------Exception message end.\n");
    // 不再继续向下执行，以便于查看异常信息
    while (1);
}

/* 在中断处理程序数组第 vec_no 个元素中注册安装中断处理程序 handler */
void register_handler(uint8_t vec_no, intr_handler handler) {
    idt_table[vec_no] = handler;
}

/*完成一般中断处理函数注册及异常名称注册*/
static void exception_init(void) {
    int i;
    for (i = 0; i < IDT_DESC_CNT; i++) {
        // idt_table 数组中的函数是在进入中断后根据中断向量号调用的
        // 见 kernel.S 的 call [idt_table = %1*4]
        idt_table[i] = general_intr_handler;    // 以后用register_handler 来注册具体的处理函数
        intr_name[i] = "unknown";
    }
    intr_name[0] = "#DE Divide Error";
    intr_name[1] = "#DB Debug Exception";
    intr_name[2] = "NMI Interrupt";
    intr_name[3] = "#BP Breakpoint Exception";
    intr_name[4] = "#OF Overflow Exception";
    intr_name[5] = "#BR BOUND Range Exceeded Exception"; 
    intr_name[6] = "#UD Invalid Opcode Exception"; 
    intr_name[7] = "#NM Device No七 Available Exception"; 
    intr_name[8] = "JIDF Double Fault Exception";
    intr_name[9] = "Coprocessor Segment Overrun";
    intr_name[10] = "#TS Invalid TSS Exception"; 
    intr_name[11] = "#NP Segment Not Present";
    intr_name[12] = "#SS Stack Fault Exception";
    intr_name[13] = "#GP General Protection Exception"; 
    intr_name[14] = "#PF Page-Fault Exception";
    // intr_name[l5]第15项是intel保留项，未使用
    intr_name[16] = "#MF x87 FPU F'loating-Point Error"; 
    intr_name[17] = "#AC Alignment Check Exception"; 
    intr_name[18] = "#MC Machine-Check Exception"; 
    intr_name[19] = "#XF SIMD Floating-Point Exception";
}

/*开中断并返回开中断前的状态*/
enum intr_status intr_enable(){
    enum intr_status old_status;
    if(INTR_ON == intr_get_status()) {
        old_status = INTR_ON;
        return old_status;
    } else {
        old_status = INTR_OFF;
        asm volatile("sti");            //开中断，sti 将 IF 位置 1
        return old_status;
    }
}
 
/*关中断并返回关中断前的状态*/
enum intr_status intr_disable(){
    enum intr_status old_status;
    if(INTR_ON == intr_get_status()) {
        old_status = INTR_ON;
        asm volatile("cli":::"memory"); //关中断，cli 将 IF 位置 0
        return old_status;
    } else {
        old_status = INTR_OFF;
        return old_status;
    }
}
 
/*将中断状态设置位 status*/
enum intr_status intr_set_status(enum intr_status status){
    return status & INTR_ON ? intr_enable() : intr_disable();
}

/*获取当前中断状态*/
enum intr_status intr_get_status(){
    uint32_t eflags = 0;
    GET_EFLAGS(eflags);
    return (EFLAGS_IF & eflags) ? INTR_ON : INTR_OFF;
}

/*完成有关中断的所有初始化工作*/
void idt_init() {
    put_str("idt_init start\n");
    idt_desc_init();        //初始化中断描述符表
    exception_init();       //异常名初始化并注册通常的中断处理程序
    pic_init();             //初始化 8259A

    /*加载 idt */
    uint64_t idt_operand = ((sizeof(idt) - 1) | ((uint64_t)((uint32_t)idt << 16)));
    asm volatile("lidt %0"::"m"(idt_operand));
    put_str("idt_init done\n");
}
