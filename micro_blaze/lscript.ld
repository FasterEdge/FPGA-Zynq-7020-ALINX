/* lscript.ld — MicroBlaze 链接脚本（Zynq-7020 ALINX vivado 工程默认内存映射）
 * 本地数据/代码内存：PL 侧 BRAM（vivado 工程的 axi_bram_ctrl + local memory）。
 * 若工程使用不同大小的 microblaze_local_memory，请同步调整 LENGTH。 */
_STACK_SIZE = 0x400;

MEMORY
{
    microblaze_local_memory (rxwx) : ORIGIN = 0x00000050, LENGTH = 0x0000FB00
}

ENTRY(_start)

SECTIONS
{
    .text : {
        *(.text)
        *(.text.*)
    } > microblaze_local_memory

    .rodata : {
        *(.rodata)
        *(.rodata.*)
    } > microblaze_local_memory

    .data : {
        *(.data)
        *(.data.*)
        __data_start = .;
        *(.sdata)
        *(.sdata.*)
        __data_end = .;
    } > microblaze_local_memory

    .bss (NOLOAD) : {
        __bss_start = .;
        *(.bss)
        *(.bss.*)
        *(.sbss)
        *(.sbss.*)
        __bss_end = .;
    } > microblaze_local_memory

    .heap : {
        __heap_start = .;
        . = ORIGIN(microblaze_local_memory) + LENGTH(microblaze_local_memory) - _STACK_SIZE;
        __heap_end = .;
    } > microblaze_local_memory

    .stack : {
        __stack_top = ORIGIN(microblaze_local_memory) + LENGTH(microblaze_local_memory);
    } > microblaze_local_memory
}
