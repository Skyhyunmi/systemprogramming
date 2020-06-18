//
// LCD1602 2 line by 16 character LCD demo
//
// Written by Larry Bank - 12/7/2017
// Copyright (c) 2017 BitBank Software, Inc.
// bitbank@pobox.com
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
//

#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include "lcd1602.h"
#include <time.h>

int main(int argc, char *argv[])
{
int rc;
	rc = lcd1602Init(0x27);
	if (rc)
	{
		printf("Initialization failed; aborting...\n");
		return 0;
	}
    // lcd1602Control(1,0,1);
    while(1){
        char cmd[20],str[20];
        int posx,posy;
        scanf("%s",cmd);
        if (!strcmp(cmd,"pos")){
            scanf("%d %d",&posx,&posy);
            lcd1602SetCursor(posx,posy);
        }
        else if (!strcmp(cmd,"quit")){
            lcd1602Clear();
            lcd1602SetCursor(1,1);
            lcd1602WriteString("Will quit");
            lcd1602SetCursor(1,2);
            lcd1602WriteString("Wait 1Sec");
            sleep(1);
            goto a;
        }
        else if (!strcmp(cmd,"clear")){
            lcd1602Clear();
        }
        else {
            lcd1602WriteString(cmd);
        }
    }
    a:
	// lcd1602Control(1,0,1); // backlight on, underline off, blink block on 
	lcd1602Shutdown();
    return 0;
} /* main() */