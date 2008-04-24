/*  $Id$
**   __      __ __             ___        __   __ __   __
**  /  \    /  \__| ____    __| _/_______/  |_|__|  | |  |   ____
**  \   \/\/   /  |/    \  / __ |/  ___/\   __\  |  | |  | _/ __ \
**   \        /|  |   |  \/ /_/ |\___ \  |  | |  |  |_|  |_\  ___/
**    \__/\  / |__|___|  /\____ /____  > |__| |__|____/____/\___  >
**         \/          \/      \/    \/                         \/
**  Copyright (C) 2007 Ingo Ruhnke <grumbel@gmx.de>
**
**  This program is free software; you can redistribute it and/or
**  modify it under the terms of the GNU General Public License
**  as published by the Free Software Foundation; either version 2
**  of the License, or (at your option) any later version.
**
**  This program is distributed in the hope that it will be useful,
**  but WITHOUT ANY WARRANTY; without even the implied warranty of
**  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
**  GNU General Public License for more details.
** 
**  You should have received a copy of the GNU General Public License
**  along with this program; if not, write to the Free Software
**  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
**  02111-1307, USA.
*/

#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <linux/uinput.h>
#include <boost/bind.hpp>
#include "uinput_driver.hpp"

UInputDriver::UInputDriver()
  : abs_bit(false),
    key_bit(false),
    fd(-1)
{
  memset(&user_dev, 0, sizeof(user_dev));
  
  // Open the input device
  char* uinput_filename[] = { "/dev/input/uinput", "/dev/uinput", "/dev/misc/uinput" };
  const int uinput_filename_count = (sizeof(uinput_filename)/sizeof(char*));

  for (int i = 0; i < uinput_filename_count; ++i) 
    {
      if ((fd = open(uinput_filename[i], O_WRONLY | O_NDELAY)) >= 0)
        {
          break;
        }
      else
        {
          std::cout << "Error: " << uinput_filename[i] << ": " << strerror(errno) << std::endl;
        }
    }

  if (fd < 0)
    {
      std::cout << "Error: No stuitable uinput device found" << std::endl;
      std::cout << "" << std::endl;
      std::cout << "Troubleshooting:" << std::endl;
      std::cout << "  * make sure uinput kernel module is loaded " << std::endl;
      std::cout << "  * make sure joydev kernel module is loaded " << std::endl;
      std::cout << "  * make sure you have permissions to access the uinput device" << std::endl;
      std::cout << "  * start the driver with ./xboxdrv -v --no-uinput to see if the driver itself works" << std::endl;
      std::cout << "" << std::endl;
      exit(EXIT_FAILURE);
    }
}

void
UInputDriver::add_abs(uint16_t code, int min, int max)
{
  if (!abs_bit)
    {
      ioctl(fd, UI_SET_EVBIT, EV_ABS);
      abs_bit = true;
    }

  ioctl(fd, UI_SET_ABSBIT, code);

  user_dev.absmin[code] = min;
  user_dev.absmax[code] = max; 


  abs_port_in.push_back(new AbsPortIn("UInput", min, max,
                                      boost::bind(&UInputDriver::on_abs, this, _1, code)));
}

void
UInputDriver::on_btn(BtnPortOut* port, uint16_t code)
{
  struct input_event ev;      
  memset(&ev, 0, sizeof(ev));

  gettimeofday(&ev.time, NULL);
  ev.type  = EV_KEY;
  ev.code  = code;
  ev.value = port->get_state();

 write(fd, &ev, sizeof(ev)); 
}

void
UInputDriver::on_abs(AbsPortOut* port, uint16_t code)
{
  struct input_event ev;      
  memset(&ev, 0, sizeof(ev));

  gettimeofday(&ev.time, NULL);
  ev.type  = EV_ABS;
  ev.code  = code;
  ev.value = port->get_state();

 write(fd, &ev, sizeof(ev)); 
}

void
UInputDriver::add_btn(uint16_t code)
{
  if (!key_bit)
    {
      ioctl(fd, UI_SET_EVBIT, EV_KEY);
      key_bit = true;
    }

  ioctl(fd, UI_SET_KEYBIT, code);
}

void
UInputDriver::finish()
{
  write(fd, &user_dev, sizeof(user_dev));
}

/* EOF */
