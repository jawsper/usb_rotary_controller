#include <Encoder.h>
#include <Bounce2.h>
#include <FastCRC.h>
#include <EEPROM.h>
//rotary_volume_mk1.ino

#define LED_PIN 13

#define DOUBLE_CLICK_TIMEOUT 500
#define LONG_PRESS_TIMEOUT 500

FastCRC32 CRC32;
struct Config
{
    uint32_t crc;
    uint16_t double_click_timeout;
    uint16_t long_press_timeout;

    void save()
    {
        crc = this->calculate_crc32();
        Serial.print("crc = "); Serial.println(crc, HEX);
        uint8_t* data = (uint8_t*)this;
        for(unsigned int i = 0; i < sizeof(Config); i++)
        {
            #ifdef USB_SERIAL
            Serial.print(i, DEC);
            Serial.print(" = ");
            Serial.println(*(data + i), HEX);
            #endif
            EEPROM.write(i, *(data + i));
        }
    }
    void load()
    {
        uint8_t* data = (uint8_t*)this;
        for(unsigned int i = 0; i < sizeof(Config); i++)
        {
            *(data + i) = EEPROM.read(i);
        }
        if(this->crc_valid())
        {
            #ifdef USB_SERIAL
            Serial.println("CRC valid!");
            Serial.print("config->crc = "); Serial.println(this->crc, HEX);
            Serial.print("config->dcto = "); Serial.println(this->double_click_timeout, DEC);
            Serial.print("config->lpto = "); Serial.println(this->long_press_timeout, DEC);
            #endif
        }
        else
        {
            #ifdef USB_SERIAL
            Serial.print("Invalid CRC! expected: "); Serial.print(this->calculate_crc32(), HEX);
            Serial.print(", got: "); Serial.println(this->crc, HEX);
            Serial.println("Saving default values into eeprom");
            #endif
            this->double_click_timeout = DOUBLE_CLICK_TIMEOUT;
            this->long_press_timeout = LONG_PRESS_TIMEOUT;
            this->save();
            #ifdef USB_SERIAL
            Serial.print("config->dcto = "); Serial.println(this->double_click_timeout, DEC);
            Serial.print("config->lpto = "); Serial.println(this->long_press_timeout, DEC);
            #endif
        }
    }
private:
    bool crc_valid()
    {
        // return false;
        return this->crc == this->calculate_crc32();
    }
    uint32_t calculate_crc32()
    {
        return CRC32.crc32(((uint8_t*)this) + sizeof(uint32_t), sizeof(Config) - sizeof(uint32_t));
    }
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
Config config;

void setup()
{
    pinMode(LED_PIN, OUTPUT);
    digitalWrite(LED_PIN, LOW);
#ifdef USB_SERIAL
    Serial.begin(115200);
    while(!Serial){}
#endif
    config.load();
}

bool alt_mode_enabled = false;
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
        long_press_timeout = millis() + config.long_press_timeout;
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
                double_click_timeout = millis() + config.double_click_timeout;
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

    #ifndef USB_SERIAL
    uint16_t center_key, left_key, right_key;

    center_key = alt_mode_enabled ? KEY_MEDIA_PLAY_PAUSE : KEY_MEDIA_MUTE;
    left_key = alt_mode_enabled ? KEY_MEDIA_PREV_TRACK : KEY_MEDIA_VOLUME_DEC;
    right_key = alt_mode_enabled ? KEY_MEDIA_NEXT_TRACK : KEY_MEDIA_VOLUME_INC;
    #endif

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

