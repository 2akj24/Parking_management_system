#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <mysql.h>

#define MAX_VEHICLES 50
#define MAX_NAME_LENGTH 50

// Structure to represent a vehicle
struct Vehicle {
    char licensePlate[20];
    char ownerName[MAX_NAME_LENGTH];
    int type; // 2 for 2-wheelers, 4 for 4-wheelers
    char parkingDate[11]; // Format: YYYY-MM-DD
};

// Function prototypes
void displayMenu();
void parkVehicle(struct Vehicle parkingLot[], int* totalVehicles, MYSQL* conn);
void removeVehicle(struct Vehicle parkingLot[], int* totalVehicles, MYSQL* conn);
void viewParkedVehicles(struct Vehicle parkingLot[], int totalVehicles, MYSQL* conn);
void searchHistorySQL(MYSQL *conn);
void generateInvoice(struct Vehicle vehicle, char departureDate[11]);
void displayHeader();
void displayFooter();

int main() {
    struct Vehicle parkingLot[MAX_VEHICLES];
    int totalVehicles = 0;
    int choice;

    // MySQL Connection
    MYSQL *conn;
    conn = mysql_init(NULL);
    if (mysql_real_connect(conn, "localhost", "root", "1234", "ParkingDB", 0, NULL, 0) == NULL) {
        fprintf(stderr, "Failed to connect to database: Error: %s\n", mysql_error(conn));
        exit(EXIT_FAILURE);
    }

    displayHeader();

    do {
        displayMenu();
        printf("Enter your choice: ");
        scanf("%d", &choice);

        switch (choice) {
            case 1: parkVehicle(parkingLot, &totalVehicles, conn); break;
            case 2: removeVehicle(parkingLot, &totalVehicles, conn); break;
            case 3: viewParkedVehicles(parkingLot, totalVehicles, conn); break;
            case 4: searchHistorySQL(conn); break;
            case 5:
                displayFooter();
                printf("Exiting the program. Thanks for using Shivaay Parking Management!\n");
                break;
            default:
                printf("Invalid choice. Please enter a valid option.\n");
        }
    } while (choice != 5);

    mysql_close(conn);
    return 0;
}

// ========== MENU & UI ==========

void displayMenu() {
    printf("\nShivaay Parking Management System\n");
    printf("1. Park a vehicle\n");
    printf("2. Remove a vehicle\n");
    printf("3. View parked vehicles (active table)\n");
    printf("4. Search history by date/number plate\n");
    printf("5. Exit\n");
}

void displayHeader() {
    printf("****************************************************\n");
    printf("**            Shivaay Parking Management           **\n");
    printf("****************************************************\n");
    printf("  \n");
    printf("**                   Created by                    **\n");
    printf("**                 Byte Grinders                   **\n");
    printf("****************************************************\n");
}

void displayFooter() {
    printf("****************************************************\n");
    printf("** Thanks for using Shivaay Parking Management!   **\n");
    printf("**        Hope you like the service               **\n");
    printf("**         Created by Byte Grinders               **\n");
    printf("****************************************************\n");
}

// ========== PARK VEHICLE ==========

void parkVehicle(struct Vehicle parkingLot[], int* totalVehicles, MYSQL* conn) {
    if (*totalVehicles < MAX_VEHICLES) {
        printf("Enter vehicle license plate: ");
        scanf(" %[^\n]s", parkingLot[*totalVehicles].licensePlate);

        printf("Enter vehicle owner's name: ");
        scanf(" %[^\n]s", parkingLot[*totalVehicles].ownerName);

        printf("Enter vehicle type (2 for 2-wheelers, 4 for 4-wheelers): ");
        scanf("%d", &parkingLot[*totalVehicles].type);

        printf("Enter parking date (YYYY-MM-DD): ");
        scanf("%s", parkingLot[*totalVehicles].parkingDate);

        // Insert into MySQL active table
        char query[512];
        sprintf(query,
            "INSERT INTO active (license_plate, owner_name, type, parking_date) VALUES ('%s', '%s', %d, '%s')",
            parkingLot[*totalVehicles].licensePlate,
            parkingLot[*totalVehicles].ownerName,
            parkingLot[*totalVehicles].type,
            parkingLot[*totalVehicles].parkingDate);

        if (mysql_query(conn, query) != 0) {
            fprintf(stderr, "Error inserting into database: %s\n", mysql_error(conn));
        } else {
            printf("Vehicle parked successfully and saved to database.\n");
            (*totalVehicles)++;
        }
    } else {
        printf("Parking lot is full. Cannot park more vehicles.\n");
    }
}

// ========== REMOVE VEHICLE ==========

void removeVehicle(struct Vehicle parkingLot[], int* totalVehicles, MYSQL* conn) {
    char licensePlate[20];
    printf("Enter vehicle license plate: ");
    scanf(" %[^\n]s", licensePlate);

    char departureDate[11];
    printf("Enter departure date (YYYY-MM-DD): ");
    scanf("%s", departureDate);

    // Fetch from `active`
    char selectQuery[256];
    sprintf(selectQuery, "SELECT * FROM active WHERE license_plate = '%s'", licensePlate);

    if (mysql_query(conn, selectQuery) == 0) {
        MYSQL_RES* res = mysql_store_result(conn);
        MYSQL_ROW row = mysql_fetch_row(res);

        if (row) {
            // Insert into history
            char insertQuery[512];
            sprintf(insertQuery,
                "INSERT INTO history (license_plate, owner_name, type, parking_date, departure_date) "
                "VALUES ('%s', '%s', %d, '%s', '%s')",
                row[0], row[1], atoi(row[2]), row[3], departureDate);
            mysql_query(conn, insertQuery);

            // Delete from active
            char deleteQuery[256];
            sprintf(deleteQuery, "DELETE FROM active WHERE license_plate = '%s'", licensePlate);
            mysql_query(conn, deleteQuery);

            // Also remove from in-memory list
            for (int i = 0; i < *totalVehicles; i++) {
                if (strcmp(parkingLot[i].licensePlate, licensePlate) == 0) {
                    generateInvoice(parkingLot[i], departureDate);
                    for (int j = i; j < *totalVehicles - 1; j++) {
                        parkingLot[j] = parkingLot[j + 1];
                    }
                    (*totalVehicles)--;
                    break;
                }
            }

            printf("Vehicle removed and moved to history.\n");
        } else {
            printf("Vehicle not found in database.\n");
        }

        mysql_free_result(res);
    } else {
        fprintf(stderr, "Query error: %s\n", mysql_error(conn));
    }
}

// ========== VIEW PARKED VEHICLES ==========

void viewParkedVehicles(struct Vehicle parkingLot[], int totalVehicles, MYSQL* conn) {
    if (mysql_query(conn, "SELECT * FROM active") == 0) {
        MYSQL_RES* res = mysql_store_result(conn);
        MYSQL_ROW row;

        printf("\n****************************************************\n");
        printf("            Currently Parked Vehicles\n");
        printf("****************************************************\n");
        printf("License Plate\tOwner's Name\tType\tParking Date\n");

        while ((row = mysql_fetch_row(res))) {
            printf("%s\t\t%s\t\t%s\t%s\n",
                   row[0], row[1],
                   (atoi(row[2]) == 2) ? "2-wheeler" : "4-wheeler",
                   row[3]);
        }

        mysql_free_result(res);
    } else {
        fprintf(stderr, "Error fetching from active table: %s\n", mysql_error(conn));
    }
}

// ========== SEARCH HISTORY ==========

void searchHistorySQL(MYSQL *conn) {
    int option;
    char query[512], input[50];

    printf("Search history by:\n1. License Plate\n2. Parking Date\nEnter your choice: ");
    scanf("%d", &option);

    if (option == 1) {
        printf("Enter license plate: ");
        scanf(" %[^\n]s", input);
        sprintf(query, "SELECT * FROM history WHERE license_plate = '%s'", input);
    } else if (option == 2) {
        printf("Enter parking date (YYYY-MM-DD): ");
        scanf("%s", input);
        sprintf(query, "SELECT * FROM history WHERE parking_date = '%s'", input);
    } else {
        printf("Invalid choice.\n");
        return;
    }

    if (mysql_query(conn, query) == 0) {
        MYSQL_RES *res = mysql_store_result(conn);
        MYSQL_ROW row;

        printf("\n****************************************************\n");
        printf("                Search Results\n");
        printf("****************************************************\n");
        printf("License\tOwner\tType\tParked\tDeparted\n");

        while ((row = mysql_fetch_row(res))) {
            printf("%s\t%s\t%s\t%s\t%s\n", row[0], row[1], row[2], row[3], row[4]);
        }

        mysql_free_result(res);
    } else {
        fprintf(stderr, "Query failed: %s\n", mysql_error(conn));
    }
}

// ========== GENERATE INVOICE ==========

void generateInvoice(struct Vehicle vehicle, char departureDate[11]) {
    const float RATE_2_WHEELER = 450.0;
    const float RATE_4_WHEELER = 650.0;

    int parkingYear, parkingMonth, parkingDay;
    int departureYear, departureMonth, departureDay;

    sscanf(vehicle.parkingDate, "%d-%d-%d", &parkingYear, &parkingMonth, &parkingDay);
    sscanf(departureDate, "%d-%d-%d", &departureYear, &departureMonth, &departureDay);

    int parkingDuration = (departureYear - parkingYear) * 365 + (departureMonth - parkingMonth) * 30 + (departureDay - parkingDay);

    float totalCharge = (vehicle.type == 2) ? RATE_2_WHEELER * parkingDuration : RATE_4_WHEELER * parkingDuration;

    printf("\n****************************************************\n");
    printf("                    Parking Invoice\n");
    printf("****************************************************\n");
    printf("Vehicle: %s\n", vehicle.licensePlate);
    printf("Owner's Name: %s\n", vehicle.ownerName);
    printf("Type: %s\n", (vehicle.type == 2) ? "2-wheeler" : "4-wheeler");
    printf("Parking Date: %s\n", vehicle.parkingDate);
    printf("Departure Date: %s\n", departureDate);
    printf("Parking Duration (in days): %d\n", parkingDuration);
    printf("Rate per day: Rs%.2f\n", (vehicle.type == 2) ? RATE_2_WHEELER : RATE_4_WHEELER);
    printf("Total charge: Rs%.2f\n", totalCharge);
    printf("****************************************************\n");
}
