#include <ESP8266WiFi.h>
#include <Stepper.h>

//////////////////////
// WiFi Definitions //
//////////////////////
const char *ssid = "NodeBot1";
const char *password = "NodeBot1"; // 8 char min

/////////////////////
// Pin Definitions //
/////////////////////

const int RED_LED_PIN = D0; // Red LED
const int BLUE_LED_PIN = D4; // Blue LED

//#define STEPPERS_LINE_STEPS 2664
//#define STEPPERS_TURN_STEPS 1512
#define STEPPERS_LINE_STEPS 2048 // one complete round 
#define STEPPERS_TURN_STEPS 1024 // half round
// 2048 should be the complete turn
#define STEPS 4096
#define MAX_SPEED 40


uint8_t status = 1; // stopped
uint8_t old_status = 1;


// GPIO Pins for Motor Driver board

Stepper stepperLeft(STEPS / 2, D9, D1, D2, D3);
Stepper stepperRight(STEPS / 2, D8, D7, D6, D5);

uint8_t speed = 20; // RPM
uint8_t test_steps = STEPPERS_TURN_STEPS;
WiFiServer server(80);

// LEFT: Blue Verde Rosa Giallo Ok

void setupWiFi()
{
    WiFi.mode(WIFI_AP);
    WiFi.softAP(ssid, password);
}


void initHardware()
{
    pinMode(RED_LED_PIN, OUTPUT);
    pinMode(BLUE_LED_PIN, OUTPUT);
    digitalWrite(RED_LED_PIN, HIGH);
    digitalWrite(BLUE_LED_PIN, HIGH);
    stepperLeft.setSpeed(speed);
    stepperRight.setSpeed(speed);
}

void setup()
{
    initHardware();
    setupWiFi();
    server.begin();
}

uint16_t steps_left = 0;

void engine() {

    pinMode(RED_LED_PIN, LOW);
    switch( status ) {
    case 0: // Stop
        pinMode(RED_LED_PIN, HIGH);
        break;
    case 1: // Forward
        stepperLeft.step(-1);
        stepperRight.step(1);
        break;
    case 2: // Backward
        stepperLeft.step(1);
        stepperRight.step(-1);
        break;
    case 3: // U-Turn
        stepperLeft.step(1);
        stepperRight.step(1);
        break;
    case 4: // Turn left
        stepperLeft.step(1);
        stepperRight.step(1);
        break;
    case 5: // Turn right
        stepperLeft.step(-1);
        stepperRight.step(-1);
        break;
    case 6: // Left
        stepperLeft.step(-1);
        break;
    case 7: // Right
        stepperRight.step(1);
        break;
    case 8: // Step
        stepperLeft.step(-1);
        stepperRight.step(1);
        break;
    }
    if ( steps_left ) {
        steps_left --;
    }
    if ( ! steps_left && status > 2 ) {
        status = old_status;
    }
    delay(100 / speed);
}

void loop()
{

    digitalWrite(BLUE_LED_PIN, LOW);


    // Check if a client has connected
    WiFiClient client = server.available();
    if (!client) {
        engine();
        return;
    }

    // Read the first line of the request
    String req = client.readStringUntil('\r');
    client.flush();

    if ( status < 3 ) {
        old_status = status;
    }

    steps_left = 0;
    
    if (req.indexOf("/st") != -1)
    {
        status = 0; // Stop
    }
    else if (req.indexOf("/fw") != -1)
    {
        status = 1; // Move forward
    }
    else if (req.indexOf("/bw") != -1)
    {
        status = 2; // Move backward
    }
    else if (req.indexOf("/ut") != -1)
    {
        steps_left = 1 + STEPPERS_TURN_STEPS * 2;
        status = 3; // U Turn
    }
    else if (req.indexOf("/tl") != -1)
    {
        steps_left = 1 + STEPPERS_TURN_STEPS;
        status = 4; // Turn left
    }
    else if (req.indexOf("/tr") != -1)
    {
        steps_left = 1 + STEPPERS_TURN_STEPS;
        status = 5; // Turn right
    }
    else if (req.indexOf("/l") != -1)
    {
        steps_left = 1 + STEPPERS_TURN_STEPS / 8;
        status = 6; // Turn right
    }
    else if (req.indexOf("/r") != -1)
    {
        steps_left = 1 + STEPPERS_TURN_STEPS / 8;
        status = 7; // Turn right
    }
    else if (req.indexOf("/s") != -1)
    {
        steps_left = 1 + STEPPERS_LINE_STEPS;
        status = 8; // Step
    }
    else if (req.indexOf("/t") != -1)
    {
        status = 100; // Test
    }
    // Otherwise request will be invalid. We'll say as much in HTML

    if ( req.indexOf("/p") != -1) {
        speed += 5;
        if ( speed > MAX_SPEED) {
            speed = MAX_SPEED;
        }
        stepperLeft.setSpeed(speed);
        stepperRight.setSpeed(speed);
    }
    if ( req.indexOf("/m") != -1 ) {
        speed -= 5;
        if ( speed == 0) {
            speed = 5;
        }
        stepperLeft.setSpeed(speed);
        stepperRight.setSpeed(speed);
    }

    client.flush();

    // Prepare the response. Start with the common header:
    String s = "HTTP/1.1 200 OK\r\n"
    "Content-Type: text/html\r\n\r\n"
    "<!DOCTYPE HTML>\r\n<html><body><head><style type=\"text/css\">"
    "* {font-family: arial;}"
    "table {width:100%; max-width: 600px; padding: 3px; font-size: 200%; }"
    "td {width:30%; border: solid 2px grey;  text-align: center; border-radius: 20%}"
    "td a {display:inline-block; font-weight: bold; valign: middle; padding: 40px 0; width: 100%;}"
    ""
    "</style></head>\r\n"
    "<table>"
    "<tr>"
        "<td><a href=\"/st\">Stop</a></td>"
        "<td><a href=\"/fw\">Forward</a></td>"
        "<td><a href=\"/t\">Test</a></td>"
    "</tr>"
    "<tr>"
        "<td><a href=\"/tl\">Turn Left</a></td>"
        "<td><a href=\"/ut\">U Turn</a></td>"
        "<td><a href=\"/tr\">Turn Right</a></td>"
    "</tr>"
    "<tr>"
        "<td><a href=\"/l\">Left</a></td>"
        "<td><a href=\"/bw\">Backward</a></td>"
        "<td><a href=\"/r\">Right</a></td>"
    "</tr>"
    "<tr>"
        "<td><a href=\"/p\">+10</a></td>"
        "<td><a href=\"/s\">Step</a></td>"
        "<td><a href=\"/m\">-10</a></td>"
    "</tr>"
    "</ul>"
    "</body></html>\n";

    // Send the response to the client
    client.print(s);
    // The client will actually be disconnected
    // when the function returns and 'client' object is detroyed
}
