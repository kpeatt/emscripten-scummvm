/* ScummVM - Scumm Interpreter
 * Copyright (C) 2001-2003 The ScummVM project
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 *
 * $Header$
 *
 */

#include "stdafx.h"
#include "simon/simon.h"
#include "simon/intern.h"

namespace Simon {

void SimonEngine::print_char_helper_1(const byte *src, uint len) {
	uint ind;

	if (_fcs_ptr_1 == NULL)
		return;

	while (len-- != 0) {
		if (*src != 12 && _fcs_ptr_1->fcs_data != NULL &&
				_fcs_data_1[ind = get_fcs_ptr_3_index(_fcs_ptr_1)] != 2) {

			_fcs_data_1[ind] = 2;
			_fcs_data_2[ind] = 1;
		}

		fcs_putchar(*src++);
	}
}

void SimonEngine::print_char_helper_5(FillOrCopyStruct *fcs) {
	uint index = get_fcs_ptr_3_index(fcs);
	print_char_helper_6(index);
	_fcs_data_1[index] = 0;
}

void SimonEngine::print_char_helper_6(uint i) {
	FillOrCopyStruct *fcs;

	if (_fcs_data_2[i]) {
		lock();
		fcs = _fcs_ptr_array_3[i];
		fcs_unk_proc_1(i, fcs->fcs_data->item_ptr, fcs->fcs_data->unk1, fcs->fcs_data->unk2);
		_fcs_data_2[i] = 0;
		unlock();
	}
}

void SimonEngine::render_string(uint num_1, uint color, uint width, uint height, const char *txt) {
	VgaPointersEntry *vpe = &_vga_buffer_pointers[2];
	byte *src, *dst, *p, *dst_org, chr;
	uint count;

	if (num_1 >= 100) {
		num_1 -= 100;
		vpe++;
	}

	src = dst = vpe->vgaFile2;

	count = 4000;
	if (num_1 == 1)
		count *= 2;

	p = dst + num_1 * 8;

	*(uint16 *)(p + 4) = TO_BE_16(height);
	*(uint16 *)(p + 6) = TO_BE_16(width);
	dst += READ_BE_UINT32(p);

	memset(dst, 0, count);
	if (_language == 20)
		dst += width - 1; // For Hebrew, start at the right edge, not the left.

	dst_org = dst;
	while ((chr = *txt++) != 0) {
		if (chr == 10) {
			dst_org += width * 10;
			dst = dst_org;
		} else if ((chr -= ' ') == 0) {
			dst += (_language == 20 ? -6 : 6); // Hebrew moves to the left, all others to the right
		} else {
			byte *img_hdr = src + 48 + chr * 4;
			uint img_height = img_hdr[2];
			uint img_width = img_hdr[3], i;
			byte *img = src + READ_LE_UINT16(img_hdr);
			if (_language == 20)
				dst -= img_width - 1; // For Hebrew, move from right edge to left edge of image.
			byte *cur_dst = dst;

			if (_game == GAME_SIMON1AMIGA) {
				// FIXME Really bad hack to allow simon1amiga to work for now
				// simon1amiga needs to be decoded a different way
				if (img_width < 1 || img_width > 49)
					img_width = 7;
				if (img_height < 1 || img_height > 49)
					img_height = 9;
			}

			assert(img_width > 0 && img_width < 50 && img_height > 0 && img_height < 50);

			do {
				for (i = 0; i != img_width; i++) {
					chr = *img++;
					if (chr) {
						if (chr == 0xF)
							chr = 207;
						else
							chr += color;
						cur_dst[i] = chr;
					}
				}
				cur_dst += width;
			} while (--img_height);

			if (_language != 20) // Hebrew character movement is done higher up
				dst += img_width - 1;
		}
	}
}

void SimonEngine::showMessageFormat(const char *s, ...) {
	char buf[1024], *str;
	va_list va;

	va_start(va, s);
	vsprintf(buf, s, va);
	va_end(va);

	if (!_fcs_data_1[_fcs_unk_1]) {
		showmessage_helper_2();
		if (!_showmessage_flag) {
			_fcs_ptr_array_3[0] = _fcs_ptr_1;
			showmessage_helper_3(_fcs_ptr_1->textLength,
                                 _fcs_ptr_1->textMaxLength);
		}
		_showmessage_flag = true;
		_fcs_data_1[_fcs_unk_1] = 1;
	}

	for (str = buf; *str; str++)
		showmessage_print_char(*str);
}

void SimonEngine::showmessage_print_char(byte chr) {
	if (chr == 12) {
		_num_letters_to_print = 0;
		_print_char_unk_1 = 0;
		print_char_helper_1(&chr, 1);
		print_char_helper_5(_fcs_ptr_1);
	} else if (chr == 0 || chr == ' ' || chr == 10) {
		if (_print_char_unk_2 - _print_char_unk_1 >= _num_letters_to_print) {
			_print_char_unk_1 += _num_letters_to_print;
			print_char_helper_1(_letters_to_print_buf, _num_letters_to_print);

			if (_print_char_unk_1 == _print_char_unk_2) {
				_print_char_unk_1 = 0;
			} else {
				if (chr)
					print_char_helper_1(&chr, 1);
				if (chr == 10)
					_print_char_unk_1 = 0;
				else if (chr != 0)
					_print_char_unk_1++;
			}
		} else {
			const byte newline_character = 10;
			_print_char_unk_1 = _num_letters_to_print;
			print_char_helper_1(&newline_character, 1);
			print_char_helper_1(_letters_to_print_buf, _num_letters_to_print);
			if (chr == ' ') {
				print_char_helper_1(&chr, 1);
				_print_char_unk_1++;
			} else {
				print_char_helper_1(&chr, 1);
				_print_char_unk_1 = 0;
			}
		}
		_num_letters_to_print = 0;
	} else {
		_letters_to_print_buf[_num_letters_to_print++] = chr;
	}
}

void SimonEngine::showmessage_helper_2() {
	if (_fcs_ptr_1)
		return;

	_fcs_ptr_1 = fcs_alloc(8, 0x90, 0x18, 6, 1, 0, 0xF);
}

void SimonEngine::showmessage_helper_3(uint a, uint b) {
	_print_char_unk_1 = a;
	_print_char_unk_2 = b;
	_num_letters_to_print = 0;
}

void SimonEngine::video_putchar(FillOrCopyStruct *fcs, byte c) {
	byte width = 6;

	if (c == 0xC) {
		video_fill_or_copy_from_3_to_2(fcs);
	} else if (c == 0xD || c == 0xA) {
		video_putchar_newline(fcs);
	} else if ((c == 1 && _language != 20) || (c == 8)) {
		if (_language == 20) { //Hebrew
			if (c >= 64 && c < 91)
				width = _hebrew_char_widths [c-64];

			if (fcs->textLength != 0) {
				fcs->textLength--;			
				fcs->textColumnOffset += width;
				if (fcs->textColumnOffset >= 8) {
					fcs->textColumnOffset -= 8;
					fcs->textColumn--;
				}
			}
		} else {
			int8 val = (c == 8) ? 6 : 4;

			if (fcs->textLength != 0) {
				fcs->textLength--;
				fcs->textColumnOffset -= val;
				if ((int8)fcs->textColumnOffset < val) {
					fcs->textColumnOffset += 8;
					fcs->textColumn--;
				}
			}
		}
	} else if (c >= 0x20) {
		if (fcs->textLength == fcs->textMaxLength) {
			video_putchar_newline(fcs);
		} else if (fcs->textRow == fcs->height) {
			video_putchar_newline(fcs);
			fcs->textRow--;
		}

		if (_language == 20) { //Hebrew
			if (c >= 64 && c < 91)
				width = _hebrew_char_widths [c-64];
			fcs->textColumnOffset  -= width;
			if (fcs->textColumnOffset >= width) {
				fcs->textColumn++;
				fcs->textColumnOffset += 8;
			}
			video_putchar_drawchar(fcs, fcs->width + fcs->x - fcs->textColumn, fcs->textRow * 8 + fcs->y, c);
			fcs->textLength++;
		} else {
			video_putchar_drawchar(fcs, fcs->textColumn + fcs->x, fcs->textRow * 8 + fcs->y, c);

			fcs->textLength++;
			fcs->textColumnOffset += 6;
			if (c == 'i' || c == 'l')
				fcs->textColumnOffset -= 2;

			if (fcs->textColumnOffset >= 8) {
				fcs->textColumnOffset -= 8;
				fcs->textColumn++;
			}
		}
	}
}

void SimonEngine::video_putchar_newline(FillOrCopyStruct *fcs) {
	fcs->textColumnOffset = 0;
	fcs->textLength = 0;
	fcs->textColumn = 0;

	if (fcs->textRow != fcs->height)
		fcs->textRow++;
}

static const byte french_video_font[] = {
	0, 0, 0, 0, 0, 0, 0, 0,
	32, 112, 112, 32, 32, 0, 32, 0,
	48, 48, 96, 0, 0, 0, 0, 0,
	32, 80, 0, 112, 136, 136, 112, 0,
	32, 80, 0, 112, 8, 248, 120, 0,
	112, 136, 128, 128, 136, 112, 32, 96,
	0, 16, 40, 16, 42, 68, 58, 0,
	48, 48, 96, 0, 0, 0, 0, 0,
	0, 4, 8, 8, 8, 8, 4, 0,
	0, 32, 16, 16, 16, 16, 32, 0,
	0, 0, 20, 8, 62, 8, 20, 0,
	112, 136, 128, 128, 136, 112, 32, 96,
	0, 0, 0, 0, 0, 48, 48, 96,
	0, 0, 0, 240, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 48, 48, 0,
	32, 64, 0, 112, 248, 128, 112, 0,
	112, 136, 152, 168, 200, 136, 112, 0,
	32, 96, 32, 32, 32, 32, 112, 0,
	112, 136, 8, 48, 64, 136, 248, 0,
	112, 136, 8, 48, 8, 136, 112, 0,
	16, 48, 80, 144, 248, 16, 56, 0,
	248, 128, 240, 8, 8, 136, 112, 0,
	48, 64, 128, 240, 136, 136, 112, 0,
	248, 136, 8, 16, 32, 32, 32, 0,
	112, 136, 136, 112, 136, 136, 112, 0,
	112, 136, 136, 120, 8, 16, 96, 0,
	0, 0, 48, 48, 0, 48, 48, 0,
	32, 16, 0, 112, 8, 248, 120, 0,
	32, 80, 0, 144, 144, 144, 104, 0,
	32, 16, 0, 112, 248, 128, 112, 0,
	32, 80, 0, 112, 248, 128, 112, 0,
	112, 136, 8, 16, 32, 0, 32, 0,
	32, 80, 0, 192, 64, 64, 224, 0,
	112, 136, 136, 248, 136, 136, 136, 0,
	240, 72, 72, 112, 72, 72, 240, 0,
	48, 72, 128, 128, 128, 72, 48, 0,
	224, 80, 72, 72, 72, 80, 224, 0,
	248, 72, 64, 112, 64, 72, 248, 0,
	248, 72, 64, 112, 64, 64, 224, 0,
	48, 72, 128, 152, 136, 72, 56, 0,
	136, 136, 136, 248, 136, 136, 136, 0,
	248, 32, 32, 32, 32, 32, 248, 0,
	24, 8, 8, 8, 136, 136, 112, 0,
	200, 72, 80, 96, 80, 72, 200, 0,
	224, 64, 64, 64, 64, 72, 248, 0,
	136, 216, 168, 168, 136, 136, 136, 0,
	136, 200, 168, 152, 136, 136, 136, 0,
	112, 136, 136, 136, 136, 136, 112, 0,
	240, 72, 72, 112, 64, 64, 224, 0,
	112, 136, 136, 136, 136, 168, 112, 8,
	240, 72, 72, 112, 72, 72, 200, 0,
	112, 136, 128, 112, 8, 136, 112, 0,
	248, 168, 32, 32, 32, 32, 112, 0,
	136, 136, 136, 136, 136, 136, 120, 0,
	136, 136, 136, 80, 80, 32, 32, 0,
	136, 136, 136, 136, 168, 216, 136, 0,
	136, 136, 80, 32, 80, 136, 136, 0,
	136, 136, 136, 112, 32, 32, 112, 0,
	248, 136, 16, 32, 64, 136, 248, 0,
	0, 14, 8, 8, 8, 8, 14, 0,
	0, 128, 64, 32, 16, 8, 4, 0,
	0, 112, 16, 16, 16, 16, 112, 0,
	0, 0, 112, 136, 128, 112, 32, 96,
	160, 0, 192, 64, 64, 64, 224, 0,
	32, 16, 0, 144, 144, 144, 104, 0,
	0, 0, 112, 8, 120, 136, 120, 0,
	192, 64, 80, 104, 72, 72, 112, 0,
	0, 0, 112, 136, 128, 136, 112, 0,
	24, 16, 80, 176, 144, 144, 112, 0,
	0, 0, 112, 136, 248, 128, 112, 0,
	48, 72, 64, 224, 64, 64, 224, 0,
	0, 0, 104, 144, 144, 112, 136, 112,
	192, 64, 80, 104, 72, 72, 200, 0,
	64, 0, 192, 64, 64, 64, 224, 0,
	8, 0, 8, 8, 8, 8, 136, 112,
	192, 64, 72, 80, 96, 80, 200, 0,
	192, 64, 64, 64, 64, 64, 224, 0,
	0, 0, 144, 216, 168, 136, 136, 0,
	0, 0, 240, 136, 136, 136, 136, 0,
	0, 0, 112, 136, 136, 136, 112, 0,
	0, 0, 176, 72, 72, 112, 64, 224,
	0, 0, 104, 144, 144, 112, 16, 56,
	0, 0, 176, 72, 72, 64, 224, 0,
	0, 0, 120, 128, 112, 8, 240, 0,
	64, 64, 240, 64, 64, 72, 48, 0,
	0, 0, 144, 144, 144, 144, 104, 0,
	0, 0, 136, 136, 136, 80, 32, 0,
	0, 0, 136, 136, 168, 216, 144, 0,
	0, 0, 136, 80, 32, 80, 136, 0,
	0, 0, 136, 136, 136, 112, 32, 192,
	0, 0, 248, 144, 32, 72, 248, 0,
	32, 80, 0, 96, 144, 144, 96, 0,
	0, 14, 8, 48, 8, 8, 14, 0,
	0, 8, 8, 8, 8, 8, 8, 0,
	0, 112, 16, 12, 16, 16, 112, 0,
	0, 0, 0, 0, 0, 0, 248, 0,
	139, 2, 178, 252, 8, 0, 176, 240,
	8, 0, 176, 0, 15, 0, 176, 255,
};

static const byte german_video_font[] = {
	0, 0, 0, 0, 0, 0, 0, 0,
	32, 112, 112, 32, 32, 0, 32, 0,
	48, 48, 96, 0, 0, 0, 0, 0,
	80, 0, 112, 8, 120, 136, 120, 0,
	80, 0, 112, 136, 136, 136, 112, 0,
	80, 0, 144, 144, 144, 144, 104, 0,
	0, 16, 40, 16, 42, 68, 58, 0,
	48, 48, 96, 0, 0, 0, 0, 0,
	0, 4, 8, 8, 8, 8, 4, 0,
	0, 32, 16, 16, 16, 16, 32, 0,
	0, 0, 20, 8, 62, 8, 20, 0,
	96, 144, 144, 160, 144, 144, 160, 128,
	0, 0, 0, 0, 0, 48, 48, 96,
	0, 0, 0, 240, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 48, 48, 0,
	80, 0, 112, 136, 248, 136, 136, 0,
	112, 136, 152, 168, 200, 136, 112, 0,
	32, 96, 32, 32, 32, 32, 112, 0,
	112, 136, 8, 48, 64, 136, 248, 0,
	112, 136, 8, 48, 8, 136, 112, 0,
	16, 48, 80, 144, 248, 16, 56, 0,
	248, 128, 240, 8, 8, 136, 112, 0,
	48, 64, 128, 240, 136, 136, 112, 0,
	248, 136, 8, 16, 32, 32, 32, 0,
	112, 136, 136, 112, 136, 136, 112, 0,
	112, 136, 136, 120, 8, 16, 96, 0,
	0, 0, 48, 48, 0, 48, 48, 0,
	136, 112, 136, 136, 136, 136, 112, 0,
	80, 0, 136, 136, 136, 136, 112, 0,
	80, 0, 144, 144, 144, 144, 104, 0,
	32, 64, 0, 112, 248, 128, 112, 0,
	112, 136, 8, 16, 32, 0, 32, 0,
	32, 80, 0, 192, 64, 64, 224, 0,
	112, 136, 136, 248, 136, 136, 136, 0,
	240, 72, 72, 112, 72, 72, 240, 0,
	48, 72, 128, 128, 128, 72, 48, 0,
	224, 80, 72, 72, 72, 80, 224, 0,
	248, 72, 64, 112, 64, 72, 248, 0,
	248, 72, 64, 112, 64, 64, 224, 0,
	48, 72, 128, 152, 136, 72, 56, 0,
	136, 136, 136, 248, 136, 136, 136, 0,
	248, 32, 32, 32, 32, 32, 248, 0,
	24, 8, 8, 8, 136, 136, 112, 0,
	200, 72, 80, 96, 80, 72, 200, 0,
	224, 64, 64, 64, 64, 72, 248, 0,
	136, 216, 168, 168, 136, 136, 136, 0,
	136, 200, 168, 152, 136, 136, 136, 0,
	112, 136, 136, 136, 136, 136, 112, 0,
	240, 72, 72, 112, 64, 64, 224, 0,
	112, 136, 136, 136, 136, 168, 112, 8,
	240, 72, 72, 112, 72, 72, 200, 0,
	112, 136, 128, 112, 8, 136, 112, 0,
	248, 168, 32, 32, 32, 32, 112, 0,
	136, 136, 136, 136, 136, 136, 120, 0,
	136, 136, 136, 80, 80, 32, 32, 0,
	136, 136, 136, 136, 168, 216, 136, 0,
	136, 136, 80, 32, 80, 136, 136, 0,
	136, 136, 136, 112, 32, 32, 112, 0,
	248, 136, 16, 32, 64, 136, 248, 0,
	0, 14, 8, 8, 8, 8, 14, 0,
	0, 128, 64, 32, 16, 8, 4, 0,
	0, 112, 16, 16, 16, 16, 112, 0,
	0, 48, 72, 64, 72, 48, 16, 48,
	0, 80, 0, 96, 32, 40, 48, 0,
	32, 16, 0, 152, 144, 144, 232, 0,
	0, 0, 112, 8, 120, 136, 120, 0,
	192, 64, 80, 104, 72, 72, 112, 0,
	0, 0, 112, 136, 128, 136, 112, 0,
	24, 16, 80, 176, 144, 144, 112, 0,
	0, 0, 112, 136, 248, 128, 112, 0,
	48, 72, 64, 224, 64, 64, 224, 0,
	0, 0, 104, 144, 144, 112, 136, 112,
	192, 64, 80, 104, 72, 72, 200, 0,
	64, 0, 192, 64, 64, 64, 224, 0,
	8, 0, 8, 8, 8, 8, 136, 112,
	192, 64, 72, 80, 96, 80, 200, 0,
	192, 64, 64, 64, 64, 64, 224, 0,
	0, 0, 144, 216, 168, 136, 136, 0,
	0, 0, 240, 136, 136, 136, 136, 0,
	0, 0, 112, 136, 136, 136, 112, 0,
	0, 0, 176, 72, 72, 112, 64, 224,
	0, 0, 104, 144, 144, 112, 16, 56,
	0, 0, 176, 72, 72, 64, 224, 0,
	0, 0, 120, 128, 112, 8, 240, 0,
	64, 64, 240, 64, 64, 72, 48, 0,
	0, 0, 144, 144, 144, 144, 104, 0,
	0, 0, 136, 136, 136, 80, 32, 0,
	0, 0, 136, 136, 168, 216, 144, 0,
	0, 0, 136, 80, 32, 80, 136, 0,
	0, 0, 136, 136, 136, 112, 32, 192,
	0, 0, 248, 144, 32, 72, 248, 0,
	32, 80, 0, 96, 144, 144, 96, 0,
	0, 14, 8, 48, 8, 8, 14, 0,
	0, 8, 8, 8, 8, 8, 8, 0,
	0, 112, 16, 12, 16, 16, 112, 0,
	0, 0, 0, 0, 0, 0, 248, 0,
	139, 2, 178, 252, 8, 0, 176, 240,
	8, 0, 176, 0, 16, 0, 176, 255,
};

static const byte hebrew_video_font[] = {
	0, 0, 0, 0, 0, 0, 0, 0,
	32, 112, 112, 32, 32, 0, 32, 0,
	48, 48, 96, 0, 0, 0, 0, 0,
	0, 144, 0, 96, 144, 144, 104, 0,
	0, 144, 0, 96, 144, 144, 96, 0,
	0, 144, 0, 144, 144, 144, 96, 0,
	0, 16, 40, 16, 42, 68, 58, 0,
	48, 48, 96, 0, 0, 0, 0, 0,
	0, 4, 8, 8, 8, 8, 4, 0,
	0, 32, 16, 16, 16, 16, 32, 0,
	0, 0, 20, 8, 62, 8, 20, 0,
	0, 112, 136, 240, 136, 136, 240, 0,
	0, 0, 0, 0, 0, 48, 48, 96,
	0, 0, 0, 240, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 48, 48, 0,
	16, 32, 0, 120, 112, 64, 56, 0,
	112, 136, 152, 168, 200, 136, 112, 0,
	32, 96, 32, 32, 32, 32, 112, 0,
	112, 136, 8, 48, 64, 136, 248, 0,
	112, 136, 8, 48, 8, 136, 112, 0,
	16, 48, 80, 144, 248, 16, 56, 0,
	248, 128, 240, 8, 8, 136, 112, 0,
	48, 64, 128, 240, 136, 136, 112, 0,
	248, 136, 8, 16, 32, 32, 32, 0,
	112, 136, 136, 112, 136, 136, 112, 0,
	112, 136, 136, 120, 8, 16, 96, 0,
	0, 0, 48, 48, 0, 48, 48, 0,
	5, 5, 4, 6, 5, 3, 4, 5,
	6, 3, 5, 5, 4, 6, 5, 3,
	4, 6, 5, 6, 6, 6, 5, 5,
	5, 6, 5, 6, 6, 6, 6, 6,
	112, 136, 8, 16, 32, 0, 32, 0,
	0, 0, 144, 80, 160, 144, 144, 0,
	0, 0, 224, 32, 32, 32, 240, 0,
	0, 0, 224, 32, 96, 160, 160, 0,
	0, 0, 248, 16, 16, 16, 16, 0,
	0, 0, 240, 16, 16, 144, 144, 0,
	0, 0, 192, 64, 64, 64, 64, 0,
	0, 0, 224, 64, 32, 64, 64, 0,
	0, 0, 240, 144, 144, 144, 144, 0,
	0, 0, 184, 168, 136, 136, 112, 0,
	0, 0, 192, 64, 0, 0, 0, 0,
	0, 0, 240, 16, 16, 16, 16, 16,
	0, 0, 224, 16, 16, 16, 224, 0,
	128, 128, 224, 32, 32, 32, 192, 0,
	0, 0, 248, 72, 72, 72, 120, 0,
	0, 0, 176, 208, 144, 144, 176, 0,
	0, 0, 192, 64, 64, 64, 64, 64,
	0, 0, 96, 32, 32, 32, 224, 0,
	0, 0, 248, 72, 72, 72, 48, 0,
	0, 0, 80, 80, 80, 80, 224, 0,
	0, 0, 248, 72, 104, 8, 8, 8,
	0, 0, 248, 72, 104, 8, 248, 0,
	0, 0, 216, 72, 48, 16, 16, 16,
	0, 0, 144, 80, 32, 16, 240, 0,
	0, 0, 240, 16, 144, 160, 128, 128,
	0, 0, 240, 16, 16, 16, 16, 0,
	0, 0, 168, 168, 200, 136, 112, 0,
	0, 0, 240, 80, 80, 80, 208, 0,
	0, 14, 8, 8, 8, 8, 14, 0,
	0, 128, 64, 32, 16, 8, 4, 0,
	0, 112, 16, 16, 16, 16, 112, 0,
	0, 48, 72, 64, 72, 48, 16, 48,
	0, 80, 0, 96, 32, 40, 48, 0,
	32, 16, 0, 152, 144, 144, 232, 0,
	0, 0, 112, 8, 120, 136, 120, 0,
	192, 64, 80, 104, 72, 72, 112, 0,
	0, 0, 112, 136, 128, 136, 112, 0,
	24, 16, 80, 176, 144, 144, 112, 0,
	0, 0, 112, 136, 248, 128, 112, 0,
	48, 72, 64, 224, 64, 64, 224, 0,
	0, 0, 104, 144, 144, 112, 136, 112,
	192, 64, 80, 104, 72, 72, 200, 0,
	64, 0, 192, 64, 64, 64, 224, 0,
	8, 0, 8, 8, 8, 8, 136, 112,
	192, 64, 72, 80, 96, 80, 200, 0,
	192, 64, 64, 64, 64, 64, 224, 0,
	0, 0, 144, 216, 168, 136, 136, 0,
	0, 0, 240, 136, 136, 136, 136, 0,
	0, 0, 112, 136, 136, 136, 112, 0,
	0, 0, 176, 72, 72, 112, 64, 224,
	0, 0, 104, 144, 144, 112, 16, 56,
	0, 0, 176, 72, 72, 64, 224, 0,
	0, 0, 120, 128, 112, 8, 240, 0,
	64, 64, 240, 64, 64, 72, 48, 0,
	0, 0, 144, 144, 144, 144, 104, 0,
	0, 0, 136, 136, 136, 80, 32, 0,
	0, 0, 136, 136, 168, 216, 144, 0,
	0, 0, 136, 80, 32, 80, 136, 0,
	0, 0, 136, 136, 136, 112, 32, 192,
	0, 0, 248, 144, 32, 72, 248, 0,
	32, 80, 0, 96, 144, 144, 96, 0,
	0, 14, 8, 48, 8, 8, 14, 0,
	0, 8, 8, 8, 8, 8, 8, 0,
	0, 112, 16, 12, 16, 16, 112, 0,
	0, 0, 0, 0, 0, 0, 248, 0,
	139, 2, 178, 252, 8, 0, 176, 240,
	8, 0, 176, 32, 33, 34, 35, 36,
};

static const byte italian_video_font[] = {
	0, 0, 0, 0, 0, 0, 0, 0,
	32, 112, 112, 32, 32, 0, 32, 0,
	48, 48, 96, 0, 0, 0, 0, 0,
	80, 0, 112, 8, 120, 136, 120, 0,
	80, 0, 112, 136, 136, 136, 112, 0,
	32, 16, 0, 112, 136, 136, 112, 0,
	0, 16, 40, 16, 42, 68, 58, 0,
	48, 48, 96, 0, 0, 0, 0, 0,
	0, 4, 8, 8, 8, 8, 4, 0,
	0, 32, 16, 16, 16, 16, 32, 0,
	0, 0, 20, 8, 62, 8, 20, 0,
	32, 16, 0, 192, 64, 64, 224, 0,
	0, 0, 0, 0, 0, 48, 48, 96,
	0, 0, 0, 240, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 48, 48, 0,
	32, 64, 0, 112, 248, 128, 112, 0,
	112, 136, 152, 168, 200, 136, 112, 0,
	32, 96, 32, 32, 32, 32, 112, 0,
	112, 136, 8, 48, 64, 136, 248, 0,
	112, 136, 8, 48, 8, 136, 112, 0,
	16, 48, 80, 144, 248, 16, 56, 0,
	248, 128, 240, 8, 8, 136, 112, 0,
	48, 64, 128, 240, 136, 136, 112, 0,
	248, 136, 8, 16, 32, 32, 32, 0,
	112, 136, 136, 112, 136, 136, 112, 0,
	112, 136, 136, 120, 8, 16, 96, 0,
	0, 0, 48, 48, 0, 48, 48, 0,
	32, 16, 0, 112, 8, 248, 120, 0,
	32, 16, 0, 112, 136, 136, 112, 0,
	32, 16, 0, 112, 248, 128, 112, 0,
	32, 80, 0, 112, 248, 128, 112, 0,
	112, 136, 8, 16, 32, 0, 32, 0,
	32, 80, 0, 96, 32, 40, 48, 0,
	112, 136, 136, 248, 136, 136, 136, 0,
	240, 72, 72, 112, 72, 72, 240, 0,
	48, 72, 128, 128, 128, 72, 48, 0,
	224, 80, 72, 72, 72, 80, 224, 0,
	248, 72, 64, 112, 64, 72, 248, 0,
	248, 72, 64, 112, 64, 64, 224, 0,
	48, 72, 128, 152, 136, 72, 56, 0,
	136, 136, 136, 248, 136, 136, 136, 0,
	248, 32, 32, 32, 32, 32, 248, 0,
	24, 8, 8, 8, 136, 136, 112, 0,
	200, 72, 80, 96, 80, 72, 200, 0,
	224, 64, 64, 64, 64, 72, 248, 0,
	136, 216, 168, 168, 136, 136, 136, 0,
	136, 200, 168, 152, 136, 136, 136, 0,
	112, 136, 136, 136, 136, 136, 112, 0,
	240, 72, 72, 112, 64, 64, 224, 0,
	112, 136, 136, 136, 136, 168, 112, 8,
	240, 72, 72, 112, 72, 72, 200, 0,
	112, 136, 128, 112, 8, 136, 112, 0,
	248, 168, 32, 32, 32, 32, 112, 0,
	136, 136, 136, 136, 136, 136, 120, 0,
	136, 136, 136, 80, 80, 32, 32, 0,
	136, 136, 136, 136, 168, 216, 136, 0,
	136, 136, 80, 32, 80, 136, 136, 0,
	136, 136, 136, 112, 32, 32, 112, 0,
	248, 136, 16, 32, 64, 136, 248, 0,
	0, 14, 8, 8, 8, 8, 14, 0,
	0, 128, 64, 32, 16, 8, 4, 0,
	0, 112, 16, 16, 16, 16, 112, 0,
	0, 0, 112, 136, 128, 112, 32, 96,
	160, 0, 192, 64, 64, 64, 224, 0,
	32, 16, 0, 144, 144, 144, 104, 0,
	0, 0, 112, 8, 120, 136, 120, 0,
	192, 64, 80, 104, 72, 72, 112, 0,
	0, 0, 112, 136, 128, 136, 112, 0,
	24, 16, 80, 176, 144, 144, 112, 0,
	0, 0, 112, 136, 248, 128, 112, 0,
	48, 72, 64, 224, 64, 64, 224, 0,
	0, 0, 104, 144, 144, 112, 136, 112,
	192, 64, 80, 104, 72, 72, 200, 0,
	64, 0, 192, 64, 64, 64, 224, 0,
	8, 0, 8, 8, 8, 8, 136, 112,
	192, 64, 72, 80, 96, 80, 200, 0,
	192, 64, 64, 64, 64, 64, 224, 0,
	0, 0, 144, 216, 168, 136, 136, 0,
	0, 0, 240, 136, 136, 136, 136, 0,
	0, 0, 112, 136, 136, 136, 112, 0,
	0, 0, 176, 72, 72, 112, 64, 224,
	0, 0, 104, 144, 144, 112, 16, 56,
	0, 0, 176, 72, 72, 64, 224, 0,
	0, 0, 120, 128, 112, 8, 240, 0,
	64, 64, 240, 64, 64, 72, 48, 0,
	0, 0, 144, 144, 144, 144, 104, 0,
	0, 0, 136, 136, 136, 80, 32, 0,
	0, 0, 136, 136, 168, 216, 144, 0,
	0, 0, 136, 80, 32, 80, 136, 0,
	0, 0, 136, 136, 136, 112, 32, 192,
	0, 0, 248, 144, 32, 72, 248, 0,
	32, 80, 0, 96, 144, 144, 96, 0,
	0, 14, 8, 48, 8, 8, 14, 0,
	0, 8, 8, 8, 8, 8, 8, 0,
	0, 112, 16, 12, 16, 16, 112, 0,
	0, 0, 0, 0, 0, 0, 248, 0,
	139, 2, 178, 252, 8, 0, 176, 240,
	8, 0, 176, 0, 16, 0, 176, 255,
};

static const byte spanish_video_font[] = {
	0, 0, 0, 0, 0, 0, 0, 0,
	32, 112, 112, 32, 32, 0, 32, 0,
	48, 48, 96, 0, 0, 0, 0, 0,
	80, 0, 112, 8, 120, 136, 120, 0,
	80, 0, 112, 136, 136, 136, 112, 0,
	80, 0, 144, 144, 144, 144, 104, 0,
	0, 16, 40, 16, 42, 68, 58, 0,
	48, 48, 96, 0, 0, 0, 0, 0,
	0, 4, 8, 8, 8, 8, 4, 0,
	0, 32, 16, 16, 16, 16, 32, 0,
	0, 0, 20, 8, 62, 8, 20, 0,
	96, 144, 144, 160, 144, 144, 160, 128,
	0, 0, 0, 0, 0, 48, 48, 96,
	0, 0, 0, 240, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 48, 48, 0,
	32, 64, 0, 112, 248, 128, 112, 0,
	112, 136, 152, 168, 200, 136, 112, 0,
	32, 96, 32, 32, 32, 32, 112, 0,
	112, 136, 8, 48, 64, 136, 248, 0,
	112, 136, 8, 48, 8, 136, 112, 0,
	16, 48, 80, 144, 248, 16, 56, 0,
	248, 128, 240, 8, 8, 136, 112, 0,
	48, 64, 128, 240, 136, 136, 112, 0,
	248, 136, 8, 16, 32, 32, 32, 0,
	112, 136, 136, 112, 136, 136, 112, 0,
	112, 136, 136, 120, 8, 16, 96, 0,
	0, 0, 48, 48, 0, 48, 48, 0,
	32, 64, 0, 112, 8, 248, 120, 0,
	32, 64, 0, 192, 64, 64, 224, 0, 
	32, 64, 0, 112, 136, 136, 112, 0,
	32, 64, 0, 144, 144, 144, 104, 0,
	112, 136, 8, 16, 32, 0, 32, 0,
	80, 160, 0, 240, 136, 136, 136, 0,
	112, 136, 136, 248, 136, 136, 136, 0,
	240, 72, 72, 112, 72, 72, 240, 0,
	48, 72, 128, 128, 128, 72, 48, 0,
	224, 80, 72, 72, 72, 80, 224, 0,
	248, 72, 64, 112, 64, 72, 248, 0,
	248, 72, 64, 112, 64, 64, 224, 0,
	48, 72, 128, 152, 136, 72, 56, 0,
	136, 136, 136, 248, 136, 136, 136, 0,
	248, 32, 32, 32, 32, 32, 248, 0,
	24, 8, 8, 8, 136, 136, 112, 0,
	200, 72, 80, 96, 80, 72, 200, 0,
	224, 64, 64, 64, 64, 72, 248, 0,
	136, 216, 168, 168, 136, 136, 136, 0,
	136, 200, 168, 152, 136, 136, 136, 0,
	112, 136, 136, 136, 136, 136, 112, 0,
	240, 72, 72, 112, 64, 64, 224, 0,
	112, 136, 136, 136, 136, 168, 112, 8,
	240, 72, 72, 112, 72, 72, 200, 0,
	112, 136, 128, 112, 8, 136, 112, 0,
	248, 168, 32, 32, 32, 32, 112, 0,
	136, 136, 136, 136, 136, 136, 120, 0,
	136, 136, 136, 80, 80, 32, 32, 0,
	136, 136, 136, 136, 168, 216, 136, 0,
	136, 136, 80, 32, 80, 136, 136, 0,
	136, 136, 136, 112, 32, 32, 112, 0,
	248, 136, 16, 32, 64, 136, 248, 0,
	0, 14, 8, 8, 8, 8, 14, 0,
	0, 128, 64, 32, 16, 8, 4, 0,
	0, 112, 16, 16, 16, 16, 112, 0,
	32, 0, 32, 64, 128, 136, 112, 0,
	32, 0, 32, 32, 112, 112, 32, 0,
	80, 0, 144, 144, 144, 144, 104, 0,
	0, 0, 112, 8, 120, 136, 120, 0,
	192, 64, 80, 104, 72, 72, 112, 0,
	0, 0, 112, 136, 128, 136, 112, 0,
	24, 16, 80, 176, 144, 144, 112, 0,
	0, 0, 112, 136, 248, 128, 112, 0,
	48, 72, 64, 224, 64, 64, 224, 0,
	0, 0, 104, 144, 144, 112, 136, 112,
	192, 64, 80, 104, 72, 72, 200, 0,
	64, 0, 192, 64, 64, 64, 224, 0,
	8, 0, 8, 8, 8, 8, 136, 112,
	192, 64, 72, 80, 96, 80, 200, 0,
	192, 64, 64, 64, 64, 64, 224, 0,
	0, 0, 144, 216, 168, 136, 136, 0,
	0, 0, 240, 136, 136, 136, 136, 0,
	0, 0, 112, 136, 136, 136, 112, 0,
	0, 0, 176, 72, 72, 112, 64, 224,
	0, 0, 104, 144, 144, 112, 16, 56,
	0, 0, 176, 72, 72, 64, 224, 0,
	0, 0, 120, 128, 112, 8, 240, 0,
	64, 64, 240, 64, 64, 72, 48, 0,
	0, 0, 144, 144, 144, 144, 104, 0,
	0, 0, 136, 136, 136, 80, 32, 0,
	0, 0, 136, 136, 168, 216, 144, 0,
	0, 0, 136, 80, 32, 80, 136, 0,
	0, 0, 136, 136, 136, 112, 32, 192,
	0, 0, 248, 144, 32, 72, 248, 0,
	32, 80, 0, 96, 144, 144, 96, 0,
	0, 14, 8, 48, 8, 8, 14, 0,
	0, 8, 8, 8, 8, 8, 8, 0,
	0, 112, 16, 12, 16, 16, 112, 0,
	0, 0, 0, 0, 0, 0, 248, 0,
	139, 2, 178, 252, 8, 0, 176, 240,
	8, 0, 176, 0, 16, 0, 176, 255,
};

static const byte video_font[] = {
	0, 0, 0, 0, 0, 0, 0, 0,
	32, 112, 112, 32, 32, 0, 32, 0,
	48, 48, 96, 0, 0, 0, 0, 0,
	0, 144, 0, 96, 144, 144, 104, 0,
	0, 144, 0, 96, 144, 144, 96, 0,
	0, 144, 0, 144, 144, 144, 96, 0,
	0, 16, 40, 16, 42, 68, 58, 0,
	48, 48, 96, 0, 0, 0, 0, 0,
	0, 4, 8, 8, 8, 8, 4, 0,
	0, 32, 16, 16, 16, 16, 32, 0,
	0, 0, 20, 8, 62, 8, 20, 0,
	0, 112, 136, 240, 136, 136, 240, 0,
	0, 0, 0, 0, 0, 48, 48, 96,
	0, 0, 0, 240, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 48, 48, 0,
	16, 32, 0, 120, 112, 64, 56, 0,
	112, 136, 152, 168, 200, 136, 112, 0,
	32, 96, 32, 32, 32, 32, 112, 0,
	112, 136, 8, 48, 64, 136, 248, 0,
	112, 136, 8, 48, 8, 136, 112, 0,
	16, 48, 80, 144, 248, 16, 56, 0,
	248, 128, 240, 8, 8, 136, 112, 0,
	48, 64, 128, 240, 136, 136, 112, 0,
	248, 136, 8, 16, 32, 32, 32, 0,
	112, 136, 136, 112, 136, 136, 112, 0,
	112, 136, 136, 120, 8, 16, 96, 0,
	0, 0, 48, 48, 0, 48, 48, 0,
	32, 16, 0, 112, 8, 248, 120, 0,
	32, 80, 0, 144, 144, 144, 104, 0,
	80, 0, 144, 144, 144, 144, 104, 0,
	32, 80, 0, 112, 248, 128, 112, 0,
	112, 136, 8, 16, 32, 0, 32, 0,
	32, 80, 0, 192, 64, 64, 224, 0,
	112, 136, 136, 248, 136, 136, 136, 0,
	240, 72, 72, 112, 72, 72, 240, 0,
	48, 72, 128, 128, 128, 72, 48, 0,
	224, 80, 72, 72, 72, 80, 224, 0,
	248, 72, 64, 112, 64, 72, 248, 0,
	248, 72, 64, 112, 64, 64, 224, 0,
	48, 72, 128, 152, 136, 72, 56, 0,
	136, 136, 136, 248, 136, 136, 136, 0,
	248, 32, 32, 32, 32, 32, 248, 0,
	24, 8, 8, 8, 136, 136, 112, 0,
	200, 72, 80, 96, 80, 72, 200, 0,
	224, 64, 64, 64, 64, 72, 248, 0,
	136, 216, 168, 168, 136, 136, 136, 0,
	136, 200, 168, 152, 136, 136, 136, 0,
	112, 136, 136, 136, 136, 136, 112, 0,
	240, 72, 72, 112, 64, 64, 224, 0,
	112, 136, 136, 136, 136, 168, 112, 8,
	240, 72, 72, 112, 72, 72, 200, 0,
	112, 136, 128, 112, 8, 136, 112, 0,
	248, 168, 32, 32, 32, 32, 112, 0,
	136, 136, 136, 136, 136, 136, 120, 0,
	136, 136, 136, 80, 80, 32, 32, 0,
	136, 136, 136, 136, 168, 216, 136, 0,
	136, 136, 80, 32, 80, 136, 136, 0,
	136, 136, 136, 112, 32, 32, 112, 0,
	248, 136, 16, 32, 64, 136, 248, 0,
	0, 14, 8, 8, 8, 8, 14, 0,
	0, 128, 64, 32, 16, 8, 4, 0,
	0, 112, 16, 16, 16, 16, 112, 0,
	0, 48, 72, 64, 72, 48, 16, 48,
	0, 80, 0, 96, 32, 40, 48, 0,
	32, 16, 0, 152, 144, 144, 232, 0,
	0, 0, 112, 8, 120, 136, 120, 0,
	192, 64, 80, 104, 72, 72, 112, 0,
	0, 0, 112, 136, 128, 136, 112, 0,
	24, 16, 80, 176, 144, 144, 112, 0,
	0, 0, 112, 136, 248, 128, 112, 0,
	48, 72, 64, 224, 64, 64, 224, 0,
	0, 0, 104, 144, 144, 112, 136, 112,
	192, 64, 80, 104, 72, 72, 200, 0,
	64, 0, 192, 64, 64, 64, 224, 0,
	8, 0, 8, 8, 8, 8, 136, 112,
	192, 64, 72, 80, 96, 80, 200, 0,
	192, 64, 64, 64, 64, 64, 224, 0,
	0, 0, 144, 216, 168, 136, 136, 0,
	0, 0, 240, 136, 136, 136, 136, 0,
	0, 0, 112, 136, 136, 136, 112, 0,
	0, 0, 176, 72, 72, 112, 64, 224,
	0, 0, 104, 144, 144, 112, 16, 56,
	0, 0, 176, 72, 72, 64, 224, 0,
	0, 0, 120, 128, 112, 8, 240, 0,
	64, 64, 240, 64, 64, 72, 48, 0,
	0, 0, 144, 144, 144, 144, 104, 0,
	0, 0, 136, 136, 136, 80, 32, 0,
	0, 0, 136, 136, 168, 216, 144, 0,
	0, 0, 136, 80, 32, 80, 136, 0,
	0, 0, 136, 136, 136, 112, 32, 192,
	0, 0, 248, 144, 32, 72, 248, 0,
	32, 80, 0, 96, 144, 144, 96, 0,
	0, 14, 8, 48, 8, 8, 14, 0,
	0, 8, 8, 8, 8, 8, 8, 0,
	0, 112, 16, 12, 16, 16, 112, 0,
	0, 0, 0, 0, 0, 0, 248, 0,
	252, 252, 252, 252, 252, 252, 252, 252,
	240, 240, 240, 240, 240, 240, 240, 240,
};

void SimonEngine::video_putchar_drawchar(FillOrCopyStruct *fcs, uint x, uint y, byte chr) {
	const byte *src;
	byte color, *dst;
	uint h, i;

	_lock_word |= 0x8000;

	dst = dx_lock_2();
	dst += y * _dx_surface_pitch + x * 8 + fcs->textColumnOffset;

	if (_language == 20) {
		src = hebrew_video_font + (chr - 0x20) * 8;
	} else if (_language == 5) {
		src = spanish_video_font + (chr - 0x20) * 8;
	} else if (_language == 3) {
		src = italian_video_font + (chr - 0x20) * 8;
	} else if (_language == 2) {
		src = french_video_font + (chr - 0x20) * 8;
	} else if (_language == 1) {
		src = german_video_font + (chr - 0x20) * 8;
	} else
		src = video_font + (chr - 0x20) * 8;

	color = fcs->text_color;

	h = 8;
	do {
		int8 b = *src++;
		i = 0;
		do {
			if (b < 0)
				dst[i] = color;
			b <<= 1;
		} while (++i != 6);
		dst += _dx_surface_pitch;
	} while (--h);

	dx_unlock_2();

	_lock_word &= ~0x8000;
}

} // End of namespace Simon
