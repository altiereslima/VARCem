/*
 * VARCem	Virtual ARchaeological Computer EMulator.
 *		An emulator of (mostly) x86-based PC systems and devices,
 *		using the ISA,EISA,VLB,MCA  and PCI system buses, roughly
 *		spanning the era between 1981 and 1995.
 *
 *		This file is part of the VARCem Project.
 *
 *		Emulation of the Laser XT series of machines.
 *
 * Version:	@(#)m_xt_laserxt.c	1.0.1	2018/02/14
 *
 * Authors:	Fred N. van Kempen, <decwiz@yahoo.com>
 *		Miran Grca, <mgrca8@gmail.com>
 *		Sarah Walker, <tommowalker@tommowalker.co.uk>
 *
 *		Copyright 2017,2018 Fred N. van Kempen.
 *		Copyright 2016-2018 Miran Grca.
 *		Copyright 2008-2018 Sarah Walker.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free  Software  Foundation; either  version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is  distributed in the hope that it will be useful, but
 * WITHOUT   ANY  WARRANTY;  without  even   the  implied  warranty  of
 * MERCHANTABILITY  or FITNESS  FOR A PARTICULAR  PURPOSE. See  the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the:
 *
 *   Free Software Foundation, Inc.
 *   59 Temple Place - Suite 330
 *   Boston, MA 02111-1307
 *   USA.
 */
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <wchar.h>
#include "../emu.h"
#include "../cpu/cpu.h"
#include "../io.h"
#include "../mem.h"
#include "../rom.h"
#include "machine.h"


static int laserxt_emspage[4];
static int laserxt_emscontrol[4];
static mem_mapping_t laserxt_ems_mapping[4];
static int laserxt_ems_baseaddr_index = 0;


static uint32_t get_laserxt_ems_addr(uint32_t addr)
{
        if(laserxt_emspage[(addr >> 14) & 3] & 0x80)
        {
                addr = (romset == ROM_LTXT ? 0x70000 + (((mem_size + 64) & 255) << 10) : 0x30000 + (((mem_size + 320) & 511) << 10)) + ((laserxt_emspage[(addr >> 14) & 3] & 0x0F) << 14) + ((laserxt_emspage[(addr >> 14) & 3] & 0x40) << 12) + (addr & 0x3FFF);
        }

        return addr;
}


static void laserxt_write(uint16_t port, uint8_t val, void *priv)
{
        int i;
        uint32_t paddr, vaddr;
        switch (port)
        {
                case 0x0208: case 0x4208: case 0x8208: case 0xC208:
                laserxt_emspage[port >> 14] = val;
                paddr = 0xC0000 + (port & 0xC000) + (((laserxt_ems_baseaddr_index + (4 - (port >> 14))) & 0x0C) << 14);
                if(val & 0x80)
                {
                        mem_mapping_enable(&laserxt_ems_mapping[port >> 14]);
                        vaddr = get_laserxt_ems_addr(paddr);
                        mem_mapping_set_exec(&laserxt_ems_mapping[port >> 14], ram + vaddr);
                }
                else
                {
                        mem_mapping_disable(&laserxt_ems_mapping[port >> 14]);
                }
                flushmmucache();
                break;
                case 0x0209: case 0x4209: case 0x8209: case 0xC209:
                laserxt_emscontrol[port >> 14] = val;
                laserxt_ems_baseaddr_index = 0;
                for(i=0; i<4; i++)
                {
                        laserxt_ems_baseaddr_index |= (laserxt_emscontrol[i] & 0x80) >> (7 - i);
                }
                if(laserxt_ems_baseaddr_index < 3)
                {
                        mem_mapping_disable(&romext_mapping);
                }
                else
                {
                        mem_mapping_enable(&romext_mapping);
                }

                mem_mapping_set_addr(&laserxt_ems_mapping[0], 0xC0000 + (((laserxt_ems_baseaddr_index + 4) & 0x0C) << 14), 0x4000);
                mem_mapping_set_addr(&laserxt_ems_mapping[1], 0xC4000 + (((laserxt_ems_baseaddr_index + 3) & 0x0C) << 14), 0x4000);
                mem_mapping_set_addr(&laserxt_ems_mapping[2], 0xC8000 + (((laserxt_ems_baseaddr_index + 2) & 0x0C) << 14), 0x4000);
                mem_mapping_set_addr(&laserxt_ems_mapping[3], 0xCC000 + (((laserxt_ems_baseaddr_index + 1) & 0x0C) << 14), 0x4000);
                flushmmucache();
                break;
        }
}


static uint8_t laserxt_read(uint16_t port, void *priv)
{
        switch (port)
        {
                case 0x0208: case 0x4208: case 0x8208: case 0xC208:
                return laserxt_emspage[port >> 14];
                case 0x0209: case 0x4209: case 0x8209: case 0xC209:
                return laserxt_emscontrol[port >> 14];
                break;
        }
        return 0xff;
}


static void mem_write_laserxtems(uint32_t addr, uint8_t val, void *priv)
{
        addr = get_laserxt_ems_addr(addr);
        if (addr < (mem_size << 10))
                ram[addr] = val;
}


static uint8_t mem_read_laserxtems(uint32_t addr, void *priv)
{
        uint8_t val = 0xFF;
        addr = get_laserxt_ems_addr(addr);
        if (addr < (mem_size << 10))
                val = ram[addr];
        return val;
}


static void laserxt_init(void)
{
        int i;

        if(mem_size > 640)
        {
                io_sethandler(0x0208, 0x0002, laserxt_read, NULL, NULL, laserxt_write, NULL, NULL,  NULL);
                io_sethandler(0x4208, 0x0002, laserxt_read, NULL, NULL, laserxt_write, NULL, NULL,  NULL);
                io_sethandler(0x8208, 0x0002, laserxt_read, NULL, NULL, laserxt_write, NULL, NULL,  NULL);
                io_sethandler(0xc208, 0x0002, laserxt_read, NULL, NULL, laserxt_write, NULL, NULL,  NULL);
                mem_mapping_set_addr(&ram_low_mapping, 0, romset == ROM_LTXT ? 0x70000 + (((mem_size + 64) & 255) << 10) : 0x30000 + (((mem_size + 320) & 511) << 10));
        }

        for (i = 0; i < 4; i++)
        {
                laserxt_emspage[i] = 0x7F;
                laserxt_emscontrol[i] = (i == 3) ? 0x00 : 0x80;
                mem_mapping_add(&laserxt_ems_mapping[i], 0xE0000 + (i << 14), 0x4000, mem_read_laserxtems, NULL, NULL, mem_write_laserxtems, NULL, NULL, ram + 0xA0000 + (i << 14), 0, NULL);
                mem_mapping_disable(&laserxt_ems_mapping[i]);
        }
        mem_set_mem_state(0x0c0000, 0x40000, MEM_READ_EXTERNAL | MEM_WRITE_EXTERNAL);
}


void
machine_xt_laserxt_init(machine_t *model)
{
        machine_xt_init(model);

        laserxt_init();
}
