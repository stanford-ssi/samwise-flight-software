# FTP Implementation

This is a document outlining how we implement an FTP designed for a space satellite. (Designed by Ayush Garg, Yao Yiheng, Marc Reyes, in tandem with others; implemented mostly by Ayush Garg).

This was specifically designed around multiple core concepts:
* Running on low-level, small memory hardware -> need low overhead & NO MALLOCS! (little-fs)
* Potential to drop packets in transit -> resiliance against packet dropping, ability to get data out-of-order
* Low RAM availability -> small file buffers & cyclic implementation
* One file uploaded at a time, for simplicity's sake (this may be upgraded later)

A few notes on this document:
1. See ftp_task.h to get in-depth information on how error and success packets are formatted from FTP to the sender.
2. See config.h to get information on types & limitations of the protocol, e.g. how many bytes maximum can be sent.

## Overall Design

The design is, in rough terms, as follows:
1. Start a file write
2. Loop:
    1. Allow N (=5 right now) "packets" of 210 bytes to be written at a time for the file. For example, when the file first starts, it will allow for packets 0..4 inclusive to be written in any order, and store each one in buffer.
    2. Once all packets in this cycle is complete, write to MRAM, and then clear buffer for the next cycle. So in the previous example, now allow packets 5..9 inclusive.
3. Once all cycles are complete, run a CRC32 check between the expected file & the actually writen file. If successful, finally finish the operation.

We implement the following functions:
* reformat filesystem
* start file write
* write packet to buffer (automatically writes to file if all packets are recieved)
* 

## Algorithm

### 0. Initialize & reformat
FTP automatically initializes little-fs. On error, it will send FILESYS_INIT_ERROR.

If this is recieved, or for any other reason, you can reformat the entire MRAM. THIS IS DESTRUCTIVE!!

To do so, simply send a FTP_REFORMAT command with no body. If successful, FILESYS_REFORMAT_SUCCESS will be sent - otherwise, FILESYS_REFORMAT_ERROR will be sent.

### 1. Start a file write
To start writing a file, first make sure that no file is currently being written. If one is, FTP will return a FTP_ERROR_ALREADY_WRITING_FILE. 

Otherwise, send a FTP_START_FILE_WRITE command with the following body:
```c
uint16_t fname_str; // Name of the file, maximum 2 bytes
uint16_t file_len; // Length of the file, maximum 2 bytes
uint32_t file_crc; // CRC32 for file validation after write
```

If successful, we will return FTP_READY_RECEIVE. Otherwise, FTP_ERROR_RECEIVE will be sent, with the nuances described in ftp_task.h.





