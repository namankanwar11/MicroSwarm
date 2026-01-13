# Micro-Swarm Generated

import requests

def get_latest_silver_price():
    url = "https://api.example.com/silver_price"
    response = requests.get(url)
    
    if response.status_code == 200:
        data = response.json()
        return data['silver_price']
    else:
        print(f"Error: Received status code {response.status_code}")
        raise Exception("Failed to fetch Silver price data")

# Test the function
try:
    silver_price = get_latest_silver_price()
    print(f"The latest Silver price is: {silver_price}")
except Exception as e:
    print(e)
