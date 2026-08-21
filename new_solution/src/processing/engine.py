"""Processing engine validation and normalization for MVP telemetry."""

from datetime import datetime, timezone
import logging

from processing.models import TelemetryRecord


logger = logging.getLogger(__name__)


class PayloadValidationError(ValueError):
    """Raised when an incoming payload does not satisfy the MVP contract."""


REQUIRED_TELEMETRY_FIELDS = ("device_id",)
OPTIONAL_TELEMETRY_FIELDS = (
    "temperature",
    "humidity",
    "pressure",
    "gas_resistance",
    "accel_x",
    "accel_y",
    "accel_z",
)


def normalize_telemetry_payload(payload: dict) -> dict:
    missing_fields = [field for field in REQUIRED_TELEMETRY_FIELDS if not payload.get(field)]
    if missing_fields:
        raise PayloadValidationError(
            f"Missing required telemetry fields: {', '.join(missing_fields)}"
        )

    normalized_payload = {field: payload[field] for field in REQUIRED_TELEMETRY_FIELDS}
    timestamp = payload.get("timestamp")
    if not timestamp:
        timestamp = datetime.now(timezone.utc).strftime("%Y-%m-%dT%H:%M:%SZ")
        logger.warning(
            "Missing timestamp in telemetry payload for device %s; generated backend UTC timestamp %s",
            normalized_payload["device_id"],
            timestamp,
        )
    normalized_payload["timestamp"] = timestamp
    for field in OPTIONAL_TELEMETRY_FIELDS:
        if field in payload and payload[field] is not None:
            normalized_payload[field] = payload[field]

    return normalized_payload


def build_telemetry_record(payload: dict) -> TelemetryRecord:
    normalized_payload = normalize_telemetry_payload(payload)
    return TelemetryRecord(**normalized_payload)


def normalize_payload(payload: dict) -> TelemetryRecord:
    return build_telemetry_record(payload)
