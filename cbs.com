HTTP/1.1 301 Moved Permanently
Server: Apache
Location: http://www.cbs.com/
Content-Type: text/html; charset=iso-8859-1
X-Real-Server: ws58
Content-Length: 227
Accept-Ranges: bytes
Date: Fri, 25 Mar 2016 02:13:56 GMT
X-Varnish: 1522569784
Age: 0
Via: 1.1 varnish
Connection: keep-alive
X-Cache: MISS
X-Hit-Count: 0

<!DOCTYPE HTML PUBLIC "-//IETF//DTD HTML 2.0//EN">
<html><head>
<title>301 Moved Permanently</title>
</head><body>
<h1>Moved Permanently</h1>
<p>The document has moved <a href="http://www.cbs.com/">here</a>.</p>
</body></html>

