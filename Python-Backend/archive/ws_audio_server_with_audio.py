import asyncio
import websockets
import numpy as np

clients = set()

async def audio_server(websocket):
    print("🎮 Unity connected")
    clients.add(websocket)
    try:
        while True:
            await asyncio.sleep(1)
    except websockets.exceptions.ConnectionClosed:
        print("❌ Unity disconnected")
    finally:
        clients.discard(websocket)

async def start_server():
    async with websockets.serve(audio_server, "127.0.0.1", 8765):
        print("🔊 WS server running on ws://127.0.0.1:8765")
        await asyncio.Future()

async def send_test_audio():
    while True:
        await asyncio.sleep(2)

        if not clients:
            continue

        # 🔊 generate 440Hz beep
        t = np.linspace(0, 0.1, 2205)
        wave = 0.3 * np.sin(2 * np.pi * 440 * t)
        pcm = (wave * 32767).astype("int16").tobytes()

        for ws in list(clients):
            await ws.send(pcm)

        print("🎵 Sent test audio")

async def main():
    await asyncio.gather(
        start_server(),
        send_test_audio()
    )

asyncio.run(main())
