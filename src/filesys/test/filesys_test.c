#include "filesys.h"

int filesys_test_write_readback()
{
    slate_t test_slate;
    lfs_ssize_t code_lfs = filesys_reformat(&test_slate); // Ensure clean slate

    if (code_lfs < 0)
    {
        LOG_ERROR(
            "Failed to reformat & initialize filesystem for testing: %d\n",
            code_lfs);
        return -1;
    }

    lfs_ssize_t res;

    FILESYS_BUFFERED_FNAME_STR_T fname_str = "A3";
    FILESYS_BUFFERED_FILE_CRC_T crc =
        0x100ECE8C; // Precomputed CRC for 0x00..0x3F

    int8_t code =
        filesys_start_file_write(&test_slate, fname_str, 64, crc, &res);

    if (code < 0)
    {
        LOG_ERROR("Failed to start file write for testing: %d\n", code);
        return -1;
    }

    LOG_DEBUG("Returned code from start_file_write: %d\n", code);
    ASSERT(test_slate.filesys_is_writing_file);

    uint8_t buffer[64];
    for (uint8_t i = 0; i < 64; i++)
        buffer[i] = i;

    code = filesys_write_data_to_buffer(&test_slate, buffer, sizeof(buffer), 0);

    if (code < 0)
    {
        LOG_ERROR("Failed to write data to buffer for testing: %d\n", code);
        return -1;
    }

    ASSERT(test_slate.filesys_buffer_is_dirty);

    code_lfs = filesys_write_buffer_to_mram(&test_slate, sizeof(buffer));

    if (code_lfs < 0)
    {
        LOG_ERROR("Failed to write buffer to MRAM for testing: %d\n", code_lfs);
        return -1;
    }

    ASSERT(!test_slate.filesys_buffer_is_dirty);

    code = filesys_complete_file_write(&test_slate);

    if (code < 0)
    {
        LOG_ERROR("Failed to complete file write for testing: %d\n", code);
        return -1;
    }

    ASSERT(!test_slate.filesys_is_writing_file);

    int8_t read_buffer[64];
    lfs_file_t read_file;
    lfs_file_opencfg(&test_slate.lfs, &read_file, fname_str, LFS_O_RDONLY,
                     &(struct lfs_file_config){.buffer = cache_buffer});

    lfs_file_read(&test_slate.lfs, &read_file, read_buffer,
                  sizeof(read_buffer));

    lfs_file_close(&test_slate.lfs, &read_file);

    for (size_t i = 0; i < sizeof(read_buffer); i++)
        if (read_buffer[i] != buffer[i])
        {
            LOG_ERROR("Data mismatch at byte %zu: expected %u, got %u\n", i,
                      buffer[i], read_buffer[i]);
            return -1;
        }

    return 0;
}

int main()
{
    LOG_DEBUG("Starting Filesys test\n");

    int res = filesys_test_write_readback();
    if (res < 0)
        return res;

    LOG_DEBUG("All Filesys tests passed!\n");

    return 0;
}
