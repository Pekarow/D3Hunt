import socket
from multiprocessing import Process
from DofusDB import DofusDB
import json
from time import sleep

def handle_client(conn, address):
    print(f"Connection from: {address}")
    ddb = DofusDB()
    while True:
        conn.settimeout(600) # 10 minutes of inactivity (not quesuest made) will close the socket
        try:
            data = conn.recv(1024).decode()  # Receive data stream
        except socket.timeout as e:
            break

        if not data:
            break

        res = json.loads(data)
        print(res)

        if "exit" in res and res["exit"] == 1:
            break
        tries = 0
        while tries < 10:
            try:
                ddb.run(**res)
                break
            except:
                tries += 1
                continue

        print(f"From connected user: {data}")
        new_data = "/".join(ddb.get_hint_position())
        conn.send(new_data.encode())  # Send data to the client

    conn.close()  # Close the connection
    ddb.quit()
    print(f"closed connection: {address}")

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
            p = Process(target=handle_client, args=(conn, address), daemon=False)
            p.start()

    except Exception as e:
        print(f"An error occurred: {e}")

    finally:
        server_socket.close()  # Ensure the server socket is closed properly

if __name__ == '__main__':
    server_program()
