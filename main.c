#include <stdio.h>
#include <string.h>
#include <stdlib.h>

EXEC SQL INCLUDE sqlca;

int check_admin_status();
void admin_main_loop();
void guest_main_loop();
void handle_user_creation();
void handle_table_deletion();
void handle_table_clearing();
void handle_link_addition();
void handle_link_update();
void handle_link_deletion();
void handle_search();
void read_input(const char* prompt, char* buffer, size_t size);

int main() {
    EXEC SQL BEGIN DECLARE SECTION;
    char dbname[256] = "links_db";
    char username[256];
    char password[256];
    EXEC SQL END DECLARE SECTION;

    read_input("Enter username: ", username, sizeof(username));
    read_input("Enter password: ", password, sizeof(password));

    EXEC SQL CONNECT TO links_db@localhost USER :username USING :password;
    if (sqlca.sqlcode != 0) {
        fprintf(stderr, "Connection error: %s\n", sqlca.sqlerrm.sqlerrmc);
        return 1;
    }

    if (check_admin_status()) {
        admin_main_loop();
    } else {
        guest_main_loop();
    }

    EXEC SQL DISCONNECT;
    return 0;
}

void read_input(const char* prompt, char* buffer, size_t size) {
    printf("%s", prompt);
    fgets(buffer, size, stdin);
    buffer[strcspn(buffer, "\n")] = '\0';
}

int check_admin_status() {
    EXEC SQL BEGIN DECLARE SECTION;
    char is_admin[2];
    EXEC SQL END DECLARE SECTION;

    EXEC SQL SELECT is_admin() INTO :is_admin;
    return (is_admin[0] == 't') ? 1 : 0;
}

void admin_main_loop() {
    int choice;
    char buffer[1024];

    do {
        printf("\nAdmin Menu:\n");
        printf("1. Create User\n2. Drop Table\n3. Clear Table\n4. Add Link\n");
        printf("5. Update Link\n6. Delete by Alias\n7. Search\n0. Exit\n");

        read_input("Choice: ", buffer, sizeof(buffer));
        choice = atoi(buffer);

        switch(choice) {
            case 1: handle_user_creation(); break;
            case 2: handle_table_deletion(); break;
            case 3: handle_table_clearing(); break;
            case 4: handle_link_addition(); break;
            case 5: handle_link_update(); break;
            case 6: handle_link_deletion(); break;
            case 7: handle_search(); break;
            case 0: printf("Exiting...\n"); break;
            default: printf("Invalid choice!\n");
        }
    } while(choice != 0);
}

void guest_main_loop() {
    int choice;
    char buffer[1024];

    do {
        printf("\nGuest Menu:\n");
        printf("1. Search\n0. Exit\n");

        read_input("Choice: ", buffer, sizeof(buffer));
        choice = atoi(buffer);

        switch(choice) {
            case 1: handle_search(); break;
            case 0: printf("Exiting...\n"); break;
            default: printf("Invalid choice!\n");
        }
    } while(choice != 0);
}

void handle_user_creation() {
    EXEC SQL BEGIN DECLARE SECTION;
    char new_user[256], new_pass[256];
    int admin_flag;
    EXEC SQL END DECLARE SECTION;

    read_input("New username: ", new_user, sizeof(new_user));
    read_input("New password: ", new_pass, sizeof(new_pass));
    read_input("Is admin (1/0): ", (char*)&admin_flag, sizeof(admin_flag));

    EXEC SQL CALL create_user(:new_user, :new_pass, :admin_flag);
    EXEC SQL COMMIT;

    if(sqlca.sqlcode == 0) {
        printf("User created successfully.\n");
    } else {
        fprintf(stderr, "Error creating user: %s\n", sqlca.sqlerrm.sqlerrmc);
    }
}

void handle_table_deletion() {
    EXEC SQL CALL drop_table();
    EXEC SQL COMMIT;
    printf("Table dropped successfully.\n");
}

void handle_search() {
    EXEC SQL BEGIN DECLARE SECTION;
    char search_text[1024];
    int result_count;

    struct {
        long id;
        char original[1024];
        char alias[256];
        char created_at[64];
    } links[100];

    EXEC SQL END DECLARE SECTION;

    read_input("Search text: ", search_text, sizeof(search_text));

    EXEC SQL SELECT COUNT(*) INTO :result_count
    FROM search_by_original(:search_text);

    if (sqlca.sqlcode != 0) {
        fprintf(stderr, "Count error: %s\n", sqlca.sqlerrm.sqlerrmc);
        return;
    }

    if(result_count > 100) {
        printf("Showing first 100 of %d results\n", result_count);
        result_count = 100;
    }

    if(result_count > 0) {
        EXEC SQL SELECT * INTO :links
        FROM search_by_original(:search_text)
        LIMIT 100;

        if (sqlca.sqlcode != 0) {
            fprintf(stderr, "Search error: %s\n", sqlca.sqlerrm.sqlerrmc);
            return;
        }

        for(int i = 0; i < result_count; i++) {
            printf("\nID: %ld\nOriginal: %s\nAlias: %s\nCreated: %s\n",
                   links[i].id,
                   links[i].original,
                   links[i].alias,
                   links[i].created_at);
        }
    } else {
        printf("No results found.\n");
    }
}

void handle_table_clearing() {
    EXEC SQL CALL clear_table();
    EXEC SQL COMMIT;

    if(sqlca.sqlcode == 0) {
        printf("Table cleared successfully.\n");
    } else {
        fprintf(stderr, "Error clearing table: %s\n", sqlca.sqlerrm.sqlerrmc);
    }
}

void handle_link_update() {
    EXEC SQL BEGIN DECLARE SECTION;
    long link_id;
    char new_original[1024];
    char new_alias[256];
    char buffer[1024];
    EXEC SQL END DECLARE SECTION;

    read_input("Link ID: ", buffer, sizeof(buffer));
    link_id = atol(buffer);

    read_input("New Original URL: ", new_original, sizeof(new_original));
    read_input("New Alias: ", new_alias, sizeof(new_alias));

    EXEC SQL CALL update_link(:link_id, :new_original, :new_alias);
    EXEC SQL COMMIT;

    if(sqlca.sqlcode == 0) {
        printf("Link updated successfully.\n");
    } else {
        fprintf(stderr, "Error updating link: %s\n", sqlca.sqlerrm.sqlerrmc);
    }
}

void handle_link_deletion() {
    EXEC SQL BEGIN DECLARE SECTION;
    char delete_alias[256];
    EXEC SQL END DECLARE SECTION;

    read_input("Alias to delete: ", delete_alias, sizeof(delete_alias));

    EXEC SQL CALL delete_by_alias(:delete_alias);
    EXEC SQL COMMIT;

    if(sqlca.sqlcode == 0) {
        printf("Link deleted successfully.\n");
    } else {
        fprintf(stderr, "Error deleting link: %s\n", sqlca.sqlerrm.sqlerrmc);
    }
}

void handle_link_addition() {
    EXEC SQL BEGIN DECLARE SECTION;
    char original_text[1024];
    char alias_text[256];
    EXEC SQL END DECLARE SECTION;

    read_input("Original URL: ", original_text, sizeof(original_text));
    read_input("Alias: ", alias_text, sizeof(alias_text));

    EXEC SQL CALL add_link(:original_text, :alias_text);
    EXEC SQL COMMIT;

    if(sqlca.sqlcode == 0) {
        printf("Link added successfully.\n");
    } else {
        fprintf(stderr, "Error adding link: %s\n", sqlca.sqlerrm.sqlerrmc);
    }
}