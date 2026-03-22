#include "lfs.h"
#include "pico/stdlib.h"
#include "hardware/flash.h"

// --- Configuration for 16MB Flash ---
// We reserve the first 1MB for the Pico Program (Binary)
// The remaining 15MB is for LittleFS
#define FS_SIZE         (15 * 1024 * 1024) 
#define FS_OFFSET       (1 * 1024 * 1024)  
#define BLOCK_SIZE      4096

// Variables for LittleFS
lfs_t lfs;
struct lfs_config cfg;

// Hardware abstraction functions for LittleFS to talk to Pico Flash
int pico_read(const struct lfs_config *c, lfs_block_t block, lfs_off_t off, void *buffer, lfs_size_t size) {
    uint32_t addr = XIP_BASE + FS_OFFSET + (block * c->block_size) + off;
    memcpy(buffer, (void *)addr, size);
    return LFS_ERR_OK;
}

int pico_prog(const struct lfs_config *c, lfs_block_t block, lfs_off_t off, const void *buffer, lfs_size_t size) {
    uint32_t addr = FS_OFFSET + (block * c->block_size) + off;
    flash_range_program(addr, (const uint8_t *)buffer, size);
    return LFS_ERR_OK;
}

int pico_erase(const struct lfs_config *c, lfs_block_t block) {
    uint32_t addr = FS_OFFSET + (block * c->block_size);
    flash_range_erase(addr, c->block_size);
    return LFS_ERR_OK;
}

int pico_sync(const struct lfs_config *c) { return LFS_ERR_OK; }

void mount_littlefs() {
    // Initialize the LittleFS configuration
    cfg.read  = pico_read;
    cfg.prog  = pico_prog;
    cfg.erase = pico_erase;
    cfg.sync  = pico_sync;

    cfg.read_size      = 16;
    cfg.prog_size      = FLASH_PAGE_SIZE; // 256
    cfg.block_size     = BLOCK_SIZE;      // 4096
    cfg.block_count    = FS_SIZE / BLOCK_SIZE;
    cfg.cache_size     = 256;
    cfg.lookahead_size = 32;
    cfg.block_cycles   = 500;

    // 1. Try to mount the filesystem
    int err = lfs_mount(&lfs, &cfg);

    // 2. If it fails (first time use), format and then mount
    if (err) {
        printf("Formatting Flash...\n");
        oled_draw_text(0, 0, "Formatting Flash...");
        lfs_format(&lfs, &cfg);
        lfs_mount(&lfs, &cfg);
    }
}
