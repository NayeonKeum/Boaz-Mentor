import socket
import signal

ADDR = input("Enter Server Address: ")
PORT = 9999	

client_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)

client_socket.connect((ADDR, PORT))
name = input("Enter Your Name : ")
client_socket.send(name.encode())

def sigint_handler(signo, frame) :
	client_socket.close()
	print("Client Closed....\n")
	exit()

signal.signal(signal.SIGINT, sigint_handler)


while True :

	message = input()

	if message == 'quit': 
		break
	
	client_socket.send(message.encode())
	
	data = client_socket.recv(1024)

	print(data.decode())

client_socket.close()
