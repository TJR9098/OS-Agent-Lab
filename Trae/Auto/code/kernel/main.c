#include "printf.h"
#include "trap.h"
#include "timer.h"
#include "panic.h"
#include "riscv.h"
#include "mem.h"
#include "vm.h"
#include "memlayout.h"
#include "sbi.h"


// 外部符号声明
extern char boot_stack_top[];
extern char kernel_stack_top[];

// 手动实现字符串比较
static int strcmp(const char *s1, const char *s2) {
    while (*s1 && *s2 && *s1 == *s2) {
        s1++;
        s2++;
    }
    return *s1 - *s2;
}

// 测试copyin/copyout
static void test_copyin_out(void) {
    kprintf("\nvm: testing copyin/copyout\n");
    
    // 创建用户页表
    pagetable_t user_pagetable = pagetable_create();
    
    // 分配用户内存
    uint64_t user_va = 0x1000;
    uint64_t user_pa = (uint64_t)kalloc();
    if (!user_pa) {
        panic("test_copyin_out: kalloc failed");
    }
    
    // 映射用户内存
    map_pages(user_pagetable, user_va, user_pa, PAGE_SIZE, USER_PERM);
    
    // 测试数据
    char src_data[] = "Hello, copyin/copyout!";
    char dst_data[sizeof(src_data)];
    uint64_t len = sizeof(src_data);
    
    // 测试copyout
    int ret = copyout(user_pagetable, user_va, src_data, len);
    if (ret != 0) {
        panic("test_copyin_out: copyout failed");
    }
    kprintf("vm: copyout passed\n");
    
    // 测试copyin
    ret = copyin(user_pagetable, dst_data, user_va, len);
    if (ret != 0) {
        panic("test_copyin_out: copyin failed");
    }
    kprintf("vm: copyin passed\n");
    
    // 验证数据
    if (strcmp(src_data, dst_data) != 0) {
        panic("test_copyin_out: data mismatch");
    }
    kprintf("vm: data verification passed\n");
    
    // 清理
    unmap_pages(user_pagetable, user_va, 1, 1);
    pagetable_destroy(user_pagetable);
    
    kprintf("vm: copyin/copyout test completed\n");
}

// 内核主函数
void kernel_main(uint64_t hartid) {
    // 打印banner
    kprintf("AgentOS RV64 - OpenSBI boot\n");
    
    // 内存初始化
    kinit(DRAM_BASE, DRAM_SIZE);
    
    // 打印hartid
    kprintf("hartid: %ld\n", hartid);
    
    // 验证栈指针
    uint64_t sp = (uint64_t)__builtin_frame_address(0);
    kprintf("sp: 0x%lx\n", sp);
    
    // 检查栈对齐
    if (sp % 16 != 0) {
        panic("bad stack alignment: sp=%lx", sp);
    }
    
    // 初始化trap
    trap_init();
    
    // 初始化定时器
    timer_init();
    
    // 启用trap
    trap_enable();
    
    // 先tick5次
    kprintf("\n=== Waiting for 2 ticks ===\n");
    uint64_t tick_count = 0;
    uint64_t last_tick = 0;
    
    while (tick_count < 2) {
        uint64_t current_tick = timer_get_tick();
        if (current_tick != last_tick) {
            kprintf("current tick: %ld\n", current_tick);
            last_tick = current_tick;
            tick_count++;
        }
        asm volatile ("wfi");
    }
    
    // Stage 4: 虚拟内存测试
    kprintf("\n=== Stage 4: Virtual Memory Test ===\n");
    
    // 创建内核页表
    kprintf("vm: creating kernel pagetable\n");
    pagetable_t kernel_pagetable = kernel_pagetable_create();
    
    // satp前自检
    kprintf("vm: running satp precheck\n");
    satp_precheck(kernel_pagetable);
    
    // 切换地址空间
    kprintf("vm: switching satp\n");
    switch_satp(kernel_pagetable);
    
    // satp回归测试
    kprintf("vm: satp regression test\n");
    kprintf("vm: satp enabled, still running\n");
    
    // 测试copyin/copyout
    test_copyin_out();
    
    // 运行虚拟内存自测框架
    vm_selftest();
    
    // 运行全面的虚拟内存测试套件
    extern void vm_test_all(void);
    vm_test_all();

    
    // 测试完成
    kprintf("\n=== Stage 4: All Tests Completed ===\n");
    sbi_shutdown();
}
