import asyncio
import websockets
import signal
import random
import string
import json
from datetime import datetime
import uuid



server_uri = "ws://51.250.66.78:8080"
interval = 0.01
sent_messages = {"id": [], "client_sent_time": []}

def gen_random_message():
    def generate_random_text(min_length, max_length):
        length = random.randint(min_length, max_length)
        return ''.join(random.choices(string.ascii_letters + string.digits, k=length))
    
    min_length = 10  # Specify the minimum length of the random text
    max_length = 30  # Specify the maximum length of the random text
    return generate_random_text(min_length, max_length)

def request_data():
    id = str(uuid.uuid4())
    msg_to_send = gen_random_message()

    data = {
        "id": id,
        "message": msg_to_send
    }

    return data

async def send_request(uri):
    global sent_messages

    async with websockets.connect(uri) as websocket:
        while True:
            data = request_data()
            client_sent_time = datetime.utcnow().isoformat()
            data["client_sent_time"] = client_sent_time
            json_data = json.dumps(data)
            # json_data = "Hello!"
            await websocket.send(json_data)

            print(f"Sent >>>>>>: {json_data}")

            # Add a delay between requests (optional)
            await asyncio.sleep(interval)

def handle_keyboard_interrupt():
    print("Interrupted, stopping client.")
    asyncio.get_event_loop().stop()

# Register the signal handler for KeyboardInterrupt
signal.signal(signal.SIGINT, lambda signal, frame: handle_keyboard_interrupt())

try:
    asyncio.get_event_loop().run_until_complete(send_request(server_uri))
except KeyboardInterrupt:
    # The signal handler will stop the event loop on KeyboardInterrupt
    pass
