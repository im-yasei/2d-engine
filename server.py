import socket

HOST = "0.0.0.0"
PORT = 8080

server_socket = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
server_socket.bind((HOST, PORT))
print(f"UDP сервер запущен на {HOST}:{PORT}")

clients = set()
while True:
    try:
        data, addr = server_socket.recvfrom(1024)
        if not data:
            continue

        message = data.decode().strip()
        print(f"[{addr}] -> {message}")

        clients.add(addr)

        for client in clients:
            if client != addr:
                server_socket.sendto(data, client)

    except KeyboardInterrupt:
        print("\nСервер остановлен.")
        break
