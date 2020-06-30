/*
   A script to request an API, parsing the input and updating the connected DMX system
   Created by Dennis Vinke, December 5, 2017
   DmxMaster library by Peter Knight
*/
#include <ArduinoHttpClient.h>
#include <Ethernet2.h>
#include <SPI.h>
#include <DmxMaster.h>

#define MAX_CHANNELS 512

byte mac[] = { 0x90, 0xA2, 0xDA, 0x10, 0x5E, 0xE2 };  //MAC ETHERNETSHIELD
char serverName[] = "http.saproto.nl";
EthernetClient Ethernetconnection;
int port = 80;
HttpClient client = HttpClient(Ethernetconnection, serverName, port);

String response;
int statusCode = 0;

int dmxChannel[MAX_CHANNELS];

/*
   Set up Serial communication, DMX connection and reset all the lights
*/
void setup()
{
  Serial.begin(9600);
  while (!Serial) {
    ; // wait for serial port to connect. Needed for native USB port only
  }

  //Setup DMX Shield
  DmxMaster.usePin(3);
  DmxMaster.maxChannel(MAX_CHANNELS);

  // Start the Ethernet connection
  if (Ethernet.begin(mac) == 0) {
    Serial.println("Failed to configure Ethernet using DHCP");
    // No point in carrying on, so do nothing forevermore:
    for (;;)
      ;
  }

  resetDMX();  //turn off all lights on startup
}

void loop()
{
  makeAPIRequest();
  client = HttpClient(Ethernetconnection, serverName, port);
  response = client.responseBody();
  parseJson(response);
  Ethernetconnection.stop();
  updateDMX();
  delay(5000);
}

/*
   Sets all the DMX channels to zero to turn all the DMX sytems off
*/
void resetDMX() {
  Serial.println("Resetting DMX");
  for (int i = 0; i < MAX_CHANNELS; i++) {
    DmxMaster.write(i, 0);
  }
}

/*
   Parses a JSON string to a set of values and update the locally stored channel values based on the parsed JSON
   Channels are seperated by \" while values use : || , as a delimiter.
*/
void parseJson(String response) {
  Serial.println("JSON is being parsed" + response);

  bool channelConCatToggle = false; //Used to determine what the parsed object is
  bool valueConCatToggle = false; //Used to determine what the parsed object is
  String channel = "";
  String value = "";
  int tempChan = 0;
  int tempVal = 0;

  //for loop looks if current char of string is a delimiter
  for (int i = 0; i < response.length(); i++) {
    if (response.charAt(i) == '\"') {
      channelConCatToggle = !channelConCatToggle;
      //if all values between two delimiters are stored in a string, store the string in a temp int
      if (!channelConCatToggle) {
        tempChan = channel.toInt();
        channel = "";
      }
    }
    else if (response.charAt(i) == ':' || response.charAt(i) == ',') {
      valueConCatToggle = !valueConCatToggle;
      if (!valueConCatToggle) {
        tempVal = value.toInt();
        // if channel and value are parsed, update the local channels value
        setDMXChannel(tempChan, tempVal);
        value = "";
      }
    }
    //Add the character to the correct string
    else if (channelConCatToggle) {
      channel += (char)response.charAt(i);
    }
    else if (valueConCatToggle) {
      value += (char)response.charAt(i);
    }

  }
}

/*
   Requests the needed data with a GET request from "Host: http.saproto.nl" to update the DMX system
*/
void makeAPIRequest() {
  //reconnect if the connection to the server is lost
  if (!Ethernetconnection.connected()) {
    Serial.println("Reconnecting");
    Ethernetconnection.connect(serverName, 80);
  }
  Serial.println("connected");
  Ethernetconnection.println("GET /dmx_proxy/ HTTP/1.1");
  Ethernetconnection.println("Host: http.saproto.nl");
  Ethernetconnection.println();
}

/*
   Send the updated values to the DMX system
*/
void updateDMX() {
  for (int i = 0; i < MAX_CHANNELS; i++) {
    DmxMaster.write(i, dmxChannel[i]);
  }
}

/*
   Set the value of a channel so it can be send during the next update
*/
void setDMXChannel(int channel, int value) {
  Serial.println("The updating values are");
  Serial.println(channel);
  Serial.println(value);
  dmxChannel[channel] = value;
}


