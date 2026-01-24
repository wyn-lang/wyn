// Wyn runtime bindings for networking
// Exposes net.c functions to Wyn code
#include "net.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Simple wrapper functions that Wyn codegen can call

// Net::connect(host: string, port: int) -> int
int wyn_runtime_net_connect(const char* host, int port) {
    char addr[256];
    
    // Resolve hostname if needed
    WynNetError error;
    char** addresses = wyn_resolve_hostname(host, &error);
    const char* ip = host;
    
    if (addresses && addresses[0]) {
        ip = addresses[0];
    }
    
    snprintf(addr, sizeof(addr), "%s:%d", ip, port);
    
    WynTcpStream* stream = wyn_tcp_stream_connect(addr, &error);
    
    if (addresses) {
        wyn_free_hostname_list(addresses);
    }
    
    if (!stream) {
        return -1;  // Error
    }
    
    // Store stream pointer as integer handle
    // In production, use a handle table
    return (int)(intptr_t)stream;
}

// Net::listen(port: int) -> int
int wyn_runtime_net_listen(int port) {
    char addr[64];
    snprintf(addr, sizeof(addr), "0.0.0.0:%d", port);
    
    WynNetError error;
    WynTcpListener* listener = wyn_tcp_listener_bind(addr, &error);
    
    if (!listener) {
        return -1;
    }
    
    return (int)(intptr_t)listener;
}

// Net::accept(listener: int) -> int
int wyn_runtime_net_accept(int listener_handle) {
    WynTcpListener* listener = (WynTcpListener*)(intptr_t)listener_handle;
    
    WynNetError error;
    WynTcpStream* stream = wyn_tcp_listener_accept(listener, &error);
    
    if (!stream) {
        return -1;
    }
    
    return (int)(intptr_t)stream;
}

// Net::send(socket: int, data: string) -> int
int wyn_runtime_net_send(int socket_handle, const char* data) {
    WynTcpStream* stream = (WynTcpStream*)(intptr_t)socket_handle;
    
    size_t len = strlen(data);
    WynNetError error = wyn_tcp_stream_send_all(stream, data, len);
    
    if (error != WYN_NET_SUCCESS) {
        return -1;
    }
    
    return (int)len;
}

// Net::recv(socket: int, max_len: int) -> string
char* wyn_runtime_net_recv(int socket_handle, int max_len) {
    WynTcpStream* stream = (WynTcpStream*)(intptr_t)socket_handle;
    
    char* buffer = malloc(max_len + 1);
    if (!buffer) return NULL;
    
    size_t received;
    WynNetError error = wyn_tcp_stream_recv(stream, buffer, max_len, &received);
    
    if (error != WYN_NET_SUCCESS) {
        free(buffer);
        return NULL;
    }
    
    buffer[received] = '\0';
    return buffer;
}

// Net::recv_line(socket: int) -> string
char* wyn_runtime_net_recv_line(int socket_handle) {
    WynTcpStream* stream = (WynTcpStream*)(intptr_t)socket_handle;
    
    WynNetError error;
    return wyn_tcp_stream_recv_line(stream, &error);
}

// Net::close(socket: int) -> int
int wyn_runtime_net_close(int socket_handle) {
    // Could be either stream or listener
    // For simplicity, try both
    WynTcpStream* stream = (WynTcpStream*)(intptr_t)socket_handle;
    wyn_tcp_stream_free(stream);
    return 0;
}

// UDP functions

// Net::udp_bind(port: int) -> int
int wyn_runtime_net_udp_bind(int port) {
    char addr[64];
    snprintf(addr, sizeof(addr), "0.0.0.0:%d", port);
    
    WynNetError error;
    WynUdpSocket* socket = wyn_udp_socket_bind(addr, &error);
    
    if (!socket) {
        return -1;
    }
    
    return (int)(intptr_t)socket;
}

// Net::udp_send_to(socket: int, data: string, host: string, port: int) -> int
int wyn_runtime_net_udp_send_to(int socket_handle, const char* data, const char* host, int port) {
    WynUdpSocket* socket = (WynUdpSocket*)(intptr_t)socket_handle;
    
    char addr[256];
    snprintf(addr, sizeof(addr), "%s:%d", host, port);
    
    size_t sent;
    WynNetError error = wyn_udp_socket_send_to(socket, data, strlen(data), addr, &sent);
    
    if (error != WYN_NET_SUCCESS) {
        return -1;
    }
    
    return (int)sent;
}

// Net::udp_recv_from(socket: int, max_len: int) -> string
// Returns "data|host:port" format
char* wyn_runtime_net_udp_recv_from(int socket_handle, int max_len) {
    WynUdpSocket* socket = (WynUdpSocket*)(intptr_t)socket_handle;
    
    char* buffer = malloc(max_len + 1);
    if (!buffer) return NULL;
    
    char* from_addr = NULL;
    size_t received;
    WynNetError error = wyn_udp_socket_recv_from(socket, buffer, max_len, &from_addr, &received);
    
    if (error != WYN_NET_SUCCESS) {
        free(buffer);
        return NULL;
    }
    
    buffer[received] = '\0';
    
    // Combine data and address
    char* result = malloc(max_len + 256);
    if (result && from_addr) {
        snprintf(result, max_len + 256, "%s|%s", buffer, from_addr);
    }
    
    free(buffer);
    free(from_addr);
    return result;
}
