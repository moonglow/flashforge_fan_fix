#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <assert.h>

int patch_branch( uint32_t from, uint32_t to, uint8_t *pmem )
{
  uint32_t offset = (to-from-4);

  uint16_t i1 = 0xF000 | ( (offset>>12) & 0x7FF );
  uint16_t i2 = 0xF800 | ( (offset>>1) & 0x7FF );

  pmem[0] = (i1&0xFF);
  pmem[1] = (i1>>8);
  pmem[2] = (i2&0xFF);
  pmem[3] = (i2>>8);

  printf( "%08X: bl %08X [ %02X %02X %02X %02X ]\n",
         from, to, pmem[0], pmem[1], pmem[2], pmem[3] );

  return 4;
}

int patch_addr( uint32_t addr, uint8_t *pmem )
{
  addr |= 0x00000001;

  pmem[0] = (addr>>0)&0xFF;
  pmem[1] = (addr>>8)&0xFF;
  pmem[2] = (addr>>16)&0xFF;
  pmem[3] = (addr>>24)&0xFF;

  printf( ": %02X %02X %02X %02X\n", pmem[0], pmem[1], pmem[2], pmem[3] );

  return 4;
}

int is_brunch( uint8_t *pmem )
{
  if( ( pmem[1] & 0xF8 ) != 0xF0 )
    return 0;

  if( ( pmem[3] & 0xF8 ) != 0xF8 )
    return 0;

  return 1;
}

/* TODO: make pattern matcher */

void *load_file( char *psz_path, int *size, int alloc_size )
{
  void *mem = 0;
  int file_size;

  FILE *f = fopen( psz_path, "rb" );
  if( !f )
    return 0;

  fseek( f, 0, SEEK_END );
  file_size = ftell( f );
  rewind( f );

  if( alloc_size < file_size )
    alloc_size = file_size;

  /* tooo big */
  if( file_size <= (1024*1024) )
  {
    mem = malloc( alloc_size );
    assert( mem );
    memset( mem, 0xFF, alloc_size );
    fread( mem, file_size, 1, f );
  }

  fclose( f );

  *size = alloc_size;
  return mem;
}

uint8_t hex2int(char ch)
{
  if (ch >= '0' && ch <= '9')
    return ch - '0';
  if (ch >= 'A' && ch <= 'F')
    return ch - 'A' + 10;
  if (ch >= 'a' && ch <= 'f')
    return ch - 'a' + 10;
  return 0;
}

int find_signature( uint8_t *mem, int size, char *psz_sig )
{
  uint8_t sig_bin[128] = { 0 };
  uint8_t sig_mask_bin[128] = { 0 };
  int len = strlen( psz_sig );
  
  if( len & 1 )
    return -1;

  /* compile signature */
  for( int i = 0; i < len; i += 2 )
  {
    sig_mask_bin[i/2] = psz_sig[i+0] == '?' ? 0: 0xF0;
    sig_mask_bin[i/2] |= psz_sig[i+1] == '?' ? 0: 0x0F;

    sig_bin[i/2] = hex2int( psz_sig[i+0] )<<4;
    sig_bin[i/2] |= hex2int( psz_sig[i+1] );
  }

  len = len / 2;

  if( size < len )
    return -1;

  int n = 0;
  for( int i = 0; i < (size-len); i++ )
  {
    for( n = 0; n < len; n ++ )
    {
      if( ( mem[i+n] & sig_mask_bin[n] ) != sig_bin[n] )
        break;
    }
    
    /* found ? */
    if( n == len )
    {
      return i;
    }
  }

  return -1;
}

#define FLASH_BASE            ( 0x08010000 )
#define FIRMWARE_FLASH_SIZE   ((1024-64)*1024)
#define TIM9_ISR_OFFSET       ( 0x000000A0 )

/* 
  Dreamer NX function hook addresses
  0x08011728 => FAN ON
  0x08011746 => FAN OFF
*/

int main(void)
{
  uint8_t *p;
  int size = 0;
  char buffer[0x100], *s;
  uint32_t isr_addr = 0, m106_addr = 0, m107_addr = 0;
  uint32_t sec_addr = 0, sec_size = 0;

  /* read addresses from map file */
  FILE *map;

  map = fopen( "patch.map", "r" );
  if( !map )
  {
    printf( "no map file was found\n" );
    return -1;
  }

  int next_line_is_sec = 0;
  while( feof( map ) == 0 )
  {
    s = fgets( buffer, sizeof( buffer ), map );
    if( !s )
      continue;

    if( next_line_is_sec )
    {
      next_line_is_sec = 0;
      sec_addr = strtoll( s, &s, 16 );
      sec_size = strtoll( s, &s, 16 );
      continue;
    }
    
    if( strncmp( s, ".patch_location", 15 ) == 0 )
    {
      next_line_is_sec = 1;
    }
    else if( strstr( s, "hook_m106" ) )
    {
      m106_addr = strtoll( s, 0, 16 );
    }
    else if( strstr( s, "hook_m107" ) )
    {
      m107_addr = strtoll( s, 0, 16 );
    }
    else if( strstr( s, "TIM1_BRK_TIM9_IRQHandler" ) )
    {
      isr_addr = strtoll( s, 0, 16 );
    }
  }

  fclose( map );

  if( !isr_addr || !m106_addr || !m107_addr || !sec_addr )
  {
    printf( "not all addresses was resolved\n" );
    return -2;
  }

  printf( "Map ( 0x%08X %u ):\n", sec_addr, sec_size );
  printf( "\tISR: 0x%08X\n", isr_addr );
  printf( "\tm106_hook: 0x%08X\n", m106_addr );
  printf( "\tm107_hook: 0x%08X\n", m107_addr );

  p = load_file( FW_NAME, &size, FIRMWARE_FLASH_SIZE );
  if( !p )
  {
    printf( "no file was found\n" );
    return -1;
  }


  /* all addresses from MAP file */
  /* TIM1_BRK_TIM9_IRQHandler */
  patch_addr( isr_addr, &p[TIM9_ISR_OFFSET] );

  /* M106 => hook_m106 */
  uint32_t bl_loc;

  bl_loc = M106_CALL_ADDRESS;

  if( !is_brunch( &p[bl_loc-FLASH_BASE] ) )
  {
    printf( "%08X hook invalid fw address\n", bl_loc );
  }
  else
  {
    patch_branch( bl_loc, m106_addr, &p[bl_loc-FLASH_BASE] );
  }

  /* M107 => hook_m107 */
  bl_loc = M107_CALL_ADDRESS;
  if( !is_brunch( &p[bl_loc-FLASH_BASE] ) )
  {
    printf( "%08X hook invalid fw address\n", bl_loc );
  }
  else
  {
    patch_branch( bl_loc, m107_addr, &p[bl_loc-FLASH_BASE] );
  }

  /* copy firmware patch payload to image */
  int payload_size = 0;
  uint8_t *p_payload;

  p_payload = load_file( "patch.bin", &payload_size, 0 );

  memcpy( &p[sec_addr-FLASH_BASE], &p_payload[sec_addr-FLASH_BASE], sec_size );

  free( p_payload );

  FILE *f = fopen( "patched_firmware.bin", "wb" );
  if( !f )
  {
    printf( "output file could not be created\n" );
  }
  else
  {
    fwrite( p, size, 1, f );
    fclose( f );
  }

  /* 
    try to find all useful patterns like that:
    MOVS    R1, #0x20   : 20 21
    LDR     R0, =GPIOF  : XX 48
    CALL    GPIO_XXX    : XX F0 XX XX
 */
  static char call_gpio_api[] = "2021??48??F0????";
  int offset = 0;

  for( ;offset < size; )
  {
    int res;

    res = find_signature( p + offset, size - offset, call_gpio_api );
    if( res < 0 )
      break;
    
    offset += res;

    /* check io port address */
    /* GPIOF = 0x40021400 */
    uint32_t inst_addr = offset;
    uint32_t port_offset =  ( offset + 2 + p[offset+2]*4 + 4 ) & (~3);
    offset += 8;
    if( (*(uint32_t*)(p + port_offset )) != 0x40021400 )
    {
      continue;
    }
    
    printf( "ADDR: 0x%08X\n", FLASH_BASE + inst_addr );
  }

  free( p );

  printf( "done\n" );
  return 0;
}
