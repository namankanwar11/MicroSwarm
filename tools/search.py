# tools/search.py
import sys
import urllib.request
import re
import urllib.parse

def search_duckduckgo(query):
    # Use DuckDuckGo Lite (HTML version, easier to scrape)
    encoded_query = urllib.parse.quote(query)
    url = f"https://lite.duckduckgo.com/lite/?q={encoded_query}"
    
    headers = {'User-Agent': 'Mozilla/5.0'}
    req = urllib.request.Request(url, headers=headers)
    
    try:
        with urllib.request.urlopen(req) as response:
            html = response.read().decode('utf-8')
            
        # Simple extraction of result snippets (filtering out navigation)
        # We look for table rows which usually contain the results in Lite version
        results = []
        clean_text = re.sub(r'<[^>]+>', ' ', html) # Strip HTML tags
        
        # Normalize whitespace
        words = clean_text.split()
        text_blob = " ".join(words)
        
        # Find the relevant section (heuristic)
        start_marker = "Web Images"
        end_marker = "Next >"
        
        start_idx = text_blob.find(start_marker)
        if start_idx != -1:
            content = text_blob[start_idx + len(start_marker):]
            end_idx = content.find(end_marker)
            if end_idx != -1:
                content = content[:end_idx]
            
            # Clean up artifacts
            print(f"--- SEARCH REPORT: {query} ---")
            print(content[:1500] + "...") # Limit to 1500 chars to save tokens
            print("\n[Source: DuckDuckGo Lite]")
            
    except Exception as e:
        print(f"Search Error: {e}")

if __name__ == "__main__":
    if len(sys.argv) > 1:
        query = " ".join(sys.argv[1:])
        search_duckduckgo(query)
    else:
        print("Usage: python search.py <query>")