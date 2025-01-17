/*
 * (C) Copyright 2002
 * Sysgo Real-Time Solutions, GmbH <www.elinos.com>
 * Marius Groeger <mgroeger@sysgo.de>
 *
 * Copyright (C) 2001  Erik Mouw (J.A.K.Mouw@its.tudelft.nl)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307	 USA
 *
 */

#include <common.h>
#include <command.h>
#include <image.h>
#include <u-boot/zlib.h>
#include <asm/byteorder.h>
#include <fdt.h>
#include <libfdt.h>
#include <fdt_support.h>
#include <fastboot.h>
#include <asm/arch/clock.h>
#include <asm/arch/drv_display.h>
#ifdef CONFIG_ALLWINNER
#include <boot_type.h>
#include <axp_power.h>
#include <sunxi_board.h>
#include <pmu.h>
#endif
#include <sys_config.h>
DECLARE_GLOBAL_DATA_PTR;

#if defined (CONFIG_SETUP_MEMORY_TAGS) || \
    defined (CONFIG_CMDLINE_TAG) || \
    defined (CONFIG_INITRD_TAG) || \
    defined (CONFIG_SERIAL_TAG) || \
    defined (CONFIG_REVISION_TAG)
static void setup_start_tag (bd_t *bd);

# ifdef CONFIG_SETUP_MEMORY_TAGS
static void setup_memory_tags (bd_t *bd);
# endif
static void setup_commandline_tag (bd_t *bd, char *commandline);

# ifdef CONFIG_INITRD_TAG
static void setup_initrd_tag (bd_t *bd, ulong initrd_start,
			      ulong initrd_end);
# endif
static void setup_end_tag (bd_t *bd);

static struct tag *params;
#endif /* CONFIG_SETUP_MEMORY_TAGS || CONFIG_CMDLINE_TAG || CONFIG_INITRD_TAG */

static ulong get_sp(void);
#if defined(CONFIG_OF_LIBFDT)
static int bootm_linux_fdt(int machid, bootm_headers_t *images);
#endif

void arch_lmb_reserve(struct lmb *lmb)
{
	ulong sp;

	/*
	 * Booting a (Linux) kernel image
	 *
	 * Allocate space for command line and board info - the
	 * address should be as high as possible within the reach of
	 * the kernel (see CONFIG_SYS_BOOTMAPSZ settings), but in unused
	 * memory, which means far enough below the current stack
	 * pointer.
	 */
	sp = get_sp();
	debug("## Current stack ends at 0x%08lx ", sp);

	/* adjust sp by 1K to be safe */
	sp -= 1024;
	lmb_reserve(lmb, sp,
		    gd->bd->bi_dram[0].start + gd->bd->bi_dram[0].size - sp);
}
static void announce_and_cleanup(void)
{
	axp_set_next_poweron_status(0x0e);
	board_display_wait_lcd_open();		//add by jerry
	board_display_set_exit_mode(1);
	sunxi_board_close_source();
#ifdef CONFIG_SMALL_MEMSIZE
        reload_config();
#endif
        tick_printf("\nStarting kernel ...\n\n");

#ifdef CONFIG_USB_DEVICE
	{
		extern void udc_disconnect(void);
		udc_disconnect();
	}
#endif
	cleanup_before_linux();
}

int do_bootm_linux(int flag, int argc, char *argv[], bootm_headers_t *images)
{
	bd_t	*bd = gd->bd;
	char	*s;
	int	machid = bd->bi_arch_number;
	void	(*kernel_entry)(int zero, int arch, uint params);

#ifdef CONFIG_CMDLINE_TAG
	char *commandline = getenv ("bootargs");
#endif

	if ((flag != 0) && (flag != BOOTM_STATE_OS_GO))
		return 1;

	s = getenv ("machid");
	if (s) {
		machid = simple_strtoul (s, NULL, 16);
		printf ("Using machid 0x%x from environment\n", machid);
	}

	show_boot_progress (15);

#ifdef CONFIG_OF_LIBFDT
	if (images->ft_len)
		return bootm_linux_fdt(machid, images);
#endif

	kernel_entry = (void (*)(int, int, uint))images->ep;

	debug ("## Transferring control to Linux (at address %08lx) ...\n",
	       (ulong) kernel_entry);

#if defined (CONFIG_SETUP_MEMORY_TAGS) || \
    defined (CONFIG_CMDLINE_TAG) || \
    defined (CONFIG_INITRD_TAG) || \
    defined (CONFIG_SERIAL_TAG) || \
    defined (CONFIG_REVISION_TAG)
	setup_start_tag (bd);
#ifdef CONFIG_SERIAL_TAG
	setup_serial_tag (&params);
#endif
#ifdef CONFIG_REVISION_TAG
	setup_revision_tag (&params);
#endif
#ifdef CONFIG_SETUP_MEMORY_TAGS
	setup_memory_tags (bd);
#endif
#ifdef CONFIG_CMDLINE_TAG
#ifdef CONFIG_READ_LOGO_FOR_KERNEL
	if(gd->fb_base != 0)
	{
            sprintf(commandline ,"%s%s%x",commandline," fb_base=0x",(uint)gd->fb_base);
        }
#endif
	setup_commandline_tag (bd, commandline);
#endif
#ifdef CONFIG_INITRD_TAG
	if (images->rd_start && images->rd_end)
		setup_initrd_tag (bd, images->rd_start, images->rd_end);
#endif
	setup_end_tag(bd);
#endif

	announce_and_cleanup();

	kernel_entry(0, machid, bd->bi_boot_params);
	/* does not return */

	return 1;
}

void update_bootargs(struct fastboot_boot_img_hdr *hdr)
{
#ifdef CONFIG_CMDLINE_TAG
	char cmdline[FASTBOOT_BOOT_ARGS_SIZE];
	char *commandline = getenv ("bootargs");
	char *s = getenv("partitions");
	char *sig = getenv("signature");
	char *serial_info = getenv("sunxi_serial");
	char *hardware_info = getenv("sunxi_hardware");
	char *verifiedbootstate_info = getenv("verifiedbootstate");
	char *rotpk_info = getenv("sunxi_rotpk");
	char data[16] = {0};
    bd_t *bd = gd->bd;

    int lcd_x,lcd_y;
    if(script_parser_fetch("lcd0_para", "lcd_x", &lcd_x, 1))
    {
        printf("lcd0_para lcd_x not found\n");
    }
    if(script_parser_fetch("lcd0_para", "lcd_y", &lcd_y, 1))
    {
        printf("lcd0_para lcd_y not found\n");
    }

    //cmdline  fix start
    memset(data, 0, 16);
    memset(cmdline, 0, sizeof(cmdline));

    strcpy(cmdline, "boot_type=");
    sprintf(data, "%d", uboot_spare_head.boot_data.storage_type);
    strcat(cmdline, data);

    strcat(cmdline, " disp_para=");
    board_display_setenv(data);
    strcat(cmdline, data);

    strcat(cmdline, " fb_base=");
    sprintf(data , "0x%x", (uint)gd->fb_base);
    strcat(cmdline, data);

    if(sig != NULL)
    {
        strcat(cmdline, " signature=");
        strcat(cmdline, sig);
    }
    if(gd->chargemode == 1)
    {
        if((0==strcmp(getenv("bootcmd"),"run setargs_mmc boot_normal"))||(0==strcmp(getenv("bootcmd"),"run setargs_nand boot_normal")))
        {
            printf("only in boot normal mode, pass charger para to kernel\n");
            strcat(cmdline," androidboot.mode=charger");
        }
    }

    strcat(cmdline, " config_size=");
    sprintf(data, "%d", script_get_length());
    strcat(cmdline, data);

    strcat(cmdline, " androidboot.lcd_x=");
    sprintf(data, "%d", lcd_x);
    strcat(cmdline, data);
    
    strcat(cmdline, " androidboot.lcd_y=");
    sprintf(data, "%d", lcd_y);
    strcat(cmdline, data);

    if(rotpk_info != NULL)
    {
        strcat(cmdline,  " androidboot.sunxi_rotpk=");
        strcat(cmdline, rotpk_info);
    }

    if(verifiedbootstate_info != NULL)
    {
        strcat(cmdline,  " androidboot.verifiedbootstate=");
        strcat(cmdline, verifiedbootstate_info);
    }
    if(serial_info != NULL)
    {
        strcat(cmdline,  " androidboot.serialno=");
        strcat(cmdline, serial_info);
    }
    if(hardware_info != NULL)
    {
        strcat(cmdline,  " androidboot.hardware=");
        strcat(cmdline, hardware_info);
    }
    //cmdline fix end

    if(strlen((const char *)hdr->cmdline))
    {
        strcat((char *)hdr->cmdline, " ");
        strcat(cmdline, " partitions=");
        strcat(cmdline, s);
        strcat((char *)hdr->cmdline, cmdline);
        setup_commandline_tag (bd, (char *)hdr->cmdline);
    }
    else
    {
        strcat(cmdline, " ");

        strcat(cmdline, commandline);
        setup_commandline_tag (bd, cmdline);
    }
#endif
}


/* Boot android style linux kernel and ramdisk */
int do_boota_linux (struct fastboot_boot_img_hdr *hdr)
{
	ulong initrd_start, initrd_end;
	void (*kernel_entry)(int zero, int arch, uint params);
	bd_t *bd = gd->bd;
    
	kernel_entry = (void (*)(int, int, uint))(hdr->kernel_addr);
	initrd_start = hdr->ramdisk_addr;
	initrd_end = initrd_start + hdr->ramdisk_size;
#if defined (CONFIG_SETUP_MEMORY_TAGS) || \
    defined (CONFIG_CMDLINE_TAG) || \
    defined (CONFIG_INITRD_TAG) || \
    defined (CONFIG_SERIAL_TAG) || \
    defined (CONFIG_REVISION_TAG)
	setup_start_tag (bd);
#ifdef CONFIG_SERIAL_TAG
	setup_serial_tag (&params);
#endif
#ifdef CONFIG_REVISION_TAG
	setup_revision_tag (&params);
#endif
#ifdef CONFIG_SETUP_MEMORY_TAGS
	setup_memory_tags (bd);
#endif

    update_bootargs(hdr);

#ifdef CONFIG_INITRD_TAG
	if (hdr->ramdisk_size)
		setup_initrd_tag (bd, initrd_start, initrd_end);
#endif
#if defined (CONFIG_VFD) || defined (CONFIG_LCD)
	setup_videolfb_tag ((gd_t *) gd);
#endif
	setup_end_tag (bd);
#endif
	/* we assume that the kernel is in place */
	announce_and_cleanup();

	kernel_entry(0, bd->bi_arch_number, bd->bi_boot_params);
	/* does not return */

	return 1;
}

#if defined(CONFIG_OF_LIBFDT)
static int fixup_memory_node(void *blob)
{
	bd_t	*bd = gd->bd;
	int bank;
	u64 start[CONFIG_NR_DRAM_BANKS];
	u64 size[CONFIG_NR_DRAM_BANKS];

	for (bank = 0; bank < CONFIG_NR_DRAM_BANKS; bank++) {
		start[bank] = bd->bi_dram[bank].start;
		size[bank] = bd->bi_dram[bank].size;
	}

	return fdt_fixup_memory_banks(blob, start, size, CONFIG_NR_DRAM_BANKS);
}

static int bootm_linux_fdt(int machid, bootm_headers_t *images)
{
	ulong rd_len;
	void (*kernel_entry)(int zero, int dt_machid, void *dtblob);
	ulong of_size = images->ft_len;
	char **of_flat_tree = &images->ft_addr;
	ulong *initrd_start = &images->initrd_start;
	ulong *initrd_end = &images->initrd_end;
	struct lmb *lmb = &images->lmb;
	int ret;

	kernel_entry = (void (*)(int, int, void *))images->ep;

	boot_fdt_add_mem_rsv_regions(lmb, *of_flat_tree);

	rd_len = images->rd_end - images->rd_start;
	ret = boot_ramdisk_high(lmb, images->rd_start, rd_len,
				initrd_start, initrd_end);
	if (ret)
		return ret;

	ret = boot_relocate_fdt(lmb, of_flat_tree, &of_size);
	if (ret)
		return ret;

	debug("## Transferring control to Linux (at address %08lx) ...\n",
	       (ulong) kernel_entry);

	fdt_chosen(*of_flat_tree, 1);

	fixup_memory_node(*of_flat_tree);

	fdt_initrd(*of_flat_tree, *initrd_start, *initrd_end, 1);

	announce_and_cleanup();

	kernel_entry(0, machid, *of_flat_tree);
	/* does not return */

	return 1;
}
#endif

#if defined (CONFIG_SETUP_MEMORY_TAGS) || \
    defined (CONFIG_CMDLINE_TAG) || \
    defined (CONFIG_INITRD_TAG) || \
    defined (CONFIG_SERIAL_TAG) || \
    defined (CONFIG_REVISION_TAG)
static void setup_start_tag (bd_t *bd)
{
	params = (struct tag *) bd->bi_boot_params;

	params->hdr.tag = ATAG_CORE;
	params->hdr.size = tag_size (tag_core);

	params->u.core.flags = 0;
	params->u.core.pagesize = 0;
	params->u.core.rootdev = 0;

	params = tag_next (params);
}


#ifdef CONFIG_SETUP_MEMORY_TAGS
static void setup_memory_tags (bd_t *bd)
{
	int i;

	for (i = 0; i < CONFIG_NR_DRAM_BANKS; i++) {
		params->hdr.tag = ATAG_MEM;
		params->hdr.size = tag_size (tag_mem32);

		params->u.mem.start = bd->bi_dram[i].start;
		params->u.mem.size = bd->bi_dram[i].size;
                if(gd->ram_size_mb && (!bd->bi_dram[i].size))
			params->u.mem.size = gd->ram_size_mb;
		params = tag_next (params);
	}
}
#endif /* CONFIG_SETUP_MEMORY_TAGS */


static void setup_commandline_tag (bd_t *bd, char *commandline)
{
	char *p;

	if (!commandline)
		return;

	/* eat leading white space */
	for (p = commandline; *p == ' '; p++);

	/* skip non-existent command lines so the kernel will still
	 * use its default command line.
	 */
	if (*p == '\0')
		return;

	params->hdr.tag = ATAG_CMDLINE;
	params->hdr.size =
		(sizeof (struct tag_header) + strlen (p) + 1 + 4) >> 2;

	strcpy (params->u.cmdline.cmdline, p);

	params = tag_next (params);
}


#ifdef CONFIG_INITRD_TAG
static void setup_initrd_tag (bd_t *bd, ulong initrd_start, ulong initrd_end)
{
	/* an ATAG_INITRD node tells the kernel where the compressed
	 * ramdisk can be found. ATAG_RDIMG is a better name, actually.
	 */
	params->hdr.tag = ATAG_INITRD2;
	params->hdr.size = tag_size (tag_initrd);

	params->u.initrd.start = initrd_start;
	params->u.initrd.size = initrd_end - initrd_start;

	params = tag_next (params);
}
#endif /* CONFIG_INITRD_TAG */

#ifdef CONFIG_SERIAL_TAG
void setup_serial_tag (struct tag **tmp)
{
	struct tag *params = *tmp;
	struct tag_serialnr serialnr;
	void get_board_serial(struct tag_serialnr *serialnr);

	get_board_serial(&serialnr);
	params->hdr.tag = ATAG_SERIAL;
	params->hdr.size = tag_size (tag_serialnr);
	params->u.serialnr.low = serialnr.low;
	params->u.serialnr.high= serialnr.high;
	params = tag_next (params);
	*tmp = params;
}
#endif

#ifdef CONFIG_REVISION_TAG
void setup_revision_tag(struct tag **in_params)
{
	u32 rev = 0;
	u32 get_board_rev(void);

	rev = get_board_rev();
	params->hdr.tag = ATAG_REVISION;
	params->hdr.size = tag_size (tag_revision);
	params->u.revision.rev = rev;
	params = tag_next (params);
}
#endif  /* CONFIG_REVISION_TAG */

static void setup_end_tag (bd_t *bd)
{
	params->hdr.tag = ATAG_NONE;
	params->hdr.size = 0;
}
#endif /* CONFIG_SETUP_MEMORY_TAGS || CONFIG_CMDLINE_TAG || CONFIG_INITRD_TAG */

static ulong get_sp(void)
{
	ulong ret;

	asm("mov %0, sp" : "=r"(ret) : );
	return ret;
}
