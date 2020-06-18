/*
	https://github.com/bitbank2/LCD1602/blob/master/main.c
	2x16 LCD display (HD44780 controller + I2C chip)

	Copyright (c) BitBank Software, Inc.
	Written by Larry Bank
	bitbank@pobox.com
	Project started 12/6/2017

	This program is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/types.h>

/*
	The LCD controller is wired to the I2C port expander with the upper 4 bits
	(D4-D7) connected to the data lines and the lower 4 bits (D0-D3) used as
	control lines. Here are the control line definitions:

	Command (0) / Data (1) (aka RS) (D0)
	R/W                             (D1)
	Enable/CLK                      (D2) 
	Backlight control               (D3)

	The data must be manually clocked into the LCD controller by toggling
	the CLK line after the data has been placed on D4-D7
*/
#define PULSE_PERIOD 500
#define CMD_PERIOD 4100
#define I2C_SLAVE 0x0703

/*
How to use
    Clear Display       0x01                    	LCD표시창을 클리어시키고 커서를 첫 줄의 첫 칸에 위치


     Return Home        0x02                    	LCD의 표시내용을 그대로 두고 커서만은 홈으로 위치


     Entry mode set     0x06                    	LCD표시창에 문자를 표시하고 커서를 오른쪽으로 이동
						0x04 + 0x02					ENTRYMODESET + ENTRYLEFT


     Display off        0x08                    	표시 off


     Display on         0x0c                    	표시 on
						0x08 + 0x04					DISPLAYCONTROL + DISPLAYON


     control            0x0E                    	표시 on 커서 on
	 					0x08 + 0x04	+ 0x02			DISPLAYCONTROL + DISPLAYON

                        0x0F                    	표시 on 커서 on 블링크 on
						0x08 + 0x04	+ 0x02 + 0x01	DISPLAYCONTROL + DISPLAYON + CURSORON + BLINKON


     Cursor or          0x1C                    	현재 LCD에 표시되어 있는 내용을 오른쪽(0x1C),
	 					0x10 + 0x08 + 0x04			Cursor Setting + DISPLAYMOVE + MOVERIGHT


     display shift      0x18                    	왼쪽(0x18)으로 한 칸씩 이동
	 					0x10 + 0x08					Cursor Setting + DISPLAYMOVE


     Function set       0x38                    	데이터 선 8비트, 2줄로 표시, 5x7도트사용
	 					0x20 + 0x10 + 0x08			FUNCTIONSET + 8BITMODE + 2LINE

                        0x28                    	데이터 선 4비트, 2줄로 표시, 5x7도트사용
						0x20 + 0x08					FUNCTIONSET + 2LINE


     CG램 주소            0x40~0x7F               	캐릭터 제너레이터 램의 주소값 설정


     DD램 주소            0x80~0xff               	데이터 디스플레이 램의 주소값 설정

출처: https://alisa2304.tistory.com/28
*/

// commands
#define LCD_CLEARDISPLAY  0x01
#define LCD_RETURNHOME  0x02
#define LCD_SETCGRAMADDR  0x40
#define LCD_SETDDRAMADDR  0x80

// flags for display entry mode
#define LCD_ENTRYMODESET  0x04 //Entry 세팅 활성화

#define LCD_ENTRYRIGHT  0x00
#define LCD_ENTRYLEFT  0x02
#define LCD_ENTRYSHIFTINCREMENT  0x01
#define LCD_ENTRYSHIFTDECREMENT  0x00

// flags for display on/off control
#define LCD_DISPLAYCONTROL  0x08

#define LCD_DISPLAYON  0x04
#define LCD_DISPLAYOFF  0x00
#define LCD_CURSORON  0x02
#define LCD_CURSOROFF  0x00
#define LCD_BLINKON  0x01
#define LCD_BLINKOFF  0x00

// flags for display/cursor shift
#define LCD_CURSORSHIFT  0x10

#define LCD_DISPLAYMOVE  0x08
#define LCD_CURSORMOVE  0x00
#define LCD_MOVERIGHT  0x04
#define LCD_MOVELEFT  0x00

// flags for function set
#define LCD_FUNCTIONSET  0x20

#define LCD_8BITMODE  0x10
#define LCD_4BITMODE  0x00
#define LCD_2LINE  0x08
#define LCD_1LINE  0x00
#define LCD_5x10DOTS  0x04
#define LCD_5x8DOTS  0x00

// flags for backlight control

#define LCD_BACKLIGHT  0x08
#define LCD_NOBACKLIGHT  0x00

#define DATA 1
static int iBackLight = LCD_BACKLIGHT;
static int file_i2c = -1;

static void WriteCommand(char ucCMD)
{
	char uc;
	uc = (ucCMD & 0xf0) | iBackLight; // most significant nibble sent first
	write(file_i2c, &uc, 1);
	usleep(PULSE_PERIOD); // manually pulse the clock line
	uc |= 4; // enable pulse
	write(file_i2c, &uc, 1);
	usleep(PULSE_PERIOD);
	uc &= ~4; // toggle pulse
	write(file_i2c, &uc, 1);
	usleep(CMD_PERIOD);
	uc = iBackLight | (ucCMD << 4); // least significant nibble
	write(file_i2c, &uc, 1);
	usleep(PULSE_PERIOD);
	uc |= 4; // enable pulse
	write(file_i2c, &uc, 1);
	usleep(PULSE_PERIOD);
	uc &= ~4; // toggle pulse
	write(file_i2c, &uc, 1);
	usleep(CMD_PERIOD);

}

int lcd1602Control(int bBacklight, int bCursor, int bBlink)
{
char ucCMD = 0xc;

	if (file_i2c < 0)
		return 1;
	iBackLight = (bBacklight) ? LCD_BACKLIGHT : LCD_NOBACKLIGHT;
	if (bCursor)
		ucCMD |= 2;
	if (bBlink)
		ucCMD |= 1;
	WriteCommand(ucCMD);
 	
	return 0;
}


int lcd1602WriteString(char *text)
{
	char ucTemp[2];
	int i=0;

	if (file_i2c < 0 || text == NULL)
		return 1;
	int len = strlen(text);
	while (i<len && *text)
	{
		ucTemp[0] = iBackLight | DATA | (*text & 0xf0);
		write(file_i2c, ucTemp, 1);
		usleep(PULSE_PERIOD);
		ucTemp[0] |= 4; // pulse E
		write(file_i2c, ucTemp, 1);
		usleep(PULSE_PERIOD);
		ucTemp[0] &= ~4;
		write(file_i2c, ucTemp, 1);
		usleep(PULSE_PERIOD);
		ucTemp[0] = iBackLight | DATA | (*text << 4);
		write(file_i2c, ucTemp, 1);
		ucTemp[0] |= 4; // pulse E
		write(file_i2c, ucTemp, 1);
		usleep(PULSE_PERIOD);
		ucTemp[0] &= ~4;
		write(file_i2c, ucTemp, 1);
		usleep(CMD_PERIOD);
		text++;
		i++;
	}
	return 0;
}

int lcd1602Clear(void)
{
	if (file_i2c < 0)
		return 1;
	WriteCommand(LCD_CLEARDISPLAY); // clear the screen
	WriteCommand(LCD_RETURNHOME);
	return 0;

}

int lcd1602Init(int iAddr)
{
	char szFile[32];
	int rc;
	sprintf(szFile, "/dev/i2c-1");
	file_i2c = open(szFile, O_RDWR);
	if (file_i2c < 0)
	{
		fprintf(stderr, "Error opening i2c device; not running as sudo?\n");
		return 1;
	}
	rc = ioctl(file_i2c, I2C_SLAVE, iAddr);
	if (rc < 0)
	{
		close(file_i2c);
		fprintf(stderr, "Error setting I2C device address\n");
		return 1;
	}
	lcd1602Clear();
	iBackLight = LCD_BACKLIGHT;
	WriteCommand(LCD_FUNCTIONSET | LCD_2LINE | LCD_5x8DOTS | LCD_4BITMODE);
	WriteCommand(LCD_DISPLAYCONTROL | LCD_DISPLAYON);
	WriteCommand(LCD_ENTRYMODESET | LCD_ENTRYLEFT);
	lcd1602Clear();

	return 0;
}

int lcd1602SetCursor(int x, int y)
{
char cCmd;
	if (file_i2c < 0 || x < 1 || x > 20 || y < 1 || y > 4)
		return 1;

	if(y==1) cCmd = LCD_SETDDRAMADDR;
	else if (y == 2) cCmd = LCD_SETDDRAMADDR+0x40;
	else if (y == 3) cCmd = LCD_SETDDRAMADDR+0x14;
	else cCmd = LCD_SETDDRAMADDR+0x54;
	cCmd |= x-1;
	WriteCommand(cCmd);
	return 0;

} /* lcd1602SetCursor() */

void lcd1602Shutdown(void)
{
	iBackLight = 0; // turn off backlight
	WriteCommand(LCD_DISPLAYCONTROL); // turn off display, cursor and blink	
	close(file_i2c);
	file_i2c = -1;
} /* lcd1602Shutdown() */