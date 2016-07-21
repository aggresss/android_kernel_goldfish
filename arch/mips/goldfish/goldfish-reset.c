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

#include <asm/io.h>
#include <asm/reboot.h>
#include <asm/mips-boards/generic.h>

#define GOLDFISH_RESET      0x1f007000
#define GOSHUTDOWN          0x43

unsigned int __iomem *softres_reg;

static void mips_machine_restart(char *command)
{
	__raw_writel(GORESET, softres_reg);
}

static void mips_machine_halt(void)
{
	__raw_writel(GOSHUTDOWN, softres_reg);
}

static int __init mips_reboot_setup(void)
{
	softres_reg = ioremap(GOLDFISH_RESET, sizeof(unsigned int));

	_machine_restart = mips_machine_restart;
	_machine_halt = mips_machine_halt;
	pm_power_off = mips_machine_halt;

	return 0;
}

arch_initcall(mips_reboot_setup);
