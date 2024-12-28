import socket
import threading
from DofusDB import DofusDB
import json
from time import sleep

def handle_client(conn, address):
    print(f"Connection from: {address}")
    ddb = DofusDB()
    while True:
        data = conn.recv(1024).decode()  # Receive data stream

        if not data:
            sleep(1)
            continue

        res = json.loads(data)
        print(res)

        if "exit" in res and res["exit"] == 1:
            break

        ddb.set_x_position(res["x"])
        ddb.set_y_position(res["y"])
        ddb.set_direction(res["direction"])
        ddb.set_hint(res["hint"])

        print(f"From connected user: {data}")
        new_data = "/".join(ddb.get_hint_position())
        conn.send(new_data.encode())  # Send data to the client

    conn.close()  # Close the connection

def server_program():
    host = socket.gethostname()
    port = 5000  # Port number above 1024
    print(f"Server hostname: {host}")

    server_socket = socket.socket()  # Create socket instance
    server_socket.bind((host, port))  # Bind host address and port together
    server_socket.listen(1000)  # Configure the server to listen for connections

    try:
        while True:
            conn, address = server_socket.accept()  # Accept new connection
            client_thread = threading.Thread(target=handle_client, args=(conn, address))
            client_thread.start()

    except Exception as e:
        print(f"An error occurred: {e}")

    finally:
        server_socket.close()  # Ensure the server socket is closed properly

if __name__ == '__main__':
    server_program()
