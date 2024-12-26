import socket
from DofusDB import DofusDB
import json
from time import sleep
def server_program():
    ddb = DofusDB()
    # get the hostname
    host = socket.gethostname()
    port = 5000  # initiate port no above 1024
    print(host)
    server_socket = socket.socket()  # get instance
    # look closely. The bind() function takes tuple as argument
    server_socket.bind((host, port))  # bind host address and port together

    # configure how many client the server can listen simultaneously
    server_socket.listen(1000)
    conn, address = server_socket.accept()  # accept new connection
    print("Connection from: " + str(address))
    while True:
        # receive data stream. it won't accept data packet greater than 1024 bytes
        data = conn.recv(1024).decode()
        if not data:
            sleep(1)
            continue
        res = json.loads(data)
        print(res)
        if "exit" in res and res["exit"]==1:
            break
        ddb.set_x_position(res["x"])
        ddb.set_y_position(res["y"])
        print(type(res["direction"]))
        ddb.set_direction(res["direction"])
        ddb.set_hint(res["hint"])
        print("from connected user: " + str(data))
        new_data = "/".join(ddb.get_hint_position())
        conn.send(new_data.encode())  # send data to the client

    conn.close()  # close the connection


if __name__ == '__main__':
    server_program()