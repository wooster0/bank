#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>
#include <ctype.h>

// To do list:
// - Maybe don't store IDs in the accounts file. Every 3 lines, add 1 to id variable.
// - Currently fgets overloading (giving too much input) is not handled.

#ifdef WIN32
  #include <conio.h>
  #include <windows.h>
  #include <io.h>

  #define BACKSPACE 8
  #define NEWLINE 13

  void sleep(int seconds) {
    Sleep(seconds * 1000);
  }

  HANDLE console_handle;
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

char account_id[23];
char account_password[23];
char account_name[20];
char account_balance[20];

void move_cursor(int x, int y) {
  #ifdef WIN32
    CONSOLE_SCREEN_BUFFER_INFO console_info;
    GetConsoleScreenBufferInfo(console_handle, &console_info);

    COORD position;
    position.X = console_info.dwCursorPosition.X + x;
    position.Y = console_info.dwCursorPosition.Y + y;

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
    retry:

    fgets(name_or_id, 23, stdin);

    if(id) {
      for(int index = 0; index < strlen(name_or_id)-1; ++index) {
        if(!isdigit(name_or_id[index])) {
          printf("Invalid ID entered.\nID:\nPassword:");
          move_cursor(-5, -1);
          goto retry;
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

    if(strcmp(password, confirmation_password) != 0) {
      puts("Password does not match.");
    } else {
      break;
    }
  }

  FILE* file = fopen("accounts", "a+b");

  int character;
  int lines = 0;

  while((character = getc(file)) != EOF) {
    if(character == '\n') { ++lines; }
  }

  int id = lines / 4 + 1;

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
  #ifdef WIN32
    if(_access_s("accounts", 0)) {
  #else
    if(access("accounts", F_OK)) {
  #endif
    puts("\nNo accounts registered.\n");
    return 0;
  }

  #ifdef WIN32
    if(_access_s("accounts", 4)) {
  #else
    if(access("accounts", R_OK)) {
  #endif
    puts("\nAccount database can not be read.");
    return 0;
  }

  puts("\nID:\nPassword:");

  move_cursor(4, -2);

  receive_bank_account_info(account_id, account_password, 1);

  FILE* file = fopen("accounts", "r");

  // The length of the maximum number 340282366920938463463374607431768211455 that could be in the file
  char line[39];
  int index = 0;
  int id_found = 0;
  int logged_in = 0;

  while(fgets(line, 39, file)) {
    // Remove the trailing newline
    line[strlen(line) - 1] = 0;

    if(logged_in) {
      if(index == 2) {
        strcpy(account_name, line);
      } else if(index == 3) {
        strcpy(account_balance, line);
        puts("Logged in.\n");
        return 1;
      }
    }

    if(index == 0) {
      if(strcmp(line, account_id) == 0) {
        id_found = 1;
      }
    } else if(id_found) {
      if(strcmp(line, account_password) == 0) {
        logged_in = 1;
        id_found = 0;
      } else {
        puts("Incorrect password.\n");
        return 0;
      }
    }

    if(index == 3) {
      index = 0;
    } else {
      ++index;
    }
  }

  puts("ID not registered.\n");
  return 0;
}

void print_introduction() {
  puts("Welcome to the bank. Please select an option using the number keys.\n" \
       "1. Sign in\n" \
       "2. Register new account\n" \
       "3. Exit");
}

void show_second_menu() {
  puts("Use the number keys to select an option.\n" \
       "1. Withdraw money\n" \
       "2. Deposit money\n" \
       "3. Account information\n" \
       "4. Log out");
}

void make_transaction(int withdraw) {
  FILE* file = fopen("accounts", "r");

  // Find out the file size by seeking to the end
  fseek(file, 0, SEEK_END);
  long file_size = ftell(file);
  rewind(file);

  char* buffer = malloc(file_size);

  if(buffer == NULL) {
    puts("\nOut of memory.\n");
    return;
  }

  size_t bytes_read = fread(buffer, 1, file_size, file);

  if(bytes_read != file_size) {
    puts("\nError reading file.\n");
    free(buffer);
    return;
  }

  fclose(file);

  int id_index = 0;
  int lines = 0;
  int id_found = 0;
  char character;

  // Advance index to the balance position in the file
  int index = 0;
  for(;index < file_size; ++index) {
    if((character = buffer[index]) == '\n') {
      ++lines;
    }

    if(id_found && lines == 2) {
      while(buffer[index] != '\n') { ++index; }
      ++index;
      break;
    }

    if(lines == 0 && !id_found) {
      id_found = 1;
      while(1) {
        if(account_id[id_index] != character) {
          id_found = 0;
          break;
        }
        ++id_index;
        ++index;
        character = buffer[index];
        if(character == '\n') { break; }
      }
      id_index = 0;
    } else if(lines == 3) {
      lines = 0;
    }
  }

  while(1) {
    if(withdraw) {
      printf("\nHow much would you like to withdraw from %s? ", account_balance);
    } else {
      printf("\nHow much would you like to deposit? ", account_balance);
    }

    char amount_string[5];
    fgets(amount_string, 5, stdin);

    if(amount_string[1] == 0) {
      puts("No amount entered.");
      continue;
    }

    // Skip the trailing newline
    int input_length = strlen(amount_string) - 1;

    int invalid_amount_entered = 0;
    for(int index = 0; index < input_length; ++index) {
      if(!isdigit(amount_string[index])) {
        puts("Invalid amount entered.");
        invalid_amount_entered = 1;
        break;
      }
    }
    if(invalid_amount_entered) { continue; }

    if(input_length > 3) {
      if(withdraw) {
        puts("Cannot withdraw more than 500 at once.");
      } else {
        puts("Cannot deposit more than 500 at once.");
      }
      continue;
    }

    int amount = strtol(amount_string, NULL, 10);

    if(amount > 500) {
      if(withdraw) {
        puts("Cannot withdraw more than 500 at once.");
      } else {
        puts("Cannot deposit more than 500 at once.");
      }
      continue;
    }

    int new_balance;
    if(withdraw) {
      new_balance = strtol(account_balance, NULL, 10) - amount;
    } else {
      new_balance = strtol(account_balance, NULL, 10) + amount;
    }

    if(withdraw && new_balance < 0) {
      puts("Your account has insufficient funds for this transaction.");
      continue;
    }

    char new_balance_string[3];
    sprintf(new_balance_string, "%i", new_balance);

    strcpy(account_balance, new_balance_string);

    // TODO: Give i a better name
    for(int i = 0; i < strlen(new_balance_string); ++i) {
      buffer[index] = new_balance_string[i];
      ++index;
    }

    FILE* filee = fopen("accounts", "w+b");

    size_t bytes_written = fwrite(buffer, 1, index, filee);

    if(bytes_written != index) {
      puts("Account database has been updated incorrectly. Deleting database.");
      remove("accounts");
    }

    fclose(filee);
    free(buffer);

    printf("Your new account balance is %s.\n\n", new_balance_string);
    return;
  }
}

int main() {
  #ifdef WIN32
    console_handle = GetStdHandle(STD_OUTPUT_HANDLE);
  #endif

  start:

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

  show_second_menu();

  while(1) {
    int option = getch();

    if(option == 49) {
      make_transaction(1);
      show_second_menu();
    } else if(option == 50) {
      make_transaction(0);
      show_second_menu();
    } else if(option == 51) {
      printf("\nName: %s\n" \
             "Balance: %s\n" \
             "Account ID: %s\n\n", account_name, account_balance, account_id);
      show_second_menu();
    } else if(option == 52) {
      putchar('\n');
      goto start;
    }
  }
}
