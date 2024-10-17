# _    _  _ _  _ _ ____ ____ _ ___
# |    |  |  \/  | | __ |__/ | |  \
# |___ |__| _/\_ | |__] |  \ | |__/
# =================================
# Luxigrid - Periodic Table Data Scraper
# Copyright (c) 2024 OverScore Media - MIT License
#
# MIT LICENSE:
# Permission is hereby granted, free of charge, to any person obtaining a copy of
# this software and associated documentation files (the "Software"), to deal in
# the Software without restriction, including without limitation the rights to
# use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
# the Software, and to permit persons to whom the Software is furnished to do so,
# subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in all
# copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
# FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
# COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
# IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
# CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

# This script parses data downloaded from Wikipedia, and converts it into a format parseable by the Luxigrid Periodic Table animation
# See lib/elements.hpp for an example of the output

from bs4 import BeautifulSoup

# This script requires downloading the HTML page at: https://en.wikipedia.org/wiki/List_of_chemical_elements
# (Right-click on page, go to "Save as" -> "Web Page, HTML Only")
# Save it to the same directory that you're running this script from
filename = 'List of chemical elements - Wikipedia.html'

with open(filename, 'r', encoding='utf-8') as file:
    soup = BeautifulSoup(file, 'html.parser')

# Find the table containing the list of elements
table = soup.find('table', {'class': 'wikitable'})

# Initialize a list to store the element data
elements = []

# Define a mapping for element blocks
block_mapping = {
    's-block': 's',
    'p-block': 'p',
    'd-block': 'd',
    'f-block': 'f'
}

# Extract the atomic weight as a numeric value
def extract_numeric_weight(weight_text):
    # Remove any non-numeric characters
    weight_text = ''.join([i for i in weight_text if i.isdigit() or i == '.'])
    return weight_text

# Iterate through each row in the table (skipping the header row)
for row in table.find_all('tr')[1:]:
    cols = row.find_all('td')
    if len(cols) > 7:
        try:
            number = int(cols[0].text.strip())
            symbol = cols[1].text.strip()
            name = cols[2].text.strip()

            block_text = cols[6].text.strip().lower()
            block = 'unknown'

            for key in block_mapping.keys():
                if key in block_text:
                    block = block_mapping[key]
                    break

            # Extract and clean the atomic weight
            weight_text = extract_numeric_weight(cols[7].text)
            atomic_weight = weight_text if weight_text else '0.0'

        except (ValueError, IndexError) as e:
            print(f"Skipping element due to data parsing error: {cols[2].text.strip()} with error {str(e)}")
            continue

        elements.append({
            "number": number,
            "symbol": symbol,
            "name": name,
            "atomicWeight": atomic_weight,
            "block": block
        })

elements_list = []

# Generate the C++ code to display the list of elements
for element in elements:
    elements_list.append(f'{{ "{element["symbol"]}", {element["number"]}, "{element["name"]}", "{element["atomicWeight"]}", \'{element["block"]}\' }}')

elements_array = "Element elements[] = {\n  " + ",\n  ".join(elements_list) + "\n};"

# Save to the ElementsArray.txt file in the current directory
with open('ElementsArray.txt', 'w') as file:
    file.write(elements_array)