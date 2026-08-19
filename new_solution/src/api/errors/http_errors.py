"""HTTP error helpers for the MVP REST API."""

from fastapi import HTTPException


def resource_not_found() -> HTTPException:
    return HTTPException(
        status_code=404,
        detail={
            "status": "error",
            "code": "RESOURCE_NOT_FOUND",
            "message": "Requested resource does not exist.",
        },
    )


def invalid_query(message: str) -> HTTPException:
    return HTTPException(
        status_code=400,
        detail={
            "status": "error",
            "code": "INVALID_QUERY",
            "message": message,
        },
    )
