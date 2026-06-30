import asyncio
import websockets
import time
import threading

clients = set()
loop = asyncio.new_event_loop()
asyncio.set_event_loop(loop)

async def server(ws):
    clients.add(ws)
    print("Unity connected")
    try:
        while True:
            await asyncio.sleep(1)
    finally:
        clients.remove(ws)

async def start():
    async with websockets.serve(server, "0.0.0.0", 8765):
        await asyncio.Future()

def run():
    loop.run_until_complete(start())

threading.Thread(target=run, daemon=True).start()

# 🔴 SEND TEST PACKETS EVERY SECOND
while True:
    time.sleep(1)
    for ws in list(clients):
        asyncio.run_coroutine_threadsafe(
            ws.send(b"HELLO_FROM_PYTHON"),
            loop
        )
        print("Sent test packet")
