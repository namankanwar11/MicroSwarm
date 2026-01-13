# Micro-Swarm Generated

# Import necessary libraries
import requests
from datetime import datetime

# Define the SilverPrice class
class SilverPrice:
    def __init__(self, date, open_price, high_price, low_price, close_price):
        self.date = date
        self.open_price = open_price
        self.high_price = high_price
        self.low_price = low_price
        self.close_price = close_price

    def get_date(self):
        return self.date

    def get_open_price(self):
        return self.open_price

    def get_high_price(self):
        return self.high_price

    def get_low_price(self):
        return self.low_price

    def get_close_price(self):
        return self.close_price

# Define the data retrieval function
def fetch_silver_prices():
    # Replace 'YOUR_API_KEY' with your actual CryptoCompare API key
    api_key = 'YOUR_API_KEY'
    url = f'https://min-api.cryptocompare.com/data/histominute?fsym=ZIL&tsym=USD&limit=100&api_key={api_key}'
    response = requests.get(url)
    if response.status_code == 200:
        data = response.json()
        silver_prices = []
        for item in data['Data']:
            silver_prices.append(SilverPrice(
                datetime.fromtimestamp(item['time']),
                item['open'],
                item['high'],
                item['low'],
                item['close']
            ))
        return silver_prices
    else:
        print(f"Failed to fetch silver prices: {response.status_code}")
        return []

# Define the data analysis function
def analyze_silver_prices(silver_prices):
    # Calculate average price
    average_price = sum(price.get_close_price() for price in silver_prices) / len(silver_prices)
    # Calculate highest price
    highest_price = max(price.get_close_price() for price in silver_prices)
    # Calculate lowest price
    lowest_price = min(price.get_close_price() for price in silver_prices)
    return average_price, highest_price, lowest_price

# Define the user interface function
def display_silver_prices(silver_prices):
    for price in silver_prices:
        print(f"Date: {price.get_date()}, Open: {price.get_open_price()}, High: {price.get_high_price()}, Low: {price.get_low_price()}, Close: {price.get_close_price()}")

# Main function
def main():
    silver_prices = fetch_silver_prices()
    if silver_prices:
        average_price, highest_price, lowest_price = analyze_silver_prices(silver_prices)
        print(f"Average Price: {average_price}")
        print(f"Highest Price: {highest_price}")
        print(f"Lowest Price: {lowest_price}")
        display_silver_prices(silver_prices)

if __name__ == "__main__":
    main()
