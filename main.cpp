/*
    This sketch shows the Ethernet event usage

*/

#define ETH_CLK_MODE ETH_CLOCK_GPIO17_OUT
#define ETH_PHY_POWER -1
#define BOARD_HAS_1BIT_SDMMC

#include <ETH.h>
#include <WiFi.h>
#include <lvgl.h>
#include <TFT_eSPI.h>
#include <ui.h>
#include <SPIFFS.h>

int32_t histogram[500];

/*Change to your screen resolution*/
static const uint16_t screenWidth = 240;
static const uint16_t screenHeight = 240;

static lv_disp_draw_buf_t draw_buf;
static lv_color_t buf[screenWidth * screenHeight / 10];

TFT_eSPI tft = TFT_eSPI(screenWidth, screenHeight); /* TFT instance */

static bool eth_connected_ok = false;

// Set web server port number to 80
WiFiServer server(80);

/* Display flushing */
void my_disp_flush(lv_disp_drv_t *disp, const lv_area_t *area, lv_color_t *color_p)
{
    uint32_t w = (area->x2 - area->x1 + 1);
    uint32_t h = (area->y2 - area->y1 + 1);

    tft.startWrite();
    tft.setAddrWindow(area->x1, area->y1, w, h);
    tft.pushColors((uint16_t *)&color_p->full, w * h, true);
    tft.endWrite();

    lv_disp_flush_ready(disp);
}

/*Read the touchpad*/
void my_touchpad_read(lv_indev_drv_t *indev_driver, lv_indev_data_t *data)
{
    uint16_t touchX = 0, touchY = 0;

    bool touched = false; // tft.getTouch( &touchX, &touchY, 600 );

    if (!touched)
    {
        data->state = LV_INDEV_STATE_REL;
    }
    else
    {
        data->state = LV_INDEV_STATE_PR;

        /*Set the coordinates*/
        data->point.x = touchX;
        data->point.y = touchY;

        Serial.print("Data x ");
        Serial.println(touchX);

        Serial.print("Data y ");
        Serial.println(touchY);
    }
}

void WiFiEvent(WiFiEvent_t event)
{
    switch (event)
    {
    case ARDUINO_EVENT_ETH_START:
        Serial.println("ETH Started");
        // set eth hostname here
        ETH.setHostname("esp32-ethernet");
        break;
    case ARDUINO_EVENT_ETH_CONNECTED:
        Serial.println("ETH Connected");
        break;
    case ARDUINO_EVENT_ETH_GOT_IP:
        Serial.print("ETH MAC: ");
        Serial.print(ETH.macAddress());
        Serial.print(", IPv4: ");
        Serial.print(ETH.localIP());
        if (ETH.fullDuplex())
        {
            Serial.print(", FULL_DUPLEX");
        }
        Serial.print(", ");
        Serial.print(ETH.linkSpeed());
        Serial.println("Mbps");
        eth_connected_ok = true;
        break;
    case ARDUINO_EVENT_ETH_DISCONNECTED:
        Serial.println("ETH Disconnected");
        eth_connected_ok = false;
        break;
    case ARDUINO_EVENT_ETH_STOP:
        Serial.println("ETH Stopped");
        eth_connected_ok = false;
        break;
    default:
        break;
    }
}

void testClient(const char *host, uint16_t port)
{
    Serial.print("\nconnecting to ");
    Serial.println(host);

    WiFiClient client;
    if (!client.connect(host, port))
    {
        Serial.println("connection failed");
        return;
    }
    client.printf("GET / HTTP/1.1\r\nHost: %s\r\n\r\n", host);
    while (client.connected() && !client.available())
        ;
    while (client.available())
    {
        Serial.write(client.read());
    }

    Serial.println("closing connection\n");
    client.stop();
}

// Přidání globálních proměnných pro konfiguraci histogramu
int32_t start_value = 0;
int32_t stop_value = 10;
int32_t step_value = 1;
int count_period = 5;
bool is_running = false;
unsigned long last_update_time = 0;

// Funkce pro generování náhodných dat
void generateHistogramData()
{
    if (!is_running)
        return;

    unsigned long current_time = millis();
    if (current_time - last_update_time >= count_period * 1000)
    {
        // Vygenerovat náhodná data
        for (int i = start_value; i < stop_value; i += step_value)
        {
            int index = (i - start_value) / step_value;
            if (index < 500)
            {                                     // Kontrola hranice pole
                histogram[index] = random(0, 20); // Náhodná hodnota 0-19
            }
        }
        last_update_time = current_time;
    }
}

// Funkce pro zpracování API požadavků
void handleApiRequest(WiFiClient &client, String &header)
{
    // Získání aktuálních dat histogramu
    if (header.startsWith("GET /data HTTP"))
    {
        client.println("HTTP/1.1 200 OK");
        client.println("Content-Type: application/json");
        client.println("Connection: close");
        client.println();

        // Generování JSON odpovědi s daty histogramu
        client.print("[");
        bool first = true;
        for (int i = start_value; i < stop_value; i += step_value)
        {
            int index = (i - start_value) / step_value;
            if (!first)
                client.print(",");
            client.print("[");
            client.print(i);
            client.print(",");
            client.print(index < 500 ? histogram[index] : 0);
            client.print("]");
            first = false;
        }
        client.println("]");
    }
    // Konfigurace histogramu
    else if (header.startsWith("POST /config HTTP"))
    {
        // Zde by mělo být parsování JSON z těla požadavku
        // Pro jednoduchost použijeme parametry v URL
        if (header.indexOf("start_value=") >= 0)
        {
            int startIdx = header.indexOf("start_value=") + 12;
            int endIdx = header.indexOf("&", startIdx);
            if (endIdx < 0)
                endIdx = header.indexOf(" HTTP", startIdx);
            start_value = header.substring(startIdx, endIdx).toInt();
        }
        if (header.indexOf("stop_value=") >= 0)
        {
            int startIdx = header.indexOf("stop_value=") + 11;
            int endIdx = header.indexOf("&", startIdx);
            if (endIdx < 0)
                endIdx = header.indexOf(" HTTP", startIdx);
            stop_value = header.substring(startIdx, endIdx).toInt();
        }
        if (header.indexOf("step_value=") >= 0)
        {
            int startIdx = header.indexOf("step_value=") + 11;
            int endIdx = header.indexOf("&", startIdx);
            if (endIdx < 0)
                endIdx = header.indexOf(" HTTP", startIdx);
            step_value = header.substring(startIdx, endIdx).toInt();
        }

        client.println("HTTP/1.1 200 OK");
        client.println("Content-Type: application/json");
        client.println("Connection: close");
        client.println();

        client.print("{\"start_value\":");
        client.print(start_value);
        client.print(",\"stop_value\":");
        client.print(stop_value);
        client.print(",\"step_value\":");
        client.print(step_value);
        client.println("}");
    }
    // Spuštění generování dat
    else if (header.startsWith("POST /start HTTP"))
    {
        if (header.indexOf("count_period=") >= 0)
        {
            int startIdx = header.indexOf("count_period=") + 13;
            int endIdx = header.indexOf("&", startIdx);
            if (endIdx < 0)
                endIdx = header.indexOf(" HTTP", startIdx);
            count_period = header.substring(startIdx, endIdx).toInt();
        }

        is_running = true;
        last_update_time = millis();

        client.println("HTTP/1.1 200 OK");
        client.println("Content-Type: application/json");
        client.println("Connection: close");
        client.println();

        client.print("{\"count_period\":");
        client.print(count_period);
        client.println(",\"status\":\"started\"}");
    }
    // Zastavení generování dat
    else if (header.startsWith("POST /stop HTTP"))
    {
        is_running = false;

        client.println("HTTP/1.1 200 OK");
        client.println("Content-Type: application/json");
        client.println("Connection: close");
        client.println();

        client.println("{\"status\":\"stopped\"}");
    }
    // Vyčištění dat
    else if (header.startsWith("POST /clear HTTP"))
    {
        for (int i = 0; i < 500; i++)
        {
            histogram[i] = 0;
        }

        client.println("HTTP/1.1 200 OK");
        client.println("Content-Type: application/json");
        client.println("Connection: close");
        client.println();

        client.println("{\"status\":\"cleared\"}");
    }
    // Výchozí HTML stránka
    else if (header.startsWith("GET / HTTP") || header.startsWith("GET /index.html HTTP"))
    {
        // Načtení HTML souboru ze SPIFFS
        File file = SPIFFS.open("/index.html", "r");
        if (!file)
        {
            // Pokud soubor neexistuje, vrátíme chybu
            client.println("HTTP/1.1 404 Not Found");
            client.println("Content-Type: text/plain");
            client.println("Connection: close");
            client.println();
            client.println("404 Not Found");
            return;
        }

        client.println("HTTP/1.1 200 OK");
        client.println("Content-Type: text/html");
        client.println("Connection: close");
        client.println();

        // Odeslání obsahu souboru
        while (file.available())
        {
            client.write(file.read());
        }
        file.close();
    }
}

void setup()
{
    Serial.begin(115200);

    // Inicializace SPIFFS
    if (!SPIFFS.begin(true))
    {
        Serial.println("An Error has occurred while mounting SPIFFS");
        return;
    }

    WiFi.onEvent(WiFiEvent);
    if (ETH.begin())
    {
        Serial.println("ETH Started OK");
    }
    else
    {
        Serial.println("ETH Failed to start");

        // Read all config for ethernet
    }

    lv_init();

    tft.begin();        /* TFT init */
    tft.setRotation(3); /* Landscape orientation, flipped */

    lv_disp_draw_buf_init(&draw_buf, buf, NULL, screenWidth * screenHeight / 10);

    /*Initialize the display*/
    static lv_disp_drv_t disp_drv;
    lv_disp_drv_init(&disp_drv);
    /*Change the following line to your display resolution*/
    disp_drv.hor_res = screenWidth;
    disp_drv.ver_res = screenHeight;
    disp_drv.flush_cb = my_disp_flush;
    disp_drv.draw_buf = &draw_buf;
    lv_disp_drv_register(&disp_drv);

    /*Initialize the (dummy) input device driver*/
    static lv_indev_drv_t indev_drv;
    lv_indev_drv_init(&indev_drv);
    indev_drv.type = LV_INDEV_TYPE_POINTER;
    indev_drv.read_cb = my_touchpad_read;
    lv_indev_drv_register(&indev_drv);

    // Enable backlihgt
    pinMode(TFT_BL, OUTPUT);
    digitalWrite(TFT_BL, HIGH);

    ui_init();

    server.begin();
    Serial.println("Setup done");
}

// Current time
unsigned long currentTime = millis();
// Previous time
unsigned long previousTime = 0;
// Define timeout time in milliseconds (example: 2000ms = 2s)
const long timeoutTime = 2000;

// Variable to store the HTTP request
String header;

void loop()
{
    // Zobrazení IP adresy v UI
    if (eth_connected_ok)
    {
        lv_label_set_text_fmt(ui_Label1, "IP: %s", ETH.localIP().toString().c_str());
    }
    else
    {
        lv_label_set_text_fmt(ui_Label1, "IP: not connected");
    }

    lv_timer_handler(); /* let the GUI do its work */

    // Generování dat histogramu
    generateHistogramData();

    WiFiClient client = server.available(); // Listen for incoming clients

    if (client)
    { // If a new client connects,
        currentTime = millis();
        previousTime = currentTime;
        Serial.println("New Client."); // print a message out in the serial port
        String currentLine = "";       // make a String to hold incoming data from the client
        header = "";                   // Clear the header variable

        while (client.connected() && currentTime - previousTime <= timeoutTime)
        { // loop while the client's connected
            currentTime = millis();
            if (client.available())
            {                           // if there's bytes to read from the client,
                char c = client.read(); // read a byte, then
                Serial.write(c);        // print it out the serial monitor
                header += c;
                if (c == '\n')
                { // if the byte is a newline character
                    // if the current line is blank, you got two newline characters in a row.
                    // that's the end of the client HTTP request, so send a response:
                    if (currentLine.length() == 0)
                    {
                        // Zpracování API požadavku
                        handleApiRequest(client, header);
                        break;
                    }
                    else
                    { // if you got a newline, then clear currentLine
                        currentLine = "";
                    }
                }
                else if (c != '\r')
                {                     // if you got anything else but a carriage return character,
                    currentLine += c; // add it to the end of the currentLine
                }
            }
        }
        // Clear the header variable
        header = "";
        // Close the connection
        client.stop();
        Serial.println("Client disconnected.");
    }
}