

#include <esp_now.h>
#include <WiFi.h>


struct GestureMessage {
  char  command;   
  float pitch;
  float roll;
};


class SingleMotor {
public:
    SingleMotor(int rpwm, int lpwm, int ren, int len)
        : rpwm_(rpwm), lpwm_(lpwm), ren_(ren), len_(len) {}

    void begin() {
        pinMode(ren_, OUTPUT);
        pinMode(len_, OUTPUT);
        digitalWrite(ren_, HIGH);  
        digitalWrite(len_, HIGH);

        ledcAttach(rpwm_, 5000, 8);
        ledcAttach(lpwm_, 5000, 8);

        stop();
    }

    void forward(int pwm) {
        ledcWrite(lpwm_, 0);
        ledcWrite(rpwm_, pwm);
    }

    void backward(int pwm) {
        ledcWrite(rpwm_, 0);
        ledcWrite(lpwm_, pwm);
    }

    void stop() {
        ledcWrite(rpwm_, 0);
        ledcWrite(lpwm_, 0);
    }

    
    void disable() {
        stop();
        digitalWrite(ren_, LOW);
        digitalWrite(len_, LOW);
    }

private:
    int rpwm_, lpwm_, ren_, len_;
};


class SideDrive {
public:
    SideDrive(SingleMotor &frontMotor, SingleMotor &rearMotor)
        : front_(frontMotor), rear_(rearMotor) {}

    void begin() {
        front_.begin();
        rear_.begin();
    }

    void forward(int pwm)  { front_.forward(pwm);  rear_.forward(pwm);  }
    void backward(int pwm) { front_.backward(pwm); rear_.backward(pwm); }
    void stop()             { front_.stop();         rear_.stop();       }

private:
    SingleMotor &front_;
    SingleMotor &rear_;
};


class MotorDriver {
public:
    MotorDriver(SideDrive &left, SideDrive &right, int speed = 200)
        : left_(left), right_(right), speed_(speed) {}

    void begin() {
        left_.begin();
        right_.begin();
        stop();
    }

    void forward()  { left_.forward(speed_);  right_.forward(speed_);  }
    void backward() { left_.backward(speed_); right_.backward(speed_); }
    void left()     { left_.backward(speed_); right_.forward(speed_);  }  // pivot left
    void right()    { left_.forward(speed_);  right_.backward(speed_); }  // pivot right
    void stop()     { left_.stop();           right_.stop();           }

private:
    SideDrive &left_;
    SideDrive &right_;
    int speed_;
};


class CarUnit {
public:
    CarUnit(MotorDriver &motors) : motors_(motors) {
        instance_ = this;
    }

    bool begin() {
        Serial.begin(115200);
        delay(500);

        motors_.begin();

        WiFi.mode(WIFI_STA);
        Serial.print("Car Unit MAC Address: ");
        Serial.println(WiFi.macAddress()); 
        if (esp_now_init() != ESP_OK) {
            Serial.println("ESP-NOW init failed!");
            return false;
        }

        esp_now_register_recv_cb(CarUnit::onDataRecvStatic);
        return true;
    }

    void handleMessage(const GestureMessage &msg) {
        Serial.print("Received -> Command: ");
        Serial.print(msg.command);
        Serial.print(" | Pitch: ");
        Serial.print(msg.pitch);
        Serial.print(" | Roll: ");
        Serial.println(msg.roll);

        switch (msg.command) {
            case 'F': motors_.forward();  break;
            case 'B': motors_.backward(); break;
            case 'L': motors_.left();     break;
            case 'R': motors_.right();    break;
            case 'S': motors_.stop();     break;
            default:  motors_.stop();     break;
        }
    }

private:
    MotorDriver &motors_;
    static CarUnit *instance_;

    static void onDataRecvStatic(const esp_now_recv_info_t *info, const uint8_t *data, int len) {
        if (instance_ == nullptr || len != sizeof(GestureMessage)) return;
        GestureMessage msg;
        memcpy(&msg, data, sizeof(msg));
        instance_->handleMessage(msg);
    }
};

CarUnit *CarUnit::instance_ = nullptr;

// ============================================================
//  Global instances + Arduino entry points
// ============================================================

// -- Left side (2 BTS7960 modules) --
SingleMotor leftFront(13,14, 27, 26);
SingleMotor leftRear (25, 33, 32, 15);
SideDrive   leftSide(leftFront, leftRear);


SingleMotor rightFront(4, 16, 17, 5);
SingleMotor rightRear (18, 19, 23, 22);
SideDrive   rightSide(rightFront, rightRear);

MotorDriver motors(leftSide, rightSide, /*speed*/200);
CarUnit car(motors);

void setup() {
    car.begin();
}

void loop() {
    
}
