#define  P(col, row)   (((row) * 5) + (col))
#define  RGB(r, g, b)  (((r) << 16) + ((g) << 8) + (b))

#include "M5Atom.h"
#include <driver/i2s.h>

#define CONFIG_I2S_BCK_PIN 19
#define CONFIG_I2S_LRCK_PIN 33
#define CONFIG_I2S_DATA_PIN 22
#define CONFIG_I2S_DATA_IN_PIN 23

#define SPAKER_I2S_NUMBER I2S_NUM_0

#define MODE_MIC 0
#define MODE_SPK 1

//extern const unsigned char audio[364808];
extern const unsigned char cracker[];
extern const unsigned char bang[];
extern const unsigned char clear[];


CRGB led(0, 0, 0);
double pitch, roll;
double r_rand = 180 / PI;
double xInte=2, yInte=2; 
size_t bytes_written;
bool state = true;
bool loop_play = false;
bool colide = false;

struct ballPlace{
    unsigned char x;
    unsigned char y;
};

ballPlace nowBallPlace,prevBallPlace,bombPlace,gollPlace;

bool InitI2SSpakerOrMic(int mode)
{
    esp_err_t err = ESP_OK;

    i2s_driver_uninstall(SPAKER_I2S_NUMBER);
    i2s_config_t i2s_config = {
        .mode = (i2s_mode_t)(I2S_MODE_MASTER),
        .sample_rate = 16000,
        .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT, // is fixed at 12bit, stereo, MSB
        .channel_format = I2S_CHANNEL_FMT_ALL_RIGHT,
        .communication_format = I2S_COMM_FORMAT_I2S,
        .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
        .dma_buf_count = 6,
        .dma_buf_len = 60,
    };
    if (mode == MODE_MIC)
    {
        i2s_config.mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX | I2S_MODE_PDM);
    }
    else
    {
        i2s_config.mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX);
        i2s_config.use_apll = false;
        i2s_config.tx_desc_auto_clear = true;
    }

    //Serial.println("Init i2s_driver_install");

    err += i2s_driver_install(SPAKER_I2S_NUMBER, &i2s_config, 0, NULL);
    i2s_pin_config_t tx_pin_config;

    tx_pin_config.bck_io_num = CONFIG_I2S_BCK_PIN;
    tx_pin_config.ws_io_num = CONFIG_I2S_LRCK_PIN;
    tx_pin_config.data_out_num = CONFIG_I2S_DATA_PIN;
    tx_pin_config.data_in_num = CONFIG_I2S_DATA_IN_PIN;

    //Serial.println("Init i2s_set_pin");
    err += i2s_set_pin(SPAKER_I2S_NUMBER, &tx_pin_config);
    //Serial.println("Init i2s_set_clk");
    err += i2s_set_clk(SPAKER_I2S_NUMBER, 44100, I2S_BITS_PER_SAMPLE_16BIT, I2S_CHANNEL_MONO);

    return true;
}

void setup(){
    M5.begin(true, true, true); 
    M5.IMU.Init(); 
    M5.IMU.SetGyroFsr(M5.IMU.GFS_500DPS); 
    M5.IMU.SetAccelFsr(M5.IMU.AFS_4G);
    pinMode(21, OUTPUT);
    pinMode(25, OUTPUT);

    digitalWrite(21, 0);
    digitalWrite(25, 1);
    randomSeed(analogRead(0));

    InitI2SSpakerOrMic(MODE_SPK);
    do
    {
        bombPlace.x = random(0,5);
        bombPlace.y = random(0,5);
    } while (bombPlace.x == 2 && bombPlace.y == 2);

    do
    {
        gollPlace.x = random(0,5);
        gollPlace.y = random(0,5);
    } while ((gollPlace.x == 2 && gollPlace.y == 2) || (gollPlace.x == bombPlace.x && gollPlace.y == bombPlace.y));

    M5.dis.drawpix(P(bombPlace.x,bombPlace.y),RGB(0xFF,0,0));
    M5.dis.drawpix(P(gollPlace.x,gollPlace.y),RGB(0,0,0xFF));
    M5.dis.drawpix(P(2,2),RGB(0,0xFF,0));
    delay(1000);
}

void loop(){
    delay(20);  //Delay 20ms
    M5.IMU.getAttitude(&pitch, &roll); //Read the attitude (pitch, heading) of the IMU and store it in relevant variables.  ポインタをブン投げて格納してもらっている
    double arc = atan2(pitch, roll) * r_rand + 180; //傾いている方向を算出
    double val = sqrt(pitch * pitch + roll * roll);  //傾いている量を算出
    //Serial.printf("%.2f,%.2f,%.2f,%.2f\n", pitch, roll, arc, val);  //serial port output the formatted string. 
    val = (val * 6) > 100 ? 100 : val * 6;
    /*
    Pitch,Rollから傾きの方向と角度を算出：PitchとRoll積算が楽そう？
    →三角関数と正規化で移動ピクセル量を算出
    →端にぶつかる場合は停止
    */

    xInte = xInte > 4 ? 4 : xInte < 0 ? 0 : xInte + (-1 * pitch) / 50;
    yInte = yInte > 4 ? 4 : yInte < 0 ? 0 : yInte + (-1 * roll)  / 50;
    

    nowBallPlace.x = xInte;
    nowBallPlace.y = yInte;

    Serial.printf("%.4f,%.4f,%d,%d\n",xInte,yInte,nowBallPlace.x,nowBallPlace.y);

    M5.dis.clear();
    if((nowBallPlace.x == bombPlace.x) && (nowBallPlace.y == bombPlace.y)){

        M5.dis.drawpix(P(gollPlace.x,gollPlace.y),RGB(0xFF,0,0));
        M5.dis.drawpix(P(bombPlace.x,bombPlace.y),RGB(0xFF,0xFF,0));
        i2s_write(SPAKER_I2S_NUMBER, bang, 86896, &bytes_written, portMAX_DELAY); 
        do
        {
            bombPlace.x = random(0,5);
            bombPlace.y = random(0,5);
        } while (bombPlace.x == 2 && bombPlace.y == 2);

        do
        {
            gollPlace.x = random(0,5);
            gollPlace.y = random(0,5);
        } while ((gollPlace.x == 2 && gollPlace.y == 2) || (gollPlace.x == bombPlace.x && gollPlace.y == bombPlace.y));

        xInte=2; yInte=2;
        nowBallPlace.x = xInte;
        nowBallPlace.y = yInte;
        M5.dis.clear();
        M5.dis.drawpix(P(bombPlace.x,bombPlace.y),RGB(0xFF,0,0));
        M5.dis.drawpix(P(gollPlace.x,gollPlace.y),RGB(0,0,0xFF));
        M5.dis.drawpix(P(nowBallPlace.x,nowBallPlace.y),RGB(0,0xFF,0));
        delay(1000);

    }else if ((nowBallPlace.x == gollPlace.x) && (nowBallPlace.y == gollPlace.y)){

        M5.dis.drawpix(P(bombPlace.x,bombPlace.y),RGB(0,0,0xFF));
        M5.dis.drawpix(P(gollPlace.x,gollPlace.y),RGB(0xFF,0xFF,0xFF));
        i2s_write(SPAKER_I2S_NUMBER, clear, 178864, &bytes_written, portMAX_DELAY); 
        do
        {
            bombPlace.x = random(0,5);
            bombPlace.y = random(0,5);
        } while (bombPlace.x == 2 && bombPlace.y == 2);

        do
        {
            gollPlace.x = random(0,5);
            gollPlace.y = random(0,5);
        } while ((gollPlace.x == 2 && gollPlace.y == 2) || (gollPlace.x == bombPlace.x && gollPlace.y == bombPlace.y));

        xInte=2; yInte=2;
        nowBallPlace.x = xInte;
        nowBallPlace.y = yInte;
        M5.dis.clear();
        M5.dis.drawpix(P(bombPlace.x,bombPlace.y),RGB(0xFF,0,0));
        M5.dis.drawpix(P(gollPlace.x,gollPlace.y),RGB(0,0,0xFF));
        M5.dis.drawpix(P(nowBallPlace.x,nowBallPlace.y),RGB(0,0xFF,0));
        delay(1000);

    }else{

        M5.dis.drawpix(P(bombPlace.x,bombPlace.y),RGB(0xFF,0,0));
        M5.dis.drawpix(P(gollPlace.x,gollPlace.y),RGB(0,0,0xFF));
        M5.dis.drawpix(P(nowBallPlace.x,nowBallPlace.y),RGB(0,0xFF,0));


        if( (xInte >= 4 && prevBallPlace.x != 4) || (xInte < 1 && prevBallPlace.x != 0) || (yInte >= 4 && prevBallPlace.y != 4) || (yInte < 1 && prevBallPlace.y != 0) ){
                i2s_write(SPAKER_I2S_NUMBER, cracker, 36528, &bytes_written, portMAX_DELAY); 
        }

        prevBallPlace = nowBallPlace;
    }
}