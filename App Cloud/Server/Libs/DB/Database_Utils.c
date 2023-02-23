#include"Database_Utils.h"

// Function which authenticates the user credentials
bool authenticate_user(const char *username, const char *pswd){
    // Connection string for the database
    const char *conn_string = "host=localhost port=5432 dbname=multichat user=postgres password=password";

    // Open a connection to the database using the connection string
    PGconn *conn = PQconnectdb(conn_string);

    // Check if the connection was successful
    if (PQstatus(conn) != CONNECTION_OK)
    {
        // Connection to the database failed, print an error message and exit
        fprintf(stdout, "Connection to database failed: %s\n", PQerrorMessage(conn));
        return false;
    }

    // Connection to the database was successful
    printf("Connected to database\n");

    // Query the "credenziali" table to find a row where username and pswd match the provided values
    const char *query = "SELECT * FROM credenziali WHERE username = $1 AND pswd = $2";
    const char *param_values[2] = { username, pswd };
    int param_lengths[2] = { strlen(username), strlen(pswd) };
    int param_formats[2] = { 0, 0 }; // 0 means text format
    PGresult *result = PQexecParams(conn, query, 2, NULL, param_values, param_lengths, param_formats, 0);

    // Check if the query was successful
    if (PQresultStatus(result) != PGRES_TUPLES_OK)
    {
        // The query failed, print an error message and exit
        fprintf(stdout, "Query failed: %s\n", PQerrorMessage(conn));
        return false;
    }

    // The query was successful, check if at least and at most one row was found
    int n_rows = PQntuples(result);
    if (n_rows < 1 || n_rows > 1)
    {
        // No rows were found, print an error message and exit
        fprintf(stdout, "Login Failed \n");
        return false;
    }

    // We print the found row just for logging
    printf("Row of the credenziali table found: \n");
    for (int i = 0; i < PQnfields(result); i++)
    {
        printf("%s: %s\n", PQfname(result, i), PQgetvalue(result, 0, i));
    }

    // Free the memory used by the query result
    PQclear(result);

    // Close the connection to the database
    PQfinish(conn);

    return true ;
}

// Function which register the user credentials inside the database
bool register_user(const char *username, const char *pswd){
    // Connection string for the database
    const char *conn_string = "host=localhost port=5432 dbname=multichat user=postgres password=password";

    // Open a connection to the database using the connection string
    PGconn *conn = PQconnectdb(conn_string);

    // Check if the connection was successful
    if (PQstatus(conn) != CONNECTION_OK)
    {
        // Connection to the database failed, print an error message and exit
        fprintf(stdout, "Connection to database failed: %s\n", PQerrorMessage(conn));
        return false;
    }

    // Connection to the database was successful
    printf("Connected to database\n");

    // Query the "credenziali" table to insert a row with given username and pswd
    const char *query = "INSERT INTO credenziali (username,pswd,userid) VALUES ($1,$2,nextval('PK_Generator'))"; // Da sistemare userID. Deve essere una sequenza
    const char *param_values[2] = { username, pswd };
    int param_lengths[2] = { strlen(username), strlen(pswd) };
    int param_formats[2] = { 0, 0 }; // 0 means text format
    PGresult *result = PQexecParams(conn, query, 2, NULL, param_values, param_lengths, param_formats, 0);

    // Check if the query was successful
    if (PQresultStatus(result) != PGRES_COMMAND_OK)
    {
        // The query failed, print an error message and exit
        fprintf(stdout, "Query failed: %s\n", PQerrorMessage(conn));
        return false;
    }

    // Free the memory used by the query result
    PQclear(result);

    // Close the connection to the database
    PQfinish(conn);

    return true ;
}
