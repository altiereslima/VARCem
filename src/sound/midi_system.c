/*
 * VARCem	Virtual ARchaeological Computer EMulator.
 *		An emulator of (mostly) x86-based PC systems and devices,
 *		using the ISA,EISA,VLB,MCA  and PCI system buses, roughly
 *		spanning the era between 1981 and 1995.
 *
 *		This file is part of the VARCem Project.
 *
 *		Interface to system MIDI driver.
 *
 * Version:	@(#)midi_system.c	1.0.3	2018/03/15
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
#include <stdlib.h>
#include <wchar.h>
#include "../emu.h"
#include "../device.h"
#include "../plat.h"
#include "midi.h"
#include "midi_system.h"


void* system_midi_init(const device_t *info)
{
        midi_device_t* dev = malloc(sizeof(midi_device_t));
        memset(dev, 0, sizeof(midi_device_t));

        dev->play_msg = plat_midi_play_msg;
        dev->play_sysex = plat_midi_play_sysex;
        dev->write = plat_midi_write;

        plat_midi_init();

        midi_init(dev);

        return dev;
}

void system_midi_close(void* p)
{
        plat_midi_close();

        midi_close();
}

int system_midi_available(void)
{
        return plat_midi_get_num_devs();
}

static const device_config_t system_midi_config[] =
{
        {
                .name = "midi",
                .description = "MIDI out device",
                .type = CONFIG_MIDI,
                .default_int = 0
        },
        {
                .type = -1
        }
};

const device_t system_midi_device =
{
        SYSTEM_MIDI_NAME,
        0, 0,
        system_midi_init,
        system_midi_close,
	NULL,
        system_midi_available,
        NULL,
        NULL,
        NULL,
        system_midi_config
};
