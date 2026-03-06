import json
import logging
import os
import time

STATE_FILE = "gs_state.json"
logger = logging.getLogger("GS.State")


class StateManager:
    """Optimized state manager that minimizes file I/O for high-throughput scenarios.

    As noted in review comments, file transfer protocol requires tens of thousands
    of packets in short duration, so we cannot afford to open/close files frequently.

    This implementation:
    - Batches state updates in memory
    - Only writes to disk every SAVE_INTERVAL_SEC or on critical state changes
    - Uses force saves for important events (boot count sync, shutdown)
    """

    SAVE_INTERVAL_SEC = 600  # Save every 10 minutes (reduced from 60s for high-throughput)

    def __init__(self, state_file=STATE_FILE):
        self.state_file = state_file
        self.boot_count = 0
        self.msg_id = 0
        self.last_save_time = 0
        self.dirty = False  # Track if we have unsaved changes
        self.load()

    def load(self):
        """Load state from persistence file"""
        try:
            # Check if file exists first to avoid exception spam
            try:
                os.stat(self.state_file)
            except OSError:
                logger.info("No existing state file %s, starting fresh.", self.state_file)
                return

            with open(self.state_file, "r") as f:
                data = json.load(f)
                self.boot_count = data.get("boot_count", 0)
                self.msg_id = data.get("msg_id", 0)
                logger.info("Loaded state: Boot=%d, MsgID=%d", self.boot_count, self.msg_id)
        except Exception as e:
            logger.error("Error loading state: %s", e)

    def save(self, force=False):
        """Save state to persistence file with optimized rate limiting.

        Args:
            force (bool): Force immediate save regardless of timing
        """
        now = time.time()

        # Skip save if:
        # 1. Not forced AND not enough time passed AND no changes made
        # 2. For high-throughput scenarios, we only save periodically or on force
        if not force and (now - self.last_save_time < self.SAVE_INTERVAL_SEC or not self.dirty):
            return

        try:
            data = {"boot_count": self.boot_count, "msg_id": self.msg_id, "last_updated": now}
            # Use atomic write to avoid corruption
            temp_file = self.state_file + ".tmp"
            with open(temp_file, "w") as f:
                json.dump(data, f, indent=2)

            # Atomic replace
            if os.name == "nt":  # Windows
                if os.path.exists(self.state_file):
                    os.replace(temp_file, self.state_file)
                else:
                    os.rename(temp_file, self.state_file)
            else:  # Unix-like
                os.rename(temp_file, self.state_file)

            self.last_save_time = now
            self.dirty = False
            logger.debug("State saved to disk: Boot=%d, MsgID=%d", self.boot_count, self.msg_id)
        except Exception as e:
            logger.error("Error saving state: %s", e)

    def update_from_beacon(self, beacon_boot_count):
        """Update local boot count record if we see a newer one from satellite"""
        if beacon_boot_count != self.boot_count:
            logger.info("Syncing Boot Count: %d -> %d", self.boot_count, beacon_boot_count)
            self.boot_count = beacon_boot_count
            self.dirty = True
            self.save(force=True)  # Critical state change, force save

    def get_next_msg_id(self):
        """Increment and return next message ID with optimized persistence.

        For high-throughput scenarios, we batch message ID updates and only
        persist to disk periodically rather than on every increment.
        """
        self.msg_id += 1
        self.dirty = True
        self.save()  # Uses rate limiting, won't actually write every time
        return self.msg_id

    def shutdown(self):
        """Final force save on exit to ensure no data loss"""
        if self.dirty:
            logger.info("Forcing final state save on shutdown")
            self.save(force=True)
        else:
            logger.debug("No unsaved changes on shutdown")


# Global singleton
state_manager = StateManager()
