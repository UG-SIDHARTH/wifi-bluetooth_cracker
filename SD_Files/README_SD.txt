# SD Card Setup Instructions

Copy the contents of this folder (`SD_Files/`) directly to the root directory of your MicroSD card (FAT32 formatted).

## Files Included:

1. **`wordlist.txt`**:
   - A text file containing passphrases (one password per line).
   - You can edit this file on your computer to add your own custom passphrases.

2. **`cracked_keys.txt`** (Created automatically by the ESP32):
   - When a password match is found during an audit, the ESP32 will write the result to this log file on your SD card.
