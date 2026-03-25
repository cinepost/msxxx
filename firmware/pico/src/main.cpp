#include <stdio.h>
#include "pico/stdlib.h"
#include "pico/multicore.h"
#include "pico/bit_ops.h"
#include "hardware/sync.h"
#include "hardware/spi.h"

#include "storage.h"


// Pin Definitions
#define PIN_NCONFIG   16  // Output to FPGA
#define PIN_NSTATUS   17  // Input from FPGA
#define PIN_DCLK      18  // SPI0 SCK Clock (Set to 0 during reset)
#define PIN_CONF_DONE 20  // Input from FPGA
#define PIN_DATA0     19  // SPI0 TX
#define PIN_CS        21  //
#define SPI_PORT      spi0

#define PIN_LED       PICO_DEFAULT_LED_PIN

#define PIN_BTN_1     10  // OSD Button 1
#define PIN_BTN_2     11  // OSD Button 2
#define PIN_BTN_3     12  // OSD Button 3
#define PIN_BTN_4     13  // OSD Button 4

#define I2C_SDA_PIN   4
#define I2C_SCL_PIN   5
#include "ssd1306/ssd1306_i2c.h"     // OLED

//#define ENCODER_A      2
//#define ENCODER_B      3
//#define ENCODER_BTN    6
#if defined(ENCODER_A) && defined(ENCODER_B) && defined(ENCODER_BTN)
#define ENCODER_ENABLED 1
#else
#define ENCODER_ENABLED 0
#endif

#define MAX_FILES 16
#define MAX_FILENAME_LEN 32

// Mutlicore
// Shared data structure
struct {
    uint32_t    some_value;
    mutex_t     data_mutex;
} shared_state;

// --- State Management ---
// Define GPIOs for the 4 buttons
const uint BUTTON_PINS[] = {PIN_BTN_1, PIN_BTN_2, PIN_BTN_3, PIN_BTN_4};
uint8_t pico_buttons_state = 0;

// Virtual dip switches
uint8_t pico_dip_switches = 0b11001100;

// Core .rbf files
char file_list[MAX_FILES][MAX_FILENAME_LEN];  // Store filenames
int selected_index = 0;
int total_rbf_files = 0;

void init_buttons() {
    for (int i = 0; i < 4; i++) {
        gpio_init(BUTTON_PINS[i]);
        gpio_set_dir(BUTTON_PINS[i], GPIO_IN);
        gpio_pull_up(BUTTON_PINS[i]); // Enable internal pull-up
    }
}

uint8_t read_dip_state() {
    uint8_t state = 0;
    for (int i = 0; i < 4; i++) {
        // Logical NOT (!) because pull-up means 0 = Pressed
        if (!gpio_get(BUTTON_PINS[i])) {
            state |= (1 << i);
        }
    }
    return state; // Returns a value from 0-15
}

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

static inline uint8_t reverse_uint8(uint8_t b) {
    // __rev reverses 32 bits. If input is 0x01 (00...0001), 
    // it becomes 0x80000000. Shift right by 24 to get 0x80.
    return (uint8_t)(__rev(b) >> 24);
}

bool load_selected_fpga_core(const char* filename) {
    lfs_file_t f;
    int err = lfs_file_open(&lfs, &f, filename, LFS_O_RDONLY);
    if (err) { 
        printf("Error opening core file: %s\n", filename);
        return false;
    }
    
    // 1. Reset FPGA (Same as previous SPI logic)
    if(!reset_fpga_sequence()) {
        drawString(0, 0, "FPGA INIT ERROR!");
        return false;
    }

    // 2. Stream from Flash to FPGA
    uint8_t buffer[256];
    int read_bytes;
    while ((read_bytes = lfs_file_read(&lfs, &f, buffer, 256)) > 0) {
        // Bit-reverse here if files aren't pre-processed
        for(int i=0; i<read_bytes; i++) {
            buffer[i] = reverse_uint8(buffer[i]) >> 24;
        }
        spi_write_blocking(spi0, buffer, read_bytes);
    }
    
    lfs_file_close(&lfs, &f);
    
    // 3. Check Success
    if (gpio_get(PIN_CONF_DONE)) {
        drawString(0, 0, "SUCCESS!");
    }
    return true;
}

void init_hardware() {
    // Initialize Standard I/O (USB Serial for debugging)
    stdio_init_all();

    // Initialize PICO on-board LED
    gpio_init(PIN_LED); gpio_set_dir(PIN_LED, GPIO_OUT);

    // Initialize FPGA Control Pins
    gpio_init(PIN_NCONFIG); gpio_set_dir(PIN_NCONFIG, GPIO_OUT);
    gpio_put(PIN_NCONFIG, 1); // Default High

    gpio_init(PIN_NSTATUS); gpio_set_dir(PIN_NSTATUS, GPIO_IN);
    gpio_pull_up(PIN_NSTATUS); // FPGA usually has pull-ups, but let's be safe

    gpio_init(PIN_CONF_DONE); gpio_set_dir(PIN_CONF_DONE, GPIO_IN);
    gpio_pull_up(PIN_CONF_DONE);

    // Initialize High-Speed SPI (for FPGA Bitstream)
    // 20MHz is a safe speed for Cyclone EP1C12
    spi_init(spi0, 20 * 1000 * 1000);
    gpio_set_function(PIN_DCLK, GPIO_FUNC_SPI);
    gpio_set_function(PIN_DATA0, GPIO_FUNC_SPI);

    // Initialize I2C (for SSD1306 OLED)
    i2c_init(I2C_PORT, I2C_CLOCK * 1000);
    gpio_set_function(I2C_SDA_PIN, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL_PIN, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA_PIN);
    gpio_pull_up(I2C_SCL_PIN);
    initSSD1306();
    invertDisplay(true);
    updateDisplay();

    // Initialize Rotary Encoder Pins
#if ENCODER_ENABLED
    gpio_init(ENCODER_A);
    gpio_set_dir(ENCODER_A, GPIO_IN);
    gpio_pull_up(ENCODER_A);

    gpio_init(ENCODER_B);
    gpio_set_dir(ENCODER_B, GPIO_IN);
    gpio_pull_up(ENCODER_B);

    gpio_init(ENCODER_BTN);
    gpio_set_dir(ENCODER_BTN, GPIO_IN);
    gpio_pull_up(ENCODER_BTN);
#endif // ENCODER_ENABLED
}

static inline int read_buttons_state() {
    return 0;
}

static inline bool button_pressed() {
    return false;
}

// --- CORE 1: UI PROCESS ---
void core1_entry() {
    // Initialize UI/Display here (e.g., LVGL init)
    
    while (1) {
        uint32_t current_val;
        
        // Safely read shared data from Core 0
        mutex_enter_blocking(&shared_state.data_mutex);
        current_val = shared_state.some_value;
        mutex_exit(&shared_state.data_mutex);

        // Update UI elements with current_val
        // lv_timer_handler(); // Example for LVGL
        sleep_ms(16); // ~60fps target

        gpio_put(PIN_LED, 1);
        sleep_ms(500);
        gpio_put(PIN_LED, 0);
        sleep_ms(500);
    }
}

int main() {
    // Init I2C for OLED, SPI for FPGA, and LittleFS
    init_hardware();

    // Launch UI on Core 1
    multicore_reset_core1();    
    multicore_launch_core1(core1_entry);

    mount_littlefs();
    init_buttons();

    mutex_init(&shared_state.data_mutex);

    while (true) {    

        // Display Menu
        clearDisplay();
        drawString(0, 0, "Select Core:");
        drawString(0, 20, file_list[selected_index]);
        updateDisplay();

        pico_buttons_state = read_buttons_state();
        printf("Buttons state: 0x%01X\n", pico_buttons_state);

        // If Button Pressed
        if (button_pressed()) {
            clearDisplay();
            drawString(0, 10, "Loading...");
            updateDisplay();
            if(load_selected_fpga_core(file_list[selected_index])) {
                // After config, switch pins 2 & 3 to SPI mode
                spi_init(spi0, 1000000);
                gpio_set_function(PIN_DCLK, GPIO_FUNC_SPI);
                gpio_set_function(PIN_DATA0, GPIO_FUNC_SPI);
                
                uint8_t fpga_readback = 0;

                while(true) {
                    gpio_put(PIN_CS, 0);
                    spi_write_read_blocking(spi0, &pico_dip_switches, &fpga_readback, 1);
                    gpio_put(PIN_CS, 1);
                    
                    printf("FPGA sent back: 0x%02X\n", fpga_readback);
                    sleep_ms(500);
                }
            } else {
                // TODO: handle configuration error
            }

            break;
        }

        // Perform logic: read sensors, calculate state, etc.
        uint32_t some_reading = (uint32_t)(get_absolute_time() / 1000);

        // Safely update shared data for the UI
        mutex_enter_blocking(&shared_state.data_mutex);
        shared_state.some_value = some_reading;
        mutex_exit(&shared_state.data_mutex);

        sleep_ms(100);
    }
}