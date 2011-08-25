/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * Sifteo prototype simulator
 * M. Elizabeth Scott <beth@sifteo.com>
 *
 * Copyright <c> 2011 Sifteo, Inc. All rights reserved.
 */

#include <stdint.h>
#include "lcd.h"

#define CMD_NOP      0x00
#define CMD_CASET    0x2A
#define CMD_RASET    0x2B
#define CMD_RAMWR    0x2C

static struct {
    int need_repaint;
    uint32_t write_count;

    /* 16-bit RGB 5-6-5 format */
    uint8_t fb_mem[FB_SIZE];

    /* Hardware interface */
    uint8_t prev_wrx;

    /* LCD Controller State */
    uint8_t current_cmd;
    unsigned cmd_bytecount;
    unsigned xs, xe, ys, ye;
    unsigned row;
} lcd;

static uint8_t clamp(uint8_t val, uint8_t min, uint8_t max)
{
    if (val < min) val = min;
    if (val > max) val = max;
    return val;
}

void lcd_init(void)
{
    lcd.current_cmd = CMD_NOP;
    lcd.need_repaint = 1;
    lcd.xs = 0;
    lcd.ys = 0;
    lcd.xe = LCD_WIDTH - 1;
    lcd.ye = LCD_HEIGHT - 1;
}

static void lcd_cmd(uint8_t op)
{
    lcd.current_cmd = op;
    lcd.cmd_bytecount = 0;

    switch (op) {

    case CMD_RAMWR:
	// Return to start row/column
	lcd.row = lcd.ys;
	lcd.cmd_bytecount = lcd.xs << 1;
	lcd.write_count++;
	break;

    }
}

static void lcd_data(uint8_t byte)
{
    switch (lcd.current_cmd) {

    case CMD_CASET:
	switch (lcd.cmd_bytecount++) {
	case 1:  lcd.xs = clamp(byte, 0, 0x83);
	case 3:  lcd.xe = clamp(byte, 0, 0x83);
	}
	break;

    case CMD_RASET:
	switch (lcd.cmd_bytecount++) {
	case 1:  lcd.ys = clamp(byte, 0, 0xa1);
	case 3:  lcd.ye = clamp(byte, 0, 0xa1);
	}
	break;

    case CMD_RAMWR:
	lcd.fb_mem[FB_MASK & ((lcd.row << FB_ROW_SHIFT) + lcd.cmd_bytecount)] = byte;
	lcd.cmd_bytecount++;
	lcd.need_repaint = 1;
	if (lcd.cmd_bytecount > 1 + (lcd.xe << 1)) {
	    lcd.cmd_bytecount = lcd.xs << 1;
	    lcd.row++;
	    if (lcd.row > lcd.ye)
		lcd.row = lcd.ys;
	}
	break;
    }
}

void lcd_cycle(struct lcd_pins *pins)
{
    /*
     * Make lots of assumptions...
     *
     * This is pretending to be an SPFD5414 controller, with the following settings:
     *
     *   - 8-bit parallel interface, in 80-series mode
     *   - 16-bit color depth, RGB-565 (3AH = 05)
     */

    // Assume we aren't driving the data output for now
    pins->data_drv = 0;

    if (!pins->csx && pins->wrx && !lcd.prev_wrx) {
	if (pins->dcx) {
	    /* Data write strobe */
	    lcd_data(pins->data_in);
	} else {
	    /* Command write strobe */
	    lcd_cmd(pins->data_in);
	}
    }

    lcd.prev_wrx = pins->wrx;
}

uint32_t lcd_write_count(void)
{
    uint32_t cnt = lcd.write_count;
    lcd.write_count = 0;
    return cnt;
}

int lcd_check_for_repaint(void)
{
    int r = lcd.need_repaint;
    lcd.need_repaint = 0;
    return r;
}

uint16_t *lcd_framebuffer(void)
{
    return (uint16_t *) lcd.fb_mem;
}
