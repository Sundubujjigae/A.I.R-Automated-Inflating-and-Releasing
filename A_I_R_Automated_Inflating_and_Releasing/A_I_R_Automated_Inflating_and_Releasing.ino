#include <Keypad.h>
#include <Servo.h>
#include <math.h>

// 키패드 설정
const byte ROWS = 4;    // 행(rows) 개수
const byte COLS = 4;    // 열(columns) 개수
char keys[ROWS][COLS] = {
  {'1','2','3','A'},
  {'4','5','6','B'},
  {'7','8','9','C'},
  {'*','0','#','D'}
};

byte rowPins[ROWS] = {6, 7, 8, 9};   // 행 핀
byte colPins[COLS] = {5, 4, 3, 2};    // 열 핀

Keypad keypad = Keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS);

// 서보 모터 설정
Servo Servo1;
int servoPin = 11;
int potPin = A0;  // 아날로그 핀 A0으로 변경

// 릴레이와 FSR 설정
int relayPin1 = 17;      // 릴레이 1 핀 번호
int relayPin2 = 16;      // 릴레이 2 핀 번호
int relayPin3 = 15;      // 릴레이 3 핀 번호
int fsrPin1 = A1;       // 첫 번째 FSR 핀 번호
int fsrPin2 = A2;       // 두 번째 FSR 핀 번호
int fsrPin3 = A3;       // 세 번째 FSR 핀 번호
int fsrPin4 = A4;       // 네 번째 FSR 핀 번호
int fsrPin5 = A5;       // 다섯 번째 FSR 핀 번호
int fsrPin6 = A6;       // 여섯 번째 FSR 핀 번호
int fsrPin7 = A7;       // 일곱 번째 FSR 핀 번호
int fsrPin8 = A8;       // 여덟 번째 FSR 핀 번호
int fsrPin9 = A9;       // 아홉 번째 FSR 핀 번호

int systemState = 0;          // 현재 상태
int fsrThreshold;          // 임계값 (초기값 없음)

int fsrReading1;        // 첫 번째 FSR의 아날로그 값
int fsrReading2;        // 두 번째 FSR의 아날로그 값
int fsrReading3;        // 세 번째 FSR의 아날로그 값
int fsrReading4;        // 네 번째 FSR의 아날로그 값
int fsrReading5;        // 다섯 번째 FSR의 아날로그 값
int fsrReading6;        // 여섯 번째 FSR의 아날로그 값
int fsrReading7;        // 일곱 번째 FSR의 아날로그 값
int fsrReading8;        // 여덟 번째 FSR의 아날로그 값
int fsrReading9;        // 아홉 번째 FSR의 아날로그 값

bool state1Active = false; // 상태 1 활성화 여부
bool state4Active = false; // 상태 4 활성화 여부
bool activateRelay2 = false; // 릴레이 2 활성화 여부
bool activateRelay3 = false; // 릴레이 3 활성화 여부
bool fsrMeasurementActive = false; // FSR 측정 활성화 여부
unsigned long previousMillis = 0; // 이전 밀리초 값 저장
const long relayInterval = 1000;       // 릴레이 1 작동 간격 (1초)
unsigned long previousPrintMillis = 0; // FSR 값 출력 이전 밀리초 값 저장
bool relay1State = false;  // 릴레이 1 상태 저장
bool relay3State = false;  // 릴레이 3 상태 저장
int relay1OnOffCount = 0;   // 릴레이 1 on/off 횟수
int relay3OnOffCount = 0;   // 릴레이 3 on/off 횟수

const int fsrNoiseThreshold = 40; // FSR 값 튐 방지 임계값
const int fsrReadingStableThreshold = 10; // FSR 입력이 안정적인지 확인하기 위한 임계값
const int fsrStableReadingsRequired = 3; // 안정적인 FSR 읽기 횟수

bool returnToState0Printed = false; // 상태 0으로 돌아가는 메시지 중복 방지

// 함수 선언
void printRelayCounts();
float convertFSRToForce(int fsrValue);
void handleState5();
void printFSRReadings(int times = 1);

void setup() {
  // 서보 모터 설정
  Servo1.attach(servoPin);

  // 릴레이 핀 설정
  pinMode(relayPin1, OUTPUT);
  pinMode(relayPin2, OUTPUT);
  pinMode(relayPin3, OUTPUT);
  digitalWrite(relayPin1, HIGH);
  digitalWrite(relayPin2, HIGH);
  digitalWrite(relayPin3, HIGH);

  // 시리얼 통신 시작
  Serial.begin(9600);
  Serial.println("Setup complete.");
}

void loop() {
  // 서보 모터 제어
  int reading = analogRead(potPin);
  int angle = map(reading, 0, 1023, 180, 0);
  Servo1.write(angle);

  // 키패드 입력 처리
  char key = keypad.getKey();

  if (key != NO_KEY) {
    returnToState0Printed = false; // 키 입력 시 상태 0 메시지 초기화
    switch (key) {
      case '1': case '2': case '3': case 'A': 
        systemState = 1;
        state1Active = true;
        state4Active = false; // 상태 4 비활성화
        relay1OnOffCount = 0;   // 릴레이 사이클 수 초기화
        fsrThreshold = (key == '1') ? 200 : (key == '2') ? 400 : (key == '3') ? 600 : 800;
        Serial.println("State changed to 1");
        Serial.print("Threshold set to: ");
        Serial.println(fsrThreshold);
        break;
      case '0':
        systemState = 2;  // 상태 2로 변경
        Serial.println("State changed to 2");
        activateRelay2 = true; // 릴레이 2 활성화
        state1Active = false;  // 상태 1 비활성화
        state4Active = false;  // 상태 4 비활성화
        relay1OnOffCount = 0;    // 릴레이 사이클 수 초기화
        break;
      case '*':
        systemState = 3; // 상태 3으로 설정
        Serial.println("State changed to 3");
        activateRelay3 = true; // 릴레이 3 활성화
        state1Active = false;  // 상태 1 비활성화
        state4Active = false;  // 상태 4 비활성화
        digitalWrite(relayPin3, HIGH);
        relay3State = false; // 릴레이 3을 항상 OFF 상태로 설정
        printRelayCounts(); // 릴레이 카운트 출력 추가
        printFSRReadings(3);
       
        systemState = 0;
        break;
      case '7': case '8': case '9': case 'C':
        systemState = 4;
        state4Active = true;
        state1Active = false;  // 상태 1 비활성화
        fsrThreshold = (key == '7') ? 200 : (key == '8') ? 400 : (key == '9') ? 600 : 800;
        Serial.println("State changed to 4");
        Serial.print("Threshold set to: ");
        Serial.println(fsrThreshold);
        break;
      case '#':
        systemState = 5;  // 상태 5로 변경
        Serial.println("State changed to 5");
        handleState5();
        break;
      case '6':
        fsrMeasurementActive = true;
        Serial.println("FSR measurement start");
        break;
      case 'B':
        fsrMeasurementActive = false;
        Serial.println("FSR measurement stop");
        break;
      default:
        systemState = 0;
        state1Active = false;
        state4Active = false;
        activateRelay2 = false;
        activateRelay3 = false;
        relay1OnOffCount = 0;
        relay3OnOffCount = 0;
        relay3State = false;
        digitalWrite(relayPin3, HIGH);
        digitalWrite(relayPin1, HIGH);
        break;
    }
  }

  // FSR 측정이 활성화된 경우
  if (fsrMeasurementActive) {
    fsrReading1 = analogRead(fsrPin1);
    fsrReading2 = analogRead(fsrPin2);
    fsrReading3 = analogRead(fsrPin3);
    fsrReading4 = analogRead(fsrPin4);
    fsrReading5 = analogRead(fsrPin5);
    fsrReading6 = analogRead(fsrPin6);
    fsrReading7 = analogRead(fsrPin7);
    fsrReading8 = analogRead(fsrPin8);
    fsrReading9 = analogRead(fsrPin9);

    unsigned long currentMillis = millis();

    if (currentMillis - previousPrintMillis >= 500) {
      previousPrintMillis = currentMillis;
      Serial.print("FSR1 = ");
      Serial.print(fsrReading1);
      Serial.print(" (");
      Serial.print(convertFSRToForce(fsrReading1));
      Serial.print("g)\t");
      Serial.print("FSR2 = ");
      Serial.print(fsrReading2);
      Serial.print(" (");
      Serial.print(convertFSRToForce(fsrReading2));
      Serial.print("g)\t");
      Serial.print("FSR3 = ");
      Serial.print(fsrReading3);
      Serial.print(" (");
      Serial.print(convertFSRToForce(fsrReading3));
      Serial.print("g)\t");
      Serial.print("FSR4 = ");
      Serial.print(fsrReading4);
      Serial.print(" (");
      Serial.print(convertFSRToForce(fsrReading4));
      Serial.print("g)\t");
      Serial.print("FSR5 = ");
      Serial.print(fsrReading5);
      Serial.print(" (");
      Serial.print(convertFSRToForce(fsrReading5));
      Serial.print("g)\t");
      Serial.print("FSR6 = ");
      Serial.print(fsrReading6);
      Serial.print(" (");
      Serial.print(convertFSRToForce(fsrReading6));
      Serial.print("g)\t");
      Serial.print("FSR7 = ");
      Serial.print(fsrReading7);
      Serial.print(" (");
      Serial.print(convertFSRToForce(fsrReading7));
      Serial.print("g)\t");
      Serial.print("FSR8 = ");
      Serial.print(fsrReading8);
      Serial.print(" (");
      Serial.print(convertFSRToForce(fsrReading8));
      Serial.print("g)\t");
      Serial.print("FSR9 = ");
      Serial.print(fsrReading9);
      Serial.print(" (");
      Serial.print(convertFSRToForce(fsrReading9));
      Serial.println("g)");
    }
  }
  
  // 상태가 1인 경우
  if (state1Active) {
    fsrReading1 = analogRead(fsrPin1); // 첫 번째 FSR의 아날로그 값을 읽습니다.
    fsrReading2 = analogRead(fsrPin2); // 두 번째 FSR의 아날로그 값을 읽습니다.
    fsrReading3 = analogRead(fsrPin3); // 세 번째 FSR의 아날로그 값을 읽습니다.
    fsrReading4 = analogRead(fsrPin4); // 네 번째 FSR의 아날로그 값을 읽습니다.
    fsrReading5 = analogRead(fsrPin5); // 다섯 번째 FSR의 아날로그 값을 읽습니다.
    fsrReading6 = analogRead(fsrPin6); // 여섯 번째 FSR의 아날로그 값을 읽습니다.
    fsrReading7 = analogRead(fsrPin7); // 일곱 번째 FSR의 아날로그 값을 읽습니다.
    fsrReading8 = analogRead(fsrPin8); // 여덟 번째 FSR의 아날로그 값을 읽습니다.
    fsrReading9 = analogRead(fsrPin9); // 아홉 번째 FSR의 아날로그 값을 읽습니다.

    unsigned long currentMillis = millis();

    // FSR 센서 값들을 0.5초에 한 번씩 시리얼 모니터에 출력합니다.
    if (currentMillis - previousPrintMillis >= 500) {
      previousPrintMillis = currentMillis;
      Serial.print("FSR1 = ");
      Serial.print(fsrReading1);
      Serial.print(" (");
      Serial.print(convertFSRToForce(fsrReading1));
      Serial.print("g)\t");
      Serial.print("FSR2 = ");
      Serial.print(fsrReading2);
      Serial.print(" (");
      Serial.print(convertFSRToForce(fsrReading2));
      Serial.print("g)\t");
      Serial.print("FSR3 = ");
      Serial.print(fsrReading3);
      Serial.print(" (");
      Serial.print(convertFSRToForce(fsrReading3));
      Serial.print("g)\t");
      Serial.print("FSR4 = ");
      Serial.print(fsrReading4);
      Serial.print(" (");
      Serial.print(convertFSRToForce(fsrReading4));
      Serial.print("g)\t");
      Serial.print("FSR5 = ");
      Serial.print(fsrReading5);
      Serial.print(" (");
      Serial.print(convertFSRToForce(fsrReading5));
      Serial.print("g)\t");
      Serial.print("FSR6 = ");
      Serial.print(fsrReading6);
      Serial.print(" (");
      Serial.print(convertFSRToForce(fsrReading6));
      Serial.print("g)\t");
      Serial.print("FSR7 = ");
      Serial.print(fsrReading7);
      Serial.print(" (");
      Serial.print(convertFSRToForce(fsrReading7));
      Serial.print("g)\t");
      Serial.print("FSR8 = ");
      Serial.print(fsrReading8);
      Serial.print(" (");
      Serial.print(convertFSRToForce(fsrReading8));
      Serial.print("g)\t");
      Serial.print("FSR9 = ");
      Serial.print(fsrReading9);
      Serial.print(" (");
      Serial.print(convertFSRToForce(fsrReading9));
      Serial.println("g)");
    }

    static int stableCount = 0;
    if ((fsrReading1 >= fsrThreshold && fsrReading1 > fsrNoiseThreshold) ||
        (fsrReading2 >= fsrThreshold && fsrReading2 > fsrNoiseThreshold) ||
        (fsrReading3 >= fsrThreshold && fsrReading3 > fsrNoiseThreshold) ||
        (fsrReading4 >= fsrThreshold && fsrReading4 > fsrNoiseThreshold) ||
        (fsrReading5 >= fsrThreshold && fsrReading5 > fsrNoiseThreshold) ||
        (fsrReading6 >= fsrThreshold && fsrReading6 > fsrNoiseThreshold) ||
        (fsrReading7 >= fsrThreshold && fsrReading7 > fsrNoiseThreshold) ||
        (fsrReading8 >= fsrThreshold && fsrReading8 > fsrNoiseThreshold) ||
        (fsrReading9 >= fsrThreshold && fsrReading9 > fsrNoiseThreshold)) {
      stableCount++;
      if (stableCount >= fsrStableReadingsRequired) {
        Serial.println("Threshold exceeded!");
        printRelayCounts();
        printFSRReadings(3);
        Serial.println("Return to state 0");
        relay1OnOffCount = 0; // 릴레이 카운트 초기화 위치 수정
        relay3OnOffCount = 0; // 릴레이 카운트 초기화 위치 수정
        state1Active = false; // 상태 1 비활성화하여 루프 종료
        stableCount = 0; // 안정적인 읽기 횟수 초기화
        systemState = 0;
      }
    } else {
      stableCount = 0; // 안정적인 읽기 횟수 초기화
      if (currentMillis - previousMillis >= relayInterval) {
        previousMillis = currentMillis;
        relay1State = !relay1State; // 릴레이 상태 반전
        digitalWrite(relayPin1, relay1State ? LOW : HIGH); // 릴레이 상태에 따라 ON/OFF 전환
        relay1OnOffCount++; // 릴레이 1 on/off 횟수 증가
      }
    }
  }

  // 상태가 4인 경우
  if (state4Active) {
    fsrReading1 = analogRead(fsrPin1);
    fsrReading2 = analogRead(fsrPin2);
    fsrReading3 = analogRead(fsrPin3);
    fsrReading4 = analogRead(fsrPin4);
    fsrReading5 = analogRead(fsrPin5);
    fsrReading6 = analogRead(fsrPin6);
    fsrReading7 = analogRead(fsrPin7);
    fsrReading8 = analogRead(fsrPin8);
    fsrReading9 = analogRead(fsrPin9);

    unsigned long currentMillis = millis();

    if (currentMillis - previousPrintMillis >= 500) {
      previousPrintMillis = currentMillis;
      Serial.print("FSR1 = ");
      Serial.print(fsrReading1);
      Serial.print(" (");
      Serial.print(convertFSRToForce(fsrReading1));
      Serial.print("g)\t");
      Serial.print("FSR2 = ");
      Serial.print(fsrReading2);
      Serial.print(" (");
      Serial.print(convertFSRToForce(fsrReading2));
      Serial.print("g)\t");
      Serial.print("FSR3 = ");
      Serial.print(fsrReading3);
      Serial.print(" (");
      Serial.print(convertFSRToForce(fsrReading3));
      Serial.print("g)\t");
      Serial.print("FSR4 = ");
      Serial.print(fsrReading4);
      Serial.print(" (");
      Serial.print(convertFSRToForce(fsrReading4));
      Serial.print("g)\t");
      Serial.print("FSR5 = ");
      Serial.print(fsrReading5);
      Serial.print(" (");
      Serial.print(convertFSRToForce(fsrReading5));
      Serial.print("g)\t");
      Serial.print("FSR6 = ");
      Serial.print(fsrReading6);
      Serial.print(" (");
      Serial.print(convertFSRToForce(fsrReading6));
      Serial.print("g)\t");
      Serial.print("FSR7 = ");
      Serial.print(fsrReading7);
      Serial.print(" (");
      Serial.print(convertFSRToForce(fsrReading7));
      Serial.print("g)\t");
      Serial.print("FSR8 = ");
      Serial.print(fsrReading8);
      Serial.print(" (");
      Serial.print(convertFSRToForce(fsrReading8));
      Serial.print("g)\t");
      Serial.print("FSR9 = ");
      Serial.print(fsrReading9);
      Serial.print(" (");
      Serial.print(convertFSRToForce(fsrReading9));
      Serial.println("g)");
    }

    static int stableCount = 0;
    if ((fsrReading1 >= fsrThreshold && fsrReading1 > fsrNoiseThreshold) ||
        (fsrReading2 >= fsrThreshold && fsrReading2 > fsrNoiseThreshold) ||
        (fsrReading3 >= fsrThreshold && fsrReading3 > fsrNoiseThreshold) ||
        (fsrReading4 >= fsrThreshold && fsrReading4 > fsrNoiseThreshold) ||
        (fsrReading5 >= fsrThreshold && fsrReading5 > fsrNoiseThreshold) ||
        (fsrReading6 >= fsrThreshold && fsrReading6 > fsrNoiseThreshold) ||
        (fsrReading7 >= fsrThreshold && fsrReading7 > fsrNoiseThreshold) ||
        (fsrReading8 >= fsrThreshold && fsrReading8 > fsrNoiseThreshold) ||
        (fsrReading9 >= fsrThreshold && fsrReading9 > fsrNoiseThreshold)) {
      stableCount++;
      if (stableCount >= fsrStableReadingsRequired) {
        Serial.println("Threshold exceeded!");
        printRelayCounts();
        printFSRReadings(3);
        Serial.println("Return to state 0");
        relay1OnOffCount = 0; // 릴레이 카운트 초기화 위치 수정
        relay3OnOffCount = 0; // 릴레이 카운트 초기화 위치 수정
        state4Active = false; // 상태 4 비활성화하여 루프 종료
        stableCount = 0; // 안정적인 읽기 횟수 초기화
        systemState = 0;
      }
    } else {
      stableCount = 0; // 안정적인 읽기 횟수 초기화
      if (currentMillis - previousMillis >= relayInterval) {
        previousMillis = currentMillis;
        relay1OnOffCount++;

        if (relay1OnOffCount % 4 == 1) {
          relay3State = true; // 릴레이 3 ON
          relay1State = true; // 릴레이 1 ON
          relay3OnOffCount++;
        } else if (relay1OnOffCount % 4 == 2) {
          relay3State = false; // 릴레이 3 OFF
          relay1State = false; // 릴레이 1 OFF
        } else if (relay1OnOffCount % 4 == 3) {
          relay1State = true; // 릴레이 1 ON
        } else if (relay1OnOffCount % 4 == 0) {
          relay1State = false; // 릴레이 1 OFF
        }

        digitalWrite(relayPin3, relay3State ? LOW : HIGH);
        digitalWrite(relayPin1, relay1State ? LOW : HIGH);
      }
    }
  }

  // 상태가 2인 경우
  if (activateRelay2) {
    Serial.println("State 2");

    if (!relay1State) {
      // 릴레이 1이 OFF 상태일 때
      digitalWrite(relayPin2, LOW);   // 릴레이 2 켜기
      Serial.println("Relay 2: ON");
      delay(3000);         // 3초 대기
      digitalWrite(relayPin2, HIGH);  // 릴레이 2 끄기
      Serial.println("Relay 2: OFF");
    } else {
      // 릴레이 1이 ON 상태일 때 기존 동작 수행
      digitalWrite(relayPin2, LOW);   // 릴레이 2 켜기
      Serial.println("Relay 2: ON");
      delay(500);          // 0.5초 대기
      digitalWrite(relayPin1, HIGH);  // 릴레이 1 끄기
      Serial.println("Relay 1: OFF");
      delay(2500);         // 2.5초 동안 유지
      digitalWrite(relayPin2, HIGH);  // 릴레이 2 끄기
      Serial.println("Relay 2: OFF");
      relay1State = false; // 릴레이 1 상태 초기화
    }

    activateRelay2 = false; // 릴레이 2 활성화 여부 초기화
    systemState = 0;           // 상태를 0으로 초기화
    Serial.println("State 2 complete");
    Serial.println("Return to state 0");
  }

  // 상태가 3인 경우
  if (activateRelay3) {
    activateRelay3 = false; // 릴레이 3 활성화 여부 초기화
    systemState = 0;           // 상태를 0으로 초기화
    Serial.println("Return to state 0");
  }
}

void handleState5() {
  if (relay1State) {
    // 릴레이 1 ON
    if (!relay3State) {
      // 릴레이 3 OFF
      digitalWrite(relayPin2, LOW);   // 릴레이 2 켜기
      Serial.println("Relay 2: ON");
      delay(500);                    // 0.5초 대기
      digitalWrite(relayPin1, HIGH);  // 릴레이 1 끄기
      Serial.println("Relay 1: OFF");
      digitalWrite(relayPin3, LOW);   // 릴레이 3 켜기
      Serial.println("Relay 3: ON");
      delay(3000);                   // 3초 대기
      digitalWrite(relayPin2, HIGH);  // 릴레이 2 끄기
      Serial.println("Relay 2: OFF");
      delay(500);                    // 0.5초 대기
      digitalWrite(relayPin3, HIGH);  // 릴레이 3 끄기
      Serial.println("Relay 3: OFF");
      relay3OnOffCount++;
    }
  } else {
    // 릴레이 1 OFF
    if (!relay3State) {
      // 릴레이 3 OFF
      digitalWrite(relayPin2, LOW);   // 릴레이 2 켜기
      Serial.println("Relay 2: ON");
      delay(500);                    // 0.5초 대기
      digitalWrite(relayPin3, LOW);   // 릴레이 3 켜기
      Serial.println("Relay 3: ON");
      delay(3000);                   // 3초 대기
      digitalWrite(relayPin2, HIGH);  // 릴레이 2 끄기
      Serial.println("Relay 2: OFF");
      delay(500);                    // 0.5초 대기
      digitalWrite(relayPin3, HIGH);  // 릴레이 3 끄기
      Serial.println("Relay 3: OFF");
      relay3OnOffCount++;
    }
  }

  systemState = 0; // 상태를 0으로 초기화
  Serial.println("State 5 complete");
  Serial.println("Return to state 0");
}

void printFSRReadings(int times = 1) {
  for (int t = 0; t < times; t++) {
    fsrReading1 = analogRead(fsrPin1);
    fsrReading2 = analogRead(fsrPin2);
    fsrReading3 = analogRead(fsrPin3);
    fsrReading4 = analogRead(fsrPin4);
    fsrReading5 = analogRead(fsrPin5);
    fsrReading6 = analogRead(fsrPin6);
    fsrReading7 = analogRead(fsrPin7);
    fsrReading8 = analogRead(fsrPin8);
    fsrReading9 = analogRead(fsrPin9);
    Serial.print("FSR1 = ");
    Serial.print(fsrReading1);
    Serial.print(" (");
    Serial.print(convertFSRToForce(fsrReading1));
    Serial.print("g)\t");
    Serial.print("FSR2 = ");
    Serial.print(fsrReading2);
    Serial.print(" (");
    Serial.print(convertFSRToForce(fsrReading2));
    Serial.print("g)\t");
    Serial.print("FSR3 = ");
    Serial.print(fsrReading3);
    Serial.print(" (");
    Serial.print(convertFSRToForce(fsrReading3));
    Serial.print("g)\t");
    Serial.print("FSR4 = ");
    Serial.print(fsrReading4);
    Serial.print(" (");
    Serial.print(convertFSRToForce(fsrReading4));
    Serial.print("g)\t");
    Serial.print("FSR5 = ");
    Serial.print(fsrReading5);
    Serial.print(" (");
    Serial.print(convertFSRToForce(fsrReading5));
    Serial.print("g)\t");
    Serial.print("FSR6 = ");
    Serial.print(fsrReading6);
    Serial.print(" (");
    Serial.print(convertFSRToForce(fsrReading6));
    Serial.print("g)\t");
    Serial.print("FSR7 = ");
    Serial.print(fsrReading7);
    Serial.print(" (");
    Serial.print(convertFSRToForce(fsrReading7));
    Serial.print("g)\t");
    Serial.print("FSR8 = ");
    Serial.print(fsrReading8);
    Serial.print(" (");
    Serial.print(convertFSRToForce(fsrReading8));
    Serial.print("g)\t");
    Serial.print("FSR9 = ");
    Serial.print(fsrReading9);
    Serial.print(" (");
    Serial.print(convertFSRToForce(fsrReading9));
    Serial.println("g)");
    delay(1000); // 1초 대기
  }
}

void printRelayCounts() {
  Serial.print("Relay 1 On/Off count: ");
  Serial.println(relay1OnOffCount);
  Serial.print("Relay 3 On/Off count: ");
  Serial.println(relay3OnOffCount);
  relay1OnOffCount = 0; // 카운트 초기화
  relay3OnOffCount = 0; // 카운트 초기화
}

// 다항 회귀 모델의 계수
const double COEFF_1 = -1.15893906;
const double COEFF_2 = 0.0474553682;
const double COEFF_3 = -0.000419471763;
const double COEFF_4 = 1.64971200e-06;
const double COEFF_5 = -3.20697097e-09;
const double COEFF_6 = 3.02602988e-12;
const double COEFF_7 = -1.10046712e-15;

// FSR 값을 힘으로 변환하는 함수
float convertFSRToForce(int fsrValue) {
  // 다항 회귀식을 사용하여 힘 계산 (절편을 0으로 설정)
  double force = COEFF_1 * fsrValue +
                 COEFF_2 * pow(fsrValue, 2) +
                 COEFF_3 * pow(fsrValue, 3) +
                 COEFF_4 * pow(fsrValue, 4) +
                 COEFF_5 * pow(fsrValue, 5) +
                 COEFF_6 * pow(fsrValue, 6) +
                 COEFF_7 * pow(fsrValue, 7);
  return (force < 0) ? 0 : (float) force;
}
