import asyncio
import websockets

clients = set()

async def audio_server(websocket):
    print("🎮 Unity connected")
    clients.add(websocket)
    try:
        while True:
            await asyncio.sleep(1)  # keep alive
    except websockets.exceptions.ConnectionClosed:
        print("❌ Unity disconnected")
    finally:
        clients.remove(websocket)

async def main():
    async with websockets.serve(audio_server, "127.0.0.1", 8765):
        print("🔊 WS server running on ws://127.0.0.1:8765")
        await asyncio.Future()  # run forever

asyncio.run(main())
