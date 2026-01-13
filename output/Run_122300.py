// Micro-Swarm Generated

import requests

def get_latest_bitcoin_price():
    url = "https://api.coindesk.com/v1/bpi/currentprice/BTC.json"
    response = requests.get(url)
    
    if response.status_code == 200:
        data = response.json()
        price = data['bpi']['USD']['rate']
        return price
    else:
        return None

# Example usage
latest_price = get_latest_bitcoin_price()
if latest_price is not None:
    print(f"The latest Bitcoin price is: {latest_price}")
else:
    print("Failed to retrieve the latest Bitcoin price.")
