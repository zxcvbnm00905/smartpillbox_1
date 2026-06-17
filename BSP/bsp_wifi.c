/**
 * @file    bsp_wifi.c
 * @brief   ESP8266 WiFi AT driver for SmartPillBox
 */
#include "bsp_wifi.h"
#include "stm32f10x_gpio.h"
#include "stm32f10x_rcc.h"
#include "stm32f10x_usart.h"
#include "misc.h"
#include <stdio.h>
#include <string.h>

extern volatile uint32_t g_SysTickCount;

static volatile uint16_t g_WiFiRxLen = 0;
static char g_WiFiRxBuf[WIFI_RX_BUFFER_SIZE];
static volatile WiFi_Step g_WiFiLastStep = WIFI_STEP_IDLE;
static uint32_t g_WiFiActiveBaudrate = WIFI_DEFAULT_BAUDRATE;

static void WiFi_DelayMs(uint32_t ms)
{
    uint32_t startTick;

    startTick = g_SysTickCount;
    while((g_SysTickCount - startTick) < ms);
}

static void WiFi_USART_SendByte(uint8_t data)
{
    while(USART_GetFlagStatus(WIFI_USART, USART_FLAG_TXE) == RESET);
    USART_SendData(WIFI_USART, data);
}

static void WiFi_USART_SendData(const char *data, uint16_t len)
{
    uint16_t i;

    for(i = 0; i < len; i++) {
        WiFi_USART_SendByte((uint8_t)data[i]);
    }
    while(USART_GetFlagStatus(WIFI_USART, USART_FLAG_TC) == RESET);
}

void WiFi_SetBaudrate(uint32_t baudrate)
{
    USART_InitTypeDef usart;

    g_WiFiActiveBaudrate = baudrate;
    USART_DeInit(WIFI_USART);
    usart.USART_BaudRate = baudrate;
    usart.USART_WordLength = USART_WordLength_8b;
    usart.USART_StopBits = USART_StopBits_1;
    usart.USART_Parity = USART_Parity_No;
    usart.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
    usart.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
    USART_Init(WIFI_USART, &usart);

    WiFi_ClearRxBuffer();
    USART_ITConfig(WIFI_USART, USART_IT_RXNE, ENABLE);
    USART_Cmd(WIFI_USART, ENABLE);
}

static uint8_t WiFi_BufferContains(const char *text)
{
    uint8_t found;

    __disable_irq();
    found = (strstr(g_WiFiRxBuf, text) != 0);
    __enable_irq();

    return found;
}

static WiFi_Status WiFi_WaitFor(const char *expect, uint32_t timeoutMs)
{
    uint32_t startTick;

    startTick = g_SysTickCount;
    while((g_SysTickCount - startTick) < timeoutMs) {
        if(WiFi_BufferContains(expect)) {
            return WIFI_OK;
        }
        if(WiFi_BufferContains("ERROR") || WiFi_BufferContains("FAIL")) {
            return WIFI_ERROR;
        }
    }

    return WIFI_TIMEOUT;
}

void WiFi_Init(uint32_t baudrate)
{
    GPIO_InitTypeDef gpio;
    NVIC_InitTypeDef nvic;

    RCC_APB2PeriphClockCmd(WIFI_GPIO_CLK | RCC_APB2Periph_AFIO, ENABLE);
    WIFI_USART_CLK_CMD(WIFI_USART_CLK, ENABLE);
    g_WiFiActiveBaudrate = baudrate;
    g_WiFiLastStep = WIFI_STEP_INIT;

    gpio.GPIO_Pin = WIFI_EN_PIN | WIFI_RST_PIN;
    gpio.GPIO_Speed = GPIO_Speed_50MHz;
    gpio.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_Init(WIFI_GPIO_PORT, &gpio);
    GPIO_SetBits(WIFI_GPIO_PORT, WIFI_EN_PIN);
    GPIO_SetBits(WIFI_GPIO_PORT, WIFI_RST_PIN);

    gpio.GPIO_Pin = WIFI_TX_PIN;
    gpio.GPIO_Speed = GPIO_Speed_50MHz;
    gpio.GPIO_Mode = GPIO_Mode_AF_PP;
    GPIO_Init(WIFI_GPIO_PORT, &gpio);

    gpio.GPIO_Pin = WIFI_RX_PIN;
    gpio.GPIO_Mode = GPIO_Mode_IN_FLOATING;
    GPIO_Init(WIFI_GPIO_PORT, &gpio);

    WiFi_SetBaudrate(baudrate);

    nvic.NVIC_IRQChannel = WIFI_USART_IRQn;
    nvic.NVIC_IRQChannelPreemptionPriority = 1;
    nvic.NVIC_IRQChannelSubPriority = 1;
    nvic.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&nvic);

    GPIO_ResetBits(WIFI_GPIO_PORT, WIFI_RST_PIN);
    WiFi_DelayMs(50);
    GPIO_SetBits(WIFI_GPIO_PORT, WIFI_RST_PIN);
    WiFi_DelayMs(3000);
}

void WiFi_IRQHandler(void)
{
    uint16_t data;

    if(USART_GetITStatus(WIFI_USART, USART_IT_RXNE) != RESET) {
        data = USART_ReceiveData(WIFI_USART);
        if(g_WiFiRxLen < (WIFI_RX_BUFFER_SIZE - 1u)) {
            g_WiFiRxBuf[g_WiFiRxLen++] = (char)(data & 0xFFu);
            g_WiFiRxBuf[g_WiFiRxLen] = '\0';
        }
        USART_ClearITPendingBit(WIFI_USART, USART_IT_RXNE);
    }
}

void WiFi_ClearRxBuffer(void)
{
    __disable_irq();
    memset(g_WiFiRxBuf, 0, sizeof(g_WiFiRxBuf));
    g_WiFiRxLen = 0;
    __enable_irq();
}

const char* WiFi_GetRxBuffer(void)
{
    return g_WiFiRxBuf;
}

uint16_t WiFi_GetRxLength(void)
{
    return g_WiFiRxLen;
}

uint8_t WiFi_ReadTcpPayload(char *out, uint16_t maxLen)
{
    char *ipd;
    char *colon;
    char *p;
    uint16_t i;
    uint16_t expectedLen = 0;
    uint16_t copyLen;
    uint16_t availableLen;
    uint16_t colonOffset;

    if(out == 0 || maxLen == 0) {
        return 0;
    }

    out[0] = '\0';

    __disable_irq();
    ipd = strstr(g_WiFiRxBuf, "+IPD,");
    if(ipd == 0) {
        __enable_irq();
        return 0;
    }

    colon = strchr(ipd, ':');
    if(colon == 0) {
        __enable_irq();
        return 0;
    }

    p = colon;
    while(p > ipd && *(p - 1) >= '0' && *(p - 1) <= '9') {
        p--;
    }
    while(p < colon && *p >= '0' && *p <= '9') {
        expectedLen = (uint16_t)(expectedLen * 10u + (uint16_t)(*p - '0'));
        p++;
    }

    if(expectedLen == 0) {
        __enable_irq();
        return 0;
    }

    colonOffset = (uint16_t)(colon - g_WiFiRxBuf);
    availableLen = (uint16_t)(g_WiFiRxLen - colonOffset - 1u);
    if(availableLen < expectedLen) {
        __enable_irq();
        return 0;
    }

    colon++;
    copyLen = expectedLen;
    if(copyLen >= maxLen) {
        copyLen = (uint16_t)(maxLen - 1u);
    }

    for(i = 0; i < copyLen; i++) {
        out[i] = colon[i];
    }
    out[copyLen] = '\0';

    memset(g_WiFiRxBuf, 0, sizeof(g_WiFiRxBuf));
    g_WiFiRxLen = 0;
    __enable_irq();

    return 1;
}

uint8_t WiFi_ReadIpdPayload(uint8_t *out, uint16_t maxLen, uint16_t *outLen)
{
    uint16_t i;
    uint16_t colonIndex = 0xFFFFu;
    uint16_t ipdIndex = 0xFFFFu;
    uint16_t expectedLen = 0;
    uint16_t availableLen;
    uint16_t copyLen;
    char *p;

    if(out == 0 || maxLen == 0) {
        return 0;
    }
    if(outLen != 0) {
        *outLen = 0;
    }

    __disable_irq();
    if(g_WiFiRxLen < 7u) {
        __enable_irq();
        return 0;
    }

    for(i = 0; i <= (uint16_t)(g_WiFiRxLen - 5u); i++) {
        if(memcmp(&g_WiFiRxBuf[i], "+IPD,", 5u) == 0) {
            ipdIndex = i;
            break;
        }
    }
    if(ipdIndex == 0xFFFFu) {
        __enable_irq();
        return 0;
    }

    for(i = ipdIndex; i < g_WiFiRxLen; i++) {
        if(g_WiFiRxBuf[i] == ':') {
            colonIndex = i;
            break;
        }
    }
    if(colonIndex == 0xFFFFu) {
        __enable_irq();
        return 0;
    }

    p = &g_WiFiRxBuf[colonIndex];
    while(p > &g_WiFiRxBuf[ipdIndex] && *(p - 1) >= '0' && *(p - 1) <= '9') {
        p--;
    }
    while(p < &g_WiFiRxBuf[colonIndex] && *p >= '0' && *p <= '9') {
        expectedLen = (uint16_t)(expectedLen * 10u + (uint16_t)(*p - '0'));
        p++;
    }
    if(expectedLen == 0u) {
        __enable_irq();
        return 0;
    }

    availableLen = (uint16_t)(g_WiFiRxLen - colonIndex - 1u);
    if(availableLen < expectedLen) {
        __enable_irq();
        return 0;
    }

    copyLen = expectedLen;
    if(copyLen > maxLen) {
        copyLen = maxLen;
    }
    memcpy(out, &g_WiFiRxBuf[colonIndex + 1u], copyLen);
    if(outLen != 0) {
        *outLen = copyLen;
    }

    memset(g_WiFiRxBuf, 0, sizeof(g_WiFiRxBuf));
    g_WiFiRxLen = 0;
    __enable_irq();

    return 1;
}

WiFi_Status WiFi_SendCommand(const char *cmd, const char *expect, uint32_t timeoutMs)
{
    WiFi_ClearRxBuffer();
    WiFi_USART_SendData(cmd, (uint16_t)strlen(cmd));

    if(expect == 0 || expect[0] == '\0') {
        return WIFI_OK;
    }

    return WiFi_WaitFor(expect, timeoutMs);
}

WiFi_Status WiFi_Test(uint32_t timeoutMs)
{
    WiFi_Status status;
    uint8_t i;

    for(i = 0; i < 5u; i++) {
        status = WiFi_SendCommand("AT\r\n", "OK", timeoutMs);
        if(status == WIFI_OK) {
            return WIFI_OK;
        }
        WiFi_DelayMs(300);
    }

    return status;
}

WiFi_Status WiFi_Restore(uint32_t timeoutMs)
{
    return WiFi_SendCommand("AT+RST\r\n", "ready", timeoutMs);
}

WiFi_Status WiFi_EchoOff(uint32_t timeoutMs)
{
    return WiFi_SendCommand("ATE0\r\n", "OK", timeoutMs);
}

WiFi_Status WiFi_SetStationMode(uint32_t timeoutMs)
{
    return WiFi_SendCommand("AT+CWMODE=1\r\n", "OK", timeoutMs);
}

WiFi_Status WiFi_DisconnectAP(uint32_t timeoutMs)
{
    return WiFi_SendCommand("AT+CWQAP\r\n", "OK", timeoutMs);
}

WiFi_Status WiFi_EnableStationDhcp(uint32_t timeoutMs)
{
    return WiFi_SendCommand("AT+CWDHCP=1,1\r\n", "OK", timeoutMs);
}

WiFi_Status WiFi_SetSingleConnection(uint32_t timeoutMs)
{
    return WiFi_SendCommand("AT+CIPMUX=0\r\n", "OK", timeoutMs);
}

WiFi_Status WiFi_JoinAP(const char *ssid, const char *password, uint32_t timeoutMs)
{
    char cmd[WIFI_AT_BUFFER_SIZE];

    sprintf(cmd, "AT+CWJAP=\"%s\",\"%s\"\r\n", ssid, password);
    return WiFi_SendCommand(cmd, "OK", timeoutMs);
}

WiFi_Status WiFi_TCPConnect(const char *host, uint16_t port, uint32_t timeoutMs)
{
    char cmd[WIFI_AT_BUFFER_SIZE];

    sprintf(cmd, "AT+CIPSTART=\"TCP\",\"%s\",%u\r\n", host, port);
    return WiFi_SendCommand(cmd, "OK", timeoutMs);
}

WiFi_Status WiFi_UDPConnect(const char *host, uint16_t remotePort, uint16_t localPort, uint32_t timeoutMs)
{
    char cmd[WIFI_AT_BUFFER_SIZE];

    sprintf(cmd, "AT+CIPSTART=\"UDP\",\"%s\",%u,%u,0\r\n", host, remotePort, localPort);
    return WiFi_SendCommand(cmd, "OK", timeoutMs);
}

WiFi_Status WiFi_SendData(const char *data, uint16_t len, uint32_t timeoutMs)
{
    char cmd[32];
    WiFi_Status status;

    g_WiFiLastStep = WIFI_STEP_SEND;
    sprintf(cmd, "AT+CIPSEND=%u\r\n", len);
    status = WiFi_SendCommand(cmd, ">", timeoutMs);
    if(status != WIFI_OK) {
        return status;
    }

    WiFi_USART_SendData(data, len);

    return WiFi_WaitFor("SEND OK", timeoutMs);
}

WiFi_Status WiFi_SendString(const char *text, uint32_t timeoutMs)
{
    return WiFi_SendData(text, (uint16_t)strlen(text), timeoutMs);
}

WiFi_Status WiFi_Close(uint32_t timeoutMs)
{
    return WiFi_SendCommand("AT+CIPCLOSE\r\n", "OK", timeoutMs);
}

WiFi_Status WiFi_ConnectAPDefault(void)
{
    WiFi_Status status;

    g_WiFiLastStep = WIFI_STEP_AT;
    status = WiFi_Test(1000);
    if(status != WIFI_OK) {
        return status;
    }

    g_WiFiLastStep = WIFI_STEP_ECHO;
    WiFi_EchoOff(1000);

    g_WiFiLastStep = WIFI_STEP_MODE;
    status = WiFi_SetStationMode(1000);
    if(status != WIFI_OK) {
        return status;
    }

    WiFi_DisconnectAP(1000);
    WiFi_EnableStationDhcp(1000);

    g_WiFiLastStep = WIFI_STEP_MUX;
    status = WiFi_SetSingleConnection(1000);
    if(status != WIFI_OK) {
        return status;
    }

    g_WiFiLastStep = WIFI_STEP_AP;
    status = WiFi_JoinAP(WIFI_AP_SSID, WIFI_AP_PASSWORD, 30000);
    if(status != WIFI_OK) {
        return status;
    }

    return WIFI_OK;
}

WiFi_Status WiFi_ConnectDefault(void)
{
    WiFi_Status status;

    status = WiFi_ConnectAPDefault();
    if(status != WIFI_OK) {
        return status;
    }

    g_WiFiLastStep = WIFI_STEP_TCP;
    status = WiFi_TCPConnect(WIFI_SERVER_IP, WIFI_SERVER_PORT, 3000);
    return status;
}

WiFi_Step WiFi_GetLastStep(void)
{
    return g_WiFiLastStep;
}

const char* WiFi_GetLastStepString(void)
{
    switch(g_WiFiLastStep) {
    case WIFI_STEP_IDLE: return "IDLE";
    case WIFI_STEP_INIT: return "INIT";
    case WIFI_STEP_AT:   return "AT";
    case WIFI_STEP_RST:  return "RST";
    case WIFI_STEP_ECHO: return "ECHO";
    case WIFI_STEP_MODE: return "MODE";
    case WIFI_STEP_MUX:  return "MUX";
    case WIFI_STEP_AP:   return "AP";
    case WIFI_STEP_TCP:  return "TCP";
    case WIFI_STEP_SEND: return "SEND";
    default:             return "UNK";
    }
}

uint32_t WiFi_GetActiveBaudrate(void)
{
    return g_WiFiActiveBaudrate;
}
