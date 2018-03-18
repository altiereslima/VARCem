/*
 * VARCem	Virtual ARchaeological Computer EMulator.
 *		An emulator of (mostly) x86-based PC systems and devices,
 *		using the ISA,EISA,VLB,MCA  and PCI system buses, roughly
 *		spanning the era between 1981 and 1995.
 *
 *		This file is part of the VARCem Project.
 *
 *		Emulation for C&T 82C206/82c597 based 4GLX3 board.
 *
 * NOTE:	The NEAT 82c206 code should be moved into a 82c206 module,
 *		so it can be re-used by other boards.
 *
 * Version:	@(#)m_4gpv31.c	1.0.2	2018/03/15
 *
 * Author:	Fred N. van Kempen, <decwiz@yahoo.com>
 *
 *		Copyright 2017,2018 Fred N. van Kempen.
 *
 *		Redistribution and  use  in source  and binary forms, with
 *		or  without modification, are permitted  provided that the
 *		following conditions are met:
 *
 *		1. Redistributions of  source  code must retain the entire
 *		   above notice, this list of conditions and the following
 *		   disclaimer.
 *
 *		2. Redistributions in binary form must reproduce the above
 *		   copyright  notice,  this list  of  conditions  and  the
 *		   following disclaimer in  the documentation and/or other
 *		   materials provided with the distribution.
 *
 *		3. Neither the  name of the copyright holder nor the names
 *		   of  its  contributors may be used to endorse or promote
 *		   products  derived from  this  software without specific
 *		   prior written permission.
 *
 * THIS SOFTWARE  IS  PROVIDED BY THE  COPYRIGHT  HOLDERS AND CONTRIBUTORS
 * "AS IS" AND  ANY EXPRESS  OR  IMPLIED  WARRANTIES,  INCLUDING, BUT  NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE  ARE  DISCLAIMED. IN  NO  EVENT  SHALL THE COPYRIGHT
 * HOLDER OR  CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL,  EXEMPLARY,  OR  CONSEQUENTIAL  DAMAGES  (INCLUDING,  BUT  NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE  GOODS OR SERVICES;  LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED  AND ON  ANY
 * THEORY OF  LIABILITY, WHETHER IN  CONTRACT, STRICT  LIABILITY, OR  TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING  IN ANY  WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>
#include "../emu.h"
#include "../io.h"
#include "../device.h"
#include "../keyboard.h"
#include "../floppy/fdd.h"
#include "../floppy/fdc.h"
#include "machine.h"


typedef struct {
    uint8_t	regs[256];
    int		indx;

    int		emspg[4];
} neat_t;


#if NOT_USED
static void
neat_wrems(uint32_t addr, uint8_t val, void *priv)
{
    neat_t *dev = (neat_t *)priv;

    ram[(dev->emspg[(addr >> 14) & 3] << 14) + (addr & 0x3fff)] = val;
}


static uint8_t
neat_rdems(uint32_t addr, void *priv)
{
    neat_t *dev = (neat_t *)priv;

    return(ram[(dev->emspg[(addr >> 14) & 3] << 14) + (addr & 0x3fff)]);
}
#endif


static void
neat_write(uint16_t port, uint8_t val, void *priv)
{
    neat_t *dev = (neat_t *)priv;

    pclog("NEAT: write(%04x, %02x)\n", port, val);

    switch (port) {
	case 0x22:
		dev->indx = val;
		break;

	case 0x23:
		dev->regs[dev->indx] = val;
		switch (dev->indx) {
			case 0x6e:	/* EMS page extension */
				dev->emspg[3] = (dev->emspg[3] & 0x7F) | (( val       & 3) << 7);
				dev->emspg[2] = (dev->emspg[2] & 0x7F) | (((val >> 2) & 3) << 7);
				dev->emspg[1] = (dev->emspg[1] & 0x7F) | (((val >> 4) & 3) << 7);
				dev->emspg[0] = (dev->emspg[0] & 0x7F) | (((val >> 6) & 3) << 7);
				break;
		}
		break;

	case 0x0208:
	case 0x0209:

	case 0x4208:
	case 0x4209:

	case 0x8208:
	case 0x8209:

	case 0xc208:
	case 0xc209:
		dev->emspg[port >> 14] = (dev->emspg[port >> 14] & 0x180) | (val & 0x7F);		
		break;
    }
}


static uint8_t
neat_read(uint16_t port, void *priv)
{
    neat_t *dev = (neat_t *)priv;
    uint8_t ret = 0xff;

    switch (port) {
	case 0x22:
		ret = dev->indx;
		break;

	case 0x23:
		ret = dev->regs[dev->indx];
		break;
    }

    pclog("NEAT: read(%04x) = %02x\n", port, ret);

    return(ret);
}


static void
neat_init(void)
{
    neat_t *dev;

    dev = (neat_t *)malloc(sizeof(neat_t));
    memset(dev, 0x00, sizeof(neat_t));

    io_sethandler(0x0022, 2,
		  neat_read,NULL,NULL, neat_write,NULL,NULL, dev);
    io_sethandler(0x0208, 2,
		  neat_read,NULL,NULL, neat_write,NULL,NULL, dev);
    io_sethandler(0x4208, 2,
		  neat_read,NULL,NULL, neat_write,NULL,NULL, dev);
    io_sethandler(0x8208, 2,
		  neat_read,NULL,NULL, neat_write,NULL,NULL, dev);
    io_sethandler(0xc208, 2,
		  neat_read,NULL,NULL, neat_write,NULL,NULL, dev);
}


void
machine_at_4gpv31_init(const machine_t *model)
{
    machine_at_common_ide_init(model);
    device_add(&keyboard_at_ami_device);
    device_add(&fdc_at_device);

    neat_init();
}
