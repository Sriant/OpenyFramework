# OpenyFramework
    主要参考 zephyr 将部分功能简化裁剪出来，用于裸机(Bare-Metal)开发的一个轻量级框架。
    主要功能，足以应付大部分中小型的开发：
        1、timer + work 处理任务。提交工作项时，不存在内存拷贝；
        2、msgq 实现自定义事件队列。提交事件时，存在内存拷贝；
        3、环形缓冲区 ringbuffer。
        4、层次状态机 k_hsm。
# Guidance
    提供 port 的实现：
    配置文件：k_config.h
        K_CONFIG_TICKLESS_KERNEL                TICKLESS 功能开关
        K_CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC    开启 TICKLESS 时，等于定时器的主频。否则无效。
        K_CONFIG_SYS_CLOCK_TICKS_PER_SEC        开启 TICKLESS 时，用户自定义。否则必须 = 1000；
   
    日志调试：k_log.h、k_assert.h
        void k_print(int level, const char *fmt, ...)  weak 函数，可重写。
    
    原子操作、中断屏蔽、定时器等：port.c
        TICKLESS 开启时提供以下实现，否则无需理会
            sys_clock_isr()
            sys_clock_set_timeout()
            sys_clock_elapsed()
        TICKLESS 关闭时，在 1ms 中断加入 sys_clock_announce(1) 即可。