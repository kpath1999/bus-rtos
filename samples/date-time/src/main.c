#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include "rtc.h"

LOG_MODULE_REGISTER(main, CONFIG_LOG_DEFAULT_LEVEL);

void main(void)
{
    LOG_INF("Starting RTC application");
    
    // Initialize RTC
    if (!init_rtc()) {
        LOG_ERR("Failed to initialize RTC");
        return;
    }
    
    // Set initial time (March 19, 2025, 8:25 PM)
    set_rtc_time(2025, 3, 19, 20, 25, 0);
    
    while (1) {
        // Get current time
        struct datetime dt;
        get_datetime(&dt);
        
        // Print time to serial console
        printk("\033[1;1H");  // Move cursor to top-left
        printk("\033[2J");    // Clear screen
        
        printk("Current Date and Time\n");
        printk("-------------------------------------------------------------------------------\n");
        printf("Date: %04d-%02d-%02d\n", dt.year, dt.month, dt.day);
        printf("Time: %02d:%02d:%02d\n", dt.hour, dt.minute, dt.second);
        printk("-------------------------------------------------------------------------------\n");
        
        // Wait 1 second before updating
        k_sleep(K_SECONDS(1));
    }
}