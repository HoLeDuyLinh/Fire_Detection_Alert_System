import serial
import time


try:
    ser = serial.Serial('/dev/serial0', baudrate=115200, timeout=1)
    print("Serial port opened.")
except serial.SerialException as e:
    print("No serial:", e)
    exit(1)


try:
    while True:
        if ser.in_waiting > 0:
            data = ser.readline().decode('utf-8', errors='replace').strip()
            print("Received:", data)
        time.sleep(0.1)  

except KeyboardInterrupt:
    print("User STop")

finally:
    ser.close()
    print("serial Fail")
