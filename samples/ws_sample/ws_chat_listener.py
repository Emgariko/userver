import asyncio
import websockets
import signal
import json
from datetime import datetime
import pandas as pd




server_uri = "ws://51.250.66.78:8080"
received_messages = {"id": [], "client_sent_time": [], "client_received_time": []}

async def send_request(uri):
    global received_messages

    async with websockets.connect(uri) as websocket:
        while True:
            print("Connected, receiving data")
            response = await websocket.recv()
            data = json.loads(response)
            client_received_time = datetime.utcnow().isoformat()
            client_sent_time = data['client_sent_time']
            id = data['id']
            message = data['message']
            received_messages['id'].append(id)
            received_messages['client_sent_time'].append(client_sent_time)
            received_messages['client_received_time'].append(client_received_time)
            print(f"Received: {response}")

def handle_keyboard_interrupt():
    print("Interrupted, stopping client.")
    df = pd.DataFrame(data=received_messages)
    df.to_csv("ws_messages3.csv", index=None)
    asyncio.get_event_loop().stop()

# Register the signal handler for KeyboardInterrupt
signal.signal(signal.SIGINT, lambda signal, frame: handle_keyboard_interrupt())

try:
    asyncio.get_event_loop().run_until_complete(send_request(server_uri))
except KeyboardInterrupt:
    # The signal handler will stop the event loop on KeyboardInterrupt
    pass
