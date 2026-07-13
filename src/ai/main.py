#!/usr/bin/env python3
import socket
import sys
from random import choice

def read_and_get_arguments(argument_type, default=None):

    if argument_type in sys.argv:
        return sys.argv[sys.argv.index(argument_type) + 1]

    return default

def receive_response_from_server(server_connection):

    data = b""

    while not data.endswith(b"\n"):
        part_of_response = server_connection.recv(1)

        if not part_of_response:
            sys.exit(84)
        data += part_of_response
    normal_text = data.decode().strip()

    if normal_text == "dead":
        sys.exit(0)
    return normal_text

def send_command_to_server(server_connection, command):

    server_connection.sendall((command + "\n").encode())
    return receive_response_from_server(server_connection)

def parse_inventory_response(response):

    inventory = {}
    items = response.strip("[]").split(",")

    for item in items:
        parts = item.split()

        if len(parts) == 2 and parts[1].isdigit():
            inventory[parts[0]] = int(parts[1])

    return inventory

def keep_player_alive(server_connection):

    inventory = parse_inventory_response(send_command_to_server(server_connection, "Inventory"))
    if inventory.get("food", 0) > 5:
        return False
    look = send_command_to_server(server_connection, "Look")
    tile = look.strip("[]").split(",")[0].split()
    if "food" in tile:
        send_command_to_server(server_connection, "Take food")
        return True
    send_command_to_server(server_connection, choice(["Forward", "Left", "Right"]))
    return True

def collect_visible_resource(server_connection, tile):
    for resource in [
        "linemate",
        "deraumere",
        "sibur",
        "mendiane",
        "phiras", "thystame",
    ]:
        if resource in tile:
            send_command_to_server(server_connection, "Take " + resource)
            return True
    return False

def main():

    port_argument = int(read_and_get_arguments("-p"))
    team_argument = read_and_get_arguments("-n")
    host_argument = read_and_get_arguments("-h", "localhost")

    with socket.create_connection((host_argument, port_argument)) as server_connection:
        receive_response_from_server(server_connection)
        send_command_to_server(server_connection, team_argument)
        receive_response_from_server(server_connection)

        while True:
            if keep_player_alive(server_connection):
                continue
            look = send_command_to_server(server_connection, "Look")
            tile = look.strip("[]").split(",")[0].split()

            if "food" in tile:
                send_command_to_server(server_connection, "Take food")
            elif collect_visible_resource(server_connection, tile):
                continue
            else:
                send_command_to_server(server_connection, choice(["Forward", "Left", "Right"]))

if __name__ == "__main__":
    main()
