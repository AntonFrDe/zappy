import select
import sys

CONNECT_WAIT = 5
ANSWER_WAIT = 360
SAVED_ANSWERS = {}


def stop_with_error(message):

    print("zappy_ai: " + message, file=sys.stderr)
    sys.exit(84)


def wait_for_server_data(server_connection):

    poller = select.poll()

    try:
        poller.register(server_connection, select.POLLIN | select.POLLERR | select.POLLHUP | select.POLLNVAL)
        events = poller.poll(ANSWER_WAIT * 1000)
    except (OSError, ValueError):
        stop_with_error("connection to the server was lost")
    if not events:
        stop_with_error("the server took too long to answer")
    event_mask = events[0][1]
    if event_mask & select.POLLIN:
        return
    stop_with_error("connection to the server was lost")


def read_server_line(server_connection):

    saved_data = SAVED_ANSWERS.get(server_connection, b"")

    while b"\n" not in saved_data:
        wait_for_server_data(server_connection)
        try:
            new_data = server_connection.recv(4096)
        except TimeoutError:
            stop_with_error("the server took too long to answer")
        except OSError:
            stop_with_error("connection to the server was lost")
        if not new_data:
            stop_with_error("the server closed the connection")
        saved_data += new_data

    line, saved_data = saved_data.split(b"\n", 1)
    SAVED_ANSWERS[server_connection] = saved_data
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
            from incantation import remember_incantation_call

            remember_incantation_call(normal_text)
            continue
        if normal_text.startswith("eject:"):
            continue
        if normal_text.startswith("Current level:"):
            from incantation import update_player_level

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
