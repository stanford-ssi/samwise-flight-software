"""
'serialize_command'
===============================

A suite of commands to serialize all payload
commands to send up to the satellite

* Author:
    - Marc Aaron Reyes

Implementation Notes
--------------------
"""

CODE_DIR = "/home/pi/code"
IMAGES_DIR = "/home/pi/images"
VIDEOS_DIR = "/home/pi/videos"

class SerializeCommand:
    def serialize_ping():
        return "\"ping\", [], {}"

    def serialize_shutdown():
        return "\"shutdown\", [], {}"

    def serialize_reboot():
        return "\"reboot\", [], {}"

    def serialize_info():
        return "\"info\", [], {}"

    def serialize_eval_python(expression: str):
        return f"\"eval_python\", [{expression!r}], {{}}"

    def serialize_exec_python(code: str):
        return f"\"exec_python\", [{code!r}], {{}}"

    def serialize_exec_terminal(command: str):
        return f"\"exec_terminal\", [{command!r}], {{}}"

    def serialize_list_dir(path: str):
        return f"\"list_dir\", [{path!r}], {{}}"

    def serialize_delete_file(filepath: str):
        return f"\"delete_file\", [{filepath!r}], {{}}"

    def serialize_receive_file(filepath: str):
        return f"\"receive_file\", [{filepath!r}], {{}}"

    def serialize_send_file(filepath: str, compressed: bool = False):
        return "\"\", [], {}"

    def serialize_send_file_packet(filepath: str, packet_num: int, packet_size: int = 250):
        return "\"\", [], {}"

    def serialize_turn_on_2400():
        return "\"\", [], {}"

    def serialize_turn_off_2400():
        return "\"\", [], {}"

    def serialize_send_file_2400(filepath: str):
        return "\"\", [], {}"

    def serialize_send_packets_2400(filepath: str, packets: list[int], packet_size: int = 253):
        return "\"\", [], {}"

    def serialize_get_num_packets(filepath: str, packet_size: int = 250):
        return "\"\", [], {}"

    def serialize_crc_file(filepath: str):
        return "\"\", [], {}"

    def serialize_get_photo_config():
        return "\"\", [], {}"

    def serialize_set_photo_config(config: dict):
        return "\"\", [], {}"

    def serialize_get_video_config():
        return "\"\", [], {}"

    def serialize_set_video_config(config: dict):
        return "\"\", [], {}"

    def serialize_take_photo(image_id: str, camera_name='A', config="default", w=800, h=600, quality=100, cells_x=1, cells_y=1):
        return "\"\", [], {}"

    def serialize_compress_photo(image_id: str, w=800, h=600, quality=100, cells_x=1, cells_y=1):
        return "\"\", [], {}"

    def serialize_take_vid(vid_id: str, camera_name='A', libcamera_config='default', ffmpeg_in_config='default', ffmpeg_out_config='default'):
        return "\"\", [], {}"

    def serialize_take_process_send_image(image_id: str, camera_name='A', config='default', w=800, h=600, quality=100, cells_x=1, cells_y=1, ssdv=True, radio_type:str='2400'):
        return "\"\", [], {}"

    def serialize_process_send_image(image_id: str, w=800, h=600, quality=100, cells_x=1, cells_y=1, ssdv=True, radio_type:str='2400'):
        return "\"\", [], {}"

