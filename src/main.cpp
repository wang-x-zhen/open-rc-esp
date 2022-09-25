#include <ESP8266WiFi.h>
#include <WiFiUdp.h>
#include <Servo.h>
#include <ArduinoJson.h>
#include <WebOTA.h>

const char *version = "1.0.3";
const char *host = "ESP-OTA"; // Used for MDNS resolution
const char *ssid = "OpenRc";     // 手机热点网络名称
const char *password = "10241024"; // 手机热点网络密码
ADC_MODE(ADC_VCC);
WiFiUDP Udp;
unsigned int localUdpPort = 12345;
char incomingPacket[537];         // 接收缓冲区
// 默认是0 设置为-1
int oldGpioData[17][2] = {{-1, -1},
                          {-1, -1},
                          {-1, -1},
                          {-1, -1},
                          {-1, -1},
                          {-1, -1},
                          {-1, -1},
                          {-1, -1},
                          {-1, -1},
                          {-1, -1},
                          {-1, -1},
                          {-1, -1},
                          {-1, -1},
                          {-1, -1},
                          {-1, -1},
                          {-1, -1},
                          {-1, -1},};
int newGpioData[17][2] = {{-1, -1},
                          {-1, -1},
                          {-1, -1},
                          {-1, -1},
                          {-1, -1},
                          {-1, -1},
                          {-1, -1},
                          {-1, -1},
                          {-1, -1},
                          {-1, -1},
                          {-1, -1},
                          {-1, -1},
                          {-1, -1},
                          {-1, -1},
                          {-1, -1},
                          {-1, -1},
                          {-1, -1},};
Servo servoList[17];
const int GPIO_COUNT = 9;
int gpioList[GPIO_COUNT] = {16, 14, 12, 13, 1, 3, 5, 4, 2};

void excuteCmd();

void setup() {

    Serial.begin(115200);
    Serial.println();
    IPAddress IP = WiFi.softAPIP();
    Serial.printf("version:%s", version);
//    WiFi.softAP(ssid, password);
    WiFi.setAutoConnect(true);
    init_wifi(ssid, password, host);

    //以下开启UDP监听并打印输出信息
    Udp.begin(localUdpPort);
    Serial.printf("Now listening at IP %s, UDP port %d\n", WiFi.localIP().toString().c_str(), localUdpPort);
}

long lastTimeSendIp = 0;

void sendIp() {
    long now = millis();
    if ((now - lastTimeSendIp) < 5000) {
        return;
    }
    lastTimeSendIp = now;
    Udp.beginPacket(("" + String(WiFi.localIP()[0]) +
                     "." + String(WiFi.localIP()[1])
                     + "." + String(WiFi.localIP()[2]) + ".255").c_str(), 18888);

    String data = "EspRcRx" + String(ESP.getChipId()) + ",ADC:" + String(ESP.getVcc()) + ",Version:" + String(version);
    Udp.write(data.c_str());
    Udp.endPacket();
//    Serial.println("sendIp");
}

void executeCmd() {
    for (int i = 0; i < 17; ++i) {
        if (newGpioData[i][0] == -1) {
            if (oldGpioData[i][0] != -1) {
                pinMode(i, INPUT);
            }
            oldGpioData[i][0] = -1;
            oldGpioData[i][1] = -1;
            Serial.println(" continue ");
            continue;
        }
        if (newGpioData[i][0] != oldGpioData[i][0]) { // 需要转变PWM模式
            Serial.println(" change PWM ");
            pinMode(i, OUTPUT);
            if (newGpioData[i][0] == 0) { // 舵机
                Servo servo;
                servo.attach(i);
                servo.write(newGpioData[i][1]);
                servoList[i] = servo;
            } else if (newGpioData[i][0] == 1) {  // PWM直驱
                analogWrite(i, newGpioData[i][1] * 6);// 1024
                Serial.println(" analogWrite  1 port:" + String(i) + " v:" + String(newGpioData[i][1] * 6));
            } else {
                if (&servoList[i] != nullptr) {
                    servoList[i].detach();
                }
                digitalWrite(i, LOW);
                Serial.println(" digitalWrite 1  LOW port:" + String(i) + " v:" + String(newGpioData[i][1] * 6));
            }
        } else { // 不需要转变PWM模式
            if (newGpioData[i][0] == 0) { // 舵机
                servoList[i].write(newGpioData[i][1]);
            } else if (newGpioData[i][0] == 1) {  // PWM直驱
                analogWrite(i, newGpioData[i][1] * 6);// 1024
            } else {
                if (&servoList[i] != nullptr) {
                    servoList[i].detach();
                }
                digitalWrite(i, LOW);
            }
        }
        oldGpioData[i][0] = newGpioData[i][0];
        oldGpioData[i][1] = newGpioData[i][1];
    }
}


void loop() {
    webota.handle();
    sendIp();
    int packetSize = Udp.parsePacket(); //获取当前队首数据包长度
    if (packetSize)                     // 有数据可用
    {
//        Serial.printf("Received %d bytes from %s, port %d\n", packetSize, Udp.remoteIP().toString().c_str(),
//                      Udp.remotePort());
        if (Udp.remoteIP().toString() == WiFi.localIP().toString()) {
            // 自己发送放入广播 不处理
            return;
        }
        int len = Udp.read(incomingPacket, 600); // 读取数据到incomingPacket
        if (len > 0)                             // 如果正确读取
        {
            incomingPacket[len] = 0; //末尾补0结束字符串
            unsigned char count = 0;
            unsigned char count2 = 0;
            for (int i = 0; i < len; ++i) {
                if ((char) incomingPacket[i] == '{') {
                    count++;
                }
                if ((char) incomingPacket[i] == '}') {
                    count2++;
                }
            }
            if (count != count2) {
                return;
            }
//            Serial.printf("---count  %s\n", String(count).c_str());
            String data = String(incomingPacket);
            Serial.printf("UDP packet contents: %s\n", incomingPacket);
            if (data.indexOf("g") != -1) {
                DynamicJsonDocument doc(600);
                deserializeJson(doc, data);
                int gpio = 0;
                int pwmMode = 0;
                int value = 0;
                for (int k = 0; k < count; k++) {
                    gpio = doc[k]["g"];
                    pwmMode = doc[k]["p"];
                    value = doc[k]["v"];
                    newGpioData[gpio][0] = pwmMode;
                    newGpioData[gpio][1] = value;
                }
                executeCmd();
            }
        }
    }
}