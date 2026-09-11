#include "main.h"
#include "hw_conf.h"
#include "cdi_r8_ota.h"
#include "shci.h"

#include <string.h>

static bool take_flash(void)
{
  uint32_t tries=100000u;
  while(tries--!=0u)
    if(HAL_HSEM_FastTake(CFG_HW_FLASH_SEMID)==HAL_OK)return true;
  return false;
}

static void release_flash(void){HAL_HSEM_Release(CFG_HW_FLASH_SEMID,0u);}

static bool erase_pages(uint32_t address,uint32_t count)
{
  FLASH_EraseInitTypeDef e={0};uint32_t error=0xffffffffu;HAL_StatusTypeDef s;
  e.TypeErase=FLASH_TYPEERASE_PAGES;e.Page=(address-FLASH_BASE)/FLASH_PAGE_SIZE;
  e.NbPages=count;
  (void)SHCI_C2_FLASH_EraseActivity(ERASE_ACTIVITY_ON);
  if(!take_flash()){(void)SHCI_C2_FLASH_EraseActivity(ERASE_ACTIVITY_OFF);return false;}
  if(HAL_FLASH_Unlock()!=HAL_OK){release_flash();(void)SHCI_C2_FLASH_EraseActivity(ERASE_ACTIVITY_OFF);return false;}
  s=HAL_FLASHEx_Erase(&e,&error);(void)HAL_FLASH_Lock();release_flash();
  (void)SHCI_C2_FLASH_EraseActivity(ERASE_ACTIVITY_OFF);
  return s==HAL_OK&&error==0xffffffffu;
}

bool R8_OtaFlashErase(void *context)
{
  (void)context;
  if(!erase_pages(CDI_R8_OTA_META_ADDR,1u))return false;
  return erase_pages(CDI_R8_STAGE_ADDR,CDI_R8_APP_MAX_SIZE/FLASH_PAGE_SIZE);
}

bool R8_OtaFlashProgram(uint32_t offset,const uint8_t *data,size_t length,
                        void *context)
{
  uint8_t block[8];size_t done=0u;HAL_StatusTypeDef s=HAL_OK;(void)context;
  if(data==0||offset+length>CDI_R8_APP_MAX_SIZE||(offset&7u)!=0u||!take_flash())return false;
  if(HAL_FLASH_Unlock()!=HAL_OK){release_flash();return false;}
  while(done<length){
    size_t n=length-done>sizeof(block)?sizeof(block):length-done;
    uint64_t word;memset(block,0xff,sizeof(block));memcpy(block,data+done,n);
    memcpy(&word,block,sizeof(word));
    s=HAL_FLASH_Program(FLASH_TYPEPROGRAM_DOUBLEWORD,
        CDI_R8_STAGE_ADDR+offset+(uint32_t)done,word);
    if(s!=HAL_OK)break;
    done+=n;
  }
  (void)HAL_FLASH_Lock();release_flash();return s==HAL_OK;
}

bool R8_OtaFlashFinalize(const cdi_r8_ota_manifest_t *m,void *context)
{
  uint64_t word;size_t off;(void)context;
  if(!cdi_r8_ota_manifest_valid(m)||!take_flash())return false;
  if(HAL_FLASH_Unlock()!=HAL_OK){release_flash();return false;}
  for(off=0u;off<sizeof(*m);off+=8u){
    memcpy(&word,(const uint8_t *)m+off,8u);
    if(HAL_FLASH_Program(FLASH_TYPEPROGRAM_DOUBLEWORD,
       CDI_R8_OTA_META_ADDR+(uint32_t)off,word)!=HAL_OK)break;
  }
  (void)HAL_FLASH_Lock();release_flash();
  return off==sizeof(*m)&&cdi_r8_ota_manifest_valid(
      (const cdi_r8_ota_manifest_t *)CDI_R8_OTA_META_ADDR);
}
