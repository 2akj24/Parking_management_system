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
    int type;
    char parkingDate[11];
};

// Function prototypes
void displayMenu();
void parkVehicle(struct Vehicle parkingLot[], int* totalVehicles, MYSQL* conn);
void removeVehicle(struct Vehicle parkingLot[], int* totalVehicles, MYSQL* conn);
void viewParkedVehicles(struct Vehicle parkingLot[], int totalVehicles, MYSQL* conn);
void searchHistorySQL(MYSQL *conn);
void displayHeader();
void displayFooter();
int convertTo24Hour(const char* timeStr);
float calculateFare(const char* startDate, const char* startTime, const char* endDate, const char* endTime, int type);

int main() {
    struct Vehicle parkingLot[MAX_VEHICLES];
    int totalVehicles = 0;
    int choice;

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

void displayMenu() {
    printf("\nShivaay Parking Management System\n");
    printf("1. Park a vehicle\n");
    printf("2. Remove a vehicle\n");
    printf("3. View parked vehicles (active table)\n");
    printf("4. Search history by date/number plate\n");
    printf("5. Exit\n");
}

void displayHeader() {
    printf("******************************************************\n");
    printf("**            Shivaay Parking Management            **\n");
    printf("******************************************************\n");
    printf("** Fare: 2 Wheeler - Rs 50/hr 4 Wheeler- Rs 200/hr   **\n");
    printf("*******************************************************\n");
}

void displayFooter() {
    printf("****************************************************\n");
    printf("** Thanks for using Shivaay Parking Management!   **\n");
    printf("**        Hope you like the service               **\n");
    printf("**            Created by Abhinav                  **\n");
    printf("****************************************************\n");
}

void parkVehicle(struct Vehicle parkingLot[], int* totalVehicles, MYSQL* conn) {
    if (*totalVehicles < MAX_VEHICLES) {
        char checkinTime[10];

        printf("Enter vehicle license plate: ");
        scanf(" %[^\n]", parkingLot[*totalVehicles].licensePlate);

        printf("Enter vehicle owner's name: ");
        scanf(" %[^\n]", parkingLot[*totalVehicles].ownerName);

        printf("Enter vehicle type (2 for 2-wheelers, 4 for 4-wheelers): ");
        scanf("%d", &parkingLot[*totalVehicles].type);

        printf("Enter parking date (YYYY-MM-DD): ");
        scanf("%s", parkingLot[*totalVehicles].parkingDate);

        printf("Enter check-in time (e.g., 9AM): ");
        scanf("%s", checkinTime);

        char query[512];
        sprintf(query,
            "INSERT INTO active (license_plate, owner_name, type, parking_date, checkin_time) "
            "VALUES ('%s', '%s', %d, '%s', '%s')",
            parkingLot[*totalVehicles].licensePlate,
            parkingLot[*totalVehicles].ownerName,
            parkingLot[*totalVehicles].type,
            parkingLot[*totalVehicles].parkingDate,
            checkinTime);

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

void removeVehicle(struct Vehicle parkingLot[], int* totalVehicles, MYSQL* conn) {
    char licensePlate[20], departureDate[11], departureTime[10];
    printf("Enter vehicle license plate: ");
    scanf(" %[^\n]", licensePlate);

    printf("Enter departure date (YYYY-MM-DD): ");
    scanf("%s", departureDate);

    printf("Enter departure time (e.g., 11AM): ");
    scanf("%s", departureTime);

    char selectQuery[256];
    sprintf(selectQuery, "SELECT * FROM active WHERE license_plate = '%s'", licensePlate);

    if (mysql_query(conn, selectQuery) == 0) {
        MYSQL_RES* res = mysql_store_result(conn);
        MYSQL_ROW row = mysql_fetch_row(res);

        if (row) {
            float totalCharge = calculateFare(row[3], row[4], departureDate, departureTime, atoi(row[2]));

            char insertQuery[1024];
            sprintf(insertQuery,
                "INSERT INTO history (license_plate, owner_name, type, parking_date, departure_date, checkin_time, checkout_time, fare) "
                "VALUES ('%s', '%s', %d, '%s', '%s', '%s', '%s', %.2f)",
                row[0], row[1], atoi(row[2]), row[3], departureDate, row[4], departureTime, totalCharge);
            mysql_query(conn, insertQuery);

            char deleteQuery[256];
            sprintf(deleteQuery, "DELETE FROM active WHERE license_plate = '%s'", licensePlate);
            mysql_query(conn, deleteQuery);

            printf("Vehicle removed and moved to history. Total Fare: Rs%.2f\n", totalCharge);
        } else {
            printf("Vehicle not found in database.\n");
        }
        mysql_free_result(res);
    } else {
        fprintf(stderr, "Query error: %s\n", mysql_error(conn));
    }
}

void viewParkedVehicles(struct Vehicle parkingLot[], int totalVehicles, MYSQL* conn) {
    if (mysql_query(conn, "SELECT * FROM active") == 0) {
        MYSQL_RES* res = mysql_store_result(conn);
        MYSQL_ROW row;

        printf("\n****************************************************\n");
        printf("            Currently Parked Vehicles\n");
        printf("****************************************************\n");
        printf("License Plate\tOwner\tType\tDate\tTime\n");

        while ((row = mysql_fetch_row(res))) {
            printf("%s\t%s\t%s\t%s\t%s\n", row[0], row[1], row[2], row[3], row[4]);
        }

        mysql_free_result(res);
    } else {
        fprintf(stderr, "Error fetching from active table: %s\n", mysql_error(conn));
    }
}

void searchHistorySQL(MYSQL *conn) {
    int option;
    char query[512], input[50];

    printf("Search history by:\n1. License Plate\n2. Parking Date\nEnter your choice: ");
    scanf("%d", &option);

    if (option == 1) {
        printf("Enter license plate: ");
        scanf(" %[^\n]", input);
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
        printf("License\tOwner\tType\tIn\tOut\tFare\n");

        while ((row = mysql_fetch_row(res))) {
            printf("%s\t%s\t%s\t%s %s\t%s %s\tRs%.2f\n",
                   row[0], row[1], row[2], row[3], row[5], row[4], row[6], atof(row[7]));
        }

        mysql_free_result(res);
    } else {
        fprintf(stderr, "Query failed: %s\n", mysql_error(conn));
    }
}

int convertTo24Hour(const char* timeStr) {
    int hour;
    char ampm[3];
    sscanf(timeStr, "%d%2s", &hour, ampm);
    if ((strcmp(ampm, "PM") == 0 || strcmp(ampm, "pm") == 0) && hour != 12)
        hour += 12;
    if ((strcmp(ampm, "AM") == 0 || strcmp(ampm, "am") == 0) && hour == 12)
        hour = 0;
    return hour;
}

float calculateFare(const char* startDate, const char* startTime, const char* endDate, const char* endTime, int type) {
    int sYear, sMonth, sDay, eYear, eMonth, eDay;
    sscanf(startDate, "%d-%d-%d", &sYear, &sMonth, &sDay);
    sscanf(endDate, "%d-%d-%d", &eYear, &eMonth, &eDay);

    int durationDays = (eYear - sYear) * 365 + (eMonth - sMonth) * 30 + (eDay - sDay);
    int sHour = convertTo24Hour(startTime);
    int eHour = convertTo24Hour(endTime);
    int durationHours = durationDays * 24 + (eHour - sHour);

    if (durationHours < 1) durationHours = 1;

    float rate = (type == 2) ? 50.0 : 200.0;
    return durationHours * rate;
}
