/***
 * @author Ayush Garg, Luis Cantoran, Joziah Uribe-Lopez
 * @date   2025-10-18
 *
 *  Test for mram driver
 */

#include "mram_test.h"

void test_mram_write_read(void)
{
  LOG_DEBUG("Testing MRAM write...\n");
  mram_write(0x000000, (uint8_t *)"Hello, MRAM!", 13);

  LOG_DEBUG("Testing MRAM read...\n");
  uint8_t buffer[13];
  mram_read(0x000000, buffer, 13);
  LOG_DEBUG("Read from MRAM: %s\n", buffer);

  ASSERT(memcmp(buffer, "Hello, MRAM!", 13) == 0, "MRAM read/write failed");
}

void test_mram_write_disable_enable(void)
{
  LOG_DEBUG("Testing MRAM write disable...\n");
  mram_write_disable();
  mram_write(0x000100, (uint8_t *)"This should not be written", 26);
  ASSERT(memcmp((uint8_t *)"This should not be written", (uint8_t *)0x000100, 26) != 0, "MRAM write disable failed");
    
  LOG_DEBUG("Testing MRAM write enable...\n");
  mram_write_enable();
  mram_write(0x001000, (uint8_t *)"This should be written", 22);
  ASSERT(memcmp((uint8_t *)"This should be written", (uint8_t *)0x000100, 22) == 0, "MRAM write enable failed");
}

void test_mram_preserve_data_on_sleep(void)
{
  
}

void test_mram_clear(void)
{
  
}

void test_mram_read_status(void)
{
    LOG_DEBUG("Testing MRAM read status...\n");
}

int main()
{
  LOG_DEBUG("Starting MRAM test\n");
  mram_init();
  test_mram_write_read();
  test_mram_write_disable_enable();


  LOG_DEBUG("Running allocation tests...\n");
  mram_allocation_init();



  return 0;
}
