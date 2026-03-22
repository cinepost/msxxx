#include "pico/stdlib.h"
#include "hardware/spi.h"
#include "fs.h"          // LittleFS
#include "ssd1306.h"     // OLED

// Pin Definitions
#define PIN_NCONFIG   16  // Output to FPGA
#define PIN_NSTATUS   17  // Input from FPGA
#define PIN_DCLK      18  // SPI0 SCK Clock (Set to 0 during reset)
#define PIN_CONF_DONE 20  // Input from FPGA
#define PIN_DATA0     19  // SPI0 TX
#define SPI_PORT      spi0

#define I2C_SDA       4
#define I2C_SCL       5

#define ENCODER_A     2
#define ENCODER_B     3
#define ENCODER_BTN   6

#define MAX_FILES 16
#define MAX_FILENAME_LEN 32

// --- State Management ---
char file_list[MAX_FILES][MAX_FILENAME_LEN];  // Store filenames
int selected_index = 0;
int total_rbf_files = 0;

// Helper to check if string ends with a specific suffix
bool has_extension(const char *name, const char *ext) {
    size_t nl = strlen(name);
    size_t el = strlen(ext);
    return nl >= el && strcasecmp(name + nl - el, ext) == 0;
}

void populate_file_list() {
    lfs_dir_t d;
    lfs_dir_open(&lfs, &d, "/");
    struct lfs_info info;
    while (lfs_dir_read(&lfs, &d, &info) > 0 && total_rbf_files < MAX_FILES) {
        if (info.type == LFS_TYPE_REG) { // Regular file
            if (has_extension(info.name, ".rbf")) {
                strcpy(file_list[total_rbf_files++], info.name);
            }
        }
    }
    lfs_dir_close(&lfs, &d);
}

bool reset_fpga_sequence() {
    // 1. Ensure DCLK is low before starting
    gpio_put(PIN_DCLK, 0);

    // 2. Drive nCONFIG low to begin the reset
    gpio_put(PIN_NCONFIG, 0);

    // 3. Hold nCONFIG low (Cyclone I requires minimum 40us)
    // We use 100us to be safe and ensure the FPGA sees the pulse.
    sleep_us(100);

    // 4. Release nCONFIG (Drive High)
    gpio_put(PIN_NCONFIG, 1);

    // 5. Wait for the FPGA to finish clearing its memory
    // The FPGA signals it is ready when nSTATUS goes back to HIGH.
    uint32_t timeout = 5000; // 50ms timeout
    while (gpio_get(PIN_NSTATUS) == 0) {
        sleep_us(10);
        if (--timeout == 0) {
            // Error: FPGA is stuck in reset or nSTATUS is pulled low by hardware error
            return false; 
        }
    }

    // 6. Final Check
    // CONF_DONE must be LOW at this point because the FPGA is empty.
    if (gpio_get(PIN_CONF_DONE) == 1) {
        return false; // Error: FPGA didn't clear correctly
    }

    return true; // FPGA is ready for bitstream data
}

bool load_selected_fpga_core(const char* filename) {
    lfs_file_t f;
    lfs_file_open(&lfs, &f, filename, LFS_O_RDONLY);
    
    // 1. Reset FPGA (Same as previous SPI logic)
    if(!reset_fpga_sequence()) {
        oled_print("FPGA INIT ERROR!");
        return false;
    }

    // 2. Stream from Flash to FPGA
    uint8_t buffer[256];
    int read_bytes;
    while ((read_bytes = lfs_file_read(&lfs, &f, buffer, 256)) > 0) {
        // Bit-reverse here if files aren't pre-processed
        for(int i=0; i<read_bytes; i++) buffer[i] = __RBIT(buffer[i]) >> 24;
        
        spi_write_blocking(spi0, buffer, read_bytes);
    }
    
    lfs_file_close(&lfs, &f);
    
    // 3. Check Success
    if (gpio_get(PIN_CONF_DONE)) {
        oled_print("SUCCESS!");
    }
    return true;
}

void init_hardware() {
    // 1. Initialize Standard I/O (USB Serial for debugging)
    stdio_init_all();

    // 2. Initialize FPGA Control Pins
    gpio_init(PIN_NCONFIG); gpio_set_dir(PIN_NCONFIG, GPIO_OUT);
    gpio_put(PIN_NCONFIG, 1); // Default High

    gpio_init(PIN_NSTATUS); gpio_set_dir(PIN_NSTATUS, GPIO_IN);
    gpio_pull_up(PIN_NSTATUS); // FPGA usually has pull-ups, but let's be safe

    gpio_init(PIN_CONF_DONE); gpio_set_dir(PIN_CONF_DONE, GPIO_IN);
    gpio_pull_up(PIN_CONF_DONE);

    // 3. Initialize High-Speed SPI (for FPGA Bitstream)
    // 20MHz is a safe speed for Cyclone EP1C12
    spi_init(spi0, 20 * 1000 * 1000);
    gpio_set_function(PIN_DCLK, GPIO_FUNC_SPI);
    gpio_set_function(PIN_DATA0, GPIO_FUNC_SPI);

    // 4. Initialize I2C (for SSD1306 OLED)
    i2c_init(i2c0, 400 * 1000); // 400kHz Fast Mode
    gpio_set_function(I2C_SDA, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA);
    gpio_pull_up(I2C_SCL);

    // 5. Initialize Rotary Encoder Pins
    gpio_init(ENCODER_A);
    gpio_set_dir(ENCODER_A, GPIO_IN);
    gpio_pull_up(ENCODER_A);

    gpio_init(ENCODER_B);
    gpio_set_dir(ENCODER_B, GPIO_IN);
    gpio_pull_up(ENCODER_B);

    gpio_init(ENCODER_BTN);
    gpio_set_dir(ENCODER_BTN, GPIO_IN);
    gpio_pull_up(ENCODER_BTN);
}

int main() {
    // Init I2C for OLED, SPI for FPGA, and LittleFS
    init_hardware();
    mount_littlefs();
    populate_file_list();

    while (true) {
        // Rotary Encoder logic to change selected_index
        // ...
        
        // Display Menu
        oled_clear();
        oled_draw_text(0, 0, "Select Core:");
        oled_draw_text(0, 20, file_list[selected_index]);
        oled_display();

        // If Button Pressed
        if (button_pressed()) {
            oled_clear();
            oled_draw_text(0, 10, "Loading...");
            oled_display();
            if(!load_selected_fpga_core(file_list[selected_index])) {
                // TODO: handle configuration error
            }

            break;
        }
    }
}