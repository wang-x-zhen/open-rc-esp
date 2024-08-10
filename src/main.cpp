#include <ESP8266WiFi.h>
#include <WiFiUdp.h>
#include <Servo.h>
#include <WebOTA.h>

#define IN1 12
#define IN2 14
#define IN3 16
#define IN4 13

#define LED_RED 2
#define LED_WHITE 15
#define MOTO_A 4
#define MOTO_B 5

const char *version = "1.0.8";
const char *host = "ESP-OTA"; // Used for MDNS resolution
const char *ssid = "OpenRc";     // 手机热点网络名称
const char *password = "10241024"; // 手机热点网络密码
ADC_MODE(ADC_VCC);
WiFiUDP Udp;
unsigned int localUdpPort = 12345;
char incomingPacket[50];         // 接收缓冲区
int len = 0;


void setup() {
    pinMode(IN1, OUTPUT);
    pinMode(IN2, OUTPUT);
    pinMode(IN3, OUTPUT);
    pinMode(IN4, OUTPUT);
    pinMode(LED_RED, OUTPUT);
    pinMode(LED_WHITE, OUTPUT);
    pinMode(MOTO_A, OUTPUT);
    pinMode(MOTO_B, OUTPUT);

    Serial.begin(115200);
    Serial.println();
    Serial.printf("version:%s", version);
    WiFi.softAP(ssid, password);
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

void setStep(int in1, int in2, int in3, int in4) {
    digitalWrite(IN1, in1);
    digitalWrite(IN2, in2);
    digitalWrite(IN3, in3);
    digitalWrite(IN4, in4);
}

//        A     B     C     D     E     F     G     H
//A       1     1     0     0     0     0     0     1
//A-      0     0     0     1     1     1     0     0
//B       0     1     1     1     0     0     0     0
//B-      0     0     0     0     0     1     1     1
int speedTime = 1;
void clockwise() {
    setStep(0, 1, 1, 1);
    delay(speedTime);
    setStep(0, 1, 0, 1);
    delay(speedTime);
    setStep(1, 1, 0, 1);
    delay(speedTime);
    setStep(1, 0, 0, 1);
    delay(speedTime);
    setStep(1, 0, 1, 1);
    delay(speedTime);
    setStep(1, 0, 1, 0);
    delay(speedTime);
    setStep(1, 1, 1, 0);
    delay(speedTime);
    setStep(0, 1, 1, 0);
    delay(speedTime);
}


void counterclockwise() {
    setStep(0, 1, 1, 0);
    delay(speedTime);
    setStep(1, 1, 1, 0);
    delay(speedTime);
    setStep(1, 0, 1, 0);
    delay(speedTime);
    setStep(1, 0, 1, 1);
    delay(speedTime);
    setStep(1, 0, 0, 1);
    delay(speedTime);
    setStep(1, 1, 0, 1);
    delay(speedTime);
    setStep(0, 1, 0, 1);
    delay(speedTime);
    setStep(0, 1, 1, 1);
    delay(speedTime);
}

void loop() {
    sendIp();
    // 步进电机顺时针旋转
    for (int i = 0; i < 50; ++i) {
        clockwise();
    }
    setStep(1,1,1,1);
    delay(2000);
//     步进电机逆时针旋转
    for (int i = 0; i < 50; ++i) {
        counterclockwise();
    }
    setStep(1,1,1,1);
    delay(2000);

    int packetSize = Udp.parsePacket(); //获取当前队首数据包长度
    if (packetSize)                     // 有数据可用
    {
        if (Udp.remoteIP().toString() == WiFi.localIP().toString()) {
            // 自己发送放入广播 不处理
            return;
        }
        len = Udp.read(incomingPacket, 50); // 读取数据到incomingPacket
        if (len > 0)                             // 如果正确读取
        {

        }
    }
}
