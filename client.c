#include <stdio.h>      /* printf, sprintf */
#include <stdlib.h>     /* exit, atoi, malloc, free */
#include <unistd.h>     /* read, write, close */
#include <string.h>     /* memcpy, memset */
#include <sys/socket.h> /* socket, connect */
#include <netinet/in.h> /* struct sockaddr_in, struct sockaddr */
#include <netdb.h>      /* struct hostent, gethostbyname */
#include <arpa/inet.h>
#include "include/helpers.h"
#include "include/requests.h"
#include "include/parson.h"


#define HOST_IP "34.254.242.81"
#define HOST_PORT 8080
#define REGISTER_PAGE "/api/v1/tema/auth/register"
#define LOGIN_PAGE "/api/v1/tema/auth/login"
#define ACCESS_PAGE "/api/v1/tema/library/access"
#define BOOKS_PAGE "/api/v1/tema/library/books"
#define LOGOUT_PAGE "/api/v1/tema/auth/logout"
#define COMMAND_LEN 50
#define NAME_LEN 30


JSON_Value *convert_to_json(char *response) {
    // Create a JSON object
    JSON_Value *val = json_value_init_object();
    JSON_Object *object = json_value_get_object(val);

    // Split response by lines
    char* line = strtok((char*)response, "\n");

     if (line != NULL) {
        // Add the status line to JSON object
        line[strlen(line)-1] = '\0';
        json_object_set_string(object, "Status", line);
    }
    while (line != NULL) {

        if(line[0] == '{') {
            char *error_mesage = malloc(LINELEN);
            strcpy(error_mesage, line);
            error_mesage[strlen(error_mesage) - 2] ='\0'; 

            json_object_set_string(object, "message", error_mesage + 10);
            
            free(error_mesage);
            
            line = strtok(NULL, "\n");
            continue;
        }

        char *colon = strstr(line, ":");
        if (colon != NULL) {
            *colon = '\0';
            char *key = line;
            char *value = colon + 1;

            while (*key == ' ') key++;
            char *end = value + strlen(value) - 2;
            while (end > value && *end == ' ') end--;
            *(end + 1) = '\0';

            // Add key-value pair to JSON object
            json_object_set_string(object, key, value + 1);
        }
        line = strtok(NULL, "\n");
    } 
    return val;
}



int register_user(int sockfd) {

    printf("username=");
    char *username = malloc(NAME_LEN);
    fgets(username, NAME_LEN, stdin);
    username[strlen(username) - 1] = '\0';
    
    printf("password=");
    char *password = malloc(NAME_LEN);
    fgets(password, NAME_LEN, stdin);
    password[strlen(password) - 1] = '\0';

    // hande invalid input
    if (strlen(username) == 0 || strlen(password) == 0) {
        printf("Invalid data: complete all fields\n");
        return 0;
    }
    if (strchr(username, ' ') != NULL || strchr(password, ' ') != NULL) {
        printf("Invalid data: username and password can't contain spaces\n");
        return 0;
    }
    // create message
    JSON_Value *value = json_value_init_object();
    JSON_Object *object = json_value_get_object(value);
    
    json_object_set_string(object, "username", username);
    json_object_set_string(object, "password", password);
    
    char *body_data = json_serialize_to_string_pretty(value);
    char **data = calloc(1, sizeof(char *));

    data[0] = strdup(body_data);
    char *message = compute_post_request(HOST_IP, REGISTER_PAGE,
                             "application/json", data, 1, NULL, 0, NULL);
    // send message
    send_to_server(sockfd, message);
    char * response = receive_from_server(sockfd);
    JSON_Value *parsed_value = convert_to_json(response);
    JSON_Object *parsed_object = json_value_get_object(parsed_value);

    const char *status = json_object_get_string(parsed_object, "Status");

    if (strcmp(status, "HTTP/1.1 201 Created") == 0) {
        printf("Register success\n");
        free(body_data);
        free(message);
        free(data[0]);
        free(data);
        return 1;
    } else {
        const char *status = json_object_get_string(parsed_object, "message");
        printf("Register failed: %s\n", status);
        free(body_data);
        free(message);
        free(data[0]);
        free(data);
        return 0;
    }
}

int login_user(int sockfd, char *connect_id_cookie){

    printf("username=");
    char *username = malloc(NAME_LEN);
    fgets(username, NAME_LEN, stdin);
    username[strlen(username) - 1] = '\0';
    
    printf("password=");
    char *password = malloc(NAME_LEN);
    fgets(password, NAME_LEN, stdin);
    password[strlen(password) - 1] = '\0';

    // handle invalid input
    if (strlen(username) == 0 || strlen(password) == 0) {
        printf("Invalid data: complete all fields\n");
        return 0;
    }
    if (strchr(username, ' ') != NULL || strchr(password, ' ') != NULL) {
        printf("Invalid data: username and password can't contain spaces\n");
        return 0;
    }
    // create message
    JSON_Value *value = json_value_init_object();
    JSON_Object *object = json_value_get_object(value);
    
    json_object_set_string(object, "username", username);
    json_object_set_string(object, "password", password);
    
    char *body_data = json_serialize_to_string_pretty(value);
    char **data = calloc(1, sizeof(char *));
    data[0] = strdup(body_data);
    char *message = compute_post_request(HOST_IP, LOGIN_PAGE,
                     "application/json", data, 1, NULL, 0, NULL);
    
    // send message to server
    send_to_server(sockfd, message);
    char *response = receive_from_server(sockfd);
    
    JSON_Value *parsed_value = convert_to_json(response);
    JSON_Object *parsed_object = json_value_get_object(parsed_value);

    const char *status = json_object_get_string(parsed_object, "Status");

    if (strcmp(status, "HTTP/1.1 200 OK") == 0) {
        // save cookies
        printf("Login success\n");
        char *cookies = strdup(json_object_get_string(parsed_object, "Set-Cookie"));
        char *p = strtok(cookies, ";");
        strcpy(connect_id_cookie, p);
        free(body_data);
        free(message);
        free(data[0]);
        free(data);
        return 1;
    } else {
        // print error message
        const char *status = json_object_get_string(parsed_object, "message");
        printf("Login failed : %s\n", status);
        free(body_data);
        free(message);
        free(data[0]);
        free(data);
        return 0;
    }
}

int enter_library(int sockfd, char *connect_id_cookie, char *token) {
    
    char *message = compute_get_request(HOST_IP, ACCESS_PAGE, NULL, &connect_id_cookie, 1, NULL);
    send_to_server(sockfd, message);

    char *response = receive_from_server(sockfd);
    JSON_Value *parsed_value = convert_to_json(response);
    JSON_Object *parsed_object = json_value_get_object(parsed_value);

    const char *status = json_object_get_string(parsed_object, "Status");

    if(strcmp(status, "HTTP/1.1 200 OK") == 0) {
        // save token
        printf("You can access library\n");
        const char *status = json_object_get_string(parsed_object, "message");
        strcpy(token, status);
        free(message);
        return 1;
    }
    else {
        // print error message
        const char *status = json_object_get_string(parsed_object, "message");
        printf("Error: %s\n", status);
        free(message);
        return 0;
    }
}

void get_books(int sockfd, char *connect_id_cookie, char *token) {

    char *message = compute_get_request(HOST_IP, BOOKS_PAGE, NULL, &connect_id_cookie, 1, token);
    send_to_server(sockfd, message);
    char * response = receive_from_server(sockfd);
    char copy_response[strlen(response) + 1];
    strcpy(copy_response, response);
    
    JSON_Value *parsed_value = convert_to_json(response);
    JSON_Object *parsed_object = json_value_get_object(parsed_value);

    const char *status = json_object_get_string(parsed_object, "Status");

    if(strcmp(status, "HTTP/1.1 200 OK") == 0) {
        // print the books
        char *p = strstr(copy_response, "[{");
        if(p != NULL){
            printf("%s\n", p);
        }
        else {
           printf("[]\n");
        }
    }
    else {
        // print error message
        const char *status = json_object_get_string(parsed_object, "message");
        printf("%s\n", status);
    }
    free(message);

}

void get_book(int sockfd, char *connect_id_cookie, char *token) {

    char *book_path = malloc(LINELEN);
    strcpy(book_path, BOOKS_PAGE);
    
    printf("id=");
    char id[20];
    fgets(id, 20, stdin);
    id[strlen(id) - 1] = '\0';
    
    // Verify if id is valid
    if (strchr(id, ' ') != NULL || strlen(id) == 0) {
        printf("Invalid id: id can't contain spaces\n");
        return;
    }
    for (int i = 0; i < strlen(id); i++) {
        if (id[i] < '0' || id[i] > '9'){
            printf("Invalid id: id needs to be a number\n");
            return;
        }
    }
    strcat(book_path,"/");
    strcat(book_path, id);

    char *message = compute_get_request(HOST_IP, book_path, NULL, &connect_id_cookie, 1, token);
    send_to_server(sockfd, message);
    char * response = receive_from_server(sockfd);
    char copy_response[strlen(response) + 1];
    strcpy(copy_response, response);

    JSON_Value *parsed_value = convert_to_json(response);
    JSON_Object *parsed_object = json_value_get_object(parsed_value);
    const char *status = json_object_get_string(parsed_object, "Status");

    if(strcmp(status, "HTTP/1.1 200 OK") == 0) {

        char *p = strstr(copy_response, "{");
        if (p != NULL)
            printf("%s\n", p);
    }
    else {
            const char *status = json_object_get_string(parsed_object, "message");
            if (strcmp(message, "No book was found!")) {
                printf("Book with id=%s don't exist!\n", id);
            }
            else {    
            printf("%s\n", status);
            }
    }
    free(book_path);
    free(message);
}

void add_book(int sockfd, char *connect_id_cookie, char *token) {
    
    printf("title=");
    char *title = malloc(BUFLEN);
	fgets(title, BUFLEN, stdin);
	title[strlen(title) - 1] = '\0';
	
    printf("author=");
    char *author = malloc(BUFLEN);
	fgets(author, BUFLEN, stdin);
	author[strlen(author) - 1] = '\0';
	
    printf("genre=");
    char *genre = malloc(BUFLEN);
	fgets(genre, BUFLEN, stdin);
	genre[strlen(genre) - 1] = '\0';
    
    printf("page_count=");
    char *page_count = malloc(BUFLEN);
    fgets(page_count, BUFLEN, stdin);
    page_count[strlen(page_count) - 1] = '\0';
    
    printf("publisher=");
    char *publisher = malloc(BUFLEN);
    fgets(publisher, BUFLEN, stdin);
    publisher[strlen(publisher) - 1] = '\0';


    // Verify if id is valid
    for (int i = 0; i < strlen(page_count); i++) {
        if (page_count[i] < '0' || page_count[i] > '9'){
            printf("Invalid page_count\n");
            return;
        }
    }
    if (strlen(title) == 0 || strlen(author) == 0 || strlen(genre) == 0 ||
                         strlen(page_count) == 0 || strlen(publisher) == 0) {
        printf("Invalid data: all fields need to be completed\n");
        return;
    }

    // Parse to json all data
    JSON_Value *value = json_value_init_object();
    JSON_Object *object = json_value_get_object(value);

    json_object_set_string(object, "title", title);
    json_object_set_string(object, "author", author);
    json_object_set_string(object, "genre", genre);
    json_object_set_number(object, "page_count", atoi(page_count));
    json_object_set_string(object, "publisher", publisher);
    
    char *body_data = json_serialize_to_string_pretty(value);
    char **data = calloc(1, sizeof(char *));
    data[0] = strdup(body_data);
    char *message = compute_post_request(HOST_IP, BOOKS_PAGE, 
                "application/json", data, 1, &connect_id_cookie, 1, token);
 
    send_to_server(sockfd, message);
    char * response = receive_from_server(sockfd);

    JSON_Value *parsed_value = convert_to_json(response);
    JSON_Object *parsed_object = json_value_get_object(parsed_value);
    
    const char *status = json_object_get_string(parsed_object, "Status");
    
    if(status != NULL) {
        if(strcmp(status, "HTTP/1.1 200 OK") == 0) {
            printf("Book added successfully\n");
        }
        else {
            // print error message
            const char *status = json_object_get_string(parsed_object, "message");
            printf("%s\n", status);
        }
    }

    free(author);
    free(title);
    free(genre);
    free(page_count);
    free(publisher);
    free(body_data);
    free(message);
    free(data[0]);
    free(data);
}

void delete_book(int sockfd, char *connect_id_cookie, char *token) {
    char *book_path = malloc(LINELEN);
    strcpy(book_path, BOOKS_PAGE);
   
    printf("id=");
    char id[20];
    fgets(id, 20, stdin);
    id[strlen(id) - 1] = '\0';


    
    // Verify if id is valid
    if (strchr(id, ' ') != NULL || strlen(id) == 0) {
        printf("Invalid id: id can't contain spaces\n");
        return;
    }
    
    for (int i = 0; i < strlen(id); i++) {
        if (id[i] < '0' || id[i] > '9'){
            printf("Invalid id: id needs to be a number\n");
            return;
        }
    }
    strcat(book_path,"/");
    strcat(book_path, id);

    char *message = compute_delete_request(HOST_IP, book_path, NULL, &connect_id_cookie, 1, token);
    send_to_server(sockfd, message);
    char *response = receive_from_server(sockfd);
    char copy_response[strlen(response) + 1];
    strcpy(copy_response, response);
    JSON_Value *parsed_value = convert_to_json(response);
    JSON_Object *parsed_object = json_value_get_object(parsed_value);
    const char *status = json_object_get_string(parsed_object, "Status");

    if(strcmp(status, "HTTP/1.1 200 OK") == 0) {

        printf("Book deleted succesfully\n");
    }
    else { // print error message
         const char *status = json_object_get_string(parsed_object, "message");
         printf("%s\n", status);
    }

    free(book_path);
    free(message);
}

int main(int argc, char *argv[])
{
    int sockfd = 0;;
    char *command = malloc(COMMAND_LEN);
    int is_logged_in = 0;
    char *connect_id_cookie = malloc(LINELEN);
    char *token = malloc(LINELEN);
    int have_access = 0;
    while(1) {
        
        if(sockfd != 0) close(sockfd);

        sockfd = open_connection(HOST_IP, HOST_PORT, AF_INET, SOCK_STREAM, 0);
        if (sockfd < 0) {
            fprintf(stderr,"Error opening connection\n");
            break;
        }
 
        // red command from user
        fgets(command, COMMAND_LEN, stdin);
        command[strlen(command) - 1] = '\0';
        if (!strcmp(command, "register")) {
            register_user(sockfd);
            continue;
        }

        if (!strcmp(command, "login")) {
            if (is_logged_in) {
                printf("Already logged in!\n");
            } else {
                int succ = login_user(sockfd, connect_id_cookie);
                if (succ) {
                    is_logged_in = 1;
                }
            }
            continue;
        }

        if (!strcmp(command, "enter_library")) {
            if (!is_logged_in) {
                printf("You are not logged in!\n");
            } 
            else {
               have_access = enter_library(sockfd, connect_id_cookie, token);
            }
            continue;
        }

        if (!strcmp(command, "get_books")) {
            if (!is_logged_in) {
                printf("You are not logged in!\n");
            } 
            else if(!have_access) {
                printf("You don't have access to library!\n");
            }
            else {
                get_books(sockfd, connect_id_cookie, token);
            }
            continue;
        }

        if (!strcmp(command, "get_book")) {
            if (!is_logged_in) {
                printf("You are not logged in!\n");
            } 
            else if(!have_access) {
                printf("You don't have access to library!\n");
            }
            else {
                get_book(sockfd, connect_id_cookie, token);
            }
            continue;
        }

        if (!strcmp(command, "add_book")) {
            if (!is_logged_in) {
                printf("You are not logged in!\n");
            } 
            else if(!have_access) {
                printf("You don't have access to library!\n");
            }
            else {
                add_book(sockfd, connect_id_cookie, token);
            }
            continue;
        }

        if (!strcmp(command, "delete_book")) {
            if (!is_logged_in) {
                printf("You are not logged in!\n");
            } 
            else if(!have_access) {
                printf("You don't have access to library!\n");
            }
            else {
                delete_book(sockfd, connect_id_cookie, token);
            }
            continue;
        }
        
        if (!strcmp(command, "logout")) {
            if (is_logged_in == 0) {
                printf("You are not logged in\n");
            }
            else {
                printf("Logout successfully\n");
                is_logged_in = 0;
                have_access = 0;
                strcpy(token, "");
                continue;
            }
        }

        if (!strcmp(command, "exit")) {
            break;
        }

        printf("Invalid command!\n");

    }
     
    free(command);
    free(connect_id_cookie);
    free(token);

    return 0;
}
