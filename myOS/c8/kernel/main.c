#include "print.h"
#include "init.h"
#include "debug.h"
#include "memory.h"
#include "thread.h"

void k_thread_a(void*);
void k_thread_b(void*);

int main(){
    put_str("I am kernel\n");
    init_all();

//    asm volatile("sti");  

/*    void* addr = get_kernel_pages(5);
    put_str("\n get_kernel_page start vaddr is:");
    put_int((uint32_t)addr);
    put_str("\n");
*/

    thread_start("k_thread_a", 31, k_thread_a, "AAAA ");
    thread_start("k_thread_b", 8, k_thread_b, "BBBB ");
    intr_enable();
    while(1) {
        put_str("Main ");
    }
    return 0;
}

void k_thread_a(void* arg) {
    //用 void* 来通用表示参数，被调用的函数知道自己需要什么类型的参数，自己转换再用
    char* para = arg;
    while(1) {
        put_str(para);
    }
}

void k_thread_b(void* arg) {
    char* para = arg;
    while(1) {
        put_str(para);
    }
}
