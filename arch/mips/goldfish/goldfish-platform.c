/* arch/mips/mach-goldfish/goldfish-platform.c
**
** Copyright (C) 2007 Google, Inc.
**
** This software is licensed under the terms of the GNU General Public
** License version 2, as published by the Free Software Foundation, and
** may be copied, distributed, and modified under those terms.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
*/

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/platform_device.h>
#include <linux/delay.h>
#include <linux/of_platform.h>
#include <linux/of_fdt.h>
#include <linux/libfdt.h>

#include <linux/bootmem.h>

#include <mach/hardware.h>
#include <mach/irq.h>
#include <asm/io.h>
#include <asm/bootinfo.h>
#include <asm/mips-boards/generic.h>
#include <asm/prom.h>

#define HIGHMEM_OFFSET (0x20000000)

void __init prom_init(void)
{
	char *cmdline = (char *)fw_arg0;
	strcpy(arcs_cmdline, cmdline);
}

void prom_free_prom_memory(void)
{
}

#ifdef CONFIG_64BIT
#define GOLDFISH_TTY_PUT_CHAR (*(volatile unsigned int *)0xffffffffbf002000)
#else
#define GOLDFISH_TTY_PUT_CHAR (*(volatile unsigned int *)0xbf002000)
#endif

void prom_putchar(int c)
{
	GOLDFISH_TTY_PUT_CHAR = c;
}

const char *get_system_type(void)
{
	return "MIPS-Goldfish";
}

void __init plat_mem_setup(void)
{
	/*
	 * Load the built-in or appended device tree.
	 */
#ifdef CONFIG_MIPS_APPENDED_DTB
	pr_info("Try setup using Appended DTB\n");
	/*
	 * This causes the "memory" node to be
	 * parsed resulting in our memory appearing
	 */
	__dt_setup_arch(__appended_dtb);
#else
	unsigned int ramsize = fw_arg1;
	unsigned int highmem_ramsize = fw_arg2;

	/*
	 * Since RAM size is adjustable property in QEMU
	 * do not rely on the built-in device tree information
	 * for setting up memory, do it according to information
	 * passed through arguments fw_arg1 & fw_arg2.
	 */
	add_memory_region(0x0, ramsize, BOOT_MEM_RAM);
	if (highmem_ramsize)
		add_memory_region(HIGHMEM_OFFSET, highmem_ramsize, BOOT_MEM_RAM);

	pr_info("Try setup using Built-in DTB\n");
	__dt_setup_arch(__dtb_start);
#endif
}

void __init device_tree_init(void)
{
	unflatten_and_copy_device_tree();
}

static struct of_device_id __initdata goldfish_ids[] = {
	{ .compatible = "simple-bus", },
	{},
};

int __init plat_of_setup(void)
{
	if (!of_have_populated_dt())
		panic("device tree not present");

	return of_platform_populate(NULL, goldfish_ids, NULL, NULL);
}

arch_initcall(plat_of_setup);
