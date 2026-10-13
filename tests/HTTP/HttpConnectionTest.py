#!/usr/bin/env python3

from concurrent.futures import ThreadPoolExecutor
from pathlib import Path
import socket
import subprocess
import sys
import time


BODY = b"reply\0data"


def response(mode):
  if mode == "large-body":
    body = b"x" * (9 * 1024 * 1024)
    return b"HTTP/1.0 200 OK\r\nContent-Length: %d\r\n\r\n" % len(body) + body
  if mode == "large-header":
    return (b"HTTP/1.0 200 OK\r\nX-Large: " + b"x" * 9000 +
      b"\r\nContent-Length: 10\r\n\r\n" + BODY)
  if mode in ("empty", "wrapper"):
    return b"HTTP/1.1 204 No Content\r\n\r\n"
  if mode in ("status", "wrapper-status"):
    return b"HTTP/1.0 404 Not Found\r\nContent-Length: 0\r\n\r\n"
  if mode == "invalid":
    return b"HTTP/1.0 200 OK\r\nBad Header\r\n\r\n"
  if mode == "chunked":
    return (b"HTTP/1.1 200 OK\r\nTransfer-Encoding: chunked\r\n\r\n"
      b"5\r\nreply\r\n5\r\n\0data\r\n0\r\nX-Trailer: yes\r\n\r\n")
  if mode == "eof":
    return b"HTTP/1.0 200 OK\r\n\r\n" + BODY
  if mode == "truncated":
    return b"HTTP/1.0 200 OK\r\nContent-Length: 100\r\n\r\n" + BODY
  if mode == "header-timeout":
    return b"HTTP/1.0 200 OK\r\n"
  if mode == "body-timeout":
    return b"HTTP/1.0 200 OK\r\nContent-Length: 10\r\n\r\nr"
  status = b"302 Found" if mode == "redirect" else b"200 OK"
  return b"HTTP/1.0 " + status + b"\r\nContent-Length: 10\r\n\r\n" + BODY


def serve(listener, mode):
  requests = []
  for _ in range(2 if mode == "reconnect" else 1):
    connection, _ = listener.accept()
    with connection:
      connection.settimeout(3)
      if mode == "send-timeout":
        time.sleep(0.3)
        return requests
      data = b""
      while b"\r\n\r\n" not in data:
        part = connection.recv(65536)
        assert part, "client closed before request headers"
        data += part
      headers, body = data.split(b"\r\n\r\n", 1)
      fields = [line.split(b":", 1) for line in headers.split(b"\r\n")[1:]]
      length = int(next(value for name, value in fields if name.lower() == b"content-length"))
      while len(body) < length:
        part = connection.recv(65536)
        assert part, "client closed before request body"
        body += part
      requests.append((headers, body, len(headers) + 4 + len(body)))
      if mode == "slow-body":
        connection.sendall(response(mode)[:-len(BODY)])
        for byte in BODY:
          time.sleep(0.03)
          connection.sendall(bytes([byte]))
      elif mode not in ("no-response", "recv-timeout", "wrapper-timeout"):
        connection.sendall(response(mode))
      if mode.endswith("timeout") or mode == "no-response":
        time.sleep(0.3)
  return requests


def test(binary):
  modes = ("get", "post", "form", "proxy", "host", "empty", "eof", "chunked", "redirect",
    "no-response", "reconnect", "status", "invalid", "truncated", "recv-timeout",
    "header-timeout", "body-timeout", "send-timeout", "wrapper", "wrapper-status",
    "wrapper-timeout", "connect-timeout", "refused", "slow-body", "large-body", "large-header")
  for mode in modes:
    with socket.socket() as listener, ThreadPoolExecutor(max_workers=1) as executor:
      listener.bind(("127.0.0.1", 0))
      listener.settimeout(3)
      endpoint = "127.0.0.1:{}".format(listener.getsockname()[1])
      if mode != "refused":
        listener.listen(1)
      future = None if mode in ("connect-timeout", "refused") else executor.submit(
        serve, listener, mode)
      url = "http://{}/path?original=1".format(endpoint)
      command = [str(binary), mode, url]
      if mode == "proxy":
        command = [str(binary), mode, "http://example.invalid/path?original=1", endpoint]
      result = subprocess.run(command, stdout=subprocess.PIPE, stderr=subprocess.PIPE,
        universal_newlines=True, timeout=5)
      assert result.returncode == 0, (mode, result.stdout, result.stderr)
      requests = future.result(timeout=4) if future else []
      for index, (headers, body, size) in enumerate(requests):
        assert headers.split(b"\r\n")[0].endswith(b" HTTP/1.0"), headers
        if not mode.startswith("wrapper"):
          assert headers.count(b"X-Repeat:") == 2, headers
        if mode in ("get", "post", "proxy"):
          assert b"?original=1&x+y=a%2Bb HTTP/1.0" in headers, headers
        if mode == "proxy":
          assert headers.startswith(b"GET http://example.invalid/"), headers
        if mode in ("post", "no-response"):
          assert body == b"a\0b", body
        if mode == "form":
          assert headers.startswith(b"POST /path HTTP/1.0"), headers
          assert body == b"original=1&x+y=a%2Bb", body
        if mode == "host":
          assert b"host: override.test" in headers.lower(), headers
        line = result.stdout.splitlines()[index]
        if line[0].isdigit():
          sent, received = map(int, line.split())
          assert sent == size, (mode, sent, size)
          assert received == (0 if mode == "no-response" else len(response(mode))), (
            mode, received, len(response(mode)))
      print("PASS:", mode)


if __name__ == "__main__":
  test(Path(sys.argv[1]).resolve())
