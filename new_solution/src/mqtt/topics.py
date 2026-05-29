"""
MQTT topic helpers for the ws/<target>/<source>/<smallTopic> scheme.
"""

DOMAIN = "ws"


def build(target: str, source: str, small_topic: str) -> str:
    """Build a full topic string."""
    return f"{DOMAIN}/{target}/{source}/{small_topic}"


def parse(topic: str) -> dict | None:
    """
    Parse a full topic string into its components.
    Returns dict with keys: domain, target, source, small_topic
    Returns None if the topic does not match the expected structure.
    """
    parts = topic.split("/")
    if len(parts) < 4 or parts[0] != DOMAIN:
        return None
    return {
        "domain": parts[0],
        "target": parts[1],
        "source": parts[2],
        "small_topic": "/".join(parts[3:]),  # allow nested small topics
    }


def subscribe_filter(hub_id: str) -> str:
    """
    Subscription filter for the hub: receives all messages from any source
    addressed to the hub_id (e.g. 'rpi1').
    Pattern: ws/hub_id/+/#
    """
    return f"{DOMAIN}/{hub_id}/+/#"


def register_call_topic(hub_id: str) -> str:
    """Broadcast topic to request all sensors to register."""
    return build("all", hub_id, "registerCall")
