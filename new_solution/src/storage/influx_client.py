"""Storage client stub for InfluxDB access."""


class InfluxClient:
    def __init__(self, host: str, port: int):
        self.host = host
        self.port = port

    def write(self, payload: dict) -> bool:
        print(f"Writing to InfluxDB: {payload}")
        return True
