
/*
 * ============================================================================
 * Program: AI Linux Assistant
 * Author: Guillaume R @ GIMO
 * Description: This program serves as a GPT-powered assistant for Linux
 *              systems, designed to provide intelligent and interactive 
 *              functionality to users. It integrates AI capabilities to 
 *              enhance productivity and automate tasks within a Linux 
 *              environment.
 * 
 * License: This software is licensed for non-commercial use only. 
 *          Redistribution, modification, and use in any form for commercial 
 *          purposes are strictly prohibited.
 * 
 * Disclaimer: This program is provided "as is" without any warranty of any kind,
 *             either express or implied, including but not limited to the 
 *             implied warranties of merchantability and fitness for a 
 *             particular purpose.
 * 
 * Requirements: 
 *  - GCC Compiler
 *  - json-c, libcurl 
 *  - Linux Operating System
 *  - outgoing firewall access to OpenAI API servers
 * 
 * Usage:
 *  1. Compile and install : make
 *  2. Edit /etc/ai/ai.conf and install your OpenAI key 
 *  3. Run the executable: ai please echo Hello world ! 
 * 
 * ============================================================================
 */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <json-c/json.h>
#include <curl/curl.h>
#include <stdbool.h>
#include <sys/wait.h>
#include <signal.h>

#define CONFIG_PATH "/etc/ai/ai.conf"
#define MAX_CONVERSATION_ENTRIES 25000
#define API_KEY_BUFFER_SIZE 200
#define RESPONSE_BUFFER_SIZE 64000
#define COMMAND_BUFFER_SIZE 65000       
#define PADDING 512
#define MAXTOKENS 500

// Structure for conversation entries
typedef struct {
    char role[PADDING];
    char content[RESPONSE_BUFFER_SIZE+PADDING];
} ConversationEntry;


// Function to get concatenated configuration values based on a key prefix
char *get_multiline_config_value(const char *key_prefix) {
    FILE *config_file = fopen(CONFIG_PATH, "r");
    if (config_file == NULL) {
        perror("Failed to open config file");
        return NULL;
    }

    char *value = NULL;
    char line[1024];
    size_t value_len = 0;

    while (fgets(line, sizeof(line), config_file)) {
        // Check if the line starts with the key prefix
        if (strncmp(line, key_prefix, strlen(key_prefix)) == 0) {
            // Get the part after the key_prefix (skip key and '=')
            char *line_value = line + strlen(key_prefix) + 1;
            size_t line_value_len = strlen(line_value);

            // Remove newline character
            if (line_value[line_value_len - 1] == '\n') {
                line_value[line_value_len - 1] = '\0';
                line_value_len--;
            }

            // Reallocate and append to the cumulative value string
            char *new_value = realloc(value, value_len + line_value_len + 2); // +1 for space, +1 for '\0'
            if (new_value == NULL) {
                perror("Failed to allocate memory for configuration value");
                free(value);
                fclose(config_file);
                return NULL;
            }

            value = new_value;
            strcpy(value + value_len, line_value);
            value_len += line_value_len;

            // Add a space between lines if needed
            value[value_len] = ' ';
            value_len++;
        }
    }

    // Null-terminate the final value string
    if (value) value[value_len - 1] = '\0';

    fclose(config_file);

    if (value == NULL) {
        fprintf(stderr, "%s not found in config file\n", key_prefix);
    }

    return value;
}

// Wrapper functions for specific keys
char *get_prompt() {
    return get_multiline_config_value("PROMPT=");
}
char *get_added_prompt() {
    return get_multiline_config_value("ADDEDPROMPT=");
}



// Conversation array and a count to keep track of entries
ConversationEntry conversation[MAX_CONVERSATION_ENTRIES];
int conversation_count = 0;

// Function to decode URL-encoded content
char *url_decode(const char *encoded_str) {
    CURL *curl = curl_easy_init();
    char *decoded_str = NULL;

    if (curl) {
        decoded_str = curl_easy_unescape(curl, encoded_str, 0, NULL);
        curl_easy_cleanup(curl); // Clean up CURL
    }

    return decoded_str ? decoded_str : strdup(encoded_str); // Return decoded or original if decoding failed
}

//Function to add 

void append_conversation_entry(const char *role, const char *content) {
    if (conversation_count < MAX_CONVERSATION_ENTRIES) {
        // Store the role as-is
        strncpy(conversation[conversation_count].role, role, sizeof(conversation[conversation_count].role) - 1);

        // Initialize CURL to use curl_easy_escape
        CURL *curl = curl_easy_init();
        if (curl) {
            // URL-encode the content
            char *encoded_content = curl_easy_escape(curl, content, 0);
            if (encoded_content) {
                // Store the URL-encoded content
                strncpy(conversation[conversation_count].content, encoded_content, sizeof(conversation[conversation_count].content) - 1);
                curl_free(encoded_content); // Free encoded content memory
            } else {
                // If encoding fails, fall back to the original content
                strncpy(conversation[conversation_count].content, content, sizeof(conversation[conversation_count].content) - 1);
            }
            curl_easy_cleanup(curl); // Clean up CURL
        }

        conversation_count++;
    } else {
        printf("Conversation storage limit reached.\n");
    }
}

// function to escape double quotes and actually other special characters so that excecution goes through nicely.
void escape_double_quotes(const char *input, char *output, size_t output_size) {
    size_t j = 0; // Index for the output buffer

    for (size_t i = 0; input[i] != '\0' && j < output_size - 1; ++i) {
        if (input[i] == '"') {
            // Escape double quotes
            if (j + 1 < output_size - 1) {
                output[j++] = '\\';
                output[j++] = '"';
            } else {
                break; // No space for escaping, stop to avoid overflow
            }
        } else if (input[i] == '\\') {
            // Escape backslashes
            if (j + 1 < output_size - 1) {
                output[j++] = '\\';
                output[j++] = '\\';
            } else {
                break; // No space for escaping, stop to avoid overflow
            }
        } else if (input[i] == '$') {
            // Escape dollar signs
            if (j + 1 < output_size - 1) {
                output[j++] = '\\';
                output[j++] = '$';
            } else {
                break; // No space for escaping, stop to avoid overflow
            }
        } else if (input[i] == '`') {
            // Escape backticks
            if (j + 1 < output_size - 1) {
                output[j++] = '\\';
                output[j++] = '`';
            } else {
                break; // No space for escaping, stop to avoid overflow
            }
        } else {
            // Copy other characters as-is
            output[j++] = input[i];
        }
    }

    // Null-terminate the output string
    output[j] = '\0';
}

// Remove backticks
void strip_triple_backticks(char *response) {
    char *src = response, *dst = response;
    while (*src) {
        // Look for triple backticks
        if (strncmp(src, "```", 3) == 0) {
            src += 3; // Skip over the triple backticks
        } else {
            *dst++ = *src++; // Copy characters if not part of triple backticks
        }
    }
    *dst = '\0'; // Null-terminate the cleaned string
}

// Function to generate JSON payload from the conversation array
char *generate_json_payload() {
    struct json_object *messages_array = json_object_new_array();

    for (int i = 0; i < conversation_count; i++) {
        struct json_object *message_obj = json_object_new_object();
        json_object_object_add(message_obj, "role", json_object_new_string(conversation[i].role));
        json_object_object_add(message_obj, "content", json_object_new_string(conversation[i].content));
        json_object_array_add(messages_array, message_obj);
    }

    struct json_object *payload_obj = json_object_new_object();
    json_object_object_add(payload_obj, "model", json_object_new_string("gpt-4o"));
    json_object_object_add(payload_obj, "messages", messages_array);
    json_object_object_add(payload_obj, "temperature", json_object_new_double(0));
    json_object_object_add(payload_obj, "max_tokens", json_object_new_int(MAXTOKENS));

    char *payload_str = strdup(json_object_to_json_string(payload_obj));
    json_object_put(payload_obj); // Free JSON object memory
    return payload_str;
}


// Function to get the OpenAI API key from configuration file
char *get_api_key() {
    FILE *config_file = fopen(CONFIG_PATH, "r");
    if (config_file == NULL) {
        perror("Failed to open config file");
        return NULL;
    }

    char *api_key = NULL;
    char line[256];

    while (fgets(line, sizeof(line), config_file)) {
        // Check if line starts with "OPENAIKEY="
        if (strncmp(line, "OPENAIKEY=", 10) == 0) {
            // Allocate memory for the API key
            api_key = strdup(line + 10); // Copy the key part after "OPENAIKEY="
            if (api_key == NULL) {
                perror("Failed to allocate memory for API key");
                fclose(config_file);
                return NULL;
            }
            // Remove newline character if present
            char *newline = strchr(api_key, '\n');
            if (newline) *newline = '\0';
            break;
        }
    }

    fclose(config_file);

    if (api_key == NULL) {
        fprintf(stderr, "API key not found in config file\n");
    }

    return api_key;
}

// Function to send request to OpenAI API and get response
char *send_request_to_openai(char *api_key) {

    char *json_payload = generate_json_payload();
    static char response[RESPONSE_BUFFER_SIZE];

    char command[COMMAND_BUFFER_SIZE];
    snprintf(command, sizeof(command),
             "curl -s -X POST https://api.openai.com/v1/chat/completions "
             "-H \"Content-Type: application/json\" "
             "-H \"Authorization: Bearer %s\" "
             "-d '%s'", api_key, json_payload);

    FILE *fp = popen(command, "r");
    if (fp == NULL) {
        perror("Error sending request to OpenAI");
        free(json_payload);
        return NULL;
    }

    fread(response, sizeof(char), RESPONSE_BUFFER_SIZE, fp);
    pclose(fp);
    free(json_payload);

    return response;
}

// Process the JSON response and extract the "content" field from "choices"
char *parse_ai_response(const char *json_response) {
    struct json_object *parsed_json, *choices_array, *choice, *message, *content;

    // Parse the JSON string
    parsed_json = json_tokener_parse(json_response);
    if (parsed_json == NULL) {
        printf("%s\n", json_response);
	printf("\nAI ended the conversation.\n");
        exit(1);
        return NULL;
    }

    // Navigate through the JSON structure to reach "content"
    if (json_object_object_get_ex(parsed_json, "choices", &choices_array)) {
        choice = json_object_array_get_idx(choices_array, 0);
        if (json_object_object_get_ex(choice, "message", &message) &&
            json_object_object_get_ex(message, "content", &content)) {
            // Return the content as a string
            const char *content_str = json_object_get_string(content);
            char *decoded_content = url_decode(content_str);
            //char *result = strdup(content_str);

            json_object_put(parsed_json);
            return decoded_content;
        }
    }

    // Clean up if parsing failed
    json_object_put(parsed_json);
    printf("%s\n", json_response);
    printf("\nAI ended the conversation.\n");
    exit(1);

    return NULL;
}

// If necessary remove these annoying backslashes (not in use right now)
void remove_extra_backslashes(char *str) {
    int i, j = 0;
    int len = strlen(str);
    char result[len + 1]; // Temporary array to store the modified string

    for (i = 0; i < len; i++) {
        if (str[i] == '\\' && str[i + 1] == '\\') {
            // Keep only one backslash
            result[j++] = '\\';
            i++; // Skip the next backslash
        } else if (str[i] == '\\' && (str[i + 1] == '\"' || str[i + 1] == '\'')) {
            // Remove unnecessary backslashes before quotes
            result[j++] = str[i + 1];
            i++; // Skip the next character
        } else {
            result[j++] = str[i];
        }
    }
    result[j] = '\0'; // Null-terminate the result string
    strcpy(str, result); // Copy the result back to the original string
}

// Execute the command on the shell through bash -c 
void execute_command(char *command, char *command_output, size_t output_size) {
    int pipe_fd[2];
    if (pipe(pipe_fd) == -1) {
        perror("Pipe failed");
        return;
    }

    // Prepare a sanitized command by preserving necessary backslashes
    char sanitized_command[548];
    char re_sanitized_command[512];

//    remove_extra_backslashes(command);
    escape_double_quotes(command, re_sanitized_command, sizeof(re_sanitized_command));

    snprintf(sanitized_command, sizeof(sanitized_command), "bash -c \"%s\"", re_sanitized_command);

    printf("I need to run this command: %s\n", sanitized_command);
    printf("Do you want to proceed? (yes/no/exit) [no]: ");

    char user_input[10];
    fgets(user_input, sizeof(user_input), stdin);

    // Default to "no" if input is empty or doesn't start with "yes" or "exit"
    if (strncmp(user_input, "yes", 3) == 0) {
        pid_t pid = fork();
        if (pid == -1) {
            perror("Failed to fork");
            return;
        } else if (pid == 0) {
            // Child process: executes the command and writes output to the pipe
            close(pipe_fd[0]);  // Close the read end of the pipe

            char line[512];
            char result[RESPONSE_BUFFER_SIZE+PADDING] = "";

            FILE *fp = popen(sanitized_command, "r");
            if (fp == NULL) {
                perror("Error executing command");
                exit(1);
            }

            while (fgets(line, sizeof(line), fp) != NULL) {
                if (strlen(result) + strlen(line) < RESPONSE_BUFFER_SIZE - 512) {
                    strcat(result, line);
                } else {
                    printf("\n\nOutput buffer exceeded so we truncate.\n");
                    break;
                }
            }

            pclose(fp);


            // Write the result to the pipe
            write(pipe_fd[1], result, strlen(result) + 1);
            write(pipe_fd[1], "\n\nOutput buffer exceeded so we truncate.\n", strlen("\n\nOutput buffer exceeded so we truncate.\n")+1);
            close(pipe_fd[1]); // Close write end of pipe
            exit(0);           // Exit child process

        } else {
            // Parent process: waits with a timeout
            close(pipe_fd[1]); // Close the write end of the pipe

            int status;
            int timeout = 60; // 60 seconds timeout
            while (timeout > 0) {
                pid_t result = waitpid(pid, &status, WNOHANG);
                if (result == 0) { // Child still running
                    sleep(1);
                    timeout--;
                } else if (result == pid) { // Child finished
                    break;
                } else { // Error occurred
                    perror("Error waiting for child process");
                    break;
                }
            }

            if (timeout == 0) {
                printf("Command timed out. Killing process.\n");
                kill(pid, SIGKILL);
                waitpid(pid, &status, 0); // Clean up the terminated process
                snprintf(command_output, output_size, "command executed: <%s> status: <timeout>", command);
            } else if (WIFEXITED(status)) {
                // Read from the pipe to get command output from the child process
                read(pipe_fd[0], command_output, output_size - 1);
                command_output[output_size - 1] = '\0';  // Ensure null termination

                if (strlen(command_output) == 0) {
                    snprintf(command_output, output_size, "command executed: <%s> status: <executed> output: <Empty or Execution error>", command);
                } else {
                    char full_output[RESPONSE_BUFFER_SIZE+PADDING]="";
                    snprintf(full_output, RESPONSE_BUFFER_SIZE, "command executed: <%s> status: <executed> output: <%s>", command, command_output);
                    strncpy(command_output, full_output, output_size - 1);
                    command_output[output_size - 1] = '\0'; // Null-terminate the result
                }
            }
            close(pipe_fd[0]); // Close read end of pipe
            printf("Command output:\n%s\n", command_output);
            append_conversation_entry("user", command_output);
        }
    } else if (strncmp(user_input, "exit", 4) == 0) {
        printf("Bye Bye!\n");
        exit(0); // Exit the program immediately
    } else {
        snprintf(command_output, output_size, "command executed: <%s> status: <sysadmin declined to execute command.>", command);
        append_conversation_entry("user", command_output);
        printf("Do you want to continue the conversation? (yes/no) [no]: ");

        fgets(user_input, sizeof(user_input), stdin);
        if (strncmp(user_input, "yes", 3) == 0) {
            printf("Enter your next message: ");
            char user_message[RESPONSE_BUFFER_SIZE];
            fgets(user_message, sizeof(user_message), stdin);
            user_message[strcspn(user_message, "\n")] = '\0';
            append_conversation_entry("user", user_message);
        } else {
            printf("Bye Bye!\n");
            exit(0);
        }
    }
}

// Function to find and execute each command in the assistant's response
int process_response_for_commands(const char *response) {

    const char *start_tag = "<CMD>";
    const char *end_tag = "</CMD>";
    char *command_start = strstr(response, start_tag);
    int command_found = 0; // Track if any commands are found


    while (command_start != NULL) {
        command_found = 1; // Mark that at least one command is found

        // Move the pointer to the start of the actual command (after the start tag)
        command_start += strlen(start_tag);

        // Find the end of the command using the end tag
        char *command_end = strstr(command_start, end_tag);
        if (command_end == NULL) {
           // printf("Error: Unmatched command tags.\n");
            return 0; // Exit if there's an unmatched tag
        }

        // Extract the command between the tags
        size_t command_length = command_end - command_start;
        char command[512];
        strncpy(command, command_start, command_length);
        command[command_length] = '\0'; // Null-terminate the command string

        // Buffer to store the output of each command
        char command_output[RESPONSE_BUFFER_SIZE];
        char error_output[RESPONSE_BUFFER_SIZE];
        
        // Execute the extracted command and capture the output
        execute_command(command, command_output, sizeof(command_output));

        // Look for the next command in the response
        command_start = strstr(command_end + strlen(end_tag), start_tag);
    }

    return command_found;
}

// main program

int main(int argc, char *argv[]) {

    char *api_key = get_api_key();
    if (api_key == NULL) {
        fprintf(stderr, "Could not retrieve API key.\n");
        return 1;
    }

    // Build the prompt by joining all arguments with spaces
    char prompt[RESPONSE_BUFFER_SIZE] = "";

	// Interactive mode
	if(argc<2) {

                        printf("Enter your message: ");
                        fgets(prompt, sizeof(prompt), stdin);
                        prompt[strcspn(prompt, "\n")] = '\0'; // Remove newline


	} else {
    for (int i = 1; i < argc; i++) {
        strcat(prompt, argv[i]);
        if (i < argc - 1) {
            strcat(prompt, " "); // Add space between arguments
        }
    }
}

    char *config_prompt = get_prompt();
    char *added_prompt = get_added_prompt();

 if (config_prompt) {
        append_conversation_entry("system", config_prompt);
        free(config_prompt); // Free memory after use
    }

    if (added_prompt) {
        append_conversation_entry("system", added_prompt);
        free(added_prompt); // Free memory after use
    }

    append_conversation_entry("user", prompt);

    char command_output[RESPONSE_BUFFER_SIZE];
    int continue_conversation = 1;

    while (continue_conversation) {

        // Send conversation history to OpenAI API and display response
        char *response = send_request_to_openai(api_key);
        if (response != NULL) {
            char *ai_content = parse_ai_response(response);
            if (ai_content != NULL) {
                append_conversation_entry("assistant", ai_content);
                printf("%s\n", ai_content);

                int commands_found = process_response_for_commands(ai_content);

                if (!commands_found) {
                    // No command found, prompt the sysadmin
                    printf("Do you want to continue the conversation? (yes/no) [no]: ");
                    char user_input[10];
                    fgets(user_input, sizeof(user_input), stdin);

                    // Default to "no" if input is empty or doesn't start with "yes"
                    if (strncmp(user_input, "yes", 3) == 0) {
                        // Get input for the next user entry
                        printf("Enter your next message: ");
                        char user_message[RESPONSE_BUFFER_SIZE];
                        fgets(user_message, sizeof(user_message), stdin);
                        user_message[strcspn(user_message, "\n")] = '\0'; // Remove newline

                        // Append the user's new message to the conversation
                        append_conversation_entry("user", user_message);
                    } else {
                        continue_conversation = 0; // End the program
                    }
                }

                free(ai_content);
            }
        } else {
            fprintf(stderr, "Failed to get a response from AI.\n");
            break;
        }
    }

    return 0;
}

