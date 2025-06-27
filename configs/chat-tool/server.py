# -*- coding:utf-8 -*-
import socket
import threading
import os
import time
import hashlib
import struct
import sys

def clear_screen():
    os.system('cls' if os.name == 'nt' else 'clear')

def print_banner():
    clear_screen()
    print("=" * 60)
    print("Chat Server (Supports Text/File Transfer)")
    print("=" * 60)

def get_file_checksum(file_path):
    """Calculate MD5 checksum of a file"""
    hash_md5 = hashlib.md5()
    with open(file_path, "rb") as f:
        for chunk in iter(lambda: f.read(4096), b""):
            hash_md5.update(chunk)
    return hash_md5.hexdigest()

def recv_all(sock, length):
    """Receive specified length of data"""
    data = b''
    while len(data) < length:
        packet = sock.recv(length - len(data))
        if not packet:
            return None
        data += packet
    return data

def get_multiline_input(prompt):
    """Get multi-line input, end with //end"""
    print(prompt)
    print("Enter multi-line text (type '//end' to finish):")
    lines = []
    while True:
        line = input()
        if line.strip() == '//end':
            break
        lines.append(line)
    return '\n'.join(lines)

def handle_client(client_socket, client_address):
    def receive_thread():
        while True:
            try:
                # Read message type (1 byte)
                msg_type = recv_all(client_socket, 1)
                if not msg_type:
                    break
                    
                if msg_type == b'T':  # Text message
                    # Read text length (4 bytes, network byte order)
                    text_len_data = recv_all(client_socket, 4)
                    if not text_len_data:
                        break
                    text_len = struct.unpack('!I', text_len_data)[0]
                    
                    # Receive text content
                    message = recv_all(client_socket, text_len).decode('utf-8')
                    print("\n" + "-" * 40)
                    print(f"Phone >\n{message}")
                    print("-" * 40)
                    print("Server > ", end="", flush=True)
                    
                elif msg_type == b'F':  # File transfer
                    # Read filename length (1 byte)
                    name_len_data = recv_all(client_socket, 1)
                    if not name_len_data:
                        break
                    name_len = struct.unpack('B', name_len_data)[0]
                    
                    # Read filename
                    file_name = recv_all(client_socket, name_len).decode('utf-8')
                    
                    # Read file size (8 bytes, network byte order)
                    file_size_data = recv_all(client_socket, 8)
                    if not file_size_data:
                        break
                    file_size = struct.unpack('!Q', file_size_data)[0]
                    
                    # Read MD5 checksum (32 bytes)
                    file_md5 = recv_all(client_socket, 32).decode('utf-8')
                    
                    print("\n" + "=" * 40)
                    print(f"Receiving file: {file_name} ({file_size/1024:.1f}KB)")
                    print(f"MD5 checksum: {file_md5}")
                    print("=" * 40)
                    
                    # Create receive directory
                    if not os.path.exists("received_files"):
                        os.makedirs("received_files")
                    
                    file_path = os.path.join("received_files", file_name)
                    received = 0
                    start_time = time.time()
                    
                    with open(file_path, "wb") as f:
                        while received < file_size:
                            chunk_size = min(4096, file_size - received)
                            chunk = recv_all(client_socket, chunk_size)
                            if not chunk:
                                break
                            f.write(chunk)
                            received += len(chunk)
                    
                    # Verify file integrity
                    if received == file_size:
                        actual_md5 = get_file_checksum(file_path)
                        if actual_md5 == file_md5:
                            print(f"File received successfully! Saved to: {file_path}")
                            print(f"Transfer time: {time.time() - start_time:.2f} seconds")
                        else:
                            print("File verification failed! Transfer may be corrupted")
                    else:
                        print(f"File incomplete! ({received}/{file_size} bytes received)")
                    
                    print("Server > ", end="", flush=True)
                    
            except Exception as e:
                print(f"\nConnection error: {e}")
                break
    
    print(f"\nPhone connected: {client_address[0]}:{client_address[1]}")
    
    # Start message receiving thread
    recv_thread = threading.Thread(target=receive_thread)
    recv_thread.daemon = True
    recv_thread.start()
    
    return client_socket

def send_text(sock, text):
    """Send text message"""
    encoded_text = text.encode('utf-8')
    # Message type: T (text)
    sock.send(b'T')
    # Text length (4 bytes, network byte order)
    sock.send(struct.pack('!I', len(encoded_text)))
    # Text content
    sock.send(encoded_text)

def send_file(sock, file_path):
    """Send file"""
    if not os.path.exists(file_path):
        print(f"File not found: {file_path}")
        return False
        
    file_name = os.path.basename(file_path)
    file_size = os.path.getsize(file_path)
    file_md5 = get_file_checksum(file_path)
    
    # Message type: F (file)
    sock.send(b'F')
    
    # Send filename (length + content)
    encoded_name = file_name.encode('utf-8')
    sock.send(struct.pack('B', len(encoded_name)))  # Filename length (1 byte)
    sock.send(encoded_name)                        # Filename content
    
    # File size (8 bytes, network byte order)
    sock.send(struct.pack('!Q', file_size))
    
    # MD5 checksum (32 bytes)
    sock.send(file_md5.encode('utf-8'))
    
    print(f"Sending file: {file_name} ({file_size/1024:.1f}KB)")
    start_time = time.time()
    
    # Send file content
    with open(file_path, "rb") as f:
        while True:
            chunk = f.read(4096)
            if not chunk:
                break
            sock.send(chunk)
    
    print(f"File sent successfully! Time: {time.time() - start_time:.2f} seconds")
    return True
    
def get_local_ip():
    """Get local IP using socket connections to self"""
    try:
        # Create a temporary UDP socket
        with socket.socket(socket.AF_INET, socket.SOCK_DGRAM) as s:
            try:
                # Connect to a local address (doesn't send any packets)
                s.connect(("10.255.255.255", 1))
                ip = s.getsockname()[0]
                if not ip.startswith('127.'):
                    return ip
            except:
                pass
            
            # Try connecting to a private IP range
            s.connect(("192.168.1.1", 1))
            ip = s.getsockname()[0]
            if not ip.startswith('127.'):
                return ip
    except:
        pass
    
    # Final fallback
    return socket.gethostbyname(socket.gethostname())

def start_server(host='0.0.0.0', port=12345):
    server = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    server.bind((host, port))
    server.listen(1)
    
    print_banner()
    print(f"Server started, listening on {host}:{port}")
    print(f"Local IP: {get_local_ip()}")
    print("-" * 60)
    print("Waiting for phone connection... (Press Ctrl+C to exit)")
    
    try:
        client, addr = server.accept()
        client_socket = handle_client(client, addr)
    except KeyboardInterrupt:
        print("\nServer shutdown by user.")
        server.close()
        return

    # ADDED: Clear connection status display
    clear_screen()
    print("=" * 60)
    print(f"CONNECTED TO PHONE: {addr[0]}:{addr[1]}")
    print("=" * 60)
    print("Command options:")
    print("  /text       - Send multi-line text")
    print("  /sendfile   - Send a file")
    print("  /exit       - Exit the program")
    print("-" * 60)
    print("Waiting for input...")
    print("=" * 60)
    
    try:
        while True:
            try:
                print("Server > ", end="", flush=True)
                command = sys.stdin.readline().strip()
                
                if not command:
                    continue
                    
                if command.lower() == 'exit':
                    break
                    
                if command.startswith('/sendfile '):
                    file_path = command.split(' ', 1)[1]
                    send_file(client_socket, file_path)
                    
                elif command == '/text':
                    text = get_multiline_input("Enter text content:")
                    if text:
                        send_text(client_socket, text)
                else:
                    send_text(client_socket, command)
                    
            except KeyboardInterrupt:
                print("\nUse 'exit' to quit properly")
            except Exception as e:
                print(f"\nError processing command: {e}")
                
    except KeyboardInterrupt:
        print("\nServer interrupted by user")
    finally:
        try:
            client_socket.close()
        except:
            pass
        server.close()
        print("Connection closed")

if __name__ == "__main__":
    try:
        start_server()
    except KeyboardInterrupt:
        print("\nServer terminated by user")
    except Exception as e:
        print(f"Unexpected error: {e}")
