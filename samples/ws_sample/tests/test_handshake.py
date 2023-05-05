
import socket
import asyncio

REQUEST = [
    "GET /chat HTTP/1.1 ",
    "Host: server.example.com ",
    "Upgrade: websocket ",
    "Connection: Upgrade" ,
    "Sec-WebSocket-Key: dGhlIHNhbXBsZSBub25jZQ== ",
    "Origin: http://example.com ",
    "Sec-WebSocket-Protocol: chat, superchat ",
    "Sec-WebSocket-Version: 13 "
]

EXPECTED_RESPONSE = b"""
HTTP/1.1 101 Switching Protocols
Upgrade: websocket
Connection: Upgrade
Sec-WebSocket-Accept: s3pPLMBiTxaQ9kYGzzhZRbK+xOo=
"""

# async def send_all_data(s, loop):
    # for data in REQUEST:
    #     await loop.sock_sendall(s, data)
    # res = await loop.sock_sendall(s, bytes("\n".join(REQUEST), 'utf-8'))
    # breakpoint()

# async def recv_all_data(s, loop):
#     # breakpoint()
#     answer = b''
#     while len(answer) < len(EXPECTED_RESPONSE):
#         answer += await loop.sock_recv(s, len(EXPECTED_RESPONSE) - len(answer))
#         # breakpoint()
#
#     print(answer)
#
#     assert answer == EXPECTED_RESPONSE

async def test_handshake(service_client, loop):

    #
    #
    # # send_task = asyncio.create_task(send_all_data(sock, loop))
    # # await send_task
    # has_been_sent = await loop.sock_sendall(sock, bytes("\n".join(REQUEST), 'utf-8'))
    #
    # resp = await loop.sock_recv(sock, 1)
    # breakpoint()
    # await recv_all_data(sock, loop)



    # sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    # sock.connect(('localhost', 8080))
    #
    # await loop.sock_sendall(sock, b'hi')
    # hello = await loop.sock_recv(sock, 5)
    # breakpoint()
    # # assert hello == b'hello'

    sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    await loop.sock_connect(sock, ('localhost', 8080))
    # breakpoint()
    has_been_sent = await loop.sock_sendall(sock, bytes("\r\n".join(REQUEST) + "\r\n", 'utf-8'))
    breakpoint()
    # has_been_sent = await loop.sock_sendall(sock, b"hello")
    # breakpoint()
    # resp = sock.recv(4096)
    hello = await loop.sock_recv(sock, 5)
    # breakpoint()
    assert hello == [1, 2, 3, 4, 5]
    # assert resp == EXPECTED_RESPONSE
    # resp = await loop.sock_recv(sock, 1)xw
    # breakpoint()
