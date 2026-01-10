'''
Logging utilities
'''

import logging
import sys
import json
import os

STATUS_PATH = '/home/pi/code/status.json'
LOGS_PATH = '/home/pi/logs'

FILE_FORMAT = '%(name)s-%(levelname)s-%(message)s'
STREAM_FORMAT = '%(name)s-%(levelname)s-%(message)s'
FILE_LEVEL = logging.DEBUG
STREAM_LEVEL = logging.DEBUG

MAX_LOGS = 100

log = logging.getLogger(__name__)

def setup_logger(boot_count:int, file_level: int = FILE_LEVEL, stream_level: int = STREAM_LEVEL):
    '''
    Sets up the logging handlers
    - file_handlers handles the .log files
    - stream_handler handles the stdout.
    '''
    logpath = f'/home/pi/logs/{boot_count}.log'

    file_formatter = logging.Formatter(FILE_FORMAT)
    stream_formatter = logging.Formatter(STREAM_FORMAT)

    file_handler = logging.FileHandler(filename=logpath)
    file_handler.setFormatter(file_formatter)
    file_handler.setLevel(file_level)

    stream_handler = logging.StreamHandler(stream=sys.stdout)
    stream_handler.setFormatter(stream_formatter)
    stream_handler.setLevel(stream_level)

    logging.handlers = [] #cleans up previous handlers
    logging.basicConfig(
        handlers = [file_handler,stream_handler],
        level = 0,
        format = ""
    )
    log.debug('Setting the logger up')

def clean_logs():
    '''deletes the MAX_LOGS oldest logs (sorted by their boot_count)'''
    log.debug('Cleaning up the old logs')
    files = os.listdir(LOGS_PATH)
    files = sorted(files, key = lambda file : int(file.split('.')[0]), reverse=True)
    todelete = files[MAX_LOGS:]
    for file in todelete:
        filename : str = os.path.join(LOGS_PATH, file)
        os.remove(filename)


def update_boot_count():
    # Make boot count file if it does not exist
    if not os.path.exists(STATUS_PATH):
        print(f"First time boot! Creating status file in {STATUS_PATH}")
        with open(STATUS_PATH, 'w+') as file:
            file.write('{"boot_count": 0}')
            
    try:
        with open(STATUS_PATH, 'r') as file:
            status: dict = json.load(file)
            status['boot_count'] += 1
        with open(STATUS_PATH, 'w') as file:
            json.dump(status, file)
        return status['boot_count']
    except:
        # If error opening (which may happen if the file is corrupted for whatever reason)
        # Return a grabage boot count
        return 999999

def get_boot_count():
    try:
        with open(STATUS_PATH, 'r') as file:
            status : dict = json.load(file)

        return status['boot_count']
    
    except FileNotFoundError:
        # If not found, return a very large garbage value
        return 999999
