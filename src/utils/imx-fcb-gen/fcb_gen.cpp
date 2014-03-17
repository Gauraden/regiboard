/*
 * generate firmware control block (FCB) - required to boot iMX53 from NAND
 *
 * bad block handling of ROM bootloader is disabled
 *
 * fcb length is 4kB
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

static const uint32_t kFCBSizeKb  = 4;
static const uint32_t kPageSizeKb = 4;
static const uint32_t kImagePage  = ((kFCBSizeKb / kPageSizeKb) * (kPageSizeKb * 1024));

static uint32_t Lit2Big(uint32_t val) {
  uint32_t res = 0;
  printf("DEBUG: %s: %d: val = %u\n", __func__, __LINE__, val);
  for (int i = 0; i < 32; i++)
    res |= (val & (1 << i)) << (31 - (i * 2));
  return res;
}

struct fcb_entry
{
  uint32_t addr;
  uint32_t value;
};

struct fcb_entry fcb_entries[] =
{
  { 0x00, 0x00000000 }, // res.
  { 0x04, 0x46434220 }, // fingerprint #2 (ascii FCB)
  { 0x08, 0x01000000 }, // fingerprint #3 (version)
  { 0x68, kImagePage }, // primary image start page
  { 0x6C, kImagePage }, // secondary image start page
//  { 0x68, Lit2Big(kImagePage) }, // primary image start page
//  { 0x6C, Lit2Big(kImagePage) }, // secondary image start page
  { 0x78, 0x00000000 }, // Start page address of DBBT Search Area (0 means no bad blocks)
  { 0x7C, 0x00000000 }, // Bad Block Marker Offset in page data buffer
  { 0xAC, 0x00000000 }, // Bad Block Marker Swapping Enable
  { 0xB0, 0x00000000 }, // Bad Block Marker Offset to Spare Byte
};

int main()
{
  int i, ret;
  char *fname = "fcb.bin";
  FILE *fp;
  uint8_t fcbimage[(kFCBSizeKb * 1024) + 0x400];

  memset(fcbimage, 0, sizeof(fcbimage));

  for(i = 0; i < (int) (sizeof(fcb_entries) / sizeof(fcb_entries[0])); i++)
  {
    if((fcb_entries[i].addr + 3) >= sizeof(fcbimage))
    {
      printf("Invalid FCB entry (index:%d)\n", i);
      continue;
    }
    fcbimage[fcb_entries[i].addr + 0] = fcb_entries[i].value >> 24;
    fcbimage[fcb_entries[i].addr + 1] = fcb_entries[i].value >> 16;
    fcbimage[fcb_entries[i].addr + 2] = fcb_entries[i].value >> 8;
    fcbimage[fcb_entries[i].addr + 3] = fcb_entries[i].value >> 0;
  }

  if((fp = fopen(fname, "w")) == NULL)
  {
    printf("could not open %s!\n", fname);
    return -1;
  }

  ret = fwrite(fcbimage, 1, sizeof(fcbimage), fp);
  printf("wrote %d bytes.\n", ret);
  if(ret != sizeof(fcbimage))
    printf("error - incomplete file!\n");
  fclose(fp);

  return 0;
}
