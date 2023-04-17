#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <signal.h>

char* history[20]; // tablica przechowuj¹ca historiê poleceñ
int history_count = 0; // licznik poleceñ w historii

void execute_command(char** args, int background, int output_fd) {
    pid_t pid = fork();

    // proces potomny
    if (pid == 0) {
        
        // Sprawdza czy przekierowano std wyj
        if (output_fd != -1) {
            dup2(output_fd, STDOUT_FILENO);
            close(output_fd);
        }

        execvp(args[0], args); 
        perror("execvp");
        exit(EXIT_FAILURE);
    }
    
    // proces rodzicielski
    else if (pid > 0) {
        if (background == 0) { // czekanie na zakoñczenie procesu
            int status;
            waitpid(pid, &status, 0);
        }
    }
    else { // b³¹d podczas tworzenia procesu
        perror("fork");
    }
}


void change_directory(char** args) {
    if (args[1] == NULL) { 
        chdir(getenv("HOME"));
    }
    else {
        if (chdir(args[1]) != 0) {
            perror("chdir");
        }
    }
}

void add_to_history(char* command) {
    
    if (history_count < 20) {
        history[history_count++] = strdup(command);
    }
    else {
        free(history[0]);
        for (int i = 0; i < 19; i++) {
            history[i] = history[i + 1];
        }
        history[19] = strdup(command);
    }

    FILE* plik;
    plik = fopen("history.txt", "a");
    if (plik == NULL) {
        perror("fopen");
        return;
    }
    fprintf(plik, "%s\n", command);
    fclose(plik);
}


// wyœwietlenie historii poleceñ po otrzymaniu sygna³u SIGQUIT
void handle_signal(int sig) {
    if (sig == SIGQUIT) {
        for (int i = 0; i < history_count; i++) {
            printf("%d %s\n", i + 1, history[i]);
        }
    }
}

int main() 
{
    char input[128]; // bufor na wejœcie u¿ytkownika
    char* args[64]; // tablica na argumenty polecenia
    char* redirect_output = NULL; // nazwa pliku, do którego przekierowaæ wyjœcie
    int background = 0; // czy uruchomiæ program w tle
    
    signal(SIGQUIT, handle_signal); // obs³uga sygna³u SIGQUIT

    while (1) {
        
        //Wyœwietla wiersz poleceñ razem z aktualnym katalogiem
        char cwd[1024];
        getcwd(cwd, sizeof(cwd));
        printf("%s$ ", cwd);
        fflush(stdout);

        if (strlen(input) == 1) { // pusty wiersz
            continue;
        }

        input[strlen(input) - 1] = '\0'; // usuniêcie znaku nowej linii z koñca wejœcia

        if (strcmp(input, "exit") == 0) { // wyjœcie z pow³oki
            break;
        }
        
        // dodanie polecenia do historii
        add_to_history(input);

        // podzielenie wejœcia na tokeny
        char* token = strtok(input, " ");
        int i = 0;
        while (token != NULL) {
            
            // program ma byæ uruchomiony w tle
            if (strcmp(token, "&") == 0) {
                background = 1;
                break;
            }
            // przekierowanie wyjœcia
            else if (strcmp(token, ">") == 0) {
                redirect_output = strtok(NULL, " ");
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
            change_directory(args);
        }
        
        // uruchomienie programu
        else { 
            
            //Deskryptor pliku w przypadku przekierowania
            int output_fd = -1;
            //Sprawdza czy wyjœcie zosta³o przekierowane
            if (redirect_output != NULL) { 
                output_fd = open(redirect_output, O_CREAT | O_TRUNC | O_WRONLY);
                if (output_fd == -1) {
                    perror("open");
                    continue;
                }
            }
            execute_command(args, background, output_fd);
            if (output_fd != -1) {
                close(output_fd);
            }
        }

        // resetowanie flag i wskaŸników
        redirect_output = NULL;
        background = 0;
        for (int j = 0; j < 64; j++) {
            args[j] = NULL;
        }
    }
    return 0;
}