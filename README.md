# NCP-Assignment-1: C-Based Web Proxy Server
This project is a simple implementation of a web proxy server that accepts client requests, parses them, forms a new request to the end server with this parsed information, and forwards the end server response back to the client.

This proxy handles requests iteratively (i.e. one at a time); if multiple requests are made simulataneously, one is immediately handled, while the others block until the proxy completes serving the request and waits for another connection.

The project can be run as follows:
1. Clone the repository: `git clone https://github.com/neilpat1995/NCP-Assignment-1.git`
2. Change directories to the project directory: `cd NCP-Assignment-1/proxylab-handout`
3. Build the project: `make`
4. Run the server: `./proxy <port>`
5. Configure your web browser to use your proxy; for Firefox, go to Preferences->Advanced->Network->Settings->Manual proxy configuration, and specify HTTP Proxy: 127.0.0.1 and Port: port (specified when running the proxy server)
6. Navigate to a website!

**NOTE: The proxy is capable of rendering text, but often fails to render all of the images.**
