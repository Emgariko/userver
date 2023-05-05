import time
import requests
import json
import string
import random
import pandas as pd
import datetime

url = 'http://51.250.66.78:8087/v1/chat?author={}'
interval = 0.05 # time in seconds

sent_messages = {"id": [], "client_sent_time": []}

def gen_random_message():
    def generate_random_text(min_length, max_length):
        length = random.randint(min_length, max_length)
        return ''.join(random.choices(string.ascii_letters + string.digits, k=length))
    
    min_length = 10  # Specify the minimum length of the random text
    max_length = 30  # Specify the maximum length of the random text
    return generate_random_text(min_length, max_length)

def add_stat(msg_id_response, client_sent_time):
    global sent_messages

    sent_messages['id'].append(msg_id_response)
    sent_messages['client_sent_time'].append(client_sent_time)

def send_request(name):
    msg_to_send = gen_random_message()
    payload = json.dumps({"message": msg_to_send})
    headers = {'Content-Type': 'application/json'}
    msg_id_response = requests.request("POST", url.format(name), headers=headers, data=payload)
    client_sent_time = datetime.datetime.now(datetime.timezone.utc)
    add_stat(msg_id_response.text, client_sent_time)
    print(f"Sent request >>>>>>: {msg_to_send}")


if __name__ == '__main__':
    try:
        print("Enter the writer's name:")
        name = input()
    
        while True:
            send_request(name)
            time.sleep(interval)
    
    except KeyboardInterrupt:
        df = pd.DataFrame(data=sent_messages)
        df.to_csv("sent_messages2.csv", index=None)
