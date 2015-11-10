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
#include <linux/irqchip.h>
#include <linux/of.h>
#include <linux/of_address.h>

#include <mach/hardware.h>
#include <mach/irq.h>
#include <asm/io.h>
#include <asm/irq_cpu.h>
#include <asm/setup.h>

#define GOLDFISH_PIC_STATUS       0x00 // number of pending interrupts
#define GOLDFISH_PIC_NUMBER       0x04
#define GOLDFISH_PIC_DISABLE_ALL  0x08
#define GOLDFISH_PIC_DISABLE      0x0c
#define GOLDFISH_PIC_ENABLE       0x10

static struct irq_domain *goldfish_pic_domain;
static void __iomem *goldfish_pic_base;

void goldfish_mask_irq(struct irq_data *d)
{
	writel(d->irq-GOLDFISH_IRQ_BASE,
	       goldfish_pic_base + GOLDFISH_PIC_DISABLE);
}

void goldfish_unmask_irq(struct irq_data *d)
{
	writel(d->irq-GOLDFISH_IRQ_BASE,
	       goldfish_pic_base + GOLDFISH_PIC_ENABLE);
}

static struct irq_chip goldfish_irq_chip = {
	.name	= "goldfish",
	.irq_mask	= goldfish_mask_irq,
	.irq_mask_ack = goldfish_mask_irq,
	.irq_unmask = goldfish_unmask_irq,
};

void goldfish_irq_dispatch(void)
{
	uint32_t irq;
	/*
	 * Disable all interrupt sources
	 */
	irq = readl(goldfish_pic_base + GOLDFISH_PIC_NUMBER);
	do_IRQ(GOLDFISH_IRQ_BASE+irq);
}

void goldfish_fiq_dispatch(void)
{
	panic("goldfish_fiq_dispatch");
}

asmlinkage void plat_irq_dispatch(void)
{
	unsigned int pending = read_c0_cause() & read_c0_status() & ST0_IM;

	if (pending & CAUSEF_IP2)
		goldfish_irq_dispatch();
	else if (pending & CAUSEF_IP3)
		goldfish_fiq_dispatch();
	else if (pending & CAUSEF_IP7)
		do_IRQ(MIPS_CPU_IRQ_BASE + 7);
	else
		spurious_interrupt();
}

static struct irqaction cascade = {
	.handler	= no_action,
	.flags      = IRQF_NO_THREAD,
	.name		= "cascade",
};

static void mips_timer_dispatch(void)
{
	do_IRQ(MIPS_CPU_IRQ_BASE + MIPS_CPU_IRQ_COMPARE);
}

static int goldfish_pic_map(struct irq_domain *d, unsigned int irq, irq_hw_number_t hw)
{
	struct irq_chip *chip = &goldfish_irq_chip;

	if (hw < GOLDFISH_IRQ_BASE)
		return 0;

	irq_set_chip_and_handler(hw, chip, handle_level_irq);

	return 0;
}

static const struct irq_domain_ops irq_domain_ops = {
	.xlate = irq_domain_xlate_onetwocell,
	.map = goldfish_pic_map,
};

int __init goldfish_pic_of_init(struct device_node *node, struct device_node *parent)
{
	struct resource res;

	if (of_address_to_resource(node, 0, &res))
		panic("Failed to get icu memory range");

	if (request_mem_region(res.start, resource_size(&res),
				res.name) < 0)
		pr_err("Failed to request icu memory");

	goldfish_pic_base = ioremap_nocache(res.start, resource_size(&res));

	if (!goldfish_pic_base)
		panic("Failed to remap icu memory");

	/*
	 * Disable all interrupt sources
	 */
	writel(1, goldfish_pic_base + GOLDFISH_PIC_DISABLE_ALL);

	mips_cpu_irq_init();

	if (cpu_has_vint) {
		pr_info("Setting up vectored interrupts\n");
		set_vi_handler(MIPS_CPU_IRQ_PIC, goldfish_irq_dispatch);
		set_vi_handler(MIPS_CPU_IRQ_PIC, goldfish_fiq_dispatch);
	}

	setup_irq(MIPS_CPU_IRQ_BASE+MIPS_CPU_IRQ_PIC, &cascade);
	setup_irq(MIPS_CPU_IRQ_BASE+MIPS_CPU_IRQ_FIQ, &cascade);

	if (cpu_has_vint)
		set_vi_handler(MIPS_CPU_IRQ_COMPARE, mips_timer_dispatch);

	goldfish_pic_domain = irq_domain_add_linear(node, NR_IRQS, &irq_domain_ops, 0);

	return 0;
}

void __init arch_init_irq(void)
{
	irqchip_init();
}

static const struct of_device_id irqchip_of_match_goldfish_pic
__used __section(__irqchip_of_table)
= { .compatible = "generic,goldfish-pic", .data = goldfish_pic_of_init };
