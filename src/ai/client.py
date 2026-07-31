import sys

CONNECT_WAIT = 5
ANSWER_WAIT = 360
SERVER_DATA_BUFFER = b""


def stop_with_error(message):

    print("zappy_ai: " + message, file=sys.stderr)
    sys.exit(84)


def read_server_line(server_connection):

    global SERVER_DATA_BUFFER

    while b"\n" not in SERVER_DATA_BUFFER:
        try:
            new_data = server_connection.recv(4096)
        except TimeoutError:
            stop_with_error("the server took too long to answer")
        except OSError:
            stop_with_error("connection to the server was lost")
        if not new_data:
            stop_with_error("the server closed the connection")
        SERVER_DATA_BUFFER += new_data

    line, SERVER_DATA_BUFFER = SERVER_DATA_BUFFER.split(b"\n", 1)
    try:
        return line.decode().strip()
    except UnicodeDecodeError:
        stop_with_error("the server sent an invalid message")


def receive_response_from_server(server_connection, expected_command=None):

    while True:
        normal_text = read_server_line(server_connection)

        if normal_text == "dead":
            sys.exit(0)

        if normal_text.startswith("message "):

            from coordination import handle_broadcast_message

            handle_broadcast_message(normal_text)
            continue
        if normal_text.startswith("eject:"):
            continue
        if normal_text.startswith("Current level:"):

            from coordination import update_player_level

            level_updated = update_player_level(normal_text)

            if expected_command == "Incantation":
                return normal_text
            if level_updated:
                continue
        if normal_text == "Elevation underway" and expected_command != "Incantation":
            continue

        return normal_text


def send_command_to_server(server_connection, command):

    try:
        server_connection.sendall((command + "\n").encode())
    except OSError:
        stop_with_error("could not send a command to the server")

    return receive_response_from_server(server_connection, command)


def join_team(server_connection, team):

    if receive_response_from_server(server_connection) != "WELCOME":
        stop_with_error("the server did not send WELCOME")

    free_places = send_command_to_server(server_connection, team)

    if not free_places.isdigit():
        stop_with_error("the server refused the team")

    map_size = receive_response_from_server(server_connection).split()

    if len(map_size) != 2 or not all(value.isdigit() for value in map_size):
        stop_with_error("the server sent an invalid map size")
    if int(map_size[0]) <= 0 or int(map_size[1]) <= 0:
        stop_with_error("the server sent an invalid map size")
