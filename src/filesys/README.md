# Filesys Implementation

Filesys is meant to be used for interacting with the MRAM chip onboard SAMWISE. It is how FTP writes data to disk as well.

It uses Little-FS behind the hood (https://github.com/littlefs-project/littlefs) to read/write files.

## Design
Designed for use with FTP to facilitate easy file writes to a buffer before writing to MRAM. This means it balances out-of-order writes (in RAM) while making RAM usage small by appending to disk (in MRAM).

It also is meant to use CRC checking to make sure there are no issues.

Here is a "happy path" for a filesystem write operation:
```c
1. filesys_initialize // Initialize file system
   - OR: filesys_reformat_initialize // Completely wipe MRAM & initialize file system
2. filesys_start_file_write // Start a file write
Loop:
   3. filesys_write_data_to_buffer // Write at any position to buffer until buffer is full
   4. filesys_write_buffer_to_mram // Write entire buffer to MRAM, appending to end of file
   // REPEAT UNTIL WHOLE FILE WRITTEN
5. filesys_complete_file_write // Complete the file write and check CRC
   - OR: filesys_cancel_file_write // Cancel the file write and remove it completely
```

## Limitations ("Design Choices")
* Only allows 2 bytes per file name
* Buffers around 1KB currently in RAM before you must manually write to MRAM
* Reading is not handled by filesys & should be done directly with LFS (TODO: should we change this?)
