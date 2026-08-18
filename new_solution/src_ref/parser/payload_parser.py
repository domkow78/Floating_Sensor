"""
Schema-driven payload parser.
Field definitions are loaded from config.yaml under payload_schema.<msg_type>.
Unknown fields are kept as-is (forward-compatible).
Missing required fields are logged as warnings.
"""

import logging
from typing import Any

logger = logging.getLogger(__name__)

_TYPE_MAP = {
    "int": int,
    "float": float,
    "str": str,
    "bool": bool,
}


class PayloadParser:
    def __init__(self, schema: dict):
        """
        schema: the 'payload_schema' section from config.yaml.
        E.g. { 'measurement': [...], 'diag': [...] }
        """
        self._schema: dict[str, list[dict]] = schema

    def parse(self, msg_type: str, raw: dict | str) -> dict[str, Any] | None:
        """
        Parse and validate a payload dict against the schema for msg_type.

        Returns a dict with:
          - known fields cast to declared types
          - unknown fields kept as received (str / number)
          - 'ts' always present if required (returns None if missing)
        Returns None if raw is not a dict (e.g. plain string payload).
        """
        if not isinstance(raw, dict):
            logger.debug("Non-JSON payload for '%s': %r", msg_type, raw)
            return None

        field_defs = self._schema.get(msg_type, [])
        result: dict[str, Any] = {}

        # Process known fields from schema
        known_keys = set()
        for field in field_defs:
            key = field["key"]
            known_keys.add(key)
            required = field.get("required", False)
            cast = _TYPE_MAP.get(field.get("type", "str"), str)

            if key not in raw:
                if required:
                    logger.warning(
                        "Required field '%s' missing in '%s' payload: %s",
                        key, msg_type, raw,
                    )
                    return None
                continue  # optional field absent — skip

            try:
                result[key] = cast(raw[key])
            except (ValueError, TypeError) as exc:
                logger.warning(
                    "Field '%s' cast to %s failed (%s). Using raw value.",
                    key, cast.__name__, exc,
                )
                result[key] = raw[key]

        # Pass through unknown fields (forward-compatible with new firmware)
        for key, value in raw.items():
            if key not in known_keys:
                logger.debug("Unknown field '%s' in '%s' — passed through", key, msg_type)
                result[key] = value

        return result
