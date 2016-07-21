/* arch/mips/goldfish/goldfish-reset.c
 *
 * Goldfish Machine Reset Routines
 *
 * Copyright (C) 2007 Google, Inc.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#include <linux/init.h>
#include <linux/pm.h>
#include <linux/io.h>
#include <linux/of.h>

#include <asm/idle.h>
#include <asm/reboot.h>
#include <asm/mips-boards/generic.h>

#define GORESET           0x42
#define GOSHUTDOWN        0x43

static struct device_node *dn;
static uint32_t base;
static unsigned int __iomem *softres_reg;

static void mips_machine_restart(char *command)
{
	writel(GORESET, softres_reg);
}

static void mips_machine_halt(void)
{
	writel(GOSHUTDOWN, softres_reg);
}

static int __init mips_reboot_setup(void)
{
	if ((dn = of_find_node_by_name(NULL, "goldfish_reset")) == NULL) {
		printk(KERN_WARNING "mips_reboot_setup() failed to "
			"fetch device node \'goldfish_reset\'!\n");
		return -1;
	}

	if (of_property_read_u32(dn, "reg", &base) < 0) {
		printk(KERN_WARNING "mips_reboot_setup() failed to "
			"fetch device base address property \'reg\'! %x\n", base);
		return -1;
	}

	softres_reg = ioremap(base, sizeof(unsigned int));

	_machine_restart = mips_machine_restart;
	_machine_halt = mips_machine_halt;
	pm_power_off = mips_machine_halt;

	return 0;
}

arch_initcall(mips_reboot_setup);
