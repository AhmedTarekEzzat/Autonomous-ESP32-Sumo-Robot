/* =====================================================================
   SUMO ROBOT — AGGRESSIVE SPIN & ATTACK FIRMWARE (WITH EDGE ESCAPE)
   - تم التحديث ليتوافق مع مكتبة ESP32 Core V3.x (حل مشكلة ledcSetup) -
   ===================================================================== */

// --- مفتاح تشغيل/إيقاف الألتراسونيك ---
#define ENABLE_ULTRASONIC false  // خليه false للإيقاف المؤقت، و true للتشغيل

// ---------------- PIN DEFINITIONS ----------------
#define ENA  25
#define IN1  26
#define IN2  27
#define IN3  13
#define IN4  14
#define ENB  33

#define TRIG 23
#define ECHO 22

// حساسات الحواف (IR Sensors)
#define IR_FL 16  // أمام يسار
#define IR_FR 17  // أمام يمين
#define IR_BL 34  // خلف يسار
#define IR_BR 35  // خلف يمين

// ---------------- TUNABLE PARAMETERS ----------------
const unsigned long START_DELAY_MS   = 5000;  // 5 ثواني انتظار (قانون السومو)
const int ATTACK_DISTANCE_CM         = 40;    // مسافة الهجوم
const int LOST_DISTANCE_CM           = 60;    // مسافة فقدان الخصم
const int FULL_SPEED                 = 255;   // سرعة الهجوم القصوى
const int TURN_SPEED                 = 255;   // سرعة الدوران في المركز

const unsigned long LOST_CONFIRM_MS  = 400;   // الوقت اللازم لتأكيد فقدان الخصم
const unsigned long ATTACK_LOCK_MS   = 600;   // أقل مدة إجبارية في وضع الهجوم

#define WHITE_STATE LOW                      // اجعلها HIGH إذا كان الروبوت يهرب من الأسود
const unsigned long ESCAPE_REVERSE_MS = 300; // مدة الرجوع للخلف عند رؤية الخط من الأمام
const unsigned long ESCAPE_TURN_MS    = 250; // مدة الدوران بعد الرجوع للخلف
bool escapeFromFront = true;                 

// PWM (إعدادات الإصدار الجديد 3.x)
const int PWM_FREQ_HZ  = 1000;
const int PWM_RES_BITS = 8;

struct EdgeReading {
    bool fl, fr, bl, br;
    bool any;
};

// ---------------- STATE MACHINE ----------------
enum RobotState { INIT_DELAY, SEARCHING, ATTACKING, EDGE_ESCAPE };
RobotState state = INIT_DELAY;
unsigned long stateEnteredAt = 0;
unsigned long lastGoodDetectionAt = 0;

// ---------------- LOW-LEVEL MOTOR CONTROL ----------------
// تعديل: الآن نستقبل رقم منفذ السرعة (enablePin) بدلاً من القناة
void setMotor(int in1, int in2, int enablePin, int speed) {
    speed = constrain(speed, -255, 255);

    if (speed >= 0) {
        digitalWrite(in1, HIGH);
        digitalWrite(in2, LOW);
    } else {
        digitalWrite(in1, LOW);
        digitalWrite(in2, HIGH);
        speed = -speed;
    }
    // في التحديث الجديد للإسب32 نكتب السرعة مباشرة على الـ Pin
    ledcWrite(enablePin, speed); 
}

void setMotors(int left, int right) {
    // نرسل ENA و ENB مباشرة
    setMotor(IN1, IN2, ENA, left);
    setMotor(IN3, IN4, ENB, right);
}

void stopMotors() {
    setMotors(0, 0);
}

// ---------------- SENSORS ----------------
long readDistanceCM_raw() {
    if (!ENABLE_ULTRASONIC) {
        return -1; 
    }

    digitalWrite(TRIG, LOW);
    delayMicroseconds(2);
    digitalWrite(TRIG, HIGH);
    delayMicroseconds(10);
    digitalWrite(TRIG, LOW);

    long duration = pulseIn(ECHO, HIGH, 15000UL);
    if (duration == 0) return -1;

    long distance = duration / 58;
    return (distance <= 0 || distance > 300) ? -1 : distance;
}

long readDistanceCM() {
    if (!ENABLE_ULTRASONIC) return -1;

    long best = -1;
    for (int i = 0; i < 3; i++) {
        long d = readDistanceCM_raw();
        if (d > 0 && (best < 0 || d < best)) best = d;
    }
    return best;
}

EdgeReading readEdges() {
    EdgeReading e;
    e.fl = (digitalRead(IR_FL) == WHITE_STATE);
    e.fr = (digitalRead(IR_FR) == WHITE_STATE);
    e.bl = (digitalRead(IR_BL) == WHITE_STATE);
    e.br = (digitalRead(IR_BR) == WHITE_STATE);
    
    e.any = (e.fl || e.fr || e.bl || e.br);
    return e;
}

void enterState(RobotState s) {
    state = s;
    stateEnteredAt = millis();
    if (s == ATTACKING) {
        lastGoodDetectionAt = stateEnteredAt;
    }
}

// ---------------- SETUP ----------------
void setup() {
    pinMode(IN1, OUTPUT); pinMode(IN2, OUTPUT);
    pinMode(IN3, OUTPUT); pinMode(IN4, OUTPUT);
    pinMode(TRIG, OUTPUT); pinMode(ECHO, INPUT);

    pinMode(IR_FL, INPUT); 
    pinMode(IR_FR, INPUT);
    pinMode(IR_BL, INPUT); 
    pinMode(IR_BR, INPUT);

    // الطريقة الجديدة لتعريف الـ PWM في ESP32 الإصدار 3.0+
    ledcAttach(ENA, PWM_FREQ_HZ, PWM_RES_BITS);
    ledcAttach(ENB, PWM_FREQ_HZ, PWM_RES_BITS);

    stopMotors();
    enterState(INIT_DELAY);
}

// ---------------- MAIN LOOP ----------------
void loop() {
    unsigned long now = millis();
    EdgeReading edges = readEdges();

    if (state != INIT_DELAY && state != EDGE_ESCAPE) {
        if (edges.any) {
            if (edges.fl || edges.fr) {
                escapeFromFront = true;  
            } else {
                escapeFromFront = false; 
            }
            enterState(EDGE_ESCAPE);
            return; 
        }
    }

    switch (state) {

        case INIT_DELAY: {
            stopMotors();
            if (now - stateEnteredAt >= START_DELAY_MS) {
                enterState(SEARCHING);
            }
            break;
        }

        case EDGE_ESCAPE: {
            unsigned long elapsed = now - stateEnteredAt;
            
            if (escapeFromFront) {
                if (elapsed < ESCAPE_REVERSE_MS) {
                    setMotors(-FULL_SPEED, -FULL_SPEED); 
                } else if (elapsed < (ESCAPE_REVERSE_MS + ESCAPE_TURN_MS)) {
                    setMotors(FULL_SPEED, -FULL_SPEED);  
                } else {
                    enterState(SEARCHING); 
                }
            } else {
                if (elapsed < ESCAPE_REVERSE_MS) {
                    setMotors(FULL_SPEED, FULL_SPEED);   
                } else {
                    enterState(SEARCHING); 
                }
            }
            break;
        }

        case SEARCHING: {
            long d = readDistanceCM();

            if (d > 0 && d <= ATTACK_DISTANCE_CM) {
                setMotors(FULL_SPEED, FULL_SPEED); 
                enterState(ATTACKING);
                break;
            }

            setMotors(TURN_SPEED, -TURN_SPEED);
            break;
        }

        case ATTACKING: {
            setMotors(FULL_SPEED, FULL_SPEED);

            long d = readDistanceCM();

            if (d < 0 || d <= LOST_DISTANCE_CM) {
                lastGoodDetectionAt = now;
            }

            bool lockPeriodOver   = (now - stateEnteredAt) >= ATTACK_LOCK_MS;
            bool trulyLostByTime  = (now - lastGoodDetectionAt) >= LOST_CONFIRM_MS;

            if (lockPeriodOver && trulyLostByTime) {
                enterState(SEARCHING);
                break;
            }
            break;
        }
    }
}