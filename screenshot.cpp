/*******************************************************************************
  Snes9x - Portable Super Nintendo Entertainment System (TM) emulator.
 
  (c) Copyright 1996 - 2002 Gary Henderson (gary.henderson@ntlworld.com) and
                            Jerremy Koot (jkoot@snes9x.com)

  (c) Copyright 2001 - 2004 John Weidman (jweidman@slip.net)

  (c) Copyright 2002 - 2004 Brad Jorsch (anomie@users.sourceforge.net),
                            funkyass (funkyass@spam.shaw.ca),
                            Joel Yliluoma (http://iki.fi/bisqwit/)
                            Kris Bleakley (codeviolation@hotmail.com),
                            Matthew Kendora,
                            Nach (n-a-c-h@users.sourceforge.net),
                            Peter Bortas (peter@bortas.org) and
                            zones (kasumitokoduck@yahoo.com)

  C4 x86 assembler and some C emulation code
  (c) Copyright 2000 - 2003 zsKnight (zsknight@zsnes.com),
                            _Demo_ (_demo_@zsnes.com), and Nach

  C4 C++ code
  (c) Copyright 2003 Brad Jorsch

  DSP-1 emulator code
  (c) Copyright 1998 - 2004 Ivar (ivar@snes9x.com), _Demo_, Gary Henderson,
                            John Weidman, neviksti (neviksti@hotmail.com),
                            Kris Bleakley, Andreas Naive

  DSP-2 emulator code
  (c) Copyright 2003 Kris Bleakley, John Weidman, neviksti, Matthew Kendora, and
                     Lord Nightmare (lord_nightmare@users.sourceforge.net

  OBC1 emulator code
  (c) Copyright 2001 - 2004 zsKnight, pagefault (pagefault@zsnes.com) and
                            Kris Bleakley
  Ported from x86 assembler to C by sanmaiwashi

  SPC7110 and RTC C++ emulator code
  (c) Copyright 2002 Matthew Kendora with research by
                     zsKnight, John Weidman, and Dark Force

  S-DD1 C emulator code
  (c) Copyright 2003 Brad Jorsch with research by
                     Andreas Naive and John Weidman
 
  S-RTC C emulator code
  (c) Copyright 2001 John Weidman
  
  ST010 C++ emulator code
  (c) Copyright 2003 Feather, Kris Bleakley, John Weidman and Matthew Kendora

  Super FX x86 assembler emulator code 
  (c) Copyright 1998 - 2003 zsKnight, _Demo_, and pagefault 

  Super FX C emulator code 
  (c) Copyright 1997 - 1999 Ivar, Gary Henderson and John Weidman


  SH assembler code partly based on x86 assembler code
  (c) Copyright 2002 - 2004 Marcus Comstedt (marcus@mc.pp.se) 

 
  Specific ports contains the works of other authors. See headers in
  individual files.
 
  Snes9x homepage: http://www.snes9x.com
 
  Permission to use, copy, modify and distribute Snes9x in both binary and
  source form, for non-commercial purposes, is hereby granted without fee,
  providing that this license information and copyright notice appear with
  all copies and any derived work.
 
  This software is provided 'as-is', without any express or implied
  warranty. In no event shall the authors be held liable for any damages
  arising from the use of this software.
 
  Snes9x is freeware for PERSONAL USE only. Commercial users should
  seek permission of the copyright holders first. Commercial use includes
  charging money for Snes9x or software derived from Snes9x.
 
  The copyright holders request that bug fixes and improvements to the code
  should be forwarded to them so everyone can benefit from the modifications
  in future versions.
 
  Super NES and Super Nintendo Entertainment System are trademarks of
  Nintendo Co., Limited and its subsidiary companies.
*******************************************************************************/

#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <png.h>

#include "snes9x.h"
#include "memmap.h"
#include "gfx.h"
#include "ppu.h"
#include "screenshot.h"

typedef struct {
	png_bytep buffer;
	png_size_t size;
	png_size_t buf_size;
} ScreenshotPriv;

static void write_data(png_structp png_ptr, png_bytep data, png_size_t length)
{
	ScreenshotPriv *p = (ScreenshotPriv*) png_get_io_ptr(png_ptr);
	png_size_t new_size = p->size + length;

	if (!p->buffer) {
		p->buf_size = new_size + 256;
		p->size = 0;
		p->buffer = (png_bytep) malloc(p->buf_size);
		if (!p->buffer) {
			png_error(png_ptr, "Out of memory");
			return;
		}
	}

	if (new_size > p->buf_size) {
		png_size_t new_buf_size = p->buf_size;
		do {
			new_buf_size *= 2;
		} while (new_size > new_buf_size);
		png_bytep new_buf = (png_bytep) realloc(p->buffer, new_buf_size);
		if (!new_buf) {
			png_error(png_ptr, "Out of memory");
			return;
		}
		p->buffer = new_buf;
		p->buf_size = new_buf_size;
	}

	memcpy(p->buffer + p->size, data, length);
	p->size += length;

}

static void flush_data(png_structp png_ptr)
{
	ScreenshotPriv *p = (ScreenshotPriv*) png_get_io_ptr(png_ptr);
	if (p->size < p->buf_size) {
		png_bytep newbuf = (png_bytep) realloc(p->buffer, p->size);
		if (!newbuf) {
			png_error(png_ptr, "Out of memory");
			return;
		}
		p->buffer = newbuf;
		p->buf_size = p->size;
	}
}

static void write_png(png_structp png_ptr, png_infop info_ptr)
{
	const int width = IPPU.RenderedScreenWidth, height = IPPU.RenderedScreenHeight;

	png_set_IHDR(png_ptr, info_ptr, width, height, 8, PNG_COLOR_TYPE_RGB,
		PNG_INTERLACE_NONE,	PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);

	/* 5 bits per color is around what SNES is capable */
	png_color_8 sig_bit;
	sig_bit.red = 5;
	sig_bit.green = 5;
	sig_bit.blue = 5;
	png_set_sBIT(png_ptr, info_ptr, &sig_bit);
	png_set_shift(png_ptr, &sig_bit);

	png_write_info(png_ptr, info_ptr);

	png_byte *row_data = new png_byte[png_get_rowbytes(png_ptr, info_ptr)];
	uint8 *screen = GFX.Screen;
	for (int y = 0; y < height; y++, screen += GFX.Pitch) {
		png_byte *pix_data = row_data;
		for (int x = 0; x < width; x++) {
			uint32 r, g, b;
			DECOMPOSE_PIXEL((*(uint16*)(screen+2*x)), r, g, b);
			*(pix_data++) = r;
			*(pix_data++) = g;
			*(pix_data++) = b;
		}
		png_write_row(png_ptr, row_data);
	}
	delete [] row_data;

	png_write_end(png_ptr, info_ptr);
}

void * S9xScreenshot(size_t *size)
{
	ScreenshotPriv priv = { 0 };

	png_structp png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, 0, 0, 0);
	if (!png_ptr) {
		return 0;
	}

	png_infop info_ptr = png_create_info_struct(png_ptr);
	if (!info_ptr) {
		goto clean_png;
	}

	if(setjmp(png_jmpbuf(png_ptr))){
		if (priv.buffer) {
			free(priv.buffer);
			priv.buffer = 0;
		}
		priv.size = 0;
		goto clean_png;
	}

	png_set_write_fn(png_ptr, &priv, write_data, flush_data);
	png_set_compression_level(png_ptr, Z_BEST_SPEED);

	write_png(png_ptr, info_ptr);

clean_png:
	png_destroy_write_struct(&png_ptr, &info_ptr);

	if (priv.size && size) *size = priv.size;
	return priv.buffer;
}

bool S9xSaveScreenshot(const char * file)
{
	FILE *f = fopen(file, "wb");
	if (!f) return false;

	png_structp png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, 0, 0, 0);
	if (!png_ptr) {
		return 0;
	}

	png_infop info_ptr = png_create_info_struct(png_ptr);
	if (!info_ptr) {
		fclose(f);
		png_destroy_write_struct(&png_ptr, 0);
		return false;
	}

	if(setjmp(png_jmpbuf(png_ptr))){
		fclose(f);
		png_destroy_write_struct(&png_ptr, &info_ptr);
		return false;
	}

	png_init_io(png_ptr, f);
	png_set_compression_level(png_ptr, Z_BEST_COMPRESSION);

	write_png(png_ptr, info_ptr);

	png_destroy_write_struct(&png_ptr, &info_ptr);

	fclose(f);

	return true;
}

