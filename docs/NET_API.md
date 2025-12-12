# Net Module - Clean API Guide

## HTTP Client (Simple & Clean)

```wyn
import net

# GET request
const data: str = net.http_get("https://api.example.com/users")

# POST request
const response: str = net.http_post("https://api.example.com/users", "name=Alice")

# Other methods
net.http_put(url, body)
net.http_delete(url)
net.http_head(url)
```

## HTTP Server (Ergonomic)

```wyn
import net

fn main() {
    # Create server
    const server: int = net.server_create(8080)
    
    # Accept connection
    const client: int = net.server_accept(server)
    
    # Read and parse request
    const request: str = net.server_read_request(client)
    const method: str = net.parse_method(request)
    const path: str = net.parse_path(request)
    const body: str = net.parse_body(request)
    
    # Send response
    net.server_send_response(client, 200, "Hello!")
    # or
    net.server_send_json(client, 200, "{status:ok}")
    
    # Cleanup
    net.server_close_client(client)
    net.server_close(server)
}
```

## Simplified Server Pattern

```wyn
import net

fn handle_request(method: str, path: str, body: str) -> str {
    if path == "/api" {
        return "API response"
    }
    return "Not found"
}

fn main() {
    const server: int = net.server_create(8080)
    
    while true {
        const client: int = net.server_accept(server)
        const req: str = net.server_read_request(client)
        const path: str = net.parse_path(req)
        const method: str = net.parse_method(req)
        const body: str = net.parse_body(req)
        
        const response: str = handle_request(method, path, body)
        net.server_send_response(client, 200, response)
        net.server_close_client(client)
    }
}
```

## TCP (Clean)

```wyn
import net

# Connect
const sock: int = net.tcp_connect("localhost", 8080)

# Send/receive
net.tcp_send(sock, "Hello")
const data: str = net.tcp_recv(sock, 1024)

# Close
net.tcp_close(sock)
```

## UDP (Simple)

```wyn
import net

# Send datagram
net.udp_send("localhost", 8080, "Hello UDP")

# Receive datagram
const data: str = net.udp_recv(8080, 1024)
```

## Utilities

```wyn
import net

# DNS resolution
const ip: str = net.resolve_host("example.com")

# Get local IP
const my_ip: str = net.get_local_ip()
```

## Best Practices

### Simple REST API Server

```wyn
import net
import json

fn main() {
    const server: int = net.server_create(3000)
    print("API server on :3000")
    
    while true {
        const client: int = net.server_accept(server)
        const req: str = net.server_read_request(client)
        const path: str = net.parse_path(req)
        
        if path == "/health" {
            net.server_send_json(client, 200, "{status:healthy}")
        } else if path == "/users" {
            net.server_send_json(client, 200, "{users:[]}")
        } else {
            net.server_send_json(client, 404, "{error:not_found}")
        }
        
        net.server_close_client(client)
    }
}
```

### HTTP Client with Error Handling

```wyn
import net

fn fetch_data(url: str) -> str {
    const response: str = net.http_get(url)
    if response.len() == 0 {
        return "Error: empty response"
    }
    return response
}
```

## Function Reference

### HTTP Client
- `http_get(url)` - GET request
- `http_post(url, body)` - POST request
- `http_put(url, body)` - PUT request
- `http_delete(url)` - DELETE request
- `http_head(url)` - HEAD request
- `http_get_headers(url, headers)` - GET with custom headers

### HTTP Server
- `server_create(port)` - Create server
- `server_accept(server)` - Accept connection
- `server_read_request(client)` - Read request
- `parse_method(request)` - Get HTTP method
- `parse_path(request)` - Get URL path
- `parse_body(request)` - Get request body
- `server_send_response(client, status, body)` - Send text
- `server_send_json(client, status, json)` - Send JSON
- `server_close_client(client)` - Close connection
- `server_close(server)` - Close server

### TCP
- `tcp_connect(host, port)` - Connect
- `tcp_send(fd, data)` - Send data
- `tcp_recv(fd, size)` - Receive data
- `tcp_close(fd)` - Close socket

### UDP
- `udp_send(host, port, data)` - Send datagram
- `udp_recv(port, size)` - Receive datagram

### Utilities
- `resolve_host(hostname)` - DNS lookup
- `get_local_ip()` - Get local IP address

## Notes

- All functions use real BSD sockets
- HTTP client uses curl for simplicity
- HTTP server is pure sockets (no dependencies)
- Supports HTTP/1.1 protocol
- JSON responses have proper Content-Type headers
