#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>
#include <ctype.h>

#ifdef WIN32
  #include <conio.h>
  #include <windows.h>

  #define BACKSPACE 8
  #define NEWLINE 13

  void sleep(int seconds) {
    Sleep(seconds * 1000);
  }
#else
  #include <unistd.h>
  #include <termios.h>

  #define BACKSPACE 127
  #define NEWLINE 10

  int getch() {
    static struct termios old, current;

    tcgetattr(0, &old);
    current = old;
    current.c_lflag &= ~ICANON; // Disable buffered I/O
    current.c_lflag &= ~ECHO; // Disable echo
    tcsetattr(0, TCSANOW, &current);

    int key = getchar();
    tcsetattr(0, TCSANOW, &old);
    return key;
  }
#endif

void move_cursor(int x, int y) {
  #ifdef WIN32
    HANDLE console_handle = GetStdHandle(STD_OUTPUT_HANDLE);

    PCONSOLE_SCREEN_BUFFER_INFO console_info;
    GetConsoleScreenBufferInfo(console_handle, console_info);

    COORD position;
    position.X = console_info->dwCursorPosition.X + x;
    position.Y = console_info->dwCursorPosition.Y + y;

    SetConsoleCursorPosition(console_handle, position);
  #else
    char x_direction, y_direction;

    if(x == 0 && y == 0) { return; }

    if(y < 0) {
      y_direction = 'A';
    } else {
      y_direction = 'B';
    }

    if(x < 0) {
      x_direction = 'D';
    } else {
      x_direction = 'C';
    }

    if(x == 0) {
      printf("\e[%i%c", abs(y), y_direction);
    } else if(y == 0) {
      printf("\e[%i%c", abs(x), x_direction);
    } else {    
      printf("\e[%i%c\e[%i%c", abs(x), x_direction, abs(y), y_direction);
    }
  #endif
}

int get_password(char* password) {
  int password_index = 0;

  while(1) {
    int key = getch();

    if(key == BACKSPACE) {
      if(password_index != 0) {
        password_index--;
        printf("\b \b");
      }
      continue;
    } else if(key == NEWLINE) {
      if(password_index < 8) {
        puts("\nPassword must be at least 8 characters long.");
        return -1;
      }
      putchar('\n');
      break;
    }

    putchar('*');
    password[password_index] = key;
    ++password_index;

    if(password_index == 21) {
      puts("\nPassword can not be longer than 20 characters.");
      return -1;
    }
  }

  password[password_index] = 0;
}

void receive_bank_account_info(char* name_or_id, char* password, int id) {
  while(1) {
    fgets(name_or_id, 23, stdin);

    if(id) {
      for(int index = 0; index < strlen(name_or_id)-1; ++index) {
        if(!isdigit(name_or_id[index])) {
          printf("Invalid ID entered.\nID:\nPassword:");
          move_cursor(-5, -1);
          break;
        }
      }
      if(name_or_id[1] == 0) {
        printf("No ID entered.\nID:\nPassword:");
        move_cursor(-5, -1);
      } else {
        break;
      }
    } else {
      if(strlen(name_or_id) > 21) {
        printf("Name can not be longer than 20 characters.\nName:\nPassword:");
        move_cursor(-3, -1);
      } else if(name_or_id[1] == 0) {
        printf("No name entered.\nName:\nPassword:");
        move_cursor(-3, -1);
      } else {
        break;
      }
    }
  }

  // Remove the trailing newline
  name_or_id[strlen(name_or_id) - 1] = 0;

  move_cursor(10, 0);

  while(get_password(password) == -1) { printf("Password: "); }
}

void register_new_account() {
  char name[23];
  char password[23];

  while(1) {
    puts("\nPlease enter your name and a password for your bank account.\n" \
         "Name:\nPassword:");

    move_cursor(6, -2);

    receive_bank_account_info(name, password, 0);

    char confirmation_password[23];

    printf("Please confirm your password: ");
    get_password(confirmation_password);

    if(strcmp(password, confirmation_password) == 0) {
      puts("Password does not match.");
    } else {
      break;
    }
  }

  FILE* file = fopen("accounts", "a+");

  // The length of the maximum number 340282366920938463463374607431768211455
  char line[39];

  // Skip to the last line
  while(fgets(line, 39, file)) { }

  int id = strtoll(line, NULL, 10) + 1;

  if(id == pow(2, 128) - 1) {
    puts("Error registering account. Too many accounts registered.\n");
    return;
  }

  fprintf(file, "%i\n%s\n%s\n0\n", id, password, name);

  fclose(file);

  printf("Account successfully registered!\n" \
         "Your ID is %i.\n", id);
  sleep(2);
  putchar('\n');
}

int sign_in() {
  char id[23];
  char password[23];

  puts("\nID:\nPassword:");

  move_cursor(4, -2);

  receive_bank_account_info(id, password, 1);

  FILE* file = fopen("accounts", "r");

  char line[39];
  int index = 0;
  int account_found = 0;

  while(fgets(line, 39, file)) {
    // Remove the trailing newline
    line[strlen(line) - 1] = 0;

    if(index == 0) {
      if(strcmp(line, id) == 0) {
        account_found = 1;
      }
    } else if(account_found) {
      if(strcmp(password, line) == 0) {
        puts("Logged in.\n");
        return 1;
      } else {
        puts("Incorrect password.\n");
        return 0;
      }
    } else if(index == 4) {
      index = 0;
    }
    ++index;
  }

  puts("ID not registered.");
  return 0;
}

void print_introduction() {
  puts("Welcome to the bank. Please select an option using the number keys.\n" \
       "1. Sign in\n" \
       "2. Register new account\n" \
       "3. Exit");
}

int main() {
  print_introduction();

  while(1) {
    int option = getch();

    if(option == 49) {
      if(sign_in()) { break; }
      print_introduction();
    } else if(option == 50) {
      register_new_account();
      print_introduction();
    } else if(option == 51) {
      return 0;
    }
  }
}
