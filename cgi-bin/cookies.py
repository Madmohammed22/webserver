#!/usr/bin/env python3

import os

cookie_header = os.getenv("HTTP_COOKIE", "")
cookies = {}
if cookie_header:
    for cookie in cookie_header.split(";"):
        if "=" in cookie:
            name, value = cookie.strip().split("=", 1)
            cookies[name] = value

# CSS styles
css = """
<style>
body {
    font-family: Arial, sans-serif;
    background: #f4f4f4;
    margin: 0;
    padding: 0;
}
.container {
    background: #fff;
    max-width: 600px;
    margin: 40px auto;
    padding: 30px 40px;
    border-radius: 8px;
    box-shadow: 0 2px 8px rgba(0,0,0,0.08);
}
h1 {
    color: #333;
    border-bottom: 2px solid #4CAF50;
    padding-bottom: 10px;
}
h2 {
    color: #4CAF50;
    margin-top: 30px;
}
ul {
    list-style: none;
    padding: 0;
}
li {
    background: #f9f9f9;
    margin: 8px 0;
    padding: 10px 15px;
    border-radius: 4px;
    border-left: 4px solid #4CAF50;
}
strong {
    color: #222;
}
</style>
"""

# Generate the response body
response_lines = []
response_lines.append("<html>")
response_lines.append("<head><title>Cookie Test</title>")
response_lines.append(css)
response_lines.append("</head>")
response_lines.append("<body>")
response_lines.append('<div class="container">')
response_lines.append("<h1>Cookie Test</h1>")

# Display incoming cookies
if cookies:
    response_lines.append("<h2>Incoming Cookies:</h2>")
    response_lines.append("<ul>")
    for name, value in cookies.items():
        response_lines.append(f"<li><strong>{name}:</strong> {value}</li>")
    response_lines.append("</ul>")
else:
    response_lines.append("<p>No incoming cookies.</p>")

# Confirm outgoing cookies
response_lines.append("<h2>Outgoing Cookies:</h2>")
response_lines.append("<p>Cookies have been set:</p>")
response_lines.append("<ul>")
if "username" not in cookies:
    response_lines.append("<li><strong>username:</strong> Mohammed</li>")
if "theme" not in cookies:
    response_lines.append("<li><strong>theme:</strong> dark</li>")
response_lines.append("</ul>")

response_lines.append("</div>")
response_lines.append("</body>")
response_lines.append("</html>")

response_body = "\n".join(response_lines)
response_bytes = response_body.encode("utf-8")
content_length = len(response_bytes)

# Print CGI headers
print("HTTP/1.1 200 OK")
if "username" not in cookies:
    print("Set-Cookie: username=Mohammed; Path=/; HttpOnly")
if "theme" not in cookies:
    print("Set-Cookie: theme=dark; Path=/; Max-Age=3600")
print("Content-Type: text/html")
print(f"Content-Length: {content_length}")
print()  # End of headers

# Print response body
print(response_body)
