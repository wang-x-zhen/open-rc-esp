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
int CMD_LEN = 6;
int direction = 24;
int directionMax = 50;
int directionCmd = 24;
int motoCmd = 90;
int lightRedCmd = 0;
int lightWhiteCmd = 0;


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
int speedTime = 2000;

void clockwise() {
    setStep(0, 1, 1, 1);
    delayMicroseconds(speedTime);
    setStep(0, 1, 0, 1);
    delayMicroseconds(speedTime);
    setStep(1, 1, 0, 1);
    delayMicroseconds(speedTime);
    setStep(1, 0, 0, 1);
    delayMicroseconds(speedTime);
    setStep(1, 0, 1, 1);
    delayMicroseconds(speedTime);
    setStep(1, 0, 1, 0);
    delayMicroseconds(speedTime);
    setStep(1, 1, 1, 0);
    delayMicroseconds(speedTime);
    setStep(0, 1, 1, 0);
    delayMicroseconds(speedTime);
}


void counterclockwise() {
    setStep(0, 1, 1, 0);
    delayMicroseconds(speedTime);
    setStep(1, 1, 1, 0);
    delayMicroseconds(speedTime);
    setStep(1, 0, 1, 0);
    delayMicroseconds(speedTime);
    setStep(1, 0, 1, 1);
    delayMicroseconds(speedTime);
    setStep(1, 0, 0, 1);
    delayMicroseconds(speedTime);
    setStep(1, 1, 0, 1);
    delayMicroseconds(speedTime);
    setStep(0, 1, 0, 1);
    delayMicroseconds(speedTime);
    setStep(0, 1, 1, 1);
    delayMicroseconds(speedTime);
}


void clockwise2() {
    setStep(1, 0, 0, 0);
    delayMicroseconds(speedTime);
    setStep(1, 0, 1, 0);
    delayMicroseconds(speedTime);
    setStep(0, 0, 1, 0);
    delayMicroseconds(speedTime);
    setStep(0, 1, 1, 0);
    delayMicroseconds(speedTime);
    setStep(0, 1, 0, 0);
    delayMicroseconds(speedTime);
    setStep(0, 1, 0, 1);
    delayMicroseconds(speedTime);
    setStep(0, 0, 0, 1);
    delayMicroseconds(speedTime);
    setStep(1, 0, 0, 1);
    delayMicroseconds(speedTime);
}


void counterclockwise2() {
    setStep(1, 0, 0, 1);
    delayMicroseconds(speedTime);
    setStep(0, 0, 0, 1);
    delayMicroseconds(speedTime);
    setStep(0, 1, 0, 1);
    delayMicroseconds(speedTime);
    setStep(0, 1, 0, 0);
    delayMicroseconds(speedTime);
    setStep(0, 1, 1, 0);
    delayMicroseconds(speedTime);
    setStep(0, 0, 1, 0);
    delayMicroseconds(speedTime);
    setStep(1, 0, 1, 0);
    delayMicroseconds(speedTime);
    setStep(1, 0, 0, 0);
    delayMicroseconds(speedTime);
}

void initDirection() {
    for (int i = 0; i < directionMax; ++i) {
        clockwise();
    }
    setStep(1, 1, 1, 1);
    delay(1000);
    for (int i = 0; i < directionMax / 2; ++i) {
        counterclockwise();
    }
}

void moveDirection() {
    if (direction < directionCmd) {
        clockwise();
        direction++;
    }
    if (direction > directionCmd) {
        counterclockwise();
        direction--;
    }
}

void moveMoto() {
    if (motoCmd < 90) {
        digitalWrite(MOTO_A, LOW);
        analogWrite(MOTO_B, (int)((90 - motoCmd)*2.8));
    } else if (motoCmd > 90) {
        analogWrite(MOTO_A, (int)((motoCmd-90)*2.8));
        digitalWrite(MOTO_B, LOW);
    } else {
        digitalWrite(MOTO_A, HIGH);
        digitalWrite(MOTO_B, HIGH);
    }
}

void lights() {
    analogWrite(LED_WHITE, (int)(lightWhiteCmd * 1.4));
    if (lightRedCmd == 180) {
        if (millis() / 500 % 2 == 0) {
            analogWrite(LED_RED, 90);
        } else {
            digitalWrite(LED_RED, LOW);
        }
    } else {
        analogWrite(LED_RED, lightRedCmd * 2);
    }

}

void parseCmd() {
    // 遍历数组中的每个字符
    for (int i = 0; i < 6; i++) {
        Serial.print(" ");
        Serial.print((int)incomingPacket[i]);  // 打印对应的十进制ASCII值
    }
    directionCmd = (int) (incomingPacket[3] / 3.6);
    motoCmd = incomingPacket[0];
    lightWhiteCmd = incomingPacket[4];
    lightRedCmd = incomingPacket[5];
    Serial.printf("\ndirectionCmd:%d motoCmd:%d\n", directionCmd, motoCmd);
}

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
//    WiFi.softAP(ssid, password);
    WiFi.setAutoConnect(true);
    init_wifi(ssid, password, host);

    //以下开启UDP监听并打印输出信息
    Udp.begin(localUdpPort);
    Serial.printf("Now listening at IP %s, UDP port %d\n", WiFi.localIP().toString().c_str(), localUdpPort);
    initDirection();
}

void loop() {
    sendIp();
    moveDirection();
    moveMoto();
    lights();
    int packetSize = Udp.parsePacket(); //获取当前队首数据包长度
    if (packetSize)                     // 有数据可用
    {
        if (Udp.remoteIP().toString() == WiFi.localIP().toString()) {
            // 自己发送放入广播 不处理
            return;
        }
        len = Udp.read(incomingPacket, 50); // 读取数据到incomingPacket
        if (len == CMD_LEN)                             // 如果正确读取
        {
            parseCmd();
        }
    }
}
