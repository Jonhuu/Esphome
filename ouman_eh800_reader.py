#!/usr/bin/env python3
"""
Read-only Ouman EH-800 web API client.

The EH-800 exposes a small HTTP interface used by its own web UI:
  /login?uid=USERNAME;pwd=PASSWORD;
  /request?S_227_85;S_259_85;...

This first version intentionally does not implement /update writes.
"""

from __future__ import annotations

import argparse
import os
import re
import sys
import time
import urllib.error
import urllib.request
from dataclasses import dataclass
from typing import Iterable


REGISTER_RE = re.compile(r"(S_\d+_\d+)=([^;]*);")


DEFAULT_REGISTERS: dict[str, tuple[str, str]] = {
    "S_227_85": ("Ulkolämpötila", "°C"),
    "S_259_85": ("L1 menoveden lämpötila", "°C"),
    "S_275_85": ("L1 menoveden pyyntö", "°C"),
    "S_272_85": ("L1 venttiilin asento", "%"),
    "S_261_85": ("L1 huonelämpötila", "°C"),
    "S_135_85": ("Kotona/poissa", ""),
}


@dataclass(frozen=True)
class OumanConfig:
    host: str
    port: int
    username: str
    password: str
    timeout: float = 5.0

    @property
    def base_url(self) -> str:
        return f"http://{self.host}:{self.port}"


class OumanEH800:
    def __init__(self, config: OumanConfig) -> None:
        self.config = config

    def _get(self, path: str) -> str:
        url = f"{self.config.base_url}{path}"
        request = urllib.request.Request(
            url,
            method="GET",
            headers={
                "User-Agent": "ouman-eh800-reader/0.1",
                "Connection": "close",
            },
        )

        try:
            with urllib.request.urlopen(request, timeout=self.config.timeout) as response:
                return response.read().decode("latin-1", errors="replace")
        except urllib.error.HTTPError as exc:
            raise RuntimeError(f"EH-800 HTTP error {exc.code} for {path}") from exc
        except urllib.error.URLError as exc:
            raise RuntimeError(f"EH-800 connection failed: {exc.reason}") from exc

    def login(self) -> str:
        # EH-800 expects semicolon-separated parameters exactly like its web UI.
        path = f"/login?uid={self.config.username};pwd={self.config.password};"
        return self._get(path)

    def read_registers(self, registers: Iterable[str]) -> tuple[dict[str, str], str]:
        register_list = list(registers)
        if not register_list:
            return {}, ""

        query = ";".join(register_list)
        raw = self._get(f"/request?{query}")

        values = {match.group(1): match.group(2) for match in REGISTER_RE.finditer(raw)}
        return values, raw


def build_config(args: argparse.Namespace) -> OumanConfig:
    host = args.host or os.getenv("OUMAN_HOST")
    username = args.username or os.getenv("OUMAN_USERNAME")
    password = args.password or os.getenv("OUMAN_PASSWORD")
    port = args.port or int(os.getenv("OUMAN_PORT", "80"))

    missing = [
        name
        for name, value in (
            ("host / OUMAN_HOST", host),
            ("username / OUMAN_USERNAME", username),
            ("password / OUMAN_PASSWORD", password),
        )
        if not value
    ]
    if missing:
        raise ValueError("Missing configuration: " + ", ".join(missing))

    return OumanConfig(
        host=str(host),
        port=int(port),
        username=str(username),
        password=str(password),
        timeout=args.timeout,
    )


def print_values(values: dict[str, str], requested: Iterable[str]) -> None:
    for register in requested:
        name, unit = DEFAULT_REGISTERS.get(register, (register, ""))
        if register not in values:
            print(f"{register:10} {name}: <ei vastausta>")
            continue

        value = values[register]
        suffix = f" {unit}" if unit else ""
        print(f"{register:10} {name}: {value}{suffix}")


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Read Ouman EH-800 values over its web UI HTTP API.")
    parser.add_argument("--host", help="EH-800 IP address or hostname (or OUMAN_HOST)")
    parser.add_argument("--port", type=int, help="EH-800 HTTP port (default: 80 / OUMAN_PORT)")
    parser.add_argument("--username", help="EH-800 web username (or OUMAN_USERNAME)")
    parser.add_argument("--password", help="EH-800 web password (or OUMAN_PASSWORD)")
    parser.add_argument("--timeout", type=float, default=5.0, help="HTTP timeout in seconds")
    parser.add_argument(
        "--register",
        action="append",
        dest="registers",
        help="Register to read, e.g. S_275_85. Can be given multiple times.",
    )
    parser.add_argument("--raw", action="store_true", help="Also print the raw EH-800 response")
    parser.add_argument(
        "--interval",
        type=float,
        default=0.0,
        help="Repeat reads every N seconds; 0 = read once",
    )
    return parser.parse_args()


def main() -> int:
    args = parse_args()

    try:
        config = build_config(args)
    except ValueError as exc:
        print(f"Configuration error: {exc}", file=sys.stderr)
        return 2

    registers = args.registers or list(DEFAULT_REGISTERS)
    client = OumanEH800(config)

    while True:
        try:
            client.login()
            values, raw = client.read_registers(registers)
        except RuntimeError as exc:
            print(f"Error: {exc}", file=sys.stderr)
            return 1

        print_values(values, registers)

        if args.raw:
            print("\nRAW:")
            print(raw)

        if args.interval <= 0:
            break

        print()
        time.sleep(args.interval)

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
