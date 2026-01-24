import json
import os
import time

STATE_FILE = "gs_state.json"

class StateManager:
    def __init__(self):
        self.boot_count = 0
        self.msg_id = 0
        self.last_save_time = 0
        self.load()

    def load(self):
        """Load state from persistence file"""
        try:
            # Check if file exists first to avoid exception spam
            try:
                os.stat(STATE_FILE)
            except OSError:
                print(f"No existing state file {STATE_FILE}, starting fresh.")
                return

            with open(STATE_FILE, "r") as f:
                data = json.load(f)
                self.boot_count = data.get("boot_count", 0)
                self.msg_id = data.get("msg_id", 0)
                print(f"Loaded state: Boot={self.boot_count}, MsgID={self.msg_id}")
        except Exception as e:
            print(f"Error loading state: {e}")

    def save(self):
        """Save state to persistence file"""
        try:
            data = {
                "boot_count": self.boot_count,
                "msg_id": self.msg_id
            }
            with open(STATE_FILE, "w") as f:
                json.dump(data, f)
            self.last_save_time = time.time()
        except Exception as e:
            print(f"Error saving state: {e}")

    def update_from_beacon(self, beacon_boot_count):
        """Update local boot count record if we see a newer one from satellite"""
        if beacon_boot_count > self.boot_count:
            print(f"Updating Boot Count: {self.boot_count} -> {beacon_boot_count}")
            self.boot_count = beacon_boot_count
            self.save()
        elif beacon_boot_count < self.boot_count:
            # Satellite reset? Or we have bad data?
            # For now, let's assume satellite truth is absolute if distinct
             print(f"Warning: Satellite boot count ({beacon_boot_count}) lower than local ({self.boot_count}). Satellite reset?")
             self.boot_count = beacon_boot_count
             self.save()

    def get_next_msg_id(self):
        """Increment and return next message ID"""
        self.msg_id += 1
        self.save()
        return self.msg_id

# Global singleton
state_manager = StateManager()
