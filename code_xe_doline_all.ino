#include <Wire.h> 
#include <LiquidCrystal_I2C.h>

// --- 1. KHAI BÁO CHÂN (Theo sơ đồ bạn đã đấu) ---
// Màn hình LCD (SDA->A4, SCL->A5)
LiquidCrystal_I2C lcd(0x27, 16, 2); 

// Cảm biến Line 5 mắt
const int S1 = A0; 
const int S2 = A1; 
const int S3 = A2; // Mắt giữa quan trọng nhất
const int S4 = A3; 
const int S5 = 7;  // Mắt phải cùng nối chân 7

// Động cơ L298N
const int ENA = 9;  // Chân tốc độ Motor Trái
// Đảo ngược vị trí chân để đảo chiều quay
const int IN1 = 3; // Đổi 3 thành 4
const int IN2 = 4; // Đổi 4 thành 3

const int IN3 = 5; // Đổi 5 thành 6
const int IN4 = 6; // Đổi 6 thành 5
const int ENB = 10; // Chân tốc độ Motor Phải

// Cảm biến siêu âm
const int trigPin = 11; 
const int echoPin = 12;

// Còi Buzzer
const int buzzer = 8;

// --- 2. CÀI ĐẶT TỐC ĐỘ (SỬA LẠI ĐỂ XE ĐỦ KHỎE) ---

// Tăng từ 70 lên 200 (hoặc 255 nếu pin yếu)
int SPEED_NORMAL = 200; 

// Tốc độ khi cua cũng phải khỏe
int SPEED_TURN = 255;   

// Tốc độ né vật
int SPEED_AVOID = 200;  

// CHỈNH LỆCH BÁNH (Giữ nguyên hoặc chỉnh lại sau khi xe đã chạy được)
int OFFSET_TRAI = 0; 
int OFFSET_PHAI = -20; // Giảm bớt bánh phải nếu nó nhanh hơn bánh trái

long duration;
int distance = 0;

void setup() {
  // Cài đặt INPUT/OUTPUT
  pinMode(S1, INPUT); pinMode(S2, INPUT); pinMode(S3, INPUT); pinMode(S4, INPUT); pinMode(S5, INPUT);
  pinMode(ENA, OUTPUT); pinMode(IN1, OUTPUT); pinMode(IN2, OUTPUT);
  pinMode(ENB, OUTPUT); pinMode(IN3, OUTPUT); pinMode(IN4, OUTPUT);
  pinMode(trigPin, OUTPUT); pinMode(echoPin, INPUT);
  pinMode(buzzer, OUTPUT);

  // Khởi động LCD
  lcd.init(); 
  lcd.backlight();
  lcd.setCursor(0, 0); lcd.print("ROBOT DO LINE");
  lcd.setCursor(0, 1); lcd.print("READY TO RUN!");
  
  // Kêu 1 tiếng báo hiệu đã bật nguồn
  digitalWrite(buzzer, HIGH); delay(200); digitalWrite(buzzer, LOW);
  delay(1000); 
  lcd.clear();
}

// --- CÁC HÀM DI CHUYỂN ---
void diThang(int tocdo) {
  // Bánh TRÁI (ENA): Giữ nguyên tốc độ
  digitalWrite(IN1, HIGH); digitalWrite(IN2, LOW); 
  analogWrite(ENA, tocdo);

  // Bánh PHẢI (ENB): Trừ bớt đi để chạy chậm lại
  // Tôi đang trừ đi 15 đơn vị. Nếu vẫn cong trái thì tăng lên trừ 20, 25...
  // Nếu chuyển sang cong phải thì giảm xuống trừ 5, 10...
  int tocdoPhai = tocdo - 15; 
  
  if (tocdoPhai < 0) tocdoPhai = 0; // Đảm bảo không bị số âm
  
  digitalWrite(IN3, HIGH); digitalWrite(IN4, LOW); 
  analogWrite(ENB, tocdoPhai);
}

void luiLai() {
  digitalWrite(IN1, LOW); digitalWrite(IN2, HIGH); analogWrite(ENA, SPEED_AVOID);
  digitalWrite(IN3, LOW); digitalWrite(IN4, HIGH); analogWrite(ENB, SPEED_AVOID);
}
void reTrai() {
  // Bánh trái đứng/lùi, Bánh phải tiến
  digitalWrite(IN1, LOW); digitalWrite(IN2, LOW); analogWrite(ENA, 0);
  digitalWrite(IN3, HIGH); digitalWrite(IN4, LOW); analogWrite(ENB, SPEED_TURN);
}
void rePhai() {
  // Bánh trái tiến, Bánh phải đứng/lùi
  digitalWrite(IN1, HIGH); digitalWrite(IN2, LOW); analogWrite(ENA, SPEED_TURN);
  digitalWrite(IN3, LOW); digitalWrite(IN4, LOW); analogWrite(ENB, 0);
}
void dungLai() {
  digitalWrite(IN1, LOW); digitalWrite(IN2, LOW); analogWrite(ENA, 0);
  digitalWrite(IN3, LOW); digitalWrite(IN4, LOW); analogWrite(ENB, 0);
}

// --- HÀM NÉ VẬT ---
void neVat() {
  dungLai();
  
  // 1. Kêu còi báo động 2 lần
  for(int i=0; i<2; i++){
    digitalWrite(buzzer, HIGH); delay(100); digitalWrite(buzzer, LOW); delay(100);
  }
  
  lcd.setCursor(0, 1); lcd.print("NE VAT CAN...   ");
  
  // 2. Lùi lại xíu
  luiLai(); delay(400);
  
  // 3. Cua Trái 90 độ (Ra khỏi line)
  // (Nếu xe quay ít quá chưa đủ 90 độ thì tăng số 600 lên)
  digitalWrite(IN1, LOW); digitalWrite(IN2, HIGH); // Trái lùi
  digitalWrite(IN3, HIGH); digitalWrite(IN4, LOW); // Phải tiến
  analogWrite(ENA, 150); analogWrite(ENB, 150);
  delay(600); 
  
  // 4. Đi thẳng qua vật
  // (Nếu vật to quá thì tăng số 800 lên)
  diThang(130);
  delay(800); 
  
  // 5. Cua Phải quay về hướng line
  digitalWrite(IN1, HIGH); digitalWrite(IN2, LOW); // Trái tiến
  digitalWrite(IN3, LOW); digitalWrite(IN4, HIGH); // Phải lùi
  analogWrite(ENA, 150); analogWrite(ENB, 150);
  delay(600); 
  
  // 6. Đi từ từ dò tìm lại vạch đen
  diThang(100);
  // Vòng lặp: Cứ đi thẳng mãi chừng nào S3 vẫn thấy TRẮNG (1)
  // Khi S3 gặp ĐEN (0) -> Thoát vòng lặp -> Quay lại dò line bình thường
  while(digitalRead(S3) == 1) { 
    delay(10);
  }
}

// Hàm đo khoảng cách
void doKhoangCach() {
  digitalWrite(trigPin, LOW); delayMicroseconds(2);
  digitalWrite(trigPin, HIGH); delayMicroseconds(10);
  digitalWrite(trigPin, LOW);
  duration = pulseIn(echoPin, HIGH);
  distance = duration * 0.034 / 2;
  
  // Hiển thị LCD hàng trên
  lcd.setCursor(0, 0); lcd.print("Dist: "); 
  lcd.print(distance); lcd.print(" cm    ");
}

void loop() {
  // 1. Đo khoảng cách
  doKhoangCach();
  
  // 2. Logic xử lý vật cản
  if (distance > 0 && distance < 15) { 
    // Nếu gần vật (<15cm) -> Còi kêu tít tít cảnh báo
    digitalWrite(buzzer, HIGH);
    
    if (distance < 10) {
      // Nếu quá gần (<10cm) -> NÉ VẬT
      digitalWrite(buzzer, LOW); 
      neVat(); 
    }
  } 
  else {
    // Không có vật cản -> Tắt còi -> Dò Line
    digitalWrite(buzzer, LOW);
    lcd.setCursor(0, 1); lcd.print("AUTO LINE...    ");
    
    // Đọc cảm biến line
    int val1 = digitalRead(S1); int val2 = digitalRead(S2);
    int val3 = digitalRead(S3); 
    int val4 = digitalRead(S4); int val5 = digitalRead(S5);
    
    // Logic dò line (0 là Đen, 1 là Trắng)
    if (val3 == 0) {
      diThang(SPEED_NORMAL); // Giữa thấy đen -> Thẳng
    }
    else if (val2 == 0 || val1 == 0) {
      reTrai(); // Bên trái thấy đen -> Cua trái
    }
    else if (val4 == 0 || val5 == 0) {
      rePhai(); // Bên phải thấy đen -> Cua phải
    }
    else {
      // Nếu trắng hết (mất line) -> Đi thẳng chậm để mò đường
      diThang(SPEED_NORMAL); 
    }
  }
}
