import time
import requests
import json
import datetime
import pandas as pd

url = 'http://51.250.66.78:8087/v1/chat?from_id={}'
interval = 0.01 # time in seconds
from_id = 0

received_messages = {"id": [], "server_received_time":[], "client_received_time": []}

def upd_last_recieved_message_id(response):
    global from_id
    
    data = json.loads(response.text)
    messages = data['messages']
    
    if messages:
        from_id = max(messages, key=lambda message: message['id'])['id']

def add_received_messages(response, client_received_time):
    global received_messages

    data = json.loads(response.text)
    messages = data['messages']

    for msg in messages:
        received_messages['id'].append(msg['id'])
        received_messages['server_received_time'].append(msg['datetime'])
        received_messages['client_received_time'].append(client_received_time)

def handle_response(response, client_received_time):
    add_received_messages(response, client_received_time)
    print(f"<<<<<< Recieved response: {response.text}")
    upd_last_recieved_message_id(response)

def send_request():
    try:
        response = requests.get(url.format(from_id))
        client_reveiced_time = datetime.datetime.now(datetime.timezone.utc)
        response.raise_for_status()  # Raise an exception if the response contains an HTTP error
        handle_response(response, client_reveiced_time)
    except requests.exceptions.RequestException as e:
        print('Error:', e)

if __name__ == '__main__':
    try:
        while True:
            send_request()
            time.sleep(interval)
    
    except KeyboardInterrupt:
        df = pd.DataFrame(data=received_messages)
        df.to_csv("received_messages2.csv", index=None)
