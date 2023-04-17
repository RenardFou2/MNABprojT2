#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <unistd.h>

char* history[20];
int history_length = 0;

void exec_command(char** args, int background, int redir_fil_desc) {
    pid_t pid = fork();

    // child
    if (pid == 0) {
        
        // Sprawdza czy przekierowano std wyj
        if (redir_fil_desc != -1) {
            dup2(redir_fil_desc, STDOUT_FILENO);
            close(redir_fil_desc);
        }

        execvp(args[0], args); 
        perror("execvp");
        exit(EXIT_FAILURE);
    }

    // parent
    else if (pid > 0) {
        if (background == 0) {
            int status;
            waitpid(pid, &status, 0);
        }
    }
    else {
        perror("fork");
    }
}


void change_dir(char** args) {
    if (args[1] == NULL) { 
        chdir(getenv("HOME"));
    }
    else {
        if (chdir(args[1]) != 0) {
            perror("chdir");
        }
    }
}

void history_add(char* input) {
    
    if (history_length < 20) {
        history[history_length++] = strdup(input);
    }
    else {
        for (int i = 0; i < 19; i++) {
            history[i] = history[i + 1];
        }
        history[19] = strdup(input);
    }

    FILE* plik;
    plik = fopen("history.txt", "a");
    if (plik == NULL) {
        perror("fopen");
        return;
    }
    fprintf(plik, "%s\n", input);
    fclose(plik);
}


// wyœwietlenie historii po otrzymaniu SIGQUIT
void signal_handler(int sig) {
    if (sig == SIGQUIT) {
        for (int i = 0; i < history_length; i++) {
            printf("%d %s\n", i + 1, history[i]);
        }
    }
}

int main() 
{
    char input[128];
    char* args[64];
    char* redirect_output_file = NULL; // nazwa pliku, do którego przekierowaæ wyjœcie
    int bg = 0; // zmienna kontroluj¹ca, czy program ma byæ uruchomiony w tle
    
    signal(SIGQUIT, signal_handler); // obs³uga sygna³u SIGQUIT

    while (1) {
        
        //Wyœwietla wiersz poleceñ razem z aktualnym katalogiem
        char cwd[1024];
        getcwd(cwd, sizeof(cwd));
        printf("%s$ ", cwd);
        fflush(stdout);

        if (strlen(input) == 1) { // pusty wiersz
            continue;
        }

        input[strlen(input) - 1] = '\0'; // us. znaku nowej linii

        if (strcmp(input, "exit") == 0) {
            break;
        }
        
        history_add(input);

        // podzielenie inputu u¿ytkownika na tokeny
        char* token = strtok(input, " ");
        int i = 0;
        while (token != NULL) {
            
            // prog. uruchomiony w tle
            if (strcmp(token, "&") == 0) {
                bg = 1;
                break;
            }
            // przekierowanie std wyj.
            else if (strcmp(token, ">") == 0) {
                redirect_output_file = strtok(NULL, " ");
                break;
            }

            else {
                args[i++] = token;
            }
            token = strtok(NULL, " ");
        }
        args[i] = NULL;

        //Zmiana katalogu
        if (strcmp(args[0], "cd") == 0) {
            change_dir(args);
        }
        
        //Odpala program
        else { 
            
            //Deskryptor pliku w przypadku przekierowania
            int redir_fil_desc = -1;
            //Sprawdza czy wyjœcie zosta³o przekierowane
            if (redirect_output_file != NULL) {
                redir_fil_desc = open(redirect_output_file, O_CREAT | O_TRUNC | O_WRONLY);
                if (redir_fil_desc == -1) {
                    perror("open");
                    continue;
                }
            }
            exec_command(args, bg, redir_fil_desc);
            if (redir_fil_desc != -1) {
                close(redir_fil_desc);
            }
        }

        // resetowanie zmiennych
        redirect_output_file = NULL;
        bg = 0;
        for (int j = 0; j < 64; j++) {
            args[j] = NULL;
        }
    }
    return 0;
}