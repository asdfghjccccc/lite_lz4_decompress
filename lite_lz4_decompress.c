#include <stdio.h>
#include <stdlib.h>
#include <string.h>


#define MINMATCH 4
#define LASTLITERALS   5
#define MFLIMIT       12
#define ML_BITS  4

size_t read_long_length_no_check(const uint8_t **pp)
{
	size_t b, l = 0;

	do {
		b = **pp;
		(*pp)++;
		l += b;
	} while (b == 255);
	return l;
}

int lite_lz4_decompress(const uint8_t * const istart, uint8_t * const ostart, int decompressedSize)
{
	const uint8_t *ip = istart;
	uint8_t *op = (uint8_t *) ostart;
	uint8_t *const oend = ostart + decompressedSize;
	const uint8_t *const prefixStart = ostart;

    /* If the highest bit is set (1), the block is uncompressed.*/
    if(*ip++ & 0x80) {
        memmove(op, ip, decompressedSize);
        return decompressedSize;
    }

	while (1) {
		/* start new sequence */
		unsigned int token = *ip++;

		/* literals */
		{
			size_t ll = token >> ML_BITS;

			if (ll == 15) {
				/* long literal length */
				ll += read_long_length_no_check(&ip);
			}
			if ((size_t) (oend - op) < ll) {
				return -1;	/* output buffer overflow */
			}
			memmove(op, ip, ll);	/* support in-place decompression */
			op += ll;
			ip += ll;
			if ((size_t) (oend - op) < MFLIMIT) {
				if (op == oend) {
					break;	/* end of block */
				}
				return -1;
			}
		}

		/* match */
		{
			size_t ml = token & 15;
			size_t const offset = (uint16_t) ((uint16_t) ip[0] + (ip[1] << 8));

			ip += 2;

			if (ml == 15) {
				/* long literal length */
				ml += read_long_length_no_check(&ip);
			}
			ml += MINMATCH;

			if ((size_t) (oend - op) < ml) {
				return -1;	/* output buffer overflow */
			}

			{
				const uint8_t *match = op - offset;

				/* out of range */
				if (offset > (size_t) (op - prefixStart)) {
					return -1;
				}
				/* match copy - slow variant, supporting overlap copy */
				{
					size_t u;

					for (u = 0; u < ml; u++) {
						op[u] = match[u];
					}
				}
			}
			op += ml;
			if ((size_t) (oend - op) < LASTLITERALS) {
				return -1;
			}
		}		/* match */
	}			/* main loop */
	return (int)(ip - istart - 1);
}
