# Minimal Ground Station

In order to test FTP, we will create a minimal ground station that will essentially just send raw packets for the long range test.

Specifically, we want to test a large file write that contains the following:

1. Multiple cycles of writes (i.e. the file must be larger than `205 * N = 205 * 256 = 52480 bytes` so that the FTP must dump entire buffer into MRAM before re-sending - see README.md)
2. Out of order writing & lots of dropped packets
   - This means we want to have long times between packet receiving and forced packet drops
   - Additionally, test what happens when we duplicate packets (TODO needs to be outlined in [#317](https://github.com/stanford-ssi/samwise-flight-software/issues/317))
3. Final write and CRC32 check!

This means that we need to essentially go through the entire "happy path" of the algorithm. However, we can make everything manual and non-reactive (i.e. the human "reacts" and acts out the algorithm on the ground station side) just to save dev time. 

So the idea is to just make like a single python script that has a "console" that you can send and receive packets through, and a bunch of commands in that console that correspond to every packet type outlined in the README. It should clearly output/log every packet it receives.
* I would recommend using `rich` (python package) for easier to parse/cleaner output, and also asking Claude Code/Copilot/Whatever as its really good at this stuff

After that, we should add automation into the mix - including the entire algorithm being done by the computer itself. This would be amazing if possible, but above is minimal/least amount of work needed.
