typedef unsigned long boolean;
#define SUCCESS 1
#define FAILURE 0

/*====================================================================*//*    
 G L O B A L   F U N C T I O N   P R O T O T Y P E S          
*//*====================================================================*/

boolean G711_linear2alaw(long            size,
                         short          *pcm_ptr,
                         unsigned char  *out_ptr);

boolean G711_alaw2linear(long            size,
                         unsigned char  *a_ptr,
                         short          *out_ptr);
boolean G711_linear2ulaw(long            size,
                         short          *pcm_ptr,
                         unsigned char  *out_ptr);
boolean G711_ulaw2linear(long            size,
                         unsigned char  *u_ptr,
                         short          *out_ptr);
boolean G711_alaw2ulaw(long              size,
                       unsigned char    *a_ptr,
                       unsigned char    *u_ptr);
boolean G711_ulaw2alaw(long              size,
                       unsigned char    *u_ptr,
                       unsigned char    *a_ptr);
void packing_function (int block_size, unsigned char *bits);
void unpacking_function (int block_size, unsigned char *in_bits, unsigned
char *out_bits);
/*====================================================================*/  
/*H I S T O R Y                                                     */
/*====================================================================*/

/*17-07-97 A. Tariq  -  Creation                                    */
/*11-11-97 S. Sattar  - Changed API                                  */
/*26-01-98 I. Elahi  - Added packing and unpacking functions         */
