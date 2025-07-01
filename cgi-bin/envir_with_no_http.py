#!/usr/bin/python3

import os
import cgi
import sys

html_content = []
html_content.append("<html>")
html_content.append("<head>")
html_content.append("<h2>Environment:</h2><br>")
html_content.append("</head>")
html_content.append("<body>")
for param in os.environ.keys():
    html_content.append("<b>%20s</b>: %s<br>" % (param, os.environ[param]))
html_content.append("</body>")
html_content.append("</html>")

full_content = "\n".join(html_content)
content_length = len(full_content.encode('utf-8'))

print("Content-Type: text/html")
print(f"Content-Length: {content_length}")
print("Connection: close")
print("\r\n", end="")

print(full_content)

sys.stdout.flush()
