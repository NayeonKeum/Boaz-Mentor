import socket
from _thread import *
import signal

ADDR = "127.0.0.1"
PORT = 9080
threadCount = 0
clients = []

server_sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
server_sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)

try:
    server_sock.bind((ADDR, PORT))
except socket.error as e:
    print(str(e))

server_sock.listen()


def handle_client(client_sock, addr):
    try:
        name = client_sock.recv(1024).decode()
        client_sock.send(str.encode("Welcome! "))

        print("Connected by: %s at %s" % (name, addr[0]))

        입장 = "%s님이 입장하셨습니다." % name

        for client in clients:
            if client != client_sock:
                client.send(입장.encode())

        while True:
            try:
                data = client_sock.recv(1024).decode()

                # 빈 문자열 수신
                if not data:
                    print("Disconnected by " + name)
                    break

                print("%s > %s" % (name, data))
                send_data = "%s > %s" % (name, data)

                for client in clients:
                    print(str(client))
                    if client != client_sock:
                        client.send(send_data.encode())

                # message = input("정")

            except ConnectionResetError as e:
                print("Disconnected by %s" % name)
                퇴장 = "- %s님이 나가셨습니다-" % name
                for client in clients:
                    if client != client_sock:
                        client.send(퇴장.encode())
                break
        if client_sock in clients:
            clients.remove(client_sock)
            print("%d person(people) Left the room... " % len(clients))
        client_sock.close()
    except ConnectionResetError as e:
        print("클라이언트가 강제로 나갔어요 ㅠㅠ")


def sigint_handler(signo, frame):
    server_sock.close()
    print("Server Closed...\n")
    exit()

signal.signal(signal.SIGINT, sigint_handler)

print("서버 시작")
try:
    while True:
        client_socket, addr = server_sock.accept()
        clients.append(client_socket)
        start_new_thread(handle_client, (client_socket, addr))
finally:
    server_sock.close()
