/**
 * @file    bsp_wifi.h
 * @brief   ESP8266 WiFi AT driver for SmartPillBox
 */
#ifndef __BSP_WIFI_H
#define __BSP_WIFI_H

#include "stm32f10x.h"
#include "stm32f10x_gpio.h"
#include "stm32f10x_rcc.h"
#include "stm32f10x_usart.h"

#define WIFI_DEFAULT_BAUDRATE      115200u
#define WIFI_ALT_BAUDRATE          9600u
#define WIFI_RX_BUFFER_SIZE        512u
#define WIFI_AT_BUFFER_SIZE        160u

#define WIFI_AP_SSID               "vivo X100"
#define WIFI_AP_PASSWORD           "qazplm01"
#define WIFI_SERVER_IP             "10.11.183.61"
#define WIFI_SERVER_IP_ALT1        "10.47.61.64"
#define WIFI_SERVER_PORT           8080u

/* Wildfire F103 Guide onboard ESP8266 wiring:
 * PB10 = USART3_TX -> ESP8266 URXD
 * PB11 = USART3_RX <- ESP8266 UTXD
 * PB8  = WIFI_EN
 * PB9  = WIFI_RST
 */
#define WIFI_USART                 USART3
#define WIFI_USART_IRQn            USART3_IRQn
#define WIFI_USART_CLK             RCC_APB1Periph_USART3
#define WIFI_USART_CLK_CMD         RCC_APB1PeriphClockCmd

#define WIFI_GPIO_PORT             GPIOB
#define WIFI_GPIO_CLK              RCC_APB2Periph_GPIOB
#define WIFI_TX_PIN                GPIO_Pin_10
#define WIFI_RX_PIN                GPIO_Pin_11
#define WIFI_EN_PIN                GPIO_Pin_8
#define WIFI_RST_PIN               GPIO_Pin_9

typedef enum {
    WIFI_OK = 0,
    WIFI_ERROR,
    WIFI_TIMEOUT
} WiFi_Status;

typedef enum {
    WIFI_STEP_IDLE = 0,
    WIFI_STEP_INIT,
    WIFI_STEP_AT,
    WIFI_STEP_RST,
    WIFI_STEP_ECHO,
    WIFI_STEP_MODE,
    WIFI_STEP_MUX,
    WIFI_STEP_AP,
    WIFI_STEP_TCP,
    WIFI_STEP_SEND
} WiFi_Step;

void WiFi_Init(uint32_t baudrate);
void WiFi_SetBaudrate(uint32_t baudrate);
void WiFi_IRQHandler(void);

void WiFi_ClearRxBuffer(void);
const char* WiFi_GetRxBuffer(void);
uint16_t WiFi_GetRxLength(void);
uint8_t WiFi_ReadTcpPayload(char *out, uint16_t maxLen);

WiFi_Status WiFi_SendCommand(const char *cmd, const char *expect, uint32_t timeoutMs);
WiFi_Status WiFi_Test(uint32_t timeoutMs);
WiFi_Status WiFi_Restore(uint32_t timeoutMs);
WiFi_Status WiFi_EchoOff(uint32_t timeoutMs);
WiFi_Status WiFi_SetStationMode(uint32_t timeoutMs);
WiFi_Status WiFi_DisconnectAP(uint32_t timeoutMs);
WiFi_Status WiFi_EnableStationDhcp(uint32_t timeoutMs);
WiFi_Status WiFi_SetSingleConnection(uint32_t timeoutMs);
WiFi_Status WiFi_JoinAP(const char *ssid, const char *password, uint32_t timeoutMs);
WiFi_Status WiFi_TCPConnect(const char *host, uint16_t port, uint32_t timeoutMs);
WiFi_Status WiFi_UDPConnect(const char *host, uint16_t remotePort, uint16_t localPort, uint32_t timeoutMs);
WiFi_Status WiFi_SendData(const char *data, uint16_t len, uint32_t timeoutMs);
WiFi_Status WiFi_SendString(const char *text, uint32_t timeoutMs);
WiFi_Status WiFi_Close(uint32_t timeoutMs);
WiFi_Status WiFi_ConnectAPDefault(void);
WiFi_Status WiFi_ConnectDefault(void);
uint8_t WiFi_ReadIpdPayload(uint8_t *out, uint16_t maxLen, uint16_t *outLen);
WiFi_Step WiFi_GetLastStep(void);
const char* WiFi_GetLastStepString(void);
uint32_t WiFi_GetActiveBaudrate(void);

#endif /* __BSP_WIFI_H */
