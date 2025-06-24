#define BLYNK_TEMPLATE_ID "TMPL6KTQu8zh6"
#define BLYNK_TEMPLATE_NAME "DO AN 1 TUOI CAY TU DONG"
#define BLYNK_AUTH_TOKEN "uTiinrK82UW3u2yeICzxu6RgaUvZTma7"


#include <WiFi.h>
#include <WiFiClient.h>
#include <BlynkSimpleEsp32.h>
#include <DHT.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <EEPROM.h>


// Định nghĩa phần cứng
#define EEPROM_ADDR       0
#define POWER_PIN         12
#define SIGNAL_PIN        39
#define DHTPIN            4
#define DHTTYPE           DHT11

#define BUTTON_MODE       33
#define BUTTON_LIMIT      26
#define BUTTON_OFF_BUZZER 32

#define RELAY_PIN         17
#define BUZZER_PIN        19
#define SENSOR_SOIL       A0  

#define GIOI_HAN_MUC_NUOC 300
#define samples           10  

#define ADC_kho 3686
#define ADC_uot 1200



// Biến toàn cục
DHT dht(DHTPIN, DHTTYPE);
LiquidCrystal_I2C lcd(0x27, 16, 2);

char ssid[] = "Rand";
char pass[] = "thienhieu089";
char auth[] = BLYNK_AUTH_TOKEN;

int change = 0, last_change = -1;
bool buzzer_disabled = false;


float t;
int h, value, moisture_percent;
int avg_raw;

int nguong_duoi = 24;  // mặc định ban đầu
int nguong_tren = 30;


void clearLine(int line) {
  lcd.setCursor(0, line);
  lcd.print("                ");  // 16 dấu cách cho LCD 16x2
}


void chong_doi() {
  if (digitalRead(BUTTON_MODE) == LOW) {
    delay(200);
    if (digitalRead(BUTTON_MODE) == LOW) {
      change = (change + 1) % 3;
      if (change != last_change) {
        lcd.clear();
        last_change = change;
      }
      while (digitalRead(BUTTON_MODE) == LOW);
    }
  }
}


void tat_chuong() {
  if (digitalRead(BUTTON_OFF_BUZZER) == LOW) {
    delay(200);
    if (digitalRead(BUTTON_OFF_BUZZER) == LOW) {
      buzzer_disabled = true;
      digitalWrite(BUZZER_PIN, LOW);
      Blynk.virtualWrite(V5, 1);
      while (digitalRead(BUTTON_OFF_BUZZER) == LOW);
    }
  }
}


void g_han_may_bom() {
if (value < GIOI_HAN_MUC_NUOC) {
  if (!buzzer_disabled) {
    digitalWrite(BUZZER_PIN, HIGH);
    Blynk.virtualWrite(V5, 1); 
  } else {
    digitalWrite(BUZZER_PIN, LOW);
    Blynk.virtualWrite(V5, 0); 
  }
  digitalWrite(RELAY_PIN, LOW);
  Blynk.virtualWrite(V4, LOW);
  tat_chuong();
} else {
  buzzer_disabled = false;
  digitalWrite(BUZZER_PIN, LOW);
  Blynk.virtualWrite(V5, 0);  

  //  CHỈ HIỆN THÔNG BÁO KHI change == 2
  if (change == 2) {
    if (moisture_percent < nguong_duoi) {
      clearLine(0);
      Serial.println("CAN TUOI (Dat kho)");
      lcd.setCursor(0,0);
      lcd.print("CAN TUOI !");
      digitalWrite(RELAY_PIN, HIGH);
    } else if (moisture_percent <= nguong_tren) {
      clearLine(0);
      Serial.println("Dat am PHU HOP");
      lcd.setCursor(0,0);
      lcd.print("Dat am PHU HOP !");
      digitalWrite(RELAY_PIN, LOW);
    } else {
      clearLine(0);
      Serial.println("Dat QUA UOT (Ngung tuoi)");
      lcd.setCursor(0,0);
      lcd.print("Dat QUA UOT !");
      digitalWrite(RELAY_PIN, LOW);
    }
  } else {
    if (moisture_percent < nguong_duoi) {
      digitalWrite(RELAY_PIN, HIGH);
      Blynk.virtualWrite(V4, HIGH);
    } else {
      digitalWrite(RELAY_PIN, LOW);
      Blynk.virtualWrite(V4, LOW);
    }
  }
}

}

int readSoilMoisture() {
  long sum = 0;
  for (int i = 0; i < samples; i++) {
    sum += analogRead(SENSOR_SOIL);
    delay(20);
  }
  avg_raw = sum / samples;
  int percent = map(avg_raw, ADC_uot, ADC_kho, 100, 0);

  return constrain(percent, 0, 100);
}


//  HÀM MỚI: đọc mực nước trung bình để chống nhiễu
int readWaterLevel() {
  long sum = 0;
  digitalWrite(POWER_PIN, HIGH);
  delay(50);
  for (int i = 0; i < samples; i++) {
    sum += analogRead(SIGNAL_PIN);
    delay(10);
  }
  digitalWrite(POWER_PIN, LOW);
  return sum / samples;
}



void setup() {
  
  Serial.begin(9600);
  Wire.begin();
  lcd.init(); 
  lcd.backlight(); 
  lcd.clear();
  dht.begin();
  delay(2000);

  pinMode(BUTTON_MODE, INPUT_PULLUP);
  pinMode(BUTTON_LIMIT, INPUT_PULLUP);
  pinMode(BUTTON_OFF_BUZZER, INPUT_PULLUP);
  pinMode(RELAY_PIN, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(POWER_PIN, OUTPUT);

  digitalWrite(POWER_PIN, LOW);
  analogSetAttenuation(ADC_11db);  
  EEPROM.begin(512);
  

  Serial.println("Khoi dong HT cam bien");
  Blynk.begin(auth, ssid, pass);

  Blynk.virtualWrite(V5, buzzer_disabled ? 1 : 0);  
    
}

void loop() {
  Blynk.run();
  chong_doi();

  h = dht.readHumidity();
  t = dht.readTemperature();
  
  if (isnan(h) || isnan(t)) {
    Serial.println("Loi doc DHT11");
    return;
  }

  value = readWaterLevel();  //  dùng hàm lọc nhiễu mới

  moisture_percent = readSoilMoisture();

  Serial.print("Do am dat: ");
  Serial.print(moisture_percent);
  Serial.print("% - ");
  Serial.print(avg_raw);

  if (moisture_percent < nguong_duoi) {
    Serial.println("CAN TUOI (Dat kho)");
  } else if (moisture_percent <= nguong_tren) {
    Serial.println("Dat am PHU HOP");
  } else {
    Serial.println("Dat QUA UOT (Ngung tuoi)");
  }

  g_han_may_bom();

  Blynk.virtualWrite(V0, t);
  Blynk.virtualWrite(V1, h);
  Blynk.virtualWrite(V2, moisture_percent);
  Blynk.virtualWrite(V3, value);

  if (change != last_change) 
    lcd.clear();
  if (change == 0) {
    lcd.setCursor(4, 0); 
    lcd.print("XIN CHAO");
    lcd.setCursor(0, 1); 
    lcd.print("HT TUOI CAY TM");
  } else if (change == 1) {
    lcd.setCursor(0, 0);
    lcd.print("ND: "); 
    lcd.print(t, 1); 
    lcd.print((char)223); 
    lcd.print("C   ");

    lcd.setCursor(0, 1);
    lcd.print("DAT:"); 
    lcd.print(moisture_percent); 
    lcd.print("% ");
    
    lcd.setCursor(9, 1);
    lcd.print("KK:"); 
    lcd.print(h); 
    lcd.print("% ");
  } else if (change == 2) {
    lcd.setCursor(0, 1);
    lcd.print("MUC NUOC: "); 
    lcd.print(value);
    lcd.print("       ");
    lcd.print("mm");
  }

  delay(500);
}

BLYNK_WRITE(V5) {
  int state = param.asInt();
  buzzer_disabled = !state; // đảo ngược logic
  if (buzzer_disabled) {
    digitalWrite(BUZZER_PIN, LOW);
  } else {
    if (value < GIOI_HAN_MUC_NUOC) {
      digitalWrite(BUZZER_PIN, HIGH);
    }
  }

  Serial.print("Blynk điều khiển còi, buzzer_disabled = ");
  Serial.println(buzzer_disabled);
}



BLYNK_WRITE(V6) {
  nguong_duoi = param.asInt();
  Serial.print("Ngưỡng dưới mới: ");
  Serial.println(nguong_duoi);
}


BLYNK_WRITE(V7) {
  nguong_tren = param.asInt();
  Serial.print("Ngưỡng trên mới: ");
  Serial.println(nguong_tren);
}
