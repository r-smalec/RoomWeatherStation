import serial
from influxdb_client import InfluxDBClient, Point, WriteOptions
import time

serial_port = '/dev/ttyACM0'
baud_rate = 9600
timeout = 1

url = "http://localhost:8086"

client = InfluxDBClient(url=url)
write_api = client.write_api() #write_options=WriteOptions(batch_size=100, flush_interval=1000)

ser = serial.Serial(serial_port, baud_rate, timeout=timeout)

try:
    time_stamp = 0
    time_stamp_prev = 0
    
    while True:
        while time_stamp_prev == time_stamp or time_stamp == 0:
            line = ser.readline().decode().strip()
            line = line [:-1]
            data_fields = line.split(',')
            t = list(data_fields[0])
            
            if t[0].isdigit() and t[1].isdigit() and t[3].isdigit() and t[4].isdigit() and  t[6].isdigit() and t[7].isdigit():
                time_stamp = (int(t[0]) *10 + int(t[1])) *3600 + (int(t[3]) *10 + int(t[4])) *60 + int(t[6]) *10 + int(t[7])
                #print(f"{time_stamp} {time_stamp_prev}")
            
        try:
            temperature = float(data_fields[1])
        except:
            temperature = 0.0
        try:
            pressure = float(data_fields[2])
        except:
            pressure = 0.0
        try:
            altitude = float(data_fields[3])
        except:
            altitude = 0.0
        try:
            humidity = float(data_fields[4])
        except:
            humidity = 0.0
        try:
            aqi = int(data_fields[5])
        except:
            aqi = 0
        try:
            tvoc = int(data_fields[6])
        except:
            tvoc = 0
        try:
            eco2 = int(data_fields[7])
        except:
            eco2 = 0
            
        time_stamp_prev = time_stamp
        unix_time = int(time.time()) * 1000000000
        print(f"{unix_time} {temperature} {pressure} {altitude} {humidity} {aqi} {tvoc} {eco2}")
        if time_stamp != 0:
            dictionary = {
                "name": "RoomStation",
                "location": "Room",
                "version": "1.0",
                "temperature": temperature, 
                "pressure": pressure,
                "altitude": altitude,
                "humidity": humidity,
                "aqi": aqi,
                "tvoc": tvoc,
                "eco2": eco2,
                "time": unix_time
            }
            point = Point.from_dict(dictionary,
                        record_measurement_key="name",
                        record_time_key="time",
                        record_tag_keys="location",
                        record_field_keys=["temperature", "pressure", "altitude", "humidity", "aqi", "tvoc", "eco2"])

            write_api.write("RoomStation", "RoomStation", point)
            
except KeyboardInterrupt:
    pass
finally:
    ser.close()
    client.close()


