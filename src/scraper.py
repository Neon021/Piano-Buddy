import sys
import requests
from bs4 import BeautifulSoup

def get_midi_url(query):
    # 1. Search BitMidi
    base_url = "https://bitmidi.com"
    search_url = f"{base_url}/search?q={query}"
    
    try:
        # Fake a user agent so we look like a browser
        headers = {'User-Agent': 'Mozilla/5.0'}
        response = requests.get(search_url, headers=headers)
        response.raise_for_status()

        # 2. Parse HTML
        soup = BeautifulSoup(response.text, 'html.parser')

        # 3. Find the first result link
        # Note: This selector is based on BitMidi's current structure. 
        # It looks for an 'a' tag with class 'search-result-item'
        result_link = soup.find('a', class_='search-result-item')

        if result_link and result_link.get('href'):
            return base_url + result_link.get('href')
        else:
            return "NOT_FOUND"

    except Exception as e:
        return "ERROR"

if __name__ == "__main__":
    if len(sys.argv) > 1:
        query = sys.argv[1]
        url = get_midi_url(query)
        print(url) # Print to stdout so C can read it
    else:
        print("ERROR: No query provided")