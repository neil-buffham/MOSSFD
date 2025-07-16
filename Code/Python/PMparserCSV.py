import serial
import time
import csv

# ---- USER CONFIGURATION ----
PORT = 'COM5'         # Change to your serial port
BAUD_RATE = 115200    # Match your ESP32 firmware baud
MODULE_IDS = [
    "14146C1C4278",
    "E0F76B1C4278",
    "0445470B65F4",
    "F893963B015C",
    "2C1A6D1C4278",
    "9CDC550B65F4",
    "5CB469BF1388",
    "1CF861BF1388",
    "848A6C1C4278"
][:9]  # Limit to 9 modules only
DELAY_BEFORE_SEND = 0.05  # seconds
CSV_FILE = 'messages.csv'  # Your CSV file path
# ----------------------------

def build_paragraph_message(chars, ids):
    total_modules = len(ids)
    chars = chars[:total_modules]  # Cut at 9 characters
    padding_needed = total_modules - len(chars)
    left_padding = padding_needed // 2
    right_padding = padding_needed - left_padding
    chars = (' ' * left_padding) + chars + (' ' * right_padding)

    message = f"****[{{{ids[0]}}}"
    for mod_id, char in zip(ids, chars):
        message += f"|{mod_id}:{char}:0"
    message += "]"
    return message

def send_to_serial(ser, message):
    ser.write((message + "\n").encode("utf-8"))

def main():
    print("Split-Flap Message Loop From CSV (CTRL+C to exit)\n")

    try:
        with open(CSV_FILE, newline='') as csvfile, serial.Serial(PORT, BAUD_RATE, timeout=2) as ser:
            reader = csv.reader(csvfile)

            for i, row in enumerate(reader):
                print(f"[Line {i+1}] Raw CSV row: {row}")  # DEBUG

                if len(row) != 2:
                    print(f"  Skipped: Expected 2 columns, got {len(row)}")
                    continue

                msg_text = row[0]
                try:
                    duration = float(row[1])
                except ValueError:
                    print("  Skipped: Invalid duration format")
                    continue

                if not msg_text:
                    print("  Skipped: Empty message")
                    continue

                msg_text = msg_text[:9]  # Enforce 9-char limit
                msg = build_paragraph_message(msg_text, MODULE_IDS)
                print(f"  Sending: \"{msg_text}\" | Duration: {duration}s")
                print(f"  Built Msg: {msg}")

                time.sleep(DELAY_BEFORE_SEND)
                send_to_serial(ser, msg)
                time.sleep(duration)

    except FileNotFoundError:
        print(f"CSV file not found: {CSV_FILE}")
    except serial.SerialException as e:
        print(f"Serial error: {e}")
    except KeyboardInterrupt:
        print("\nExited.")


if __name__ == "__main__":
    main()
