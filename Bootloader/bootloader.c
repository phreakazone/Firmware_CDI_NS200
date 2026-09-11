#include "stm32wb55xx.h"
#include "cdi_r8_ota.h"

#include <stddef.h>
#include <stdint.h>

#define FLASH_KEY1_VALUE 0x45670123u
#define FLASH_KEY2_VALUE 0xCDEF89ABu
#define FLASH_PAGE_BYTES 0x1000u
#define FLASH_ERRORS (FLASH_SR_OPERR|FLASH_SR_PROGERR|FLASH_SR_WRPERR|FLASH_SR_PGAERR|FLASH_SR_SIZERR|FLASH_SR_PGSERR|FLASH_SR_MISERR|FLASH_SR_FASTERR|FLASH_SR_RDERR|FLASH_SR_OPTVERR)

static uint32_t crc32(const void *memory,uint32_t length)
{
  const uint8_t *p=(const uint8_t *)memory;uint32_t crc=0xffffffffu,i;
  while(length--){unsigned bit;crc^=*p++;for(bit=0;bit<8;bit++)crc=(crc>>1u)^(0xedb88320u&(0u-(crc&1u)));}
  i=~crc;return i;
}

static int manifest_valid(const cdi_r8_ota_manifest_t *m)
{
  return m->magic==CDI_R8_OTA_META_MAGIC&&m->hardware_id==CDI_R8_HW_ID&&
    m->staged_address==CDI_R8_STAGE_ADDR&&m->image_length>=256u&&
    m->image_length<=CDI_R8_APP_MAX_SIZE&&
    m->header_crc32==crc32(m,offsetof(cdi_r8_ota_manifest_t,header_crc32))&&
    m->inverse_crc32==~m->header_crc32&&
    m->image_crc32==crc32((const void *)CDI_R8_STAGE_ADDR,m->image_length);
}

static int app_valid(uint32_t length,uint32_t crc)
{
  uint32_t sp=*(const uint32_t *)CDI_R8_APP_ADDR;
  uint32_t reset=*(const uint32_t *)(CDI_R8_APP_ADDR+4u);
  return sp>=0x20000000u&&sp<0x20040000u&&(reset&1u)&&
    (reset&~1u)>=CDI_R8_APP_ADDR&&(reset&~1u)<CDI_R8_APP_ADDR+CDI_R8_APP_MAX_SIZE&&
    (length==0u||crc32((const void *)CDI_R8_APP_ADDR,length)==crc);
}

static void flash_wait(void){while(FLASH->SR&FLASH_SR_BSY){} }
static void flash_unlock(void)
{
  if(FLASH->CR&FLASH_CR_LOCK){FLASH->KEYR=FLASH_KEY1_VALUE;FLASH->KEYR=FLASH_KEY2_VALUE;}
}
static int erase_page(uint32_t address)
{
  uint32_t page=(address-0x08000000u)/FLASH_PAGE_BYTES;
  flash_wait();FLASH->SR=FLASH_SR_EOP|FLASH_ERRORS;
  FLASH->CR=(FLASH->CR&~FLASH_CR_PNB)|FLASH_CR_PER|(page<<FLASH_CR_PNB_Pos);
  FLASH->CR|=FLASH_CR_STRT;flash_wait();FLASH->CR&=~(FLASH_CR_PER|FLASH_CR_PNB);
  return (FLASH->SR&FLASH_ERRORS)==0u;
}
static int program64(uint32_t address,uint64_t word)
{
  flash_wait();FLASH->SR=FLASH_SR_EOP|FLASH_ERRORS;FLASH->CR|=FLASH_CR_PG;
  *(volatile uint32_t *)address=(uint32_t)word;
  *(volatile uint32_t *)(address+4u)=(uint32_t)(word>>32u);
  flash_wait();FLASH->CR&=~FLASH_CR_PG;return (FLASH->SR&FLASH_ERRORS)==0u;
}

static int install(const cdi_r8_ota_manifest_t *m)
{
  uint32_t off,pages=(m->image_length+FLASH_PAGE_BYTES-1u)/FLASH_PAGE_BYTES;
  flash_unlock();
  for(off=0u;off<pages;++off)if(!erase_page(CDI_R8_APP_ADDR+off*FLASH_PAGE_BYTES))return 0;
  for(off=0u;off<m->image_length;off+=8u){
    uint64_t word=0xffffffffffffffffull;uint32_t n=m->image_length-off;
    uint8_t *d=(uint8_t *)&word;const uint8_t *s=(const uint8_t *)(CDI_R8_STAGE_ADDR+off);uint32_t i;
    if(n>8u)n=8u;
    for(i=0u;i<n;++i)d[i]=s[i];
    if(!program64(CDI_R8_APP_ADDR+off,word))return 0;
  }
  if(!app_valid(m->image_length,m->image_crc32))return 0;
  if(!erase_page(CDI_R8_OTA_META_ADDR))return 0;
  FLASH->CR|=FLASH_CR_LOCK;return 1;
}

static void jump_to_app(void)
{
  uint32_t sp=*(const uint32_t *)CDI_R8_APP_ADDR;
  uint32_t reset=*(const uint32_t *)(CDI_R8_APP_ADDR+4u);
  void (*entry)(void)=(void (*)(void))reset;
  __disable_irq();SysTick->CTRL=0u;SCB->VTOR=CDI_R8_APP_ADDR;
  __set_MSP(sp);__DSB();__ISB();entry();
}

int main(void)
{
  const cdi_r8_ota_manifest_t *m=(const cdi_r8_ota_manifest_t *)CDI_R8_OTA_META_ADDR;
  if(manifest_valid(m))(void)install(m);
  if(app_valid(0u,0u))jump_to_app();
  for(;;){__WFI();}
}
