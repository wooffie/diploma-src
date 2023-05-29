import socket
import struct
import logging
import PBHeader_pb2 as result
from payload import SimplePayload


logger = logging.getLogger("main")
logger.setLevel(logging.DEBUG)

file_handler = logging.FileHandler("log.log")
file_handler.setLevel(logging.DEBUG)

console_handler = logging.StreamHandler()
console_handler.setLevel(logging.DEBUG)

formatter = logging.Formatter("%(asctime)s - %(name)s - %(levelname)s - %(message)s")
file_handler.setFormatter(formatter)
console_handler.setFormatter(formatter)

logger.addHandler(file_handler)
logger.addHandler(console_handler)


def decode_data(message):
    msg = result.PBResultSet()
    if msg.ParseFromString(message):
        src_id = "{:x}".format(msg.src_id).zfill(12)
        blink_id = "{:}".format(msg.blink_id)
        unix_time = "{:}".format(msg.timestamp_sec)
        position = "{:06.2f}:{:06.2f}:{:06.2f}".format(
            float("inf"), float("inf"), float("inf")
        )
        user = "{:}:{:}:{:}".format(0, 0, 0)
        if msg.HasField("position"):
            position = "{:08.2f}:{:08.2f}:{:08.2f}".format(
                msg.position.x_position,
                msg.position.y_position,
                msg.position.z_position,
            )
        if msg.HasField("payload"):
            payload = SimplePayload(list(msg.payload))
            user = "{:}:{:}:{:}".format(
                payload.getHeartRate(),
                payload.getSaturation(),
                payload.getTemperature(),
            )
        served = set()
        for anchor in msg.rssi_entry:
            served.add("{:x}".format(anchor.anchor_id).zfill(12))

        r = " | ".join([src_id, blink_id, unix_time, position, user, ",".join(served)])
        logger.info(r)
    else:
        logger.warning("Unknown message type")


def connect(localhost, port):
    while True:
        try:
            s = socket.socket()
            s.connect((localhost, port))
            return s
        except socket.error as e:
            logger.error("Error while connecting to nanoLOC: %s", e)
        logger.warning("Reconnecting...")


def read_port(s):
    data = s.recv(2)
    length = struct.unpack("!H", data)
    data = s.recv(length[0])
    return data


if __name__ == "__main__":
    logger.info("Starting app...")
    s = connect("127.0.0.1", 3458)
    while 1:
        try:
            data = read_port(s)
        except socket.error as e:
            logger.error("Connection error: %s", e)
            s.close()
            exit(-1)
        decode_data(data)
    s.close()
