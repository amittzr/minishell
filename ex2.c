#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>
#include <sys/stat.h>
#include <fcntl.h>

// Define a node structure for the linked list
typedef struct Node {
    char key[50];
    char* value;
    struct Node* next;
} Node;

// Define a dictionary structure
typedef struct {
    Node* head;  // Pointer to the head of the linked list
    int count;   // Number of key-value pairs in the dictionary
} Dictionary;

int isExist(const Dictionary* dict, const char* key);
void removeNode(Dictionary* dict, const char* key);

// Function to initialize a dictionary
void initDictionary(Dictionary* dict) {
    dict->head = NULL;
    dict->count = 0;
}

// Function to add a key-value pair to the dictionary to the beginning
void addNode(Dictionary* dict, const char* key, const char* value) {
    // Check if the alias already exists
    Node* current = dict->head;
    while (current != NULL) {
        if (strcmp(current->key, key) == 0) {
            // Free the memory allocated for the previous value
            free(current->value);
            // Allocate memory for the new value
            current->value = (char *)malloc(strlen(value) + 1);
            if (current->value == NULL) {
                fprintf(stderr, "Failed to allocate memory for value.\n");
                exit(EXIT_FAILURE);
            }
            // Copy the new value
            strcpy(current->value, value);
            return;
        }
        current = current->next;
    }

    // Create a new node if NOT exist
    Node* newNode = (Node*)malloc(sizeof(Node));
    if (newNode == NULL) {
        fprintf(stderr, "Failed to allocate memory for new node.\n");
        exit(EXIT_FAILURE);
    }


    // Initialize the new node
    strncpy(newNode->key, key, sizeof(newNode->key) - 1);
    newNode->key[sizeof(newNode->key) - 1] = '\0';

    // Allocate memory for the value and copy it
    newNode->value = (char *)malloc(strlen(value) + 1);
    if (newNode->value == NULL) {
        fprintf(stderr, "Failed to allocate memory for value.\n");
        free(newNode);
        exit(EXIT_FAILURE);
    }
    strcpy(newNode->value, value);

    newNode->next = dict->head;

    // Insert the new node at the beginning of the list
    dict->head = newNode;
    dict->count++;
}

// Function to remove a key-value pair from the dictionary
void removeNode(Dictionary* dict, const char* key) {
    Node* current = dict->head;
    Node* prev = NULL;

    while (current != NULL) {
        if (strcmp(current->key, key) == 0) {
            if (prev == NULL) {
                // Node to be removed is the head
                dict->head = current->next;
            } else {
                // Node to be removed is in the middle or end
                prev->next = current->next;
            }
            free(current->value);
            free(current);
            dict->count--;
            return;
        }
        prev = current;
        current = current->next;
    }

    printf("Key '%s' not found.\n", key);
}

// Function to search for a value by key in the dictionary
char* searchNode(const Dictionary* dict, const char* key) {
    Node* current = dict->head;
    while (current != NULL) {
        if (strcmp(current->key, key)==0) {
            return current->value;
        }
        current = current->next;
    }
    return NULL;  // Key not found
}

// Function to check if a key exists in the dictionary
int isExist(const Dictionary* dict, const char* key) {
    Node* current = dict->head;
    while (current != NULL) {
        if (strcmp(current->key, key) == 0) {
            return 1;  // Key exists
        }
        current = current->next;
    }
    return 0;  // Key does not exist
}

// Function to free the dictionary
void freeDictionary(Dictionary* dict) {
    Node* current = dict->head;
    Node* next;
    while (current != NULL) {
        next = current->next;
        free(current->value);
        free(current);
        current = next;
    }
    dict->head = NULL;
    dict->count = 0;
}

// Function to print the dictionary
void printDictionary(const Dictionary* dict) {
    Node* current = dict->head;
    while (current != NULL) {
        printf("%s='%s'\n", current->key, current->value);
        current = current->next;
    }
}

typedef struct Job {
    int job_id;
    pid_t pid;
    char* command;
    struct Job* next;
} Job;

// Global job_list
Job* job_list = NULL;
int next_job_id = 1;

void add_job(pid_t pid, const char* command) {
    Job* job = (Job*)malloc(sizeof(Job));
    if(job == NULL){
        perror("malloc");
        exit(EXIT_FAILURE);
    }
    job->job_id = next_job_id++;
    job->pid = pid;
    job->command = strdup(command); //using malloc
    if(job->command == NULL){
        perror("malloc");
        free(job);
        exit(EXIT_FAILURE);
    }
    //job->next = job_list;
    //job_list = job;

    job->next = NULL;

    if (job_list == NULL) {
        job_list = job;
    } else {
        Job* current = job_list;
        while (current->next != NULL) {
            current = current->next;
        }
        current->next = job;
    }
}

void remove_job(pid_t pid) {
    Job* current = job_list;
    Job* previous = NULL;
    while (current != NULL && current->pid != pid) {
        previous = current;
        current = current->next;
    }
    if (current != NULL) {
        if (previous == NULL) {
            job_list = current->next;
        } else {
            previous->next = current->next;
        }
        free(current->command); // Free the stored command string
        free(current);
    }

    // Reset next_job_id if job_list is empty
    if (job_list == NULL) {
        next_job_id = 1;
    }
}

void print_jobs() {
    Job* current = job_list;
    while (current != NULL) {
        printf("[%d] %d               %s\n", current->job_id, current->pid, current->command);
        current = current->next;
    }
}
//Global Var for Succeeded command
int succeededCMD = 0;
int hasApos(char* str);
char** split_string(const char *str, int *count);
void free_split_string(char **str_array);
int checkForAlias(char* input, Dictionary* dict);

void execute_source_script(const char* filename, Dictionary* dict, int* scriptLine, int* aposCounter);
void execute_general(char* input ,Dictionary* dict, int *aposCounter);
int findEndFile (const char* filename);
int check_logic_op(char** arr);
void execute_logical_operator(char** arr, Dictionary* dict , int count, int* aposCounter);
char* separate_command(char** arr , int* counter_per_command);
char* check_redirect(char** arr, int count);
void redirect_stderr (const char* fileName, int* prevDupVal);
char* separate_befor_2arrow(char** arr);

void sigHandler(int sig) {
    pid_t pid;
    int status;

    while ((pid = waitpid(-1, &status, WNOHANG)) > 0) {
        if (WIFEXITED(status) && WEXITSTATUS(status) == 0) {
            // Check if pid is in job_list
            Job* current = job_list;
            while (current != NULL)
            {
                if (current->pid == pid) {
                    succeededCMD++;
                    // remove_job(pid);
                    break;
                }
                current = current->next;
            }
        }
        remove_job(pid);
    }
}
int main() {
    Dictionary dict;
    initDictionary(&dict);

    int aposCounter = 0;
    int scriptLine = 0,activeAlias;

    int prevDupVal = -1;


    char input[1024];

    signal( SIGCHLD,sigHandler);

    while (1) {
        activeAlias = dict.count;

        //prompt
        printf("#cmd:%d|#alias:%d|#script lines:%d> ", succeededCMD, activeAlias, scriptLine);

        if (fgets(input, sizeof(input), stdin) == NULL) {
            //printf("Error reading input or end-of-file reached.\n");
            fprintf(stderr, "ERR\n");
            exit(1);
        }

        if (strlen(input) >= 1024) {
            //printf("more than 1024 chars\n");
            fprintf(stderr, "ERR");
            exit(1);
        }

        size_t len = strlen(input);
        if (len > 0 && input[len - 1] == '\n') {
            input[len - 1] = '\0';
        }

//        if (strcmp(input, "") == 0)
//            continue;

        if (strcmp(input, "exit_shell") == 0) {
            //printf("Exiting_shell.\n");
            printf("%d\n", aposCounter);
            break;
        }

        // Check for source command
        int count = 0;
        char** arr = split_string(input, &count);

        char* fileName = check_redirect(arr,count);
        if( fileName!= NULL){
            redirect_stderr(fileName, &prevDupVal);
            char* tempCommand = separate_befor_2arrow(arr);
            strcpy(input,tempCommand);
            free_split_string(arr);
            arr = split_string(tempCommand,&count);
            free(tempCommand);
        }
        else{
            if (prevDupVal != -1){
                if(dup2(prevDupVal, STDERR_FILENO) == -1){
                    perror("dup2");
                    exit(1);
                }
                close(prevDupVal);
                prevDupVal = -1; // Reset to indicate no redirection
            }
        }

        if(count == 0)
            continue;

        /**
        add this to check if try to exec command that start with 'source' - Illegal command
        check if short-cut exist and send it to the source-execute function to handle it right
         **/
//        if(isExist(&dict, arr[0]) && strcmp(searchNode(&dict, arr[0]) , "source") == 0){
//            execute_source_script(arr[1] , &dict , &successCom, &scriptLine, &aposCounter);
//            free_split_string(arr);
//            continue;
//        }

        if ((isExist(&dict, arr[0]) && strcmp(searchNode(&dict, arr[0]) , "source") == 0) || strcmp("source", arr[0]) == 0) {
            execute_source_script(arr[1], &dict, &scriptLine, &aposCounter);
            free_split_string(arr);

            continue;
        }

        // Execute general commands
        execute_general(input, &dict, &aposCounter);
        free_split_string(arr);
    }

    freeDictionary(&dict);
    return 0;
}

// Checks if a string contains an apostrophe
int hasApos(char* str){
    char* ptr = str;
    while(*ptr){
        if(*ptr == '"' || *ptr == '\'')
            return 1;
        ptr++;
    }
    return 0;
}


// Splits a string into an array of strings based on spaces
char** split_string(const char *str, int *count) {
    // Count the number of tokens
    int token_count = 0;
    const char *ptr = str;
    while (*ptr) {
        while (*ptr && *ptr == ' ') {
            ptr++; // Skip spaces
        }
        if (*ptr) {
            token_count++;
            // Check for quoted strings
            if (*ptr == '"' || *ptr == '\'') {
                char quote = *ptr;
                ptr++;
                while (*ptr && *ptr != quote) {
                    ptr++;
                }
                if (*ptr) {
                    ptr++; // Skip closing quote
                }
            } else {
                while (*ptr && *ptr != ' ') {
                    ptr++; // Skip non-spaces
                }
            }
        }
    }

    // Allocate memory for the array of strings
    char **result = (char **)malloc((token_count + 1) * sizeof(char*));
    if (result == NULL) {
        perror("malloc");
        exit(EXIT_FAILURE);
    }

    // Split the string and fill the array
    ptr = str;
    int i = 0;
    while (*ptr != '\0') {
        while (*ptr && *ptr == ' ') {
            ptr++; // Skip spaces
        }
        if (*ptr != '\0') {
            const char *start = ptr;
            size_t length;
            if (*ptr == '"' || *ptr == '\'') {
                char quote = *ptr;
                ptr++;
                start = ptr; //update the start position to start after the first quote
                while (*ptr && *ptr != quote) {
                    ptr++;
                }
                if(*ptr == quote)
                    length = ptr - start; // update length of the argument in side the quotes
                else{
                    start--;
                    length = ptr - start;
//                    printf("ERR\n");              /** ????????????????????????????????????? */
//                    free_split_string(result);    /** ????????????????????????????????????? */
//                    return NULL;                  /** ????????????????????????????????????? */
                }
                if (*ptr) {
                    ptr++; // Skip closing quote
                }
            }
            else {
                while (*ptr && *ptr != ' ') {
                    ptr++; // Move to the end of the substring
                }
                length = ptr - start;
            }

            result[i] = (char *)malloc((length + 1) * sizeof(char));
            if (result[i] == NULL) {
                perror("malloc");
                // Free previously allocated memory on error
                for (int j = 0; j < i; j++) {
                    free(result[j]);
                }
                free(result);
                exit(EXIT_FAILURE);
            }
            strncpy(result[i], start, length); // Copy the substring
            result[i][length] = '\0'; // Null-terminate the string
            i++;
        }
    }
    result[token_count] = NULL; // Null-terminate the array

    // Set the count if the caller provided a pointer for it
    if (count != NULL) {
        *count = token_count;
    }
    return result;
}

// Function to free the array of strings
void free_split_string(char **str_array) {
    if (str_array == NULL) {
        return;
    }
    for (int i = 0; str_array[i] != NULL; i++) {
        free(str_array[i]);
    }
    free(str_array);
}

// Processes an alias command and inserts it into the dictionary
int checkForAlias(char* input, Dictionary* dict){
    int appear =0 , counter = 0;
    int countShcut =0 , eqflag =0;
    int countChars = 0 , counterCharsBeforeEquals =0 , flagBeforeEquals=0;

    if( strcmp("alias" , input) ==0 ){
        printDictionary(dict);
        return 1;
    }

    else if(strncmp("alias" , input , 5) == 0){
        char* ptr = input;
        while(*ptr){
            if(countChars >= 6 && flagBeforeEquals == 0 && *ptr != ' ')
                counterCharsBeforeEquals++;

            if(*ptr == '='){
                eqflag=1;
                flagBeforeEquals =1;
            }
            if(eqflag == 0 && countChars >= 6 && *ptr != ' '){ // after the alias and one space
                countShcut++;
            }

            if(*ptr == '\'' && appear == 0){ // -> the sign \' is represented a single quote '
                appear++;

            }
            else if(*ptr != '\'' && appear == 1)
                counter++;
            else if(*ptr == '\'' && appear == 1) {
                appear++;
                break;
            }

            ptr++;
            countChars++;
        }

        if(eqflag == 0 || appear != 2 || counterCharsBeforeEquals ==0) {
            //printf("Illegal command\n");
            fprintf(stderr, "ERR\n");
            return 0;
        }

        // add 1 to the counter to include the  '\0'
        char* temp = (char *)malloc((counter+1) * sizeof(char));
        char* shortCut = (char *)malloc((countShcut+1) * sizeof(char));
        if(temp == NULL || shortCut == NULL){
            perror("malloc");
            free(temp);
            free(shortCut);
            exit(1);
        }

        ptr = input;
        appear=0;
        eqflag = 0 , countChars=0;
        int i =0 , k=0;
        while(*ptr){
            if(*ptr == '='){
                eqflag=1;
            }
            if(eqflag == 0 && countChars >= 6 && *ptr != ' '){ // after the alias and one space
                shortCut[k] = *ptr;
                k++;
            }

            if(*ptr == '\'' && appear == 0){
                appear++;
            }
            else if(*ptr != '\'' && appear == 1) {
                temp[i] = *ptr;
                i++;
            }
            else if(*ptr == '\'' && appear == 1)
                break;
            ptr++;
            countChars++;
        }
        temp[counter] = '\0';
        shortCut[countShcut] = '\0';

        int countCheck =0;
        char **checking = split_string(temp,&countCheck);

        if( ( strcmp(checking[0] , "echo") != 0)  && countCheck >4 ) {
            //printf("try creating shortcut to more than 4 arguments\n");
            fprintf(stderr, "ERR\n");
            // Free the allocated memory
            free(temp);
            free(shortCut);
            free_split_string(checking);
            return 0 ; // Do not exit, just return to continue the shell operation
//            exit(1);
        }
        addNode(dict, shortCut, temp);
        free(temp); // Freeing temp as addNode duplicates it
        free(shortCut);
        free_split_string(checking);
    }
    else if (strncmp("unalias" , input , 7) == 0){
        char rem [50];
        int j=0;
        for (int i = 8; i < 50 && input[i] != '\0'; i++, j++) {
            rem[j]=input[i];
        }
        rem[j]='\0';
        removeNode(dict,rem);
    }
    return 1;
}

void execute_general(char* input ,Dictionary* dict, int *aposCounter){
    int count;
//    int backgroundThreadsCounter = 0;
    char strInput[1024];

    /**
     * check if the command end with & and deal with it in the parent process
     */
    int background = 0;
    if(input[strlen(input)-1] == '&'){
        background = 1;
        strcpy(strInput, input);
//        backgroundThreadsCounter++;
        input[strlen(input)-1] = '\0';
    }

    char** arr = split_string(input,&count);

    // check for 2> operator and handle it in separate function

    //char* fileName = check_redirect(arr,count);

    /*if( fileName!= NULL){
        redirect_stderr(fileName, prevDupVal);
        char* tempCommand = separate_befor_2arrow(arr);
        free_split_string(arr);
        arr = split_string(tempCommand,&count);
        free(tempCommand);
    }
    else{
        if(dup2((*prevDupVal), STDERR_FILENO) == -1){
            perror("dup2");
            exit(1);
        }
    }*/

    if(strcmp(input, "jobs")==0){
        succeededCMD++;
        print_jobs();
        free_split_string(arr);
        return;
    }

    // Check for alias / unalias
    if (strcmp("alias", arr[0]) == 0 || strcmp("unalias", arr[0]) == 0) {
        if (checkForAlias(input, dict) == 1) {
            succeededCMD++;
            if (hasApos(input)) {
                (*aposCounter)++;
            }
        }

        free_split_string(arr);
        return;
    }

    // Handle alias expansion
    if (isExist(dict, arr[0])) {
        char *aliasCommand = searchNode(dict, arr[0]);

        // Split the alias command
        int aliasCount;
        char **aliasArr = split_string(aliasCommand, &aliasCount);

        // Combine alias command with original arguments
        char **newArr = (char **)malloc((aliasCount + count) * sizeof(char *));
        if (newArr == NULL) {
            //perror("Failed to allocate memory for newArr");
            perror("malloc");
            free_split_string(aliasArr);
            free_split_string(arr);
            exit(EXIT_FAILURE);
        }

        // Copy alias command parts
        for (int i = 0; i < aliasCount; i++) {
            newArr[i] = strdup(aliasArr[i]);
            if (newArr[i] == NULL) {
//                perror("Failed to allocate memory for newArr element");
                perror("malloc");
                for (int j = 0; j < i; j++) free(newArr[j]);
                free(newArr);
                free_split_string(aliasArr);
                free_split_string(arr);
                exit(EXIT_FAILURE);
            }
        }

        // Copy original arguments, skipping the alias itself
        for (int i = 1; i < count; i++) {
            newArr[aliasCount + i - 1] = strdup(arr[i]);
            if (newArr[aliasCount + i - 1] == NULL) {
//                perror("Failed to allocate memory for newArr element");
                perror("malloc");
                for (int j = 0; j < aliasCount + i - 1; j++) free(newArr[j]);
                free(newArr);
                free_split_string(aliasArr);
                free_split_string(arr);
                exit(EXIT_FAILURE);
            }
        }
        newArr[aliasCount + count - 1] = NULL;


        // Clean up old arrays
        free_split_string(arr);
        free_split_string(aliasArr);

        arr = newArr;
        count = aliasCount + count - 1;
    }
    if(check_logic_op(arr) == 1)
        execute_logical_operator(arr,dict,count,aposCounter);
    else{
        pid_t pid = fork();
        int status;
        if (pid == -1) {
            perror("fork");
            free_split_string(arr);
            exit(1);
        }

        if (pid == 0) {
            // Child process

            // Check for command argument limits (except for 'echo')
            if (strcmp(arr[0], "echo") != 0 && count >= 6) {
//            printf("Error: command has more than 4 arguments\n");
                //printf("ERR\n");

                fprintf(stderr, "ERR\n");
                free_split_string(arr);
                //exit(1);
                return;
            }

            // Execute the command
            execvp(arr[0], arr);
            // If execvp fails
//        perror("execvp failed");
            perror("exec");
            //free_split_string(arr);
            //exit(1);
            _exit(EXIT_FAILURE);    //has to change to _exit instead exit
        }
        else {
            if(!background) {
                // Parent process
                waitpid(pid, &status, 0);
                if (WIFEXITED(status) && WEXITSTATUS(status) == 0) {
                    succeededCMD++;
                    if (hasApos(input)) {
                        (*aposCounter)++;
                    }
                }
            }
            else{
                add_job(pid,strInput);
                printf("[%d] %d\n", next_job_id-1, pid);
            }
            //printf("child process exit code: %d\n", WEXITSTATUS(status));
        }

    }
    free_split_string(arr);
}

void execute_source_script(const char* filename, Dictionary* dict, int* scriptLine, int* aposCounter) {
    if(findEndFile(filename) == 0){
        fprintf(stderr, "ERR\n"); // end of file is nor .sh
        return;
    }
    FILE* file = fopen(filename, "r");
    if (file == NULL) {
        perror("Error opening script file");
        return;
    }

    succeededCMD++;  // Count the source command itself as successful
    //int savingMyCmd = *successCom; // saving the value of success cmd before reading the file

    char line[1024];
    while (fgets(line, sizeof(line), file) != NULL) {
        (*scriptLine)++;    //increment any line script

        if((*scriptLine) == 0 && strcmp(line, "#!/bin/bash\n") != 0) {
            fprintf(stderr,"ERR\n");
            fclose(file);
            return;
        }
        if(line[0] == '#' || line[0] == '\0' || line[0] == '\n') {
            //if(line[0] == '#')
            //(*scriptLine)++;
            continue;
        }
        // Remove the newline character if present
        size_t len = strlen(line);
        if (len > 0 && line[len - 1] == '\n') {
            line[len - 1] = '\0';
        }

        //(*scriptLine)++;
        // Execute the command
        execute_general(line, dict, aposCounter);

    }
    // put back the value that saved before reading the file
    //*successCom = savingMyCmd;

    if(line[0] == '\n')
        (*scriptLine)++;

    fclose(file);
}

int findEndFile (const char* filename){
    if (filename == NULL) {
        return 0;
    }

    size_t len = strlen(filename);
    if (len >= 3 && strcmp(filename + len - 3, ".sh") == 0) {
        return 1;
    }

    return 0;
}

char* separate_command(char** arr , int* counter_per_command){
    int i =0;
    int totalLenCommand =0;
    //count how many argument for the command
    while(arr[i] != NULL && strcmp(arr[i], "&&") !=0 && strcmp(arr[i],"||") != 0 && strcmp(arr[i],"2>") != 0){
        totalLenCommand += strlen(arr[i]) + 1;
        (*counter_per_command += 1);
        i++;
    }
    char* argv = (char*)malloc(sizeof(char) * totalLenCommand +1);
    if(argv == NULL){
        perror("malloc");
        free(argv);
        exit(EXIT_FAILURE);
    }

    i=0;
    while(arr[i] != NULL && strcmp(arr[i], "&&") !=0 && strcmp(arr[i],"||") != 0){
        if(i==0)
            strcpy(argv,arr[i]);
        else
            strcat(argv,arr[i]);
        strcat(argv," ");
        i++;
    }
    argv[totalLenCommand] ='\0';
    return argv;
}

void execute_logical_operator(char** arr, Dictionary* dict , int count, int* aposCounter) {
    int i = 0;
    int prevSuccess = succeededCMD;
    while (i < count) {
        // int prevSuccess = succeededCMD;
        int counter_per_command =0;
        char* cmd = separate_command(arr + i , &counter_per_command);
        execute_general(cmd, dict,aposCounter);
        free(cmd);

         if(i+counter_per_command < count) {
            if (counter_per_command < count && strcmp(arr[counter_per_command+i], "&&") == 0 && prevSuccess == succeededCMD) {
                // If the previous command failed and operator is &&
                return;
            }
            else if (counter_per_command < count && strcmp(arr[i+counter_per_command], "||") == 0 && prevSuccess != succeededCMD) {
                // If the previous command succeeded and operator is ||
                return;
            }
         }

        i += counter_per_command +1;  // Increment to the next command
        prevSuccess = succeededCMD;
    }
}

int check_logic_op(char** arr){
    int i = 0;
    while (arr[i] !=NULL){
        if(strcmp(arr[i] , "&&") == 0  || strcmp(arr[i] , "||") == 0)
            return 1;
        i++;
    }
    return 0;
}

char* separate_befor_2arrow(char** arr){
    int i =0;
    int totalLenCommand =0;
    int flag =0;
    //count how many argument for the command
    while(arr[i] != NULL && strcmp(arr[i],"2>") != 0){
        totalLenCommand += strlen(arr[i]) + 1;
        i++;
    }
    if(arr[0][0] =='(' && arr[i-1][strlen(arr[i-1])-1] ==')'){
        totalLenCommand -= 2;
        flag =1;
    }
    char* argv = (char*)malloc(sizeof(char) * totalLenCommand +1);
    if(argv == NULL){
        perror("malloc");
        free(argv);
        exit(EXIT_FAILURE);
    }

    i=0;
    argv[0] = '\0';
    while(arr[i] != NULL && strcmp(arr[i],"2>")){
        if(i==0)
            if(flag == 1)
                strcat(argv, arr[i]+1);
            else
                strcat(argv,arr[i]);
        else
            strcat(argv,arr[i]);

        if (arr[i+1] != NULL && strcmp(arr[i+1], "2>") != 0) {
            strcat(argv, " "); // Add space between arguments
        }
        i++;
    }
    if(flag == 1)
        argv[totalLenCommand-1] ='\0';
    else
        argv[totalLenCommand] ='\0';
    return argv;
}

void redirect_stderr (const char* fileName, int* prevDupVal){

    // fd = open(fileName, O_WRONLY | O_CREAT | O_APPEND , S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
    int fd = open(fileName, O_WRONLY | O_CREAT | O_APPEND, 0664);
    if(fd == -1){
        perror("ERR failed open fd file");
        exit(1);
    }
    // Save current stderr
    *prevDupVal = dup(STDERR_FILENO);
    if (*prevDupVal == -1) {
        perror("dup");
        exit(1);
    }

    // Redirect stderr to the file
    if (dup2(fd, STDERR_FILENO) == -1) {
        perror("dup2");
        exit(1);
    }
    close(fd);
}

char* check_redirect(char** arr, int count){
    int i =0;
    while(i<count){
        if(strcmp(arr[i], "2>")==0 && i+1 <count)
            return arr[i+1];
        i++;
    }
    return NULL;
}