// AyumuBekki 2025
// blog.bekki.jp
// ----
// 定数
static const uint32_t SERIAL_BAUD_RATE = 115200; // Serialのボーレート
static const size_t POWER_LEVEL_MAX = 4;              // OFFを含めたパワーレベル数
static const int16_t POWER_LEVEL_PWM_TABLE[POWER_LEVEL_MAX] = { 0, 40, 100, 250 };  // (0-255) レベル別PWM設定テーブル

static const uint8_t PWM_PIN = 10;                    // PWM出力ピン
static const size_t POWER_INDICATOR_MAX = POWER_LEVEL_MAX - 1;  // インジケーター数
static const uint8_t POWER_INDICATOR_PINS[POWER_INDICATOR_MAX] = {
  A0,  // パワーインジケーター低出力ピン
  A1,  // パワーインジケーター中出力ピン
  A2,  // パワーインジケーター高出力ピン
};
static const uint8_t BUTTON_PIN = 12;     // ボタン入力ピン

static const uint32_t DEBOUNCE_DELAY = 20;     // チャタリングのディレイ時間（ms）
static const uint32_t LONG_PRESS_TIME = 1000;  // 長押し判定時間（ms）

// グローバル変数
static int8_t powerLevel = 0;                // パワーレベル
static bool lastButtonState = HIGH;       // 最終ボタンステータス
static uint32_t lastPushTime = 0;    // チャタリング対策・長押し判定用
static bool isExecEvent = false;          // ボタンイベント

// パワー設定
void SetPowerLevel(int16_t level) {
  if (level < 0 || POWER_LEVEL_MAX <= level) {
    // 範囲外の場合は強制OFF
    level = 0;
  }

  // パワーインジケーター設定(LED消灯・点灯制御)
  for (int16_t pinIdx = 0; pinIdx < POWER_INDICATOR_MAX; ++pinIdx) {
    digitalWrite(POWER_INDICATOR_PINS[pinIdx], (pinIdx + 1) <= level);
  }

  // PWM出力変更
  analogWrite(PWM_PIN, POWER_LEVEL_PWM_TABLE[level]);
}

// パワーレベルがOnか？
bool IsPowerOn() {
  return powerLevel != 0;
}

// パワーレベルOFF
void PowerOff() {
  Serial.println(F("Power Off"));
  powerLevel = 0;  // 0はOFF
  SetPowerLevel(powerLevel);
}

// パワーレベルON
void PowerOn() {
  Serial.println(F("Power On"));
  powerLevel = 1;
  SetPowerLevel(powerLevel);
}

// パワーレベルのローテーション
void RotatePowerLevel() {
  powerLevel = (powerLevel % (POWER_LEVEL_MAX - 1)) + 1;
  Serial.print(F("RotatePowerLevel powerLevel:"));
  Serial.println(powerLevel);

  SetPowerLevel(powerLevel);
}

// ボタン長押しイベント
void OnButtonLong() {
  // パワーレベルのON/OFF
  IsPowerOn() ? PowerOff() : PowerOn();
}

// ボタン押しイベント
void OnButton() {
  // 電源がONの時にパワーローテーションを行う
  if (IsPowerOn()) {
    RotatePowerLevel();
  }
}

// ボタン監視
// 指定時間の長押しと、押してすぐ話した場合の対応を行う(チャタリング対策入り)
void ButtonWatcher() {
  const int buttonState = digitalRead(BUTTON_PIN);

  // ボタンがおされっぱなしの状態
  if (buttonState == LOW && lastButtonState == LOW && (lastPushTime + LONG_PRESS_TIME) <= millis() && !isExecEvent) {
    Serial.println(F("Button Long Push"));
    OnButtonLong();
    isExecEvent = true;
    return;
  }

  if (buttonState == lastButtonState) {
    return;  // ボタンに変化なしの場合は終了
  }

  // ボタンの変化がある場合
  lastButtonState = buttonState;
  if (buttonState == LOW) {
    // ボタンが押された
    lastPushTime = millis();
    isExecEvent = false;
  } else {
    // ボタンが離された
    if (DEBOUNCE_DELAY < (millis() - lastPushTime) && !isExecEvent) {
      Serial.println(F("Button Push"));
      OnButton();
      isExecEvent = true;
    }
  }
}

void setup() {
  // シリアル出力設定
  Serial.begin(SERIAL_BAUD_RATE);

  // D9 D10ピンのPWM用タイマー設定を変更し、周波数を3921.16Hzに変更する
  TCCR1B = (TCCR1B & 0b11111000) | 0b00000010;

  // GPIO初期化
  pinMode(PWM_PIN, OUTPUT);
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  for (int16_t pinIdx = 0; pinIdx < POWER_INDICATOR_MAX; ++pinIdx) {
    pinMode(POWER_INDICATOR_PINS[pinIdx], OUTPUT);
  }

  // 動作状態OFFで初期化
  PowerOff();
  Serial.println(F("System Initialize"));
}

void loop() {
  // ボタンの監視を行う
  ButtonWatcher();
}