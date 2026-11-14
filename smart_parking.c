
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <mysql.h>


void parkVehicle(MYSQL *conn)
{
    char vehicleNo[20];
    char vehicleType[20];
    char query[500];
    int choice;

    printf("\nEnter Vehicle Number: ");
    scanf("%19s", vehicleNo);

    // Check whether vehicle is already parked
    snprintf(query, sizeof(query),
        "SELECT slot_id FROM parking_slots "
        "WHERE vehicle_no='%s' AND is_occupied=1",
        vehicleNo);

    if (mysql_query(conn, query))
    {
        printf("Database Error: %s\n", mysql_error(conn));
        return;
    }

    MYSQL_RES *result = mysql_store_result(conn);
    MYSQL_ROW row = mysql_fetch_row(result);

    if (row != NULL)
    {
        printf("\nThis vehicle is already parked in Slot %s!\n", row[0]);
        mysql_free_result(result);
        return;
    }

    mysql_free_result(result);

    printf("\nSelect Vehicle Type:\n");
    printf("1. Two-Wheeler\n");
    printf("2. Four-Wheeler\n");
    printf("Enter choice: ");
    scanf("%d", &choice);

    if (choice == 1)
        strcpy(vehicleType, "Two-wheeler");
    else if (choice == 2)
        strcpy(vehicleType, "Four-wheeler");
    else
    {
        printf("Invalid vehicle type!\n");
        return;
    }

    // Find an available slot of the selected type
    snprintf(query, sizeof(query),
        "SELECT slot_id FROM parking_slots "
        "WHERE is_occupied=0 AND slot_type='%s' "
        "ORDER BY slot_id LIMIT 1",
        vehicleType);

    if (mysql_query(conn, query))
    {
        printf("Database Error: %s\n", mysql_error(conn));
        return;
    }

    result = mysql_store_result(conn);
    row = mysql_fetch_row(result);

    if (row == NULL)
    {
        printf("\nSorry! No suitable parking slot available.\n");
        mysql_free_result(result);
        return;
    }

    int slotNo = atoi(row[0]);
    mysql_free_result(result);

    // Allocate the parking slot
    snprintf(query, sizeof(query),
        "UPDATE parking_slots SET vehicle_no='%s', "
        "entry_time=NOW(), is_occupied=1 "
        "WHERE slot_id=%d",
        vehicleNo, slotNo);

    if (mysql_query(conn, query))
    {
        printf("Database Error: %s\n", mysql_error(conn));
        return;
    }

    printf("\nVehicle Parked Successfully!\n");
    printf("Vehicle Number: %s\n", vehicleNo);
    printf("Vehicle Type: %s\n", vehicleType);
    printf("Allocated Slot: %d\n", slotNo);
}


void viewSlots(MYSQL *conn)
{
    if (mysql_query(conn,
       "SELECT slot_id, vehicle_no, slot_type, "
"is_occupied FROM parking_slots ORDER BY slot_id"))
    {
        printf("Error: %s\n", mysql_error(conn));
        return;
    }

    MYSQL_RES *result = mysql_store_result(conn);
    MYSQL_ROW row;

    printf("\n----- PARKING SLOT STATUS -----\n");

    while ((row = mysql_fetch_row(result)))
    {
        printf("Slot %s: ", row[0]);

        if (atoi(row[3]) == 1)
            printf("Occupied | %s | %s\n", row[1], row[2]);
        else
            printf("Available\n");
    }

    mysql_free_result(result);
} 

void removeVehicle(MYSQL *conn)
{
    char vehicleNo[20];
    char query[700];
    char vehicleType[20];
    char entryTime[30];
    char paymentStatus[20];

    int slotNo, seconds, hours, rate, paymentChoice;
    float fee;

    printf("\nEnter Vehicle Number to Remove: ");
    scanf("%19s", vehicleNo);

    snprintf(query, sizeof(query),
        "SELECT slot_id, slot_type, entry_time, "
        "TIMESTAMPDIFF(SECOND, entry_time, NOW()) "
        "FROM parking_slots "
        "WHERE vehicle_no='%s' AND is_occupied=1",
        vehicleNo);

    if (mysql_query(conn, query))
    {
        printf("Database Error: %s\n", mysql_error(conn));
        return;
    }

    MYSQL_RES *result = mysql_store_result(conn);
    MYSQL_ROW row = mysql_fetch_row(result);

    if (row == NULL)
    {
        printf("\nVehicle not found!\n");
        mysql_free_result(result);
        return;
    }

    slotNo = atoi(row[0]);
    strcpy(vehicleType, row[1]);
    strcpy(entryTime, row[2]);
    seconds = atoi(row[3]);

    mysql_free_result(result);

    hours = (seconds + 3599) / 3600;

    if (hours < 1)
        hours = 1;

    if (strcmp(vehicleType, "Two-wheeler") == 0)
        rate = 20;
    else
        rate = 40;

    fee = hours * rate;

    printf("\nTotal Parking Fee: Rs. %.2f\n", fee);
    printf("\nSelect Payment Status:\n");
    printf("1. Paid\n");
    printf("2. Pending\n");
    printf("Enter choice: ");
    scanf("%d", &paymentChoice);

    if (paymentChoice == 1)
        strcpy(paymentStatus, "Paid");
    else if (paymentChoice == 2)
        strcpy(paymentStatus, "Pending");
    else
    {
        printf("Invalid payment choice!\n");
        return;
    }

    snprintf(query, sizeof(query),
        "INSERT INTO parking_records "
        "(slot_no, vehicle_no, vehicle_type, entry_time, "
        "exit_time, fee, payment_status) "
        "VALUES (%d, '%s', '%s', '%s', NOW(), %.2f, '%s')",
        slotNo, vehicleNo, vehicleType, entryTime,
        fee, paymentStatus);

    if (mysql_query(conn, query))
    {
        printf("Record Error: %s\n", mysql_error(conn));
        return;
    }

    snprintf(query, sizeof(query),
        "UPDATE parking_slots SET vehicle_no=NULL, "
        "entry_time=NULL, is_occupied=0 WHERE slot_id=%d",
        slotNo);

    if (mysql_query(conn, query))
    {
        printf("Slot Update Error: %s\n", mysql_error(conn));
        return;
    }

    printf("\n========== PARKING RECEIPT ==========\n");
    printf("Vehicle Number: %s\n", vehicleNo);
    printf("Vehicle Type: %s\n", vehicleType);
    printf("Parking Slot: %d\n", slotNo);
    printf("Parking Duration: %d hour(s)\n", hours);
    printf("Parking Fee: Rs. %.2f\n", fee);
    printf("Payment Status: %s\n", paymentStatus);
    printf("=====================================\n");
    printf("Vehicle Removed Successfully!\n");
}


void viewRecords(MYSQL *conn)
{
    if (mysql_query(conn,
        "SELECT record_id, slot_no, vehicle_no, vehicle_type, "
        "entry_time, exit_time, fee "
        "FROM parking_records ORDER BY record_id"))
    {
        printf("Database Error: %s\n", mysql_error(conn));
        return;
    }

    MYSQL_RES *result = mysql_store_result(conn);
    MYSQL_ROW row;

    printf("\n========== PARKING HISTORY ==========\n");

    if (mysql_num_rows(result) == 0)
    {
        printf("No parking records found.\n");
    }

    while ((row = mysql_fetch_row(result)))
    {
        printf("\nRecord ID: %s\n", row[0]);
        printf("Slot Number: %s\n", row[1]);
        printf("Vehicle Number: %s\n", row[2]);
        printf("Vehicle Type: %s\n", row[3]);
        printf("Entry Time: %s\n", row[4]);
        printf("Exit Time: %s\n", row[5]);
        printf("Parking Fee: Rs. %s\n", row[6]);
        printf("-------------------------------------\n");
    }

    mysql_free_result(result);
}


int main()
{
    MYSQL *conn = mysql_init(NULL);

    if (conn == NULL)
    {
        printf("MySQL initialization failed.\n");
        return 1;
    }

    if (mysql_real_connect(conn, "localhost", "root", "",
        "smart_parking", 0, NULL, 0) == NULL)
    {
        printf("Connection failed: %s\n", mysql_error(conn));
        mysql_close(conn);
        return 1;
    }

    int choice;

    do
    {
        printf("\n==============================\n");
        printf("       SMART PARKING SYSTEM\n");
        printf("==============================\n");
        printf("1. Park Vehicle\n");
printf("2. View Parking Slots\n");
printf("3. Remove Vehicle\n");
printf("4. View Parking Records\n");
printf("5. Exit\n");
        printf("Enter your choice: ");

        if (scanf("%d", &choice) != 1)
        {
            printf("Invalid input!\n");
            break;
        }

        switch (choice)
        {
            case 1:
                parkVehicle(conn);
                break;

            case 2:
                viewSlots(conn);
                break;

           case 3:
    removeVehicle(conn);
    break;

case 4:
    viewRecords(conn);
    break;

case 5:
    printf("Thank you for using Smart Parking System!\n");
    break;

            default:
                printf("Invalid choice!\n");
        }

} while (choice != 5);

    mysql_close(conn);
    return 0;
}
