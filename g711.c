/*====================================================================*/
/*      I N C L U D E S                                               */
/*====================================================================*/
#ifdef TM1_OPT
#include <ops/custom_defs.h>
#endif
#include "g711.h"

/*====================================================================*/
/*      L O C A L   S Y M B O L   D E C L A R A T I O N S             */
/*====================================================================*/
#define    SIGN_BIT        (0x80)   /* Sign bit for a A-law byte.        */
#define    QUANT_MASK      (0xf)    /* Quantization field mask.          */
#define    NSEGS           (8)      /* Number of u-/A-law segments.      */
#define    SEG_SHIFT       (4)      /* Left shift for segment number.    */
#define    SEG_MASK        (0x70)   /* Segment field mask.               */
#define    BIAS            (0x84)   /* Bias for linear code (for u-law). */
#define    CLIPULAW        (32636)  /* Max. value for linear PCM (u-law).*/
                                 /* 8159*4                            */


/*====================================================================*/
/*      L O C A L   D A T A   D E F I N I T I O N S                   */
/*====================================================================*/
/*--------------------------------------------------------------------*/
/* Copy from CCITT G.711 specifications                               */
/* u- to A-law conversions                                            */
/*--------------------------------------------------------------------*/
static unsigned char _u2a[128] = {
    1,    1,    2,    2,    3,    3,    4,    4,
    5,    5,    6,    6,    7,    7,    8,    8,
    9,    10,    11,    12,    13,    14,    15,    16,
    17,    18,    19,    20,    21,    22,    23,    24,
    25,    27,    29,    31,    33,    34,    35,    36,
    37,    38,    39,    40,    41,    42,    43,    44,
    46,    48,    49,    50,    51,    52,    53,    54,
    55,    56,    57,    58,    59,    60,    61,    62,
    64,    65,    66,    67,    68,    69,    70,    71,
    72,    73,    74,    75,    76,    77,    78,    79,
    81,    82,    83,    84,    85,    86,    87,    88,
    89,    90,    91,    92,    93,    94,    95,    96,
    97,    98,    99,    100,    101,    102,    103,    104,
    105,    106,    107,    108,    109,    110,    111,    112,
    113,    114,    115,    116,    117,    118,    119,    120,
    121,    122,    123,    124,    125,    126,    127,    128};

/*--------------------------------------------------------------------*/
/* Copy from CCITT G.711 specifications                               */
/* A- to u-law conversions                                            */
/*--------------------------------------------------------------------*/
static unsigned char _a2u[128] = {
    1,    3,    5,    7,    9,    11,    13,    15,
    16,    17,    18,    19,    20,    21,    22,    23,
    24,    25,    26,    27,    28,    29,    30,    31,
    32,    32,    33,    33,    34,    34,    35,    35,
    36,    37,    38,    39,    40,    41,    42,    43,
    44,    45,    46,    47,    48,    48,    49,    49,
    50,    51,    52,    53,    54,    55,    56,    57,
    58,    59,    60,    61,    62,    63,    64,    64,
    65,    66,    67,    68,    69,    70,    71,    72,
    73,    74,    75,    76,    77,    78,    79,    79,
    80,    81,    82,    83,    84,    85,    86,    87,
    88,    89,    90,    91,    92,    93,    94,    95,
    96,    97,    98,    99,    100,    101,    102,    103,
    104,    105,    106,    107,    108,    109,    110,    111,
    112,    113,    114,    115,    116,    117,    118,    119,
    120,    121,    122,    123,    124,    125,    126,    127};


/*====================================================================*/
/*      F U N C T I O N S                                             */
/*====================================================================*/

/*MPF:::G.711::g711.c:G711_linear2alaw================================*/
/*                                                                    */
/*  FUNCTION NAME:      G711_linear2alaw                              */
/*  PACKAGE:            G.711                                         */
/*  SCOPE:              PACKAGE                                       */
/*  DESCRIPTION:                                                      */
/*      Converts a 16-bit linear PCM value to 8-bit A-law.            */
/*                                                                    */
/*      Linear Input Code               Compressed Code               */
/*      -----------------               ---------------               */
/*      0000000wxyza                    000wxyz                       */
/*      0000001wxyza                    001wxyz                       */
/*      000001wxyzab                    010wxyz                       */
/*      00001wxyzabc                    011wxyz                       */
/*      0001wxyzabcd                    100wxyz                       */
/*      001wxyzabcde                    101wxyz                       */
/*      01wxyzabcdef                    110wxyz                       */
/*      1wxyzabcdefg                    111wxyz                       */
/*                                                                    */
/*      The quantization interval is directly available as the four   */
/*      bits wxyz. The trailing bits (a - g) are ignored.             */
/*                                                                    */
/*  TESTS:                                                            */
/*      Tested through g711_test.c.                                   */
/*                                                                    */
/*--------------------------------------------------------------------*/
/*  CALLING SEQUENCE:                                                 */
/*--------------------------------------------------------------------*/
boolean                                 /* Returns: success or failure*/
    G711_linear2alaw(  
long             size,                   /* In: block size             */
short           *pcm_ptr,               /* In: 16-bit PCM data        */
unsigned char   *out_ptr)               /* Out: 8-bit A-law data      */
/*EMP=================================================================*/
{
    unsigned char mask;
    long           seg;
    long           i;
    long           j;
    unsigned char aval;
    unsigned long seg_end;
    short         pcm_val;

    for (j=0; j < size; j++)
    {
        seg_end = 0x80;
        pcm_val = *pcm_ptr & 0xFFF8;
        pcm_ptr++;
       
        /*------------------------------------------------------------*/
        /* Sign (7th) bit = 1                                         */
        /*------------------------------------------------------------*/
        if (pcm_val >= 0)
            mask = 0xD5;

        /*------------------------------------------------------------*/
        /* Sign bit = 0                                               */
        /*------------------------------------------------------------*/
        else
        {
            mask = 0x55;
            pcm_val = -pcm_val - 8;
        }

        /*------------------------------------------------------------*/
        /* Convert the scaled magnitude to segment number.            */
        /*------------------------------------------------------------*/
        seg = NSEGS;

        for (i = 0; i < NSEGS; i++)
        {
            seg_end <<= 1;
            if ((unsigned long)pcm_val < seg_end)
            {
                seg = i;
                break;
            }
        }
        /*------------------------------------------------------------*/
        /* Out of range, set maximum value.                           */
        /*------------------------------------------------------------*/
        aval = 0x7F;

        /*------------------------------------------------------------*/
        /* Combine the sign, segment, and quantization bits.          */
        /*------------------------------------------------------------*/
        if (seg < NSEGS)
        {
            aval = (unsigned char)(seg << SEG_SHIFT);
            if (seg < 2)
                aval |= ((unsigned long)pcm_val >> 4) & QUANT_MASK;
            else
                aval |= ((unsigned long)pcm_val >> (seg + 3)) &
                                                            QUANT_MASK;

        }
        *out_ptr = aval ^ mask;
        out_ptr++;
    }
    return SUCCESS;
}


/*MPF:::G.711::g711.c:G711_alaw2linear================================*/
/*                                                                    */
/*  FUNCTION NAME:      G711_alaw2linear                              */
/*  PACKAGE:            G.711                                         */
/*  SCOPE:              PACKAGE                                       */
/*  DESCRIPTION:                                                      */
/*      Converts an A-law value to 16-bit linear PCM.                 */
/*                                                                    */
/*  TESTS:                                                            */
/*      Tested through g711_test.c.                                   */
/*                                                                    */
/*--------------------------------------------------------------------*/
/*  CALLING SEQUENCE:                                                 */
/*--------------------------------------------------------------------*/
boolean                             /* Returns: success or failure    */
    G711_alaw2linear(
long             size,               /* In: block size                 */
unsigned char    *a_ptr,            /* In: 8-bit A-law values         */
short           *out_ptr)           /* Out: 16-bit pcm values         */
/*EMP=================================================================*/
{
    short    t;
    long      seg;
    unsigned char a_val;
    long      j;

   
    for (j=0;j < size; j++)
    {
        a_val = *a_ptr++;
        a_val ^= 0x55;
   
        t = (a_val & QUANT_MASK) << 4;
        seg = ((unsigned)a_val & SEG_MASK) >> SEG_SHIFT;

        switch (seg)
        {
            case 0:
                t += 8;
                break;

            case 1:
                t += 0x108;
                break;

            default:
                t += 0x108;
                t <<= seg - 1;
                break;
        }

        *out_ptr++ = ((a_val & SIGN_BIT) ? t : -t);
       
    }
    return SUCCESS;
}


/*MPF:::G.711::g711.c:G711_linear2ulaw================================*/
/*                                                                    */
/*  FUNCTION NAME:      G711_linear2ulaw                              */
/*  PACKAGE:            G.711                                         */
/*  SCOPE:              PACKAGE                                       */
/*  DESCRIPTION:                                                      */
/*      Converts a 16-bit linear PCM value to 8-bit u-law.            */
/*                                                                    */
/*      In order to simplify the encoding process, the original linear*/
/*      magnitude is biased by adding 33 which shifts the encoding    */
/*      range from (0 - 8158) to (33 - 8191). The result can be seen  */
/*      in the following encoding table:                              */
/*                                                                    */
/*      Linear Input Code               Compressed Code               */
/*      -----------------               ---------------               */
/*      00000001wxyza                   000wxyz                       */
/*      0000001wxyzab                   001wxyz                       */
/*      000001wxyzabc                   010wxyz                       */
/*      00001wxyzabcd                   011wxyz                       */
/*      0001wxyzabcde                   100wxyz                       */
/*      001wxyzabcdef                   101wxyz                       */
/*      01wxyzabcdefg                   110wxyz                       */
/*      1wxyzabcdefgh                   111wxyz                       */
/*                                                                    */
/*      Each biased linear code has a leading 1 which identifies the  */
/*      segment number. The value of the segment number is equal to 7 */
/*      minus the number of leading 0's. The quantization interval is */
/*      directly available as the four bits wxyz. The trailing bits   */
/*      (a - h) are ignored.                                          */
/*                                                                    */
/*      Ordinarily the complement of the resulting code word is used  */
/*      for transmission, and so the code word is complemented before */
/*      it is returned.                                               */
/*                                                                    */
/*  TESTS:                                                            */
/*      Tested through g711_test.c.                                   */
/*                                                                    */
/*--------------------------------------------------------------------*/
/*  CALLING SEQUENCE:                                                 */
/*--------------------------------------------------------------------*/
boolean                             /* Returns: success or failure    */
    G711_linear2ulaw(
long             size,               /* In: block size                 */
short           *pcm_ptr,           /* In: 16-bit PCM values          */
unsigned char   *out_ptr)           /* Out: 8-bit u-law vlaues        */
/*EMP=================================================================*/
{
    unsigned char   mask;
    long             seg;
    long             i;
    long             j;
    unsigned char   uval;
    unsigned long   seg_end;
    short           pcm_val;

    for (j=0; j< size; j++)
    {
     
        seg_end = 0x80;
        pcm_val = *pcm_ptr & 0xFFFC;
        pcm_ptr++;

        /*------------------------------------------------------------*/
        /* Get the sign and the magnitude of the value.               */
        /*------------------------------------------------------------*/
        if (pcm_val < 0)
        {
            pcm_val = -pcm_val;
            mask = 0x7F;
        }
        else
        {
            mask = 0xFF;
        }

        if (pcm_val > CLIPULAW)
            pcm_val = CLIPULAW;

        pcm_val += BIAS;

        /*------------------------------------------------------------*/
        /* Convert the scaled magnitude to segment number.            */
        /*------------------------------------------------------------*/
        seg = NSEGS;

        for (i = 0; i < NSEGS; i++)
        {
            seg_end <<= 1;
            if ((unsigned long)pcm_val < seg_end)
            {
                seg = i;
                break;
            }
        }

        /*------------------------------------------------------------*/
        /* Out of range, set maximum value.                           */
        /*------------------------------------------------------------*/
        uval = 0x7F;

        /*------------------------------------------------------------*/
        /* Combine the sign, segment, quantization bits and complement*/
        /* the code word.                                             */
        /*------------------------------------------------------------*/
        if (seg < NSEGS)
            uval = (seg << 4) | (((unsigned)pcm_val >> (seg + 3)) & 0xF);

        *out_ptr++ = (uval ^ mask);
    }
    return SUCCESS;
}


/*MPF:::G.711::g711.c:G711_ulaw2linear================================*/
/*                                                                    */
/*  FUNCTION NAME:      G711_ulaw2linear                              */
/*  PACKAGE:            G.711                                         */
/*  SCOPE:              PACKAGE                                       */
/*  DESCRIPTION:                                                      */
/*      Converts a u-law value to 16-bit linear PCM.                  */
/*                                                                    */
/*      First, a biased linear code is derived from the code word. An */
/*      unbiased output can then be obtained by subtracting 33 from   */
/*      the biased code.                                              */
/*                                                                    */
/*      Note that this function expects to be passed the complement of*/
/*      the original code word. This is in keeping with ISDN          */
/*      conventions.                                                  */
/*                                                                    */
/*  TESTS:                                                            */
/*      Tested through g711_test.c.                                   */
/*                                                                    */
/*--------------------------------------------------------------------*/
/*  CALLING SEQUENCE:                                                 */
/*--------------------------------------------------------------------*/
boolean                             /* Returns: success or failure    */
    G711_ulaw2linear(
long             size,               /* In: block size                 */
unsigned char   *u_ptr,             /* In: 8-bit u-law values         */
short           *out_ptr)           /* Out: 16-bit pcm values         */
/*EMP=================================================================*/
{
    short t;
    unsigned char u_val;
    long         i;

    for (i=0; i< size; i++)
    {

        /*------------------------------------------------------------*/
        /* Complement to obtain normal u-law value.                   */
        /*------------------------------------------------------------*/
        u_val = ~(*u_ptr);
        u_ptr++;

        /*------------------------------------------------------------*/
        /* Extract and bias the quantization bits.Then shift up by the*/
        /* segment number and subtract out the bias.                  */
        /*------------------------------------------------------------*/
        t = ((u_val & QUANT_MASK) << 3) + BIAS;
        t <<= ((unsigned)u_val & SEG_MASK) >> SEG_SHIFT;

        *out_ptr++ = ((u_val & SIGN_BIT) ? (BIAS - t) : (t - BIAS));
    }
   
    return SUCCESS;
}


/*MPF:::G.711::g711.c:G711_alaw2ulaw==================================*/
/*                                                                    */
/*  FUNCTION NAME:      G711_alaw2ulaw                                */
/*  PACKAGE:            G.711                                         */
/*  SCOPE:              PACKAGE                                       */
/*  DESCRIPTION:                                                      */
/*      Converts A-law to u-law.                                      */
/*                                                                    */
/*  TESTS:                                                            */
/*      Tested through g711_test.c.                                   */
/*                                                                    */
/*--------------------------------------------------------------------*/
/*  CALLING SEQUENCE:                                                 */
/*--------------------------------------------------------------------*/
boolean                             /* Returns: success or failure    */
    G711_alaw2ulaw(
long             size,               /* In: block size                 */
unsigned char *a_ptr,               /* In: 8-bit A-law values         */
unsigned char *u_ptr)               /* Out: 8-bit u-law values        */
/*EMP=================================================================*/
{
    unsigned char aval;
    long i;

    for (i=0;i<size; i++)
    {
        aval = *a_ptr & 0xff;
        a_ptr++;

#ifdef TM1_OPT
        *u_ptr++ = (MUX((aval & 0x80), (0xFF ^ _a2u[aval ^ 0xD5]),
                (0x7F ^ _a2u[aval ^ 0x55])));
#else
        *u_ptr++ = ((aval & 0x80) ? (0xFF ^ _a2u[aval ^ 0xD5]) :
            (0x7F ^ _a2u[aval ^ 0x55]));
#endif 
    }
    return SUCCESS;
}


/*MPF:::G.711::g711.c:G711_ulaw2alaw==================================*/
/*                                                                    */
/*  FUNCTION NAME:      G711_ulaw2alaw                                */
/*  PACKAGE:            G.711                                         */
/*  SCOPE:              PACKAGE                                       */
/*  DESCRIPTION:                                                      */
/*      Converts u-law to A-law.                                      */
/*                                                                    */
/*  TESTS:                                                            */
/*      Tested through g711_test.c.                                   */
/*                                                                    */
/*--------------------------------------------------------------------*/
/*  CALLING SEQUENCE:                                                 */
/*--------------------------------------------------------------------*/
boolean                             /* Returns: success or failure    */
    G711_ulaw2alaw(
long             size,               /* In: block size                 */
unsigned char   *u_ptr,             /* In: 8-bit u-law values         */
unsigned char   *a_ptr)             /* Out: 8-bit a-law values        */
/*EMP=================================================================*/
{
    unsigned char uval;
    long i;

    for (i=0; i<size; i++)
    {
       
        uval = *u_ptr & 0xff;
        u_ptr++;

#ifdef TM1_OPT
        *a_ptr++ = (MUX((uval & 0x80), (0xD5 ^ (_u2a[0xFF ^ uval] - 1)),
                    (0x55 ^ (_u2a[0x7F ^ uval] - 1))));
#else
        *a_ptr++ = ((uval & 0x80) ? (0xD5 ^ (_u2a[0xFF ^ uval] - 1)) :
                    (0x55 ^ (_u2a[0x7F ^ uval] - 1)));
#endif
    }
    return SUCCESS;
}

/*MPF:::G.711 Bit Packing Function for 56K G.711======================*/
/*                                                                    */
/*  FUNCTION NAME:      packing_function                               */
/*  PACKAGE:            G.711                                         */
/*  SCOPE:              PACKAGE                                       */
/*  DESCRIPTION:                                                      */
/*      Converts 64K G711 to 56K G711                                      */
/*                                                                    */
/*  TESTS:                                                            */
/*      Tested through g711_test.c.                                   */
/*                                                                    */
/*--------------------------------------------------------------------*/
/*  CALLING SEQUENCE:                                                 */
/*--------------------------------------------------------------------*/

    void packing_function (int block_size, unsigned char *bits)
    {
    unsigned char   shift_mask;
    int             i, j = 0, shift_tag  = 8;
   
    for(i = 0; i < block_size; i++)
        {

        if (shift_tag == 8)
            {
            shift_tag = 1;
            shift_mask =1;
            bits[i] >>= 1;
            i++;
            }
   
        bits[i] >>= 1;
        bits[j] = bits[i-1] | ((bits[i] & shift_mask) << (8 - shift_tag));
        bits[i] >>= shift_tag;
        shift_tag++;
        shift_mask <<= 1;
        shift_mask |= 1;
        j++;
        }
    }

/*MPF:::G.711 Bit Un-packing Function for 56K G.711======================*/
/*                                                                    */
/*  FUNCTION NAME:      packing_function                               */
/*  PACKAGE:            G.711                                         */
/*  SCOPE:              PACKAGE                                       */
/*  DESCRIPTION:                                                      */
/*      Converts 56K G711 to 64K G711                                      */
/*                                                                    */
/*  TESTS:                                                            */
/*      Tested through g711_test.c.                                   */
/*                                                                    */
/*--------------------------------------------------------------------*/
/*  CALLING SEQUENCE:                                                 */
/*--------------------------------------------------------------------*/


    void unpacking_function (int block_size, unsigned char *bits_in, unsigned
char *bits_out)
    {
    unsigned char   shift_mask;
    int             i = 0, j, shift_tag  = 8;

    for(j = 0; j < block_size ; j++)
        {
   
        if (shift_tag == 8)
            {
            shift_tag = 1;
            shift_mask =0x80;
            bits_out[i] = bits_in[j] << 1;
            i++;
            }

       
        bits_out[i] = (bits_in[j+1] << shift_tag) | ((bits_in[j] & shift_mask) >> (8
-shift_tag));
        bits_out[i] <<= 1;

        shift_tag++;

        shift_mask >>= 1;
        shift_mask |= 0x80;
        i++;
        }
    }


/*====================================================================*/
/*      H I S T O R Y                                                 */
/*====================================================================*/
/* 17-07-97 A. Tariq -  Modified                                      */
/* 22-07-97 A. Tariq -  Optimized the code                            */
/* 11-11-97 S. Sattar - Changed API                                   */
/* 26-01-98 I. Elahi  - Added packing and unpacking functions         */
