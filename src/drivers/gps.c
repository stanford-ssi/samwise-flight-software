#include "pico/printf.h"
#include "pico/stdlib.h"
#include <stdlib.h>
#include "hardware/uart.h"

#define UART_ID uart0
#define BAUD_RATE 9600

#define UART_TX_PIN 0
#define UART_RX_PIN 1

#define UART_BUFFER_SIZE 256

typedef struct
{
    double latitude;
    double longitude;
    double altitude;
} gps_data;

static char uart_buffer[UART_BUFFER_SIZE];
static int uart_buffer_index = 0;

gps_data parse_nmea_sentence(const char *sentence);

// int main()
int init()
{
    // Initialize chosen serial port
    uart_init(UART_ID, BAUD_RATE);
    stdio_init_all();

    sleep_ms(5000);
    // printf("Hello World!");

    // Set the pins for UART
    gpio_set_function(UART_TX_PIN, UART_FUNCSEL_NUM(UART_ID, UART_TX_PIN));
    gpio_set_function(UART_RX_PIN, UART_FUNCSEL_NUM(UART_ID, UART_RX_PIN));

    while (true) {
        if (uart_is_readable(UART_ID))
        {
            // printf("Yes");

            char c = uart_getc(UART_ID);
            // printf("%c", c);
            if (c == '\n')
            {
                uart_buffer[uart_buffer_index] = '\0';
                uart_buffer_index = 0;
                printf("Received: %s\n", uart_buffer);
                gps_data g = parse_nmea_sentence(uart_buffer);
                // printf("Parsed: LNG %f, LAT %f, ALT %f", g.longitude, g.latitude, g.altitude);
            }
            else
            {
                uart_buffer[uart_buffer_index++] = c;
                if (uart_buffer_index >= UART_BUFFER_SIZE)
                {
                    uart_buffer_index = 0;
                }
            }
        }
    }
}

/**
 * @brief Parse only GPGGA sentense from gps output and return useful info
 * 
 * @param s: NMEA sentence 
 * @return gps_data: struct containing long, lat, altitude
 */
gps_data parse_nmea_sentence(const char *s)
{
    char ns_indicator, ew_indicator;
    gps_data data;

    // printf("Received: %s\n", s);
    
    if (s[3] == 'G' && s[4] == 'G' && s[5] == 'A')
    {
        // Print the received sentence
        // printf("Received: %s\n", s);

        // Parse GGA sentence
        // e.g. $GPGGA,181908.00,3404.7041778,N,   07044.3966270,W,   4,       13,     1.00, 495.144,  M,    29.200,      M,    0.10,0000,*40
        //      GPGGA, timestamp,LATITUDE    ,n/s, LONGITUDE    ,e/w, quality, # sats, HDOP, LATITUDE, unit, geoidal sep, unit, 
        
        // Manually parse the GGA sentence
        const char *ptr = s;
        int field = 0;
        char buffer[20];
        int buffer_index = 0;

        while (*ptr) {
        if (*ptr == ',' || *ptr == '*') {
            buffer[buffer_index] = '\0';
            // print field and buffer
            printf("Field %d: %s\n", field, buffer);

            switch (field) {
            case 2: // Latitude
                data.latitude = atof(buffer);
                break;
            case 3: // N/S Indicator
                ns_indicator = buffer[0];
                break;
            case 4: // Longitude
                data.longitude = atof(buffer);
                break;
            case 5: // E/W Indicator
                ew_indicator = buffer[0];
                break;
            case 9: // Altitude
                data.altitude = atof(buffer);
                break;
            }
            field++;
            buffer_index = 0;
        } else {
            buffer[buffer_index++] = *ptr;
        }
        ptr++;
    }

        // Adjust latitude and longitude based on N/S and E/W indicators
        if (ns_indicator == 'S')
        {
            data.latitude = -data.latitude;
        }
        if (ew_indicator == 'W')
        {
            data.longitude = -data.longitude;
        }

        // sscanf(s, "$GPGGA,%*f,%f,%c,%f,%c,%*d,%*d,%*f,%f,M,%*f,M,%*f,%*d", &data.latitude, &ns_indicator, &data.longitude, &ew_indicator, &data.altitude);

        printf("\nParsed: LNG %f, LAT %f, ALT %f\n", data.longitude, data.latitude, data.altitude);
        
        return data;
    }
    // Add more NMEA sentence parsing as needed
    return data;
}

