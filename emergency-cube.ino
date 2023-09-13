#include <SPI.h>
#include <Ethernet.h>
#include <ArduinoHttpClient.h>
#include <ArduinoJson.h>
#include <ButtonDebounce.h>

#define BUTTON_PIN 0
#define LED_PIN 9

// set MAC address
#if defined(WIZ550io_WITH_MACADDRESS)  // Use assigned MAC address of WIZ550io
;
#else
byte mac[] = { 0x33, 0xCC, 0x2A, 0x14, 0x8F, 0x83 };
//byte mac[] = {0xC7, 0x1D, 0xD7, 0x04, 0xF1, 0xDA};
#endif

IPAddress ip(192, 168, 2, 40);

char serverAddress[] = "192.168.2.10";  // server address
int port = 4000;
unsigned msg_serial = 12290;

EthernetClient enet;
HttpClient client = HttpClient(enet, serverAddress, port);
ButtonDebounce button(BUTTON_PIN, 100); // PIN 3 with 250ms debounce time

void onButtonChange(const int state) {
  if (state == 0) {
    DynamicJsonDocument msg(128);
    msg["message_identifier"] = 4355;
    msg["serial_number"] = msg_serial++;
    msg["repetitions"] = 0;
    msg["message"] = "5G-MAG RT Emergency Warning Demo says: Do not press this button again.";
    String json;
    serializeJson(msg, json);

    client.beginRequest();
    client.post("/api/v1/warning_message");
    client.sendHeader("Content-Type", "application/json");
    client.sendHeader("Content-Length", json.length());
    client.beginBody();
    client.print(json);
    client.endRequest();
  }
}

unsigned char led_state = 0;
bool led_direction = false;
const byte analogPort = A1;

void setup() {
  Serial.begin(9600);
  Ethernet.begin(mac, ip);

  pinMode(LED_PIN, OUTPUT);
  pinMode(BUTTON_PIN, INPUT);

  unsigned long seed = getRandomSeed(31);
  Serial.print("Random seed: ");
  Serial.println(seed);
  randomSeed(seed);

  unsigned char random_msg_code = random(1,63);
  Serial.print("Random message code portion: ");
  Serial.println(random_msg_code);

  msg_serial = 0;
  msg_serial |= (random_msg_code & 0x3F) << 8;
  Serial.print("Message serial: ");
  Serial.println(msg_serial);
  button.setCallback(onButtonChange);

  // print out your local IP address
  Serial.print("My IP address is: ");
  for (byte thisByte = 0; thisByte < 4; thisByte++) {
    // print out four byte IP address
    Serial.print(Ethernet.localIP()[thisByte], DEC);
    Serial.print(".");
  }
  Serial.println();
}

void loop() {
  button.update();

  analogWrite(LED_PIN, led_state);
  if (led_direction) {
    led_state++;
    if (led_state == 255) led_direction = false;
  } else {
    led_state--;
    if (led_state == 0) led_direction = true;
  }

  delay(4);
}

long getRandomSeed(int numBits)
{
  // magic numbers tested 2016-03-28
  // try to speed it up
  // Works Well. Keep!
  //
  if (numBits > 31 or numBits <1) numBits = 31; // limit input range
 
  const int baseIntervalMs = 1UL; // minumum wait time
  const byte sampleSignificant = 7;  // modulus of the input sample
  const byte sampleMultiplier = 10;   // ms per sample digit difference

  const byte hashIterations = 3;
  int intervalMs = 0;

  unsigned long reading;
  long result = 0;
  int tempBit = 0;

  Serial.print("randomizing...");
  pinMode(analogPort, INPUT_PULLUP);
  pinMode(analogPort, INPUT);
  delay(200);
  // Now there will be a slow decay of the voltage,
  // about 8 seconds
  // so pick a point on the curve
  // offset by the processed previous sample:
  //

  for (int bits = 0; bits < numBits; bits++)
  {
      Serial.print('*');

    for (int i = 0; i < hashIterations; i++)
    {
//      Serial.print(' ');
//      Serial.print( hashIterations - i );
      delay(baseIntervalMs + intervalMs);

      // take a sample
      reading = analogRead(analogPort);
      tempBit ^= reading & 1;

      // take the low "digits" of the reading
      // and multiply it to scale it to
      // map a new point on the decay curve:
      intervalMs = (reading % sampleSignificant) * sampleMultiplier;
    }
    result |= (long)(tempBit & 1) << bits;
  }
  Serial.println(result);
//  Serial.println();
  return result;
}
