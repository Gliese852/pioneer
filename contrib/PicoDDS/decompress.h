//-----------------------------------------------------------------------------
//
// ImageLib Sources
// Copyright (C) 2000-2017 by Denton Woods
// Last modified: 02/28/2009
//
// Filename: src-IL/src/il_dds.cpp
//
// Description: Reads from a DirectDraw Surface (.dds) file.
//
//-----------------------------------------------------------------------------


//
//
// Note:  Almost all this code is from nVidia's DDS-loading example at
//	http://www.nvidia.com/view.asp?IO=dxtc_decompression_code
//	and from the specs at
//	http://msdn.microsoft.com/library/default.asp?url=/library/en-us/dx8_c/hh/dx8_c/graphics_using_0j03.asp
//	and
//	http://msdn.microsoft.com/library/default.asp?url=/library/en-us/dx8_c/directx_cpp/Graphics/ProgrammersGuide/Appendix/DDSFileFormat/ovwDDSFileFormat.asp
//	However, some not really valid .dds files are also read, for example
//	Volume Textures without the COMPLEX bit set, so the specs aren't taken
//	too strictly while reading.


// Global variables


/*
 * Assumes that the global variable CompFormat stores the format of the
 * global pointer CompData (that's the pointer to the compressed data).
 * Decompresses this data into Image->Data, returns if it was successful.
 * It also uses the globals Width and Height.
 *
 * Assumes that iCurImage has valid Width, Height, Depth, Data, SizeOfData,
 * Bpp, Bpc, Bps, SizeOfPlane, Format and Type fields. It is more or
 * less assumed that the image has u8 rgba data (perhaps not for float
 * images...)
 *
 *
 * @TODO: don't use globals, clean this function (and this file) up
 */

#include <cstring>
#include "PicoDDS.h"

// use cast to struct instead of RGBA_MAKE as struct is
//  much
struct Color8888
{
	uint8_t r;		// change the order of names to change the
	uint8_t g;		//  order of the output ARGB or BGRA, etc...
	uint8_t b;		//  Last one is MSB, 1st is LSB.
	uint8_t a;
};

//! The Fundamental Image structure
/*! Every bit of information about an image is stored in this internal structure.*/
struct ILimage
{
	uint32_t Width;       //!< the image's width
	uint32_t Height;      //!< the image's height
	uint32_t Depth;       //!< the image's depth
	uint8_t  Bpp;         //!< bytes per pixel (now number of channels)
	uint32_t Bps;         //!< bytes per scanline (components for IL)
	uint8_t* Data;        //!< the image data
	uint32_t SizeOfData;  //!< the total size of the data (in bytes)
	uint32_t SizeOfPlane; //!< SizeOfData in a 2d image, size of each plane slice in a 3d image (in bytes)
};

void iSwapUInt(uint32_t *i) {
	#ifdef USE_WIN32_ASM
		__asm {
			mov ebx, i
			mov eax, [ebx]
			bswap eax
			mov [ebx], eax
		}
	#else
	#ifdef GCC_X86_ASM
			asm("bswap %0;"
				: "+r" (*i));
	#else
		*i = ((*i)>>24) | (((*i)>>8) & 0xff00) | (((*i)<<8) & 0xff0000) | ((*i)<<24);
	#endif //GCC_X86_ASM
	#endif //USE_WIN32_ASM
}

void iSwapUShort(uint16_t *s)  {
	#ifdef USE_WIN32_ASM
		__asm {
			mov ebx, s
			mov al, [ebx+1]
			mov ah, [ebx  ]
			mov [ebx], ax
		}
	#else
	#ifdef GCC_X86_ASM
		asm("ror $8,%0"
			: "=r" (*s)
			: "0" (*s));
	#else
		*s = ((*s)>>8) | ((*s)<<8);
	#endif //GCC_X86_ASM
	#endif //USE_WIN32_ASM
}

#if (defined(__BYTE_ORDER__) && __BYTE_ORDER__ == __BIG_ENDIAN__) \
  || (defined(__BIG_ENDIAN__) && !defined(__LITTLE_ENDIAN__))
#undef __LITTLE_ENDIAN__
#define UShort(s) iSwapUShort(s)
#define UInt(i) iSwapUInt(i)
#else
#undef __BIG_ENDIAN__
#if !defined(__LITTLE_ENDIAN__)
#undef __LITTLE_ENDIAN__  // Not sure if it's defined by any compiler...
#define __LITTLE_ENDIAN__
#endif
#define UInt(i)
#define UShort(s)
#endif


void CorrectPreMult(ILimage* lImage)
{
	uint32_t i;

	for (i = 0; i < lImage->SizeOfData; i += 4) {
		if (lImage->Data[i+3] != 0) {  // Cannot divide by 0.
			lImage->Data[i]   = (uint8_t)(((uint32_t)lImage->Data[i]   << 8) / lImage->Data[i+3]);
			lImage->Data[i+1] = (uint8_t)(((uint32_t)lImage->Data[i+1] << 8) / lImage->Data[i+3]);
			lImage->Data[i+2] = (uint8_t)(((uint32_t)lImage->Data[i+2] << 8) / lImage->Data[i+3]);
		}
	}

	return;
}

void DxtcReadColors(const uint8_t* Data, Color8888* Out)
{
	uint8_t r0, g0, b0, r1, g1, b1;

	b0 = Data[0] & 0x1F;
	g0 = ((Data[0] & 0xE0) >> 5) | ((Data[1] & 0x7) << 3);
	r0 = (Data[1] & 0xF8) >> 3;

	b1 = Data[2] & 0x1F;
	g1 = ((Data[2] & 0xE0) >> 5) | ((Data[3] & 0x7) << 3);
	r1 = (Data[3] & 0xF8) >> 3;

	Out[0].r = r0 << 3 | r0 >> 2;
	Out[0].g = g0 << 2 | g0 >> 3;
	Out[0].b = b0 << 3 | b0 >> 2;

	Out[1].r = r1 << 3 | r1 >> 2;
	Out[1].g = g1 << 2 | g1 >> 3;
	Out[1].b = b1 << 3 | b1 >> 2;
}

//@TODO: Probably not safe on Big Endian.
void DxtcReadColor(uint16_t Data, Color8888* Out)
{
	uint8_t r, g, b;

	b = Data & 0x1f;
	g = (Data & 0x7E0) >> 5;
	r = (Data & 0xF800) >> 11;

	Out->r = r << 3 | r >> 2;
	Out->g = g << 2 | g >> 3;
	Out->b = b << 3 | r >> 2;
}


bool DecompressDXT3(ILimage *lImage, uint8_t *lCompData)
{
	uint32_t    x, y, z, i, j, k, Select;
	uint8_t     *Temp;
	Color8888   colours[4], *col;
	uint32_t    bitmask, Offset;
	uint16_t    word;
	uint8_t     *alpha;

	if (!lCompData)
		return false;

	Temp = lCompData;
	for (z = 0; z < lImage->Depth; z++) {
		for (y = 0; y < lImage->Height; y += 4) {
			for (x = 0; x < lImage->Width; x += 4) {
				alpha = Temp;
				Temp += 8;
				DxtcReadColors(Temp, colours);
				bitmask = ((uint32_t*)Temp)[1];
				UInt(&bitmask);
				Temp += 8;

				// Four-color block: derive the other two colors.
				// 00 = color_0, 01 = color_1, 10 = color_2, 11 = color_3
				// These 2-bit codes correspond to the 2-bit fields
				// stored in the 64-bit block.
				colours[2].b = (2 * colours[0].b + colours[1].b + 1) / 3;
				colours[2].g = (2 * colours[0].g + colours[1].g + 1) / 3;
				colours[2].r = (2 * colours[0].r + colours[1].r + 1) / 3;
				//colours[2].a = 0xFF;

				colours[3].b = (colours[0].b + 2 * colours[1].b + 1) / 3;
				colours[3].g = (colours[0].g + 2 * colours[1].g + 1) / 3;
				colours[3].r = (colours[0].r + 2 * colours[1].r + 1) / 3;
				//colours[3].a = 0xFF;

				k = 0;
				for (j = 0; j < 4; j++) {
					for (i = 0; i < 4; i++, k++) {
						Select = (bitmask & (0x03 << k*2)) >> k*2;
						col = &colours[Select];

						if (((x + i) < lImage->Width) && ((y + j) < lImage->Height)) {
							Offset = z * lImage->SizeOfPlane + (y + j) * lImage->Bps + (x + i) * lImage->Bpp;
							lImage->Data[Offset + 0] = col->r;
							lImage->Data[Offset + 1] = col->g;
							lImage->Data[Offset + 2] = col->b;
						}
					}
				}

				for (j = 0; j < 4; j++) {
					word = alpha[2*j] + 256*alpha[2*j+1];
					for (i = 0; i < 4; i++) {
						if (((x + i) < lImage->Width) && ((y + j) < lImage->Height)) {
							Offset = z * lImage->SizeOfPlane + (y + j) * lImage->Bps + (x + i) * lImage->Bpp + 3;
							lImage->Data[Offset] = word & 0x0F;
							lImage->Data[Offset] = lImage->Data[Offset] | (lImage->Data[Offset] << 4);
						}
						word >>= 4;
					}
				}

			}
		}
	}

	return true;
}

bool DecompressDXT2(ILimage *lImage, uint8_t *lCompData)
{
	// Can do color & alpha same as dxt3, but color is pre-multiplied
	//   so the result will be wrong unless corrected.
	if (!DecompressDXT3(lImage, lCompData))
		return false;
	CorrectPreMult(lImage);

	return true;
}



bool DecompressDXT5(ILimage *lImage, uint8_t *lCompData)
{
	uint32_t		x, y, z, i, j, k, Select;
	uint8_t		*Temp; //, r0, g0, b0, r1, g1, b1;
	Color8888	colours[4], *col;
	uint32_t		bitmask, Offset;
	uint8_t		alphas[8], *alphamask;
	uint32_t		bits;

	if (!lCompData)
		return false;

	Temp = lCompData;
	for (z = 0; z < lImage->Depth; z++) {
		for (y = 0; y < lImage->Height; y += 4) {
			for (x = 0; x < lImage->Width; x += 4) {
				if (y >= lImage->Height || x >= lImage->Width)
					break;
				alphas[0] = Temp[0];
				alphas[1] = Temp[1];
				alphamask = Temp + 2;
				Temp += 8;

				DxtcReadColors(Temp, colours);
				bitmask = ((uint32_t*)Temp)[1];
				UInt(&bitmask);
				Temp += 8;

				// Four-color block: derive the other two colors.
				// 00 = color_0, 01 = color_1, 10 = color_2, 11 = color_3
				// These 2-bit codes correspond to the 2-bit fields
				// stored in the 64-bit block.
				colours[2].b = (2 * colours[0].b + colours[1].b + 1) / 3;
				colours[2].g = (2 * colours[0].g + colours[1].g + 1) / 3;
				colours[2].r = (2 * colours[0].r + colours[1].r + 1) / 3;
				//colours[2].a = 0xFF;

				colours[3].b = (colours[0].b + 2 * colours[1].b + 1) / 3;
				colours[3].g = (colours[0].g + 2 * colours[1].g + 1) / 3;
				colours[3].r = (colours[0].r + 2 * colours[1].r + 1) / 3;
				//colours[3].a = 0xFF;

				k = 0;
				for (j = 0; j < 4; j++) {
					for (i = 0; i < 4; i++, k++) {

						Select = (bitmask & (0x03 << k*2)) >> k*2;
						col = &colours[Select];

						// only put pixels out < width or height
						if (((x + i) < lImage->Width) && ((y + j) < lImage->Height)) {
							Offset = z * lImage->SizeOfPlane + (y + j) * lImage->Bps + (x + i) * lImage->Bpp;
							lImage->Data[Offset + 0] = col->r;
							lImage->Data[Offset + 1] = col->g;
							lImage->Data[Offset + 2] = col->b;
						}
					}
				}

				// 8-alpha or 6-alpha block?
				if (alphas[0] > alphas[1]) {
					// 8-alpha block:  derive the other six alphas.
					// Bit code 000 = alpha_0, 001 = alpha_1, others are interpolated.
					alphas[2] = (6 * alphas[0] + 1 * alphas[1] + 3) / 7;	// bit code 010 alphas[3] = (5 * alphas[0] + 2 * alphas[1] + 3) / 7;	// bit code 011
					alphas[4] = (4 * alphas[0] + 3 * alphas[1] + 3) / 7;	// bit code 100
					alphas[5] = (3 * alphas[0] + 4 * alphas[1] + 3) / 7;	// bit code 101
					alphas[6] = (2 * alphas[0] + 5 * alphas[1] + 3) / 7;	// bit code 110
					alphas[7] = (1 * alphas[0] + 6 * alphas[1] + 3) / 7;	// bit code 111
				}
				else {
					// 6-alpha block.
					// Bit code 000 = alpha_0, 001 = alpha_1, others are interpolated.
					alphas[2] = (4 * alphas[0] + 1 * alphas[1] + 2) / 5;	// Bit code 010
					alphas[3] = (3 * alphas[0] + 2 * alphas[1] + 2) / 5;	// Bit code 011
					alphas[4] = (2 * alphas[0] + 3 * alphas[1] + 2) / 5;	// Bit code 100
					alphas[5] = (1 * alphas[0] + 4 * alphas[1] + 2) / 5;	// Bit code 101
					alphas[6] = 0x00;										// Bit code 110
					alphas[7] = 0xFF;										// Bit code 111
				}

				// Note: Have to separate the next two loops,
				//	it operates on a 6-byte system.

				// First three bytes
				//bits = *((ILint*)alphamask);
				bits = (alphamask[0]) | (alphamask[1] << 8) | (alphamask[2] << 16);
				for (j = 0; j < 2; j++) {
					for (i = 0; i < 4; i++) {
						// only put pixels out < width or height
						if (((x + i) < lImage->Width) && ((y + j) < lImage->Height)) {
							Offset = z * lImage->SizeOfPlane + (y + j) * lImage->Bps + (x + i) * lImage->Bpp + 3;
							lImage->Data[Offset] = alphas[bits & 0x07];
						}
						bits >>= 3;
					}
				}

				// Last three bytes
				//bits = *((ILint*)&alphamask[3]);
				bits = (alphamask[3]) | (alphamask[4] << 8) | (alphamask[5] << 16);
				for (j = 2; j < 4; j++) {
					for (i = 0; i < 4; i++) {
						// only put pixels out < width or height
						if (((x + i) < lImage->Width) && ((y + j) < lImage->Height)) {
							Offset = z * lImage->SizeOfPlane + (y + j) * lImage->Bps + (x + i) * lImage->Bpp + 3;
							lImage->Data[Offset] = alphas[bits & 0x07];
						}
						bits >>= 3;
					}
				}
			}
		}
	}

	return true;
}
/**********************************************************************/

// Needed for UTX and potentially others outside of DDS
bool DecompressDXT1(ILimage *lImage, uint8_t *lCompData)
{
	uint32_t		x, y, z, i, j, k, Select;
	uint8_t		*Temp;
	Color8888	colours[4], *col;
	uint16_t	color_0, color_1;
	uint32_t		bitmask, Offset;

	if (!lCompData)
		return false;

	Temp = lCompData;
	colours[0].a = 0xFF;
	colours[1].a = 0xFF;
	colours[2].a = 0xFF;
	//colours[3].a = 0xFF;
	for (z = 0; z < lImage->Depth; z++) {
		for (y = 0; y < lImage->Height; y += 4) {
			for (x = 0; x < lImage->Width; x += 4) {
				color_0 = *((uint16_t*)Temp);
				UShort(&color_0);
				color_1 = *((uint16_t*)(Temp + 2));
				UShort(&color_1);
				DxtcReadColor(color_0, colours);
				DxtcReadColor(color_1, colours + 1);
				bitmask = ((uint32_t*)Temp)[1];
				UInt(&bitmask);
				Temp += 8;

				if (color_0 > color_1) {
					// Four-color block: derive the other two colors.
					// 00 = color_0, 01 = color_1, 10 = color_2, 11 = color_3
					// These 2-bit codes correspond to the 2-bit fields
					// stored in the 64-bit block.
					colours[2].b = (2 * colours[0].b + colours[1].b + 1) / 3;
					colours[2].g = (2 * colours[0].g + colours[1].g + 1) / 3;
					colours[2].r = (2 * colours[0].r + colours[1].r + 1) / 3;
					//colours[2].a = 0xFF;

					colours[3].b = (colours[0].b + 2 * colours[1].b + 1) / 3;
					colours[3].g = (colours[0].g + 2 * colours[1].g + 1) / 3;
					colours[3].r = (colours[0].r + 2 * colours[1].r + 1) / 3;
					colours[3].a = 0xFF;
				}
				else {
					// Three-color block: derive the other color.
					// 00 = color_0,  01 = color_1,  10 = color_2,
					// 11 = transparent.
					// These 2-bit codes correspond to the 2-bit fields
					// stored in the 64-bit block.
					colours[2].b = (colours[0].b + colours[1].b) / 2;
					colours[2].g = (colours[0].g + colours[1].g) / 2;
					colours[2].r = (colours[0].r + colours[1].r) / 2;
					//colours[2].a = 0xFF;

					colours[3].b = (colours[0].b + 2 * colours[1].b + 1) / 3;
					colours[3].g = (colours[0].g + 2 * colours[1].g + 1) / 3;
					colours[3].r = (colours[0].r + 2 * colours[1].r + 1) / 3;
					colours[3].a = 0x00;
				}

				for (j = 0, k = 0; j < 4; j++) {
					for (i = 0; i < 4; i++, k++) {
						Select = (bitmask & (0x03 << k * 2)) >> k * 2;
						col = &colours[Select];

						if (((x + i) < lImage->Width) && ((y + j) < lImage->Height)) {
							Offset = z * lImage->SizeOfPlane + (y + j) * lImage->Bps + (x + i) * lImage->Bpp;
							lImage->Data[Offset + 0] = col->r;
							lImage->Data[Offset + 1] = col->g;
							lImage->Data[Offset + 2] = col->b;
							lImage->Data[Offset + 3] = col->a;
						}
					}
				}
			}
		}
	}

	return true;
}

bool DecompressDXT4(ILimage *lImage, uint8_t *lCompData)
{
	// Can do color & alpha same as dxt5, but color is pre-multiplied
	//   so the result will be wrong unless corrected.
	if (!DecompressDXT5(lImage, lCompData))
		return false;
	CorrectPreMult(lImage);

	return false;
}
