"""Processing engine stub for validation and transformation."""


def normalize_payload(payload: dict) -> dict:
    return {
        "device_id": payload.get("device_id"),
        "timestamp": payload.get("timestamp"),
        "temperature": payload.get("temperature"),
        "humidity": payload.get("humidity"),
        "pressure": payload.get("pressure"),
    }
