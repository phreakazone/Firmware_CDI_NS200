#include "main.h"
#include "cdi_r5.h"
#include <stddef.h>
#include <string.h>

#define CDI_FLASH_MAGIC 0x374D4150u /* "PAM7" little-endian */
#define CDI_PAGE0_ADDR  0x0807E000u
#define CDI_PAGE1_ADDR  0x0807F000u

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

static bool record_valid(const cdi_flash_record_t *r)
{
  return r->magic == CDI_FLASH_MAGIC &&
         r->record_crc == cdi_r5_crc32(r, offsetof(cdi_flash_record_t, record_crc)) &&
         cdi_r5_store_validate(&r->image) == CDI_R5_OK;
}

bool R5_FlashLoad(cdi_r5_store_image_t *image)
{
  const cdi_flash_record_t *a = (const cdi_flash_record_t *)CDI_PAGE0_ADDR;
  const cdi_flash_record_t *b = (const cdi_flash_record_t *)CDI_PAGE1_ADDR;
  bool va = record_valid(a), vb = record_valid(b);
  const cdi_flash_record_t *chosen;
  if (!va && !vb) return false;
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
  if (HAL_FLASH_Unlock() != HAL_OK) return false;
  status = HAL_FLASHEx_Erase(&erase, &page_error);
  if (status == HAL_OK) {
    for (size_t i = 0; i < sizeof(write_buffer.words) / sizeof(write_buffer.words[0]); ++i) {
      status = HAL_FLASH_Program(FLASH_TYPEPROGRAM_DOUBLEWORD,
                                 target + (uint32_t)(i * 8u),
                                 write_buffer.words[i]);
      if (status != HAL_OK) break;
    }
  }
  (void)HAL_FLASH_Lock();
  if (status != HAL_OK || page_error != 0xffffffffu) return false;
  return record_valid((const cdi_flash_record_t *)target);
}
