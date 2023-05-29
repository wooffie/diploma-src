class Payload:
    def __init__(self, data):
        self.version = data[0]
        self.protocol_version = data[1]
        self.device_class = data[2]
        self.power_mode = data[3]
        self.wake_up_reason = data[4]
        self.payload_len = data[5]
        self.sensor_len = data[6]
        self.user_len = self.payload_len - self.sensor_len - 1
        self.sensor_data = data[7 : self.sensor_len]
        self.user_data = data[-self.user_len:]


class SimplePayload(Payload):
    def getHeartRate(self):
        return self.user_data[0]

    def getTemperature(self):
        return self.user_data[1]

    def getSaturation(self):
        return self.user_data[2]
