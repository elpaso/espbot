#include <ESP8266WiFi.h>
#include <AccelStepper.h>

//////////////////////
// WiFi Definitions //
//////////////////////
const char *ssid = "NodeBot1";
const char *password = "NodeBot1"; // 8 char min

/////////////////////
// Pin Definitions //
/////////////////////s

const int RED_LED_PIN = D0; // Red LED
const int BLUE_LED_PIN = D4; // Blue LED

char command = 'h'; // halted

bool immediate_command = true;

// Useful for testing: enter a sequence
String commands = "";
uint8_t stack_pointer = 0;



#define MAX_SPEED 1000
#define DEFAULT_SPEED 700
#define STEPPERS_LINE_STEPS 2664
#define STEPPERS_TURN_STEPS 1512
#define HALFSTEP 8

// Motor pin definitions
#define motorPin1_1  D1     // IN1 on the ULN2003 driver 1
#define motorPin1_2  D2     // IN2 on the ULN2003 driver 1
#define motorPin1_3  D3     // IN3 on the ULN2003 driver 1
#define motorPin1_4  D9     // IN4 on the ULN2003 driver 1

// Motor pin definitions
#define motorPin2_1  D8     // IN1 on the ULN2003 driver 1
#define motorPin2_2  D7     // IN2 on the ULN2003 driver 1
#define motorPin2_3  D6     // IN3 on the ULN2003 driver 1
#define motorPin2_4  D5     // IN4 on the ULN2003 driver 1

// Initialize with pin sequence IN1-IN3-IN2-IN4 for using the AccelStepper with 28BYJ-48
AccelStepper stepperLeft(HALFSTEP, motorPin1_4, motorPin1_2, motorPin1_3, motorPin1_1);

// Initialize with pin sequence IN1-IN3-IN2-IN4 for using the AccelStepper with 28BYJ-48
AccelStepper stepperRight(HALFSTEP, motorPin2_1, motorPin2_3, motorPin2_2, motorPin2_4);


uint16_t speed = MAX_SPEED / 2;
WiFiServer server(80);


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
    stepperLeft.setMaxSpeed(MAX_SPEED);
    stepperLeft.setAcceleration(300);
    stepperLeft.setSpeed(DEFAULT_SPEED);
    stepperRight.setMaxSpeed(DEFAULT_SPEED);
    stepperRight.setAcceleration(300);
    stepperRight.setSpeed(MAX_SPEED / 2);
}


void setSpeed(bool forward=true)
{
    stepperLeft.setSpeed(forward ?  speed: -speed);
    stepperRight.setSpeed(forward ? speed: -speed);
}

void setup()
{
    initHardware();
    setupWiFi();
    server.begin();
}

void forceStop()
{
    stepperLeft.setCurrentPosition(0);
    stepperRight.setCurrentPosition(0);
}

/**
 * Process a command
 */ 
void processCommand()
{
    uint8_t repeat = 1;
    
    if ( commands.length() && stack_pointer < commands.length() ){
        if ( immediate_command || ( ! stepperLeft.isRunning() && ! stepperRight.isRunning() ) ){
            digitalWrite(RED_LED_PIN, LOW);
            if ( immediate_command ) {
                forceStop();
            }
            immediate_command = false;
            // Get command
            command = commands.charAt(stack_pointer);
            // Advance the SP
            stack_pointer++;
            // Read ahead: get repetitions & GOTO destination
            if ( stack_pointer < commands.length() ){
                char c = commands.charAt(stack_pointer);
                if ( '0' <= c && c <= '9' ){
                    repeat = c - '0';
                    stack_pointer++;
                }
                if ( command == 'G' ) // GOTO destinations
                {
                    stack_pointer = repeat;
                    repeat = 1;
                    if ( stack_pointer < commands.length() )
                    {
                        command = commands.charAt( stack_pointer );
                        stack_pointer++;
                    }
                    else // Error
                    {
                        stack_pointer = 0;
                        command = 'h';
                    }
                }
            }
            // Process commands
            switch ( command ){
                case 'h':
                    // Already stopped
                    break;
                case 'f': // Forward
                    stepperLeft.move(repeat * STEPPERS_LINE_STEPS * 100);
                    stepperRight.move(repeat * STEPPERS_LINE_STEPS * 100);
                    setSpeed();
                    break; 
                case 'b': // Backward
                    stepperLeft.move(repeat * -STEPPERS_LINE_STEPS * 100);
                    stepperRight.move(repeat * -STEPPERS_LINE_STEPS * 100);
                    setSpeed(false);
                    break;
                case 'l': // left
                    stepperRight.move(STEPPERS_TURN_STEPS / 12);
                    setSpeed();
                    break;
                case 'r': // right
                    stepperLeft.move(STEPPERS_TURN_STEPS / 12);
                    setSpeed();
                    break;
                case 'U': // U-Turn
                    stepperLeft.move(repeat * STEPPERS_TURN_STEPS * 2);
                    stepperRight.move(repeat * -STEPPERS_TURN_STEPS * 2);
                    break;
                case 'L': // Turn left
                    stepperLeft.move(repeat * -STEPPERS_TURN_STEPS);
                    stepperRight.move(repeat * STEPPERS_TURN_STEPS);
                    break;
                case 'R': // Turn right
                    stepperLeft.move(repeat * STEPPERS_TURN_STEPS);
                    stepperRight.move(repeat * -STEPPERS_TURN_STEPS);
                    break;
                case 'S':
                    stepperLeft.move(repeat * STEPPERS_LINE_STEPS);
                    stepperRight.move(repeat * STEPPERS_LINE_STEPS);
                    break;
                case 'p': // Speed +
                    speed += repeat * MAX_SPEED / 10;
                    if ( speed > MAX_SPEED) {
                        speed = MAX_SPEED;
                    }
                    setSpeed();
                    break;
                case 'm': // Speed -
                    speed -= repeat * MAX_SPEED / 10;
                    if ( speed < 0) {
                        speed = 0;
                    }
                    setSpeed();
                break;
            }
        }
    }
    else 
    {
        digitalWrite(RED_LED_PIN, HIGH);
        stack_pointer = 0;
        commands = "";
    }
}

/**
 * Move the engine
 */ 
void engine() {
    
    switch( command ) {
        case ' ': // Stop
        case 'h': // Stop
            break;
        case 'l':
            stepperRight.runSpeed();
            break;
        case 'r':
            stepperLeft.runSpeed();
            break;
        case 'b':
        case 'f':
            stepperLeft.runSpeed();
            stepperRight.runSpeed();
            break;
        default:
            stepperLeft.run();
            stepperRight.run();
            break;
    }
}

void loop()
{

    // Check if a client has connected
    WiFiClient client = server.available();
    
    if (!client) {
        processCommand();
        engine();
        digitalWrite(BLUE_LED_PIN, HIGH);
        return;
    }
    
    digitalWrite(BLUE_LED_PIN, LOW);

    // Read the first line of the request
    String req = client.readStringUntil('\r');
    if ( req.indexOf( '?' ) >= 0 ) // Strip query string
    {
        req = req.substring(0, req.indexOf( '?' ));
    }
    if ( req.indexOf( "GET " ) >= 0 ) 
    {
        req = req.substring(4);
    }
    client.flush();

    commands = ""; 
    // Immediate commands are lowercase and executed immediately
    
    if ( req.indexOf("/cmd") != -1 ){
        commands = req.substring(5);
    }
    else if (req.indexOf("/h ") != -1)
    {
        commands = 'h'; // Stop
    }
    else if (req.indexOf("/f ") != -1)
    {
        commands = 'f'; // Move forward
    }
    else if (req.indexOf("/b ") != -1)
    {
        commands = 'b'; // Move backward
    }
    else if (req.indexOf("/U ") != -1)
    {
        commands = 'U'; // U Turn
    }
    else if (req.indexOf("/L ") != -1)
    {
        commands = 'L'; // Turn left
    }
    else if (req.indexOf("/R ") != -1)
    {
        commands = 'R'; // Turn right
    }
    else if (req.indexOf("/l ") != -1)
    {
        commands = 'l'; // Turn right
    }
    else if (req.indexOf("/r ") != -1)
    {
        commands = 'r'; // Turn right
    }
    else if (req.indexOf("/S ") != -1)
    {
        commands = 'S'; // Step
    }
    else if ( req.indexOf("/p ") != -1) {
        commands = 'p';
    }
    else if ( req.indexOf("/m ") != -1) {
        commands = 'm';
    }
    
    if ( req.indexOf("/favicon") != -1){
        client.flush();
        client.print(F("HTTP/1.1 404 NOT FOUND\r\n"));
        return;
    }

    stack_pointer = 0;
    immediate_command = isLowerCase( commands.charAt(0) );
    
    client.flush();

    // Send the response to the client
    // The client will actually be disconnected
    // when the function returns and 'client' object is detroyed
    // Prepare the response. Start with the common header:
    client.print(F("HTTP/1.1 200 OK\r\n"
    "Content-Type: text/html\r\n\r\n"
    "<!DOCTYPE HTML>\r\n<html><head><style type=\"text/css\">"
    "<!--meta name=\"viewport\" content=\"target-densitydpi=device-dpi; width=device-width; initial-scale=1.0; maximum-scale=1.0; user-scalable=0;\" /-->"
    "* {font-family: arial; font-size: 80px;}"
    "table {width:100%; max-width: 1000px; padding: 10px;}"
    "td {width:30%; border: solid 2px grey;  text-align: center; border-radius: 10%;}"
    "td.i {background-color: #ccc;}"
    "td a {display:inline-block; font-weight: bold; valign: middle; padding: 40px 0; width: 100%;}"
    ""
    "</style></head><body>\r\n"
    "<div>"
    //"<p>ori: '" + ori + "'</p>"
    //"<p>req: '" + req + "'</p>"
    //"<p>old command: '" + command + "'</p>"
    //"<p>commands: '" + commands + "'</p>"
    "<table><tr>"
        "<td class=\"i\"><a href=\"/h\">Stop</a></td>"
        "<td class=\"i\"><a href=\"/f\">Forward</a></td>"
        "<td></td>"
    "</tr>"
    "<tr>"
        "<td><a href=\"/L\">Turn Left</a></td>"
        "<td><a href=\"/U\">U Turn</a></td>"
        "<td><a href=\"/R\">Turn Right</a></td>"
    "</tr>"
    "<tr>"
        "<td class=\"i\"><a href=\"/l\">Left</a></td>"
        "<td class=\"i\"><a href=\"/b\">Backward</a></td>"
        "<td class=\"i\"><a href=\"/r\">Right</a></td>"
    "</tr>"
    "<tr>"
        "<td class=\"i\"><a href=\"/p\">+10</a></td>"
        "<td><a href=\"/S\">Step</a></td>"
        "<td class=\"i\"><a href=\"/m\">-10</a></td>"
    "</tr>"
    "</table></div>"
    "<div>Syntax: S(tep) L(eft) R(ight) G(oTo)x U(-turn) </div>"
    "<form onsubmit=\"this.action+=this.c.value;this.submit()\" action=\"/cmd/\" method=\"get\"><input name=\"c\" type=\"text\" /></input> <button type=\"submit\">Send</button></form>"
    "</body></html>\n"));

}
