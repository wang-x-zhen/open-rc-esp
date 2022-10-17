#include <ESP8266WiFi.h>
#include <WiFiUdp.h>
#include <Servo.h>
#include <WebOTA.h>
#include <WiFiClient.h>
#include <WiFiServer.h>

const char *version = "1.0.7-dev.5";
const char *host = "ESP-OTA"; // Used for MDNS resolution
const char *ssid = "OpenRc";     // 手机热点网络名称
const char *password = "10241024"; // 手机热点网络密码
ADC_MODE(ADC_VCC);
WiFiUDP Udp;
unsigned int localUdpPort = 12345;
char incomingPacket[50];         // 接收缓冲区

// TCP
WiFiServer server(13026);    //创建server 端口号是13026
WiFiClient serverClient;

int len = 0;
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

void parseData();

void parseDataTcp();

void setup() {

//    for (int i = 0; i < 8; ++i) {
//        pinMode(gpioMaxList[i], OUTPUT);
//        digitalWrite(gpioMaxList[i], LOW);
//    }
    Serial.begin(115200);
    Serial.println();
    IPAddress IP = WiFi.softAPIP();
    Serial.printf("version:%s", version);
//    WiFi.softAP(ssid, password);
    WiFi.setAutoConnect(true);
    init_wifi(ssid, password, host);

    // 以下开启UDP监听并打印输出信息
    // Udp.begin(localUdpPort);

    // TCP
    server.begin();  //启动server
    server.setNoDelay(true);//关闭小包合并包功能，不会延时发送数据


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

uint8_t recvBytes[150];
int recvBytesLength = 0;

void loop() {
//    webota.handle();
    sendIp();

    if (server.hasClient()) {  //判断是否有新的client请求进来
        if (!serverClient || !serverClient.connected()) {
            if (serverClient) {
                serverClient.stop();  //停止旧的的连接
            }
        }
        serverClient = server.available();//分配最新的client
        Serial.println("server.hasClient()");
    }

    // 检测client发过来的数据
    if (serverClient && serverClient.connected()) {
        if (serverClient.available()) {//判断指定客户端是否有可读数据
            while (serverClient.available()) {
                recvBytes[recvBytesLength] = serverClient.read();
                if (recvBytesLength > 5 &&
                    recvBytesLength % 3 == 1 &&
                    recvBytes[recvBytesLength - 1] == 200 &&
                    recvBytes[recvBytesLength] == 201) {
                    parseDataTcp();
                    recvBytesLength = 0;
                } else {
                    recvBytesLength++;
                }
            }

        }
    }
}

void parseData() {
    for (int j = 0; j < len; j += 3) {
        int gpio = 0;
        int pwmMode = 0;
        int value = 0;
        gpio = incomingPacket[j];
        value = incomingPacket[j + 1];
        pwmMode = incomingPacket[j + 2];
        newGpioData[gpio][0] = pwmMode;
        newGpioData[gpio][1] = value;
    }
    executeCmd();
}


void parseDataTcp() {
    for (int j = 0; j <= recvBytesLength - 2; j += 3) {
        if (j + 2 > recvBytesLength - 2) {
            break;
        }
        int gpio = 0;
        int pwmMode = 0;
        int value = 0;
        gpio = recvBytes[j + 0];
        value = recvBytes[j + 1];
        pwmMode = recvBytes[j + 2];
        newGpioData[gpio][0] = pwmMode;
        newGpioData[gpio][1] = value;
    }
    executeCmd();
}
