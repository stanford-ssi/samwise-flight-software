/**
 * @author  Joziah Uribe-Lopez, Luis Cantoran
 * @date    2025-11-01
 *
 * Writing, then power off and power on, then reading to verify data was saved.
 */
 #include "interrupt_task.h"
 #include "neopixel.h"
 
 // AI START
 #include "hardware/irq.h"
 #include "hardware/flash.h"
 #include "pico/stdlib.h"
 
 #define TEST_ADDR (XIP_BASE + 0x100000) // Example flash address
 #define TEST_PATTERN 0xDEADBEEF

 volatile bool interrupt_triggered = false;

// Interrupt handler
void test_irq_handler(void) {
    interrupt_triggered = true;
    printf("Interrupt occurred!\n");
    // Optionally simulate power loss or other effects here
}
// Task function
void interrupt_test_task(void) {
    uint32_t write_data = TEST_PATTERN;
    uint32_t read_data = 0;

    // Step 1: Write data to flash
    printf("Writing test pattern to flash...\n");
    flash_range_program(TEST_ADDR - XIP_BASE, (uint8_t*)&write_data, sizeof(write_data));

    // Step 2: Trigger interrupt (software)
    printf("Triggering software interrupt...\n");
    irq_set_exclusive_handler(SOME_IRQ_NUMBER, test_irq_handler);
    irq_set_enabled(SOME_IRQ_NUMBER, true);
    irq_set_pending(SOME_IRQ_NUMBER);

    // Wait for interrupt to be handled
    while (!interrupt_triggered) {
        tight_loop_contents();
    }

    // Step 3: Read back data
    read_data = *(volatile uint32_t*)TEST_ADDR;
    printf("Read back: 0x%08x\n", read_data);

    // Step 4: Check integrity
    if (read_data == TEST_PATTERN) {
        printf("Data integrity OK after interrupt.\n");
    } else {
        printf("Data corruption detected!\n");
    }
}

//AI END

 void interrupt_task_init(slate_t *slate){
  LOG_INFO("Interrupt task is initializing...");
  // Add initialization code here
 }

 void interrupt_task_dispatch(slate_t *slate){
   neopixel_set_color_rgb(INTERRUPT_TASK_COLOR);

   LOG_INFO("Interrupt task is running...");
   interrupt_task_init(slate);

   int counter = 0;
   char buffer[8];

   const char *payload = "Persist";
   memcpy(buffer, payload, 7);
   buffer[7] = '\0';
  //  mram_write(0x010, buffer, sizeof(buffer));

   /*
    * Avoid a tight busy-loop. Use the project's microsecond sleep helper
    * (sleep_us is used elsewhere in the codebase) to yield the CPU.
    */
   while (true) {
      LOG_INFO("The interrupt loop is on loop #: %d", counter);
      counter++;

      /* 250 ms delay to avoid flooding the log and to yield */
      sleep_us(250000);
   }

   // char read_buffer[8];
   // mram_read(0x010, read_buffer, sizeof(read_buffer));
   // LOG_DEBUG("The read resulted in: %s", read_buffer);

   neopixel_set_color_rgb(0, 0, 0); // Turn off neopixel when done with task
 }

 extern sched_task_t interrupt_task = {
   .name = "interrupt_task",
   .dispatch_period_ms = 1000,  // Adjust period as needed
   .task_init = &interrupt_task_init,
   .task_dispatch = &interrupt_task_dispatch,
   .next_dispatch = 0
};
