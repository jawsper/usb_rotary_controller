#include <Encoder.h>
#include <Bounce2.h>
//rotary_volume_mk1.ino

#define LED_PIN 13

class Config
{
private:
public:
    void save(){}
    void read(){}
};


class Rotary
{
private:
    Encoder encoder;
    Bounce bouncer;
    int32_t oldval;

public:
    Rotary(uint8_t pin_enc_A, uint8_t pin_enc_B, uint8_t pin_switch) :
        encoder(pin_enc_A, pin_enc_B), bouncer(), oldval(0)
    {
        pinMode(pin_switch, INPUT_PULLUP);
        bouncer.attach(pin_switch);
        bouncer.interval(5);
    }

    inline void update() { bouncer.update(); }
    inline bool fell() { return bouncer.fell(); }
    inline bool rose() { return bouncer.rose(); }

    int rotation()
    {
        int32_t newval = encoder.read();
        if(oldval != newval)
        {
            if(newval >= (oldval + 4))
            {
                oldval = newval;
                return -1;
            }
            if(newval <= (oldval - 4))
            {
                oldval = newval;
                return 1;
            }
        }
        return 0;
    }

  
};

Rotary rot1(14, 15, 12);

void setup()
{
    pinMode(LED_PIN, OUTPUT);
    digitalWrite(LED_PIN, LOW);
#ifdef USB_SERIAL
    Serial.begin(115200);
#endif
}

bool alt_mode_enabled = false;
#define DOUBLE_CLICK_TIMEOUT 500
#define LONG_PRESS_TIMEOUT 1000
enum click_action_t
{
    NO_CLICK = 0,
    SINGLE_CLICK,
    DOUBLE_CLICK,
    LONG_PRESS
};

bool one_click = false;
uint32_t double_click_timeout = 0;
uint32_t long_press_timeout = 0;
bool held = false;
bool did_long_click = false;

void loop()
{
    click_action_t click_action = NO_CLICK;
    // Update debouncer 
    rot1.update();
    if(rot1.fell())
    {
        held = true;
        long_press_timeout = millis() + LONG_PRESS_TIMEOUT;
    }
    else if(rot1.rose())
    {
        held = false;
        long_press_timeout = 0;

        if(!one_click)
        {
            if(!did_long_click)
            {
                one_click = true;
                double_click_timeout = millis() + DOUBLE_CLICK_TIMEOUT;
            }
            else
            {
                did_long_click = false;
            }
        }
        else
        {
            one_click = false;
            double_click_timeout = 0;
            click_action = DOUBLE_CLICK;
        }
    }

    if(one_click)
    {
        if(!held && double_click_timeout > 0 && millis() > double_click_timeout)
        {
            one_click = false;
            click_action = SINGLE_CLICK;
        }
    }
    if(!one_click && held && long_press_timeout > 0 && millis() > long_press_timeout)
    {
        click_action = LONG_PRESS;
        long_press_timeout = 0;
        double_click_timeout = 0;
        did_long_click = true;
    }

    uint16_t center_key, left_key, right_key;

    center_key = alt_mode_enabled ? KEY_MEDIA_PLAY_PAUSE : KEY_MEDIA_MUTE;
    left_key = alt_mode_enabled ? KEY_MEDIA_PREV_TRACK : KEY_MEDIA_VOLUME_DEC;
    right_key = alt_mode_enabled ? KEY_MEDIA_NEXT_TRACK : KEY_MEDIA_VOLUME_INC;

    switch(click_action)
    {
        case SINGLE_CLICK:
            #ifdef USB_SERIAL
                Serial.println("SINGLE_CLICK");
            #else
                Keyboard.press(center_key);
                Keyboard.release(center_key);
            #endif
            break;
        case DOUBLE_CLICK:
            #ifdef USB_SERIAL
                Serial.println("DOUBLE_CLICK");
            #else
                alt_mode_enabled = !alt_mode_enabled;
                digitalWrite(LED_PIN, alt_mode_enabled);
            #endif
            break;
        case LONG_PRESS:
            #ifdef USB_SERIAL
                Serial.println("LONG_PRESS");
            #else
                Keyboard.press(KEY_MEDIA_STOP);
                Keyboard.release(KEY_MEDIA_STOP);
            #endif
            break;
        default:
            break;
    }

    switch(rot1.rotation())
    {
        case -1:
            #ifdef USB_SERIAL
                Serial.println("ROT_LEFT");
            #else
                Keyboard.press(left_key);
                Keyboard.release(left_key);
            #endif
            break;
        case 1:
            #ifdef USB_SERIAL
                Serial.println("ROT_RIGHT");
            #else
                Keyboard.press(right_key);
                Keyboard.release(right_key);
            #endif
            break;
    }
}

