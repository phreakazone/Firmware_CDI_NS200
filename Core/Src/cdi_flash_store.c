#include "main.h"
#include "hw_conf.h"
#include "shci.h"
#include "cdi_r5.h"
#include <stddef.h>
#include <string.h>

#define CDI_FLASH_MAGIC 0x374D4150u /* "PAM7" little-endian */
#define CDI_PAGE0_ADDR  0x0807E000u
#define CDI_PAGE1_ADDR  0x0807F000u
#define CDI_PROOF_ADDR  0x0807D000u
#define CDI_PROOF_MAGIC 0x38524650u /* "PFR8" */
#define CDI_PROOF_TOKEN 0xA55A3CC3u

typedef struct {
  uint32_t magic;
  uint32_t sequence;
  cdi_r5_store_image_t image;
  uint32_t record_crc;
} cdi_flash_record_t;

typedef union {
  cdi_flash_record_t record;
  uint64_t words[(sizeof(cdi_flash_record_t) + 7u) / 8u];
} cdi_flash_buffer_t;

static cdi_flash_buffer_t write_buffer;

typedef struct {
  uint32_t magic; uint16_t version; uint8_t stage,pickup_edge;
  uint16_t trigger_angle_cdeg; int16_t side_offset_cdeg;
  uint16_t pulses_per_revolution,gate_pulse_us;
  uint16_t tps_closed_adc,tps_open_adc;
  uint16_t first_start_hv_volts,first_start_rpm_limit;
  uint16_t first_start_advance_cap_cdeg;
  uint8_t center_enabled,side_enabled,fan_mode,reserved;
} cdi_r7_setup_legacy_t;
typedef struct {
  uint32_t magic;uint16_t version;uint8_t active_slot,reserved;
  cdi_r5_map_t slots[CDI_R5_MAP_SLOTS];cdi_r7_setup_legacy_t setup;uint32_t crc32;
} cdi_r7_store_legacy_t;
typedef struct {uint32_t magic,sequence;cdi_r7_store_legacy_t image;uint32_t record_crc;} cdi_r7_record_legacy_t;

bool R5_FlashSave(const cdi_r5_store_image_t *image);
extern bool R8_BleStackIsReady(void);

typedef struct { uint32_t magic, token, crc, inverse_crc; } cdi_proof_t;

static bool flash_take(void)
{
  uint32_t tries=100000u;
  while(tries--!=0u)
    if(HAL_HSEM_FastTake(CFG_HW_FLASH_SEMID)==HAL_OK) return true;
  return false;
}

static void flash_release(void)
{
  HAL_HSEM_Release(CFG_HW_FLASH_SEMID,0u);
}

static bool record_valid(const cdi_flash_record_t *r)
{
  return r->magic == CDI_FLASH_MAGIC &&
         r->record_crc == cdi_r5_crc32(r, offsetof(cdi_flash_record_t, record_crc)) &&
         cdi_r5_store_validate(&r->image) == CDI_R5_OK;
}

static bool legacy_valid(const cdi_r7_record_legacy_t *r)
{
  return r->magic==CDI_FLASH_MAGIC&&r->image.magic==CDI_R5_STORE_MAGIC&&
    r->image.version==3u&&r->image.active_slot<CDI_R5_MAP_SLOTS&&
    r->image.crc32==cdi_r5_crc32(&r->image,offsetof(cdi_r7_store_legacy_t,crc32))&&
    r->record_crc==cdi_r5_crc32(r,offsetof(cdi_r7_record_legacy_t,record_crc));
}

static bool migrate_legacy(const cdi_r7_store_legacy_t *old,cdi_r5_store_image_t *now)
{
  uint8_t i;cdi_r5_load_defaults(now);now->active_slot=old->active_slot;
  for(i=0u;i<CDI_R5_MAP_SLOTS;++i){now->slots[i]=old->slots[i];
    if(now->slots[i].mode==CDI_R5_MODE_PRO)now->slots[i].hv_target_volts=345u;}
  now->setup.stage=old->setup.stage;now->setup.pickup_edge=old->setup.pickup_edge;
  now->setup.trigger_angle_cdeg=old->setup.trigger_angle_cdeg;
  now->setup.side_offset_cdeg=old->setup.side_offset_cdeg;
  now->setup.pulses_per_revolution=old->setup.pulses_per_revolution;
  now->setup.gate_pulse_us=old->setup.gate_pulse_us;
  now->setup.tps_closed_adc=old->setup.tps_closed_adc;now->setup.tps_open_adc=old->setup.tps_open_adc;
  now->setup.first_start_hv_volts=old->setup.first_start_hv_volts;
  now->setup.first_start_rpm_limit=old->setup.first_start_rpm_limit;
  now->setup.first_start_advance_cap_cdeg=old->setup.first_start_advance_cap_cdeg;
  now->setup.fan_mode=old->setup.fan_mode;
  now->setup.operating_mode=CDI_R8_OP_MANUAL_SETUP;
  now->setup.center_enabled=0u;now->setup.side_enabled=0u;
  if(old->setup.stage==CDI_R7_STAGE_READY)now->setup.stage=CDI_R7_STAGE_TDC_SAVED;
  cdi_r5_store_seal(now);return cdi_r5_store_validate(now)==CDI_R5_OK;
}

bool R5_FlashLoad(cdi_r5_store_image_t *image)
{
  const cdi_flash_record_t *a = (const cdi_flash_record_t *)CDI_PAGE0_ADDR;
  const cdi_flash_record_t *b = (const cdi_flash_record_t *)CDI_PAGE1_ADDR;
  bool va = record_valid(a), vb = record_valid(b);
  const cdi_flash_record_t *chosen;
  if (!va && !vb) {
    const cdi_r7_record_legacy_t *la=(const cdi_r7_record_legacy_t *)CDI_PAGE0_ADDR;
    const cdi_r7_record_legacy_t *lb=(const cdi_r7_record_legacy_t *)CDI_PAGE1_ADDR;
    bool vla=legacy_valid(la),vlb=legacy_valid(lb);const cdi_r7_record_legacy_t *legacy;
    if(!vla&&!vlb)return false;
    legacy=(!vlb||(vla&&(int32_t)(la->sequence-lb->sequence)>0))?la:lb;
    if(!migrate_legacy(&legacy->image,image))return false;
    (void)R5_FlashSave(image);return true;
  }
  chosen = (!vb || (va && (int32_t)(a->sequence - b->sequence) > 0)) ? a : b;
  memcpy(image, &chosen->image, sizeof(*image));
  return true;
}

bool R5_FlashSave(const cdi_r5_store_image_t *image)
{
  const cdi_flash_record_t *a = (const cdi_flash_record_t *)CDI_PAGE0_ADDR;
  const cdi_flash_record_t *b = (const cdi_flash_record_t *)CDI_PAGE1_ADDR;
  bool va = record_valid(a), vb = record_valid(b);
  uint32_t sequence = 1u;
  uint32_t target = CDI_PAGE0_ADDR;
  uint32_t page_error = 0xffffffffu;
  FLASH_EraseInitTypeDef erase = {0};
  HAL_StatusTypeDef status;

  if (cdi_r5_store_validate(image) != CDI_R5_OK) return false;
  if (va || vb) {
    const cdi_flash_record_t *latest = (!vb || (va && (int32_t)(a->sequence - b->sequence) > 0)) ? a : b;
    sequence = latest->sequence + 1u;
    target = latest == a ? CDI_PAGE1_ADDR : CDI_PAGE0_ADDR;
  }

  memset(&write_buffer, 0xff, sizeof(write_buffer));
  write_buffer.record.magic = CDI_FLASH_MAGIC;
  write_buffer.record.sequence = sequence;
  memcpy(&write_buffer.record.image, image, sizeof(*image));
  write_buffer.record.record_crc = cdi_r5_crc32(&write_buffer.record,
      offsetof(cdi_flash_record_t, record_crc));

  erase.TypeErase = FLASH_TYPEERASE_PAGES;
  erase.Page = (target - FLASH_BASE) / FLASH_PAGE_SIZE;
  erase.NbPages = 1u;
  if (!flash_take()) return false;
  if (HAL_FLASH_Unlock() != HAL_OK) { flash_release(); return false; }
  if(R8_BleStackIsReady())(void)SHCI_C2_FLASH_EraseActivity(ERASE_ACTIVITY_ON);
  status = HAL_FLASHEx_Erase(&erase, &page_error);
  if(R8_BleStackIsReady())(void)SHCI_C2_FLASH_EraseActivity(ERASE_ACTIVITY_OFF);
  if (status == HAL_OK) {
    for (size_t i = 0; i < sizeof(write_buffer.words) / sizeof(write_buffer.words[0]); ++i) {
      status = HAL_FLASH_Program(FLASH_TYPEPROGRAM_DOUBLEWORD,
                                 target + (uint32_t)(i * 8u),
                                 write_buffer.words[i]);
      if (status != HAL_OK) break;
    }
  }
  (void)HAL_FLASH_Lock();
  flash_release();
  if (status != HAL_OK || page_error != 0xffffffffu) return false;
  return record_valid((const cdi_flash_record_t *)target);
}

bool R8_FirstStartProofLoad(void)
{
  const cdi_proof_t *p=(const cdi_proof_t *)CDI_PROOF_ADDR;
  return p->magic==CDI_PROOF_MAGIC && p->token==CDI_PROOF_TOKEN &&
         p->crc==cdi_r5_crc32(p,offsetof(cdi_proof_t,crc)) &&
         p->inverse_crc==~p->crc;
}

bool R8_FirstStartProofWrite(void)
{
  union {cdi_proof_t proof;uint64_t words[2];} p={{CDI_PROOF_MAGIC,CDI_PROOF_TOKEN,0u,0u}};
  HAL_StatusTypeDef s;
  p.proof.crc=cdi_r5_crc32(&p.proof,offsetof(cdi_proof_t,crc)); p.proof.inverse_crc=~p.proof.crc;
  if(R8_FirstStartProofLoad()) return true;
  if(*(const uint32_t *)CDI_PROOF_ADDR!=0xffffffffu || !flash_take()) return false;
  if(HAL_FLASH_Unlock()!=HAL_OK){flash_release();return false;}
  s=HAL_FLASH_Program(FLASH_TYPEPROGRAM_DOUBLEWORD,CDI_PROOF_ADDR,
      p.words[0]);
  if(s==HAL_OK) s=HAL_FLASH_Program(FLASH_TYPEPROGRAM_DOUBLEWORD,
      CDI_PROOF_ADDR+8u,p.words[1]);
  (void)HAL_FLASH_Lock(); flash_release();
  return s==HAL_OK && R8_FirstStartProofLoad();
}

bool R8_FirstStartProofClear(void)
{
  FLASH_EraseInitTypeDef erase={0}; uint32_t page_error=0xffffffffu;
  HAL_StatusTypeDef s;
  if(*(const uint32_t *)CDI_PROOF_ADDR==0xffffffffu) return true;
  erase.TypeErase=FLASH_TYPEERASE_PAGES;
  erase.Page=(CDI_PROOF_ADDR-FLASH_BASE)/FLASH_PAGE_SIZE; erase.NbPages=1u;
  if(!flash_take())return false;
  if(HAL_FLASH_Unlock()!=HAL_OK){flash_release();return false;}
  if(R8_BleStackIsReady())(void)SHCI_C2_FLASH_EraseActivity(ERASE_ACTIVITY_ON);
  s=HAL_FLASHEx_Erase(&erase,&page_error);
  if(R8_BleStackIsReady())(void)SHCI_C2_FLASH_EraseActivity(ERASE_ACTIVITY_OFF);
  (void)HAL_FLASH_Lock(); flash_release();
  return s==HAL_OK && page_error==0xffffffffu &&
         *(const uint32_t *)CDI_PROOF_ADDR==0xffffffffu;
}
