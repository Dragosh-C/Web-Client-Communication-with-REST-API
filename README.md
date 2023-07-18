Web Client. Communication with REST API

For data parsing, I used the Parson library [1] for C. It includes the files parson.c and parson.h. The function json_value_init_object() initializes a JSON object, and json_object_set_string() adds a string field to the JSON object. The JSON is then serialized with json_serialize_to_string() and sent to the server using the send_to_server() function. To parse the data received from the server, I created the convert_to_json function, which takes the server response as a parameter and transforms it into a JSON object.

For message formation, I started with the function provided in the laboratory, to which I added an authentication field with the JWT token. I also created a function for message deletion.

In the implementation of the task, I used a separate socket for each command to make it easier to send and receive data from the server. For each user-entered command, I created a corresponding function, and I handled cases where the command is not valid.

The register_user() function reads the username and password from the keyboard, checks if they are valid, and then creates a JSON object using the Parson library functions. It sends a POST message to the server. If the response from the server is 201 Created, then the user was successfully created; otherwise, an error message is displayed.

The login_user() function parses the data in the same way, but it sends the data to the login page and saves the received cookie if the login is successful.

The enter_library() function sends a GET message to the .../library/access page and saves the JWT token received from the server. In case of an error, the error message from the server is displayed.

The get_books() function sends a GET message to the .../library/books page and displays the available books. In case of an error, the error message from the server is displayed.

The get_book() function sends a GET message to the .../library/books/id page and displays the book with the specified ID. It also handles cases of invalid input and when the book does not exist.

The add_book() function reads the book data from the keyboard, checks if it is valid, creates a JSON object, and sends a POST message to the .../library/books page. In case of an error, the error message from the server is displayed.

The delete_book() function sends a DELETE message to the .../library/books/id page. It also handles cases of invalid input and when the book with that ID does not exist.

[1] https://github.com/kgabis/parson
