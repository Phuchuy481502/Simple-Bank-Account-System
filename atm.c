#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_ACCOUNTS    5
#define MAX_HISTORY     20
#define PIN_LENGTH      4
#define MAX_ATTEMPTS    3

typedef struct {
    char description[50];
    double amount;
    double balance_after;
} Transaction;

typedef struct {
    int    id;
    char   name[50];
    char   pin[PIN_LENGTH + 1];
    double balance;
    Transaction history[MAX_HISTORY];
    int    history_count;
} Account;

static Account accounts[MAX_ACCOUNTS];
static int account_count = 0;

static char account_file_path[260] = "accounts.txt";

/* Build path to accounts.txt relative to the executable */
void build_account_path(const char *exe_path) {
    strncpy(account_file_path, exe_path, sizeof(account_file_path) - 1);
    /* Find last slash or backslash */
    char *last_sep = strrchr(account_file_path, '\\');
    char *last_fwd = strrchr(account_file_path, '/');
    if (last_fwd && (!last_sep || last_fwd > last_sep))
        last_sep = last_fwd;
    if (last_sep)
        strcpy(last_sep + 1, "accounts.txt");
    else
        strcpy(account_file_path, "accounts.txt");
}

/* Load accounts from file */
int load_accounts(void) {
    FILE *fp = fopen(account_file_path, "r");
    if (!fp) {
        printf("Error: Cannot open %s\n", account_file_path);
        return 0;
    }

    fscanf(fp, "%d", &account_count);
    for (int i = 0; i < account_count && i < MAX_ACCOUNTS; i++) {
        fscanf(fp, "%d %s %s %lf",
               &accounts[i].id,
               accounts[i].name,
               accounts[i].pin,
               &accounts[i].balance);
        accounts[i].history_count = 0;

        /* Convert underscores to spaces in name */
        for (int j = 0; accounts[i].name[j]; j++) {
            if (accounts[i].name[j] == '_')
                accounts[i].name[j] = ' ';
        }
    }

    fclose(fp);
    return 1;
}

/* Save accounts to file */
void save_accounts(void) {
    FILE *fp = fopen(account_file_path, "w");
    if (!fp) {
        printf("Error: Cannot write to %s\n", account_file_path);
        return;
    }

    fprintf(fp, "%d\n", account_count);
    for (int i = 0; i < account_count; i++) {
        /* Convert spaces to underscores for saving */
        char name_copy[50];
        strncpy(name_copy, accounts[i].name, sizeof(name_copy) - 1);
        name_copy[sizeof(name_copy) - 1] = '\0';
        for (int j = 0; name_copy[j]; j++) {
            if (name_copy[j] == ' ')
                name_copy[j] = '_';
        }

        fprintf(fp, "%d %s %s %.1f\n",
                accounts[i].id,
                name_copy,
                accounts[i].pin,
                accounts[i].balance);
    }

    fclose(fp);
}

void print_line(void) {
    printf("========================================\n");
}

void add_transaction(Account *acc, const char *desc, double amount) {
    if (acc->history_count >= MAX_HISTORY) {
        for (int i = 0; i < MAX_HISTORY - 1; i++)
            acc->history[i] = acc->history[i + 1];
        acc->history_count--;
    }
    Transaction *t = &acc->history[acc->history_count++];
    strncpy(t->description, desc, sizeof(t->description) - 1);
    t->amount = amount;
    t->balance_after = acc->balance;
}

/* PIN authentication */
Account *authenticate(void) {
    int acc_id;
    char pin[20];

    printf("\n");
    print_line();
    printf("       ATM - Login\n");
    print_line();
    printf("  Account number: ");
    scanf("%d", &acc_id);

    Account *acc = NULL;
    for (int i = 0; i < account_count; i++) {
        if (accounts[i].id == acc_id) {
            acc = &accounts[i];
            break;
        }
    }

    if (!acc) {
        printf("Account not found!\n");
        return NULL;
    }

    for (int attempt = 1; attempt <= MAX_ATTEMPTS; attempt++) {
        printf("  Enter PIN (%d/%d): ", attempt, MAX_ATTEMPTS);
        scanf("%s", pin);

        if (strcmp(pin, acc->pin) == 0) {
            printf("  Authentication successful!\n");
            return acc;
        }
        printf("  Wrong PIN!\n");
    }

    printf("  Account locked after %d failed attempts.\n", MAX_ATTEMPTS);
    return NULL;
}

/* Check balance */
void check_balance(Account *acc) {
    print_line();
    printf("       Account Balance\n");
    print_line();
    printf("  Owner  : %s\n", acc->name);
    printf("  Account: %d\n", acc->id);
    printf("  Balance: %.0f VND\n", acc->balance);
    print_line();
}

/* Deposit */
void deposit(Account *acc) {
    double amount;
    print_line();
    printf("       Deposit\n");
    print_line();
    printf("  Current balance: %.0f VND\n", acc->balance);
    printf("  Enter amount: ");
    scanf("%lf", &amount);

    if (amount <= 0) {
        printf("  Invalid amount!\n");
        return;
    }

    acc->balance += amount;
    add_transaction(acc, "Deposit", amount);
    save_accounts();
    printf("  Deposited %.0f VND successfully\n", amount);
    printf("  New balance: %.0f VND\n", acc->balance);
}

/* Withdraw */
void withdraw(Account *acc) {
    double amount;
    print_line();
    printf("       Withdraw\n");
    print_line();
    printf("  Current balance: %.0f VND\n", acc->balance);
    printf("\n  Quick select:\n");
    printf("    1. 100,000 VND\n");
    printf("    2. 200,000 VND\n");
    printf("    3. 500,000 VND\n");
    printf("    4. 1,000,000 VND\n");
    printf("    5. Enter custom amount\n");
    printf("  Choice: ");

    int choice;
    scanf("%d", &choice);

    switch (choice) {
        case 1: amount = 100000;  break;
        case 2: amount = 200000;  break;
        case 3: amount = 500000;  break;
        case 4: amount = 1000000; break;
        case 5:
            printf("  Enter amount: ");
            scanf("%lf", &amount);
            break;
        default:
            printf("  Invalid choice!\n");
            return;
    }

    if (amount <= 0) {
        printf("  Invalid amount!\n");
        return;
    }
    if (amount > acc->balance) {
        printf("  Insufficient balance! (Need: %.0f | Have: %.0f)\n",
               amount, acc->balance);
        return;
    }

    acc->balance -= amount;
    add_transaction(acc, "Withdraw", -amount);
    save_accounts();
    printf("  Withdrew %.0f VND successfully\n", amount);
    printf("  New balance: %.0f VND\n", acc->balance);
}

/* Transfer */
void transfer(Account *acc) {
    int dest_id;
    double amount;

    print_line();
    printf("       Transfer\n");
    print_line();
    printf("  Current balance: %.0f VND\n", acc->balance);
    printf("  Recipient account number: ");
    scanf("%d", &dest_id);

    Account *dest = NULL;
    for (int i = 0; i < account_count; i++) {
        if (accounts[i].id == dest_id && accounts[i].id != acc->id) {
            dest = &accounts[i];
            break;
        }
    }

    if (!dest) {
        printf("  Recipient account not found!\n");
        return;
    }

    printf("  Recipient: %s\n", dest->name);
    printf("  Enter amount: ");
    scanf("%lf", &amount);

    if (amount <= 0) {
        printf("  Invalid amount!\n");
        return;
    }
    if (amount > acc->balance) {
        printf("  Insufficient balance!\n");
        return;
    }

    printf("\n  Confirm transfer %.0f VND to %s? (1=Yes, 0=No): ",
           amount, dest->name);
    int confirm;
    scanf("%d", &confirm);

    if (confirm != 1) {
        printf("  Transaction cancelled.\n");
        return;
    }

    acc->balance  -= amount;
    dest->balance += amount;

    char desc_send[50], desc_recv[50];
    snprintf(desc_send, sizeof(desc_send), "Transfer to %d", dest->id);
    snprintf(desc_recv, sizeof(desc_recv), "Received from %d", acc->id);
    add_transaction(acc,  desc_send, -amount);
    add_transaction(dest, desc_recv,  amount);
    save_accounts();

    printf("  Transferred %.0f VND successfully!\n", amount);
    printf("  New balance: %.0f VND\n", acc->balance);
}

/* Transaction history */
void show_history(Account *acc) {
    print_line();
    printf("       Transaction History\n");
    print_line();

    if (acc->history_count == 0) {
        printf("  No transactions yet.\n");
        print_line();
        return;
    }

    printf("  %-4s  %-20s  %12s  %12s\n", "#", "Description", "Amount", "Balance");
    printf("  ----  --------------------  ------------  ------------\n");

    for (int i = 0; i < acc->history_count; i++) {
        Transaction *t = &acc->history[i];
        printf("  %-4d  %-20s  %+12.0f  %12.0f\n",
               i + 1, t->description, t->amount, t->balance_after);
    }
    print_line();
}

/* Change PIN */
void change_pin(Account *acc) {
    char old_pin[20], new_pin[20], confirm_pin[20];

    print_line();
    printf("       Change PIN\n");
    print_line();

    printf("  Enter current PIN: ");
    scanf("%s", old_pin);
    if (strcmp(old_pin, acc->pin) != 0) {
        printf("  Wrong PIN!\n");
        return;
    }

    printf("  Enter new PIN (%d digits): ", PIN_LENGTH);
    scanf("%s", new_pin);
    if (strlen(new_pin) != PIN_LENGTH) {
        printf("  PIN must be %d digits!\n", PIN_LENGTH);
        return;
    }

    printf("  Confirm new PIN: ");
    scanf("%s", confirm_pin);
    if (strcmp(new_pin, confirm_pin) != 0) {
        printf("  PINs do not match!\n");
        return;
    }

    strncpy(acc->pin, new_pin, PIN_LENGTH);
    save_accounts();
    printf("  PIN changed successfully!\n");
}

/* Main menu */
void main_menu(Account *acc) {
    int choice;

    while (1) {
        printf("\n");
        print_line();
        printf("ATM - Main Menu\n");
        print_line();
        printf("  Welcome, %s!\n\n", acc->name);
        printf("  1. Check balance\n");
        printf("  2. Deposit\n");
        printf("  3. Withdraw\n");
        printf("  4. Transfer\n");
        printf("  5. Transaction history\n");
        printf("  6. Change PIN\n");
        printf("  0. Exit\n");
        print_line();
        printf("  Choice: ");
        scanf("%d", &choice);

        switch (choice) {
            case 1: check_balance(acc); break;
            case 2: deposit(acc);       break;
            case 3: withdraw(acc);      break;
            case 4: transfer(acc);      break;
            case 5: show_history(acc);  break;
            case 6: change_pin(acc);    break;
            case 0:
                printf("\n  Thank you for using our service!\n\n");
                return;
            default:
                printf("  Invalid choice!\n");
        }

        printf("\n  Press Enter to continue...");
        getchar(); getchar();
    }
}

int main(int argc, char *argv[]) {
    printf("\n  ATM Simulation v1.0\n");

    build_account_path(argv[0]);

    if (!load_accounts()) {
        printf("  Failed to load account data!\n");
        return 1;
    }

    printf("  Loaded %d accounts from %s\n", account_count, account_file_path);

    Account *acc = authenticate();
    if (acc) main_menu(acc);

    return 0;
}
