import sys
import requests
from bs4 import BeautifulSoup

def get_midi_url(query):
    base_url = "https://bitmidi.com"
    search_url = f"{base_url}/search?q={query}"
    
    try:
        headers = {'User-Agent': 'Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36'}
        response = requests.get(search_url, headers=headers)
        response.raise_for_status()

        soup = BeautifulSoup(response.text, 'html.parser')

        # Save HTML if we fail
        def dump_html():
            with open("debug_scraper_dump.html", "w", encoding="utf-8") as f:
                f.write(response.text)

        # --- Flexible Selectors ---
        # Attempt A: Look for the specific class on the LINK
        link = soup.find('a', class_='search-result-item')
        
        # Attempt B: Look for the specific class on a DIV, then get the link inside it
        if not link:
            container = soup.find('div', class_='search-result-item')
            if container:
                link = container.find('a')

        # Attempt C: Look for ANY link that looks like a download page
        if not link:
            # Find all links inside the main results area (usually a div with id='main' or class='container')
            # This is a "fuzzy" search for any link that isn't a navigation link
            for a_tag in soup.find_all('a', href=True):
                href = a_tag['href']
                # Check if it looks like a song page (usually /some-song-name or /uploads/)
                # and ignore utility links (login, search, etc)
                if "/search" not in href and "/login" not in href and len(href) > 2:
                    # If the link text resembles our query, it's a strong candidate
                    if query.lower().split()[0] in a_tag.text.lower():
                        link = a_tag
                        break

        if link and link.get('href'):
            return base_url + link.get('href')
        else:
            dump_html() # Save the file so you can inspect it!
            return "NOT_FOUND"

    except Exception as e:
        return "ERROR"

if __name__ == "__main__":
    if len(sys.argv) > 1:
        query = sys.argv[1]
        url = get_midi_url(query)
        print(url)
    else:
        print("ERROR: No query provided")