#include "cdi_r8_ota.h"
#include "cdi_r5.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

static uint8_t stage[1024];static cdi_r8_ota_manifest_t saved;
static bool erase(void *x){(void)x;memset(stage,0xff,sizeof(stage));return true;}
static bool program(uint32_t o,const uint8_t*d,size_t n,void*x){(void)x;if(o+n>sizeof(stage))return false;memcpy(stage+o,d,n);return true;}
static bool finish(const cdi_r8_ota_manifest_t*m,void*x){(void)x;saved=*m;return true;}
int main(void){cdi_r8_ota_t o;uint8_t image[512];uint32_t crc;unsigned i;
 for(i=0;i<sizeof(image);++i)image[i]=(uint8_t)i;
 crc=cdi_r5_crc32(image,sizeof(image));
 cdi_r8_ota_init(&o,erase,program,finish,NULL);
 assert(!cdi_r8_ota_begin(&o,2,sizeof(image),crc,false,true));
 cdi_r8_ota_init(&o,erase,program,finish,NULL);
 assert(cdi_r8_ota_begin(&o,2,sizeof(image),crc,true,true));
 assert(cdi_r8_ota_write(&o,0,image,208));assert(cdi_r8_ota_write(&o,208,image+208,208));
 assert(cdi_r8_ota_write(&o,416,image+416,96));assert(cdi_r8_ota_commit(&o));
 assert(cdi_r8_ota_manifest_valid(&saved));assert(!memcmp(stage,image,sizeof(image)));
 puts("cdi_r8_ota: all tests passed");return 0;}
