

#include <esp_now.h>
#include <WiFi.h>


struct GestureMessage {
  char  command;   // 'F','B','L','R','S'
  float pitch;
  float roll;
};



class SingleMotor {
public:
    SingleMotor(int en, int in1, int in2)
        : en_(en), in1_(in1), in2_(in2) {}

    void begin() {
        pinMode(in1_, OUTPUT);
        pinMode(in2_, OUTPUT);
        ledcAttach(en_, 5000, 8);   // Arduino-ESP32 core 3.x PWM API
        stop();
    }

    void forward(int pwm) {
        digitalWrite(in1_, HIGH);
        digitalWrite(in2_, LOW);
        ledcWrite(en_, pwm);
    }

    void backward(int pwm) {
        digitalWrite(in1_, LOW);
        digitalWrite(in2_, HIGH);
        ledcWrite(en_, pwm);
    }

    void stop() {
        digitalWrite(in1_, LOW);
        digitalWrite(in2_, LOW);
        ledcWrite(en_, 0);
    }

private:
    int en_, in1_, in2_;
};


class SideDrive {
public:
    SideDrive(SingleMotor &frontMotor, SingleMotor &rearMotor)
        : front_(frontMotor), rear_(rearMotor) {}

    void begin() {
        front_.begin();
        rear_.begin();
    }

    void forward(int pwm) { front_.forward(pwm); rear_.forward(pwm); }
    void backward(int pwm) { front_.backward(pwm); rear_.backward(pwm); }
    void stop() { front_.stop(); rear_.stop(); }

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
    void left()     { left_.backward(speed_); right_.forward(speed_);  }  
    void right()    { left_.forward(speed_);  right_.backward(speed_); }  
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
        Serial.println(WiFi.macAddress());  // copy into glove's carAddress[]

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


// -- Driver 1 (Left side) --
SingleMotor leftFront(/*EN*/14, /*IN1*/27, /*IN2*/26);
SingleMotor leftRear (/*EN*/25, /*IN1*/33, /*IN2*/32);
SideDrive   leftSide(leftFront, leftRear);

// -- Driver 2 (Right side) --
SingleMotor rightFront(/*EN*/4,  /*IN1*/16, /*IN2*/17);
SingleMotor rightRear (/*EN*/5,  /*IN1*/18, /*IN2*/19);
SideDrive   rightSide(rightFront, rightRear);

MotorDriver motors(leftSide, rightSide, /*speed*/200);
CarUnit car(motors);

void setup() {
    car.begin();
}

void loop() {
    
}
