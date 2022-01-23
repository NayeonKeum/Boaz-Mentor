import socket
from _thread import *
import signal
import multi_client.py

MAX_CLIENT 2
MAX_BUF 1024

ADDR = 
PORT = 9999

clients = [MultiClient()] * MAX_CLIENT

server_sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
server_sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
server_sock.bind((ADDR, PORT))
server_sock.listen()

def handle_client(client_sock, addr) :

	name = client_sock.recv(1024).decode()
	
	print("Connected by: %s at %s" %(name, addr[0]))

	while True :
		try :
			data = client_sock.recv(1024).decode()
			
			if not data :
				print("Disconnected by " + name)
				break

			print("%s > %s" %(name, data))
			send_data = "%s > %s" %(name, data)
			client_sock.send(send_data.encode())
		except ConnectionResetError as e :
			print("Disconnected by %s" % name)
			break
	client_sock.close()

def sigint_handler(signo, frame) :
	server_sock.close()
	print("Server Closed...\n")
	exit()

signal.signal(signal.SIGINT, sigint_handler)

print("Server Started...")

while True :
	
	client_socket, addr = server_sock.accept()
	start_new_thread(handle_client, (client_socket, addr))

server_sock.close()
