#include <Adafruit_NeoPixel.h>
#include <ESP8266WiFi.h>

#include <cstdlib>

void serve(Stream &client, String method, String path, String payload);
void sendHeaders(Stream &client, int code, String codeDesc, bool codeDescBody);
void parsePost(String payload);
void handleField(String key, String value);
void sendForm(Stream &client);

const int ledCount = 30;
const int ledPin = 2;
const int ledMode = NEO_GRB + NEO_KHZ800;

Adafruit_NeoPixel strip = Adafruit_NeoPixel(ledCount, ledPin, ledMode);

const char *wifiSsid = "IntranetOfThings";
const char *wifiPass = "hunter2";

WiFiServer server(80);

void setup()
{
    Serial.begin(9600);
    strip.begin();
    strip.setPixelColor(0, 255, 0, 0);
    strip.show();
    WiFi.begin(wifiSsid, wifiPass);
    while (WiFi.status() != WL_CONNECTED) {
        delay(100);
    }
    strip.setPixelColor(0, 0, 255, 0);
    strip.show();
    server.begin();
    delay(500);
    strip.setPixelColor(0, 0, 0, 0);
    strip.show();
}

void loop()
{
    WiFiClient client = server.available();
    if (!client) {
        return;
    }
    String method = client.readStringUntil(' ');
    String path = client.readStringUntil(' ');
    String version = client.readStringUntil('\r');
    client.read();
    String line;
    int length = 0;
    String payload = "";
    do {
        line = client.readStringUntil('\r');
        client.read();
        line.toLowerCase();
        if (line.startsWith("content-length: ")) {
            length = line.substring(16).toInt();
        }
    } while (line != "");
    if (length > 0) {
        char *buffer = (char *) std::calloc(length + 1, 1);
        client.readBytes(buffer, length);
        payload = buffer;
        std::free(buffer);
    }
    serve(client, method, path, payload);
}

void serve(Stream &client, String method, String path, String payload)
{
    if (path != "/") {
        sendHeaders(client, 404, "Not Found", true);
        return;
    }
    if (method == "POST") {
        parsePost(payload);
    } else if (method != "GET") {
        sendHeaders(client, 405, "Method Not Allowed", true);
        return;
    }
    sendHeaders(client, 200, "OK", false);
    sendForm(client);
}

void sendHeaders(Stream &client, int code, String codeDesc, bool codeDescBody)
{
    client.printf("HTTP/1.1 %d %s \r\n", code, codeDesc.c_str());
    client.printf("Content-Type: text/html\r\n");
    client.printf("\r\n");
    if (codeDescBody) {
        client.printf("<h1>%d %s</h1>", code, codeDesc.c_str());
    }
}

void parsePost(String payload)
{
    String key;
    String value;
    String *dest = &key;
    for (int i = 0; i < payload.length(); ++i) {
        char c = payload.charAt(i);
        switch (c) {
        case '&':
            handleField(key, value);
            key = "";
            dest = &key;
            break;
        case '=':
            value = "";
            dest = &value;
            break;
        default:
            *dest += c;
        }
    }
    handleField(key, value);
    strip.show();
}

void handleField(String key, String value)
{
    if (!key.startsWith("led")) {
        return;
    }
    uint16_t led = key.substring(3).toInt();
    uint32_t color = strtol(value.c_str(), NULL, 16);
    strip.setPixelColor(led, color);
}

void sendForm(Stream &client)
{
    client.println("<!doctype html>");
    client.println("<html>");
    client.println("<body>");
    client.println("<form action=\"/\" method=\"post\">");
    client.println("<input type=\"submit\" value=\"Update\">");
    for (int i = 0; i < ledCount; ++i) {
        uint32_t v = strip.getPixelColor(i);
        client.printf("<br><input name=\"led%d\" type=\"text\" value=\"%06X\">\n", i, v);
    }
    client.println("</form>");
    client.println("</body>");
    client.println("</html>");
}
