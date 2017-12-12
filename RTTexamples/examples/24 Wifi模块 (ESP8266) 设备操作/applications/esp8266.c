/***************************2012-2016, NJUT, Edu.******************************* 
FileName: esp8266.c 
Author:  �ﶬ÷       Version :  1.0       Date: 2016.11.26
Description:   wifiģ��ͨ��
Version:         1.0
History:         
      <author>  <time>   <version >   <desc> 
      Sundm    16/11/10    1.0       �ļ�����  

Description���豸��ʼ��������wifi����(UART4)�豸�򿪣�ע��ص���������ʼ�����ڽ����¼���
�����̣߳����Ӵ������ݡ�
����ATָ��ʱ���رմ��ڼ����̡߳�
�����յ����ݺ��������ݵ���ص�����,�����¼���wifi_send_data_package��
wifi_send_data_package�н������ݣ���������ݡ�
ʹ��  PC10-USART4Tx PC11-USART4Rx CS-PC9
Others:   ���ڽ������ݺ󣬼�� �ؼ���{"value":**}����ȡ GET�����õ�������
  Function List:  
   1. wificonfig() ����wifi�����豸���򿪣�ע��ص���������ʼ�����ڽ����¼�
   2. wifiinit(); wifi����
   3. wifijap() ������AP
   4. wificonnect(); ��Զ����������
   5. wifisend("abc"); �������ݷ���
   6. wificloseconnect(); �ر�Զ����������
   7. wifiexit();  wifi�˳�AP ���ر�
*******************************************************************************/ 
#include  <rtthread.h >
#include "esp8266.h"
#include "string.h"
#include "stdio.h"
#include "stdlib.h"
#include "led.h"


//����wifiʹ�ô��� �� ���ƿڣ�CS��
#define WIFI_PORT GPIOC
#define WIFI_PORT_RCC        RCC_APB2Periph_GPIOC
#define WIFI_CS_PIN GPIO_Pin_9

//����ATָ��
#define ESP8266_ATCMD "AT\x00D\x00A"               // AT��ѯ
#define ESP8266_RESET "AT+RST\x00D\x00A"           // ģ�鸴λ

#define ESP8266_CWMODE_STA "AT+CWMODE=1\x00D\x00A"      // ѡ��WiFiӦ��ģʽ  Stationģʽ
#define ESP8266_CWMODE_AP "AT+CWMODE=2\x00D\x00A"      // ѡ��WiFiӦ��ģʽ  APģʽ
#define ESP8266_CWMODE_APSTA "AT+CWMODE=3\x00D\x00A"      // ѡ��WiFiӦ��ģʽ  Station+APģʽ

#define ESP8266_CWLAP "AT+CWLAP\x00D\x00A"      // �г���ǰ�����
#define ESP8266_CWQAP "AT+CWQAP\x00D\x00A"      // �˳�
#define ESP8266_CIFSR "AT+CIFSR\x00D\x00A"      // ��ȡ����IP��ַ

#define ESP8266_CWJAP "AT+CWJAP=\"sundm75\",\"121111215\"\x00D\x00A"      // �������� TP-LINK_sundm
#define ESP8266_CIPMUX "AT+CIPMUX=0\x00D\x00A"      // ���õ�����
#define ESP8266_CIPMODE "AT+CIPMODE=1\x00D\x00A"      // ����͸��ģʽ

#define ESP8266_CIPSTART "AT+CIPSTART=\"TCP\",\"api.yeelink.net\",80\x00D\x00A"      // ����TCP/UDP����
#define ESP8266_CIPSTATUS "AT+CIPSTATUS\x00D\x00A"      // ���TCP/UDP����״̬
#define ESP8266_CIPSEND "AT+CIPSEND="      // ��������

#define ESP8266_CIPCLOSE "AT+CIPCLOSE\x00D\x00A"      // �ر�TCP/UDP����

/***************************WIFIģ�鴮�ڽ��Ք����¼�******************************/
#define REV_DATA      0x01
#define REV_WATCH      0x02
#define REV_STOPWATCH      0x04

#define REV_MASK      ( REV_DATA | REV_WATCH | REV_STOPWATCH )


static struct rt_event rev_event;
static rt_device_t wifi_device;


/* ����WIFI�����߳����*/
void wifiwatch_entry(void* parameter)
{
  rt_err_t result = RT_EOK;
  rt_uint32_t event;
  char wifi_rx_buffer[512]={0x00};
  rt_size_t  readnum;
  char * charaddr;
  char * charstartaddr;
 char *  charendaddr;
  uint8_t valuestr[3] = {0x00};
  uint8_t value = 0;
  
  while(1)
  {
      result = rt_event_recv(&rev_event, 
         REV_MASK, RT_EVENT_FLAG_OR | RT_EVENT_FLAG_CLEAR, 
         RT_WAITING_FOREVER, &event);
      if (result == RT_EOK)
      {
        if (event & REV_DATA)
        {
          rt_memset(wifi_rx_buffer,0x00,sizeof(wifi_rx_buffer));
          rt_thread_delay(RT_TICK_PER_SECOND/2);
          readnum = rt_device_read(wifi_device, 0, wifi_rx_buffer, 512);
          rt_kprintf(wifi_rx_buffer);
          /*���»�ȡ���ڽ������� �е� valueֵ */
          charaddr = rt_strstr(wifi_rx_buffer,"\"value\":");
          if(charaddr!=RT_NULL)
          {
            rt_kprintf(charaddr);    
            charstartaddr = charaddr + 8;
            charendaddr = rt_strstr(charaddr,"}") ;
            if(charendaddr!=RT_NULL)
            {
              int i=0;
              while (charstartaddr!=charendaddr)
              {
                valuestr[i] = *charstartaddr;
                charstartaddr++;i++;
              }
              value = atoi((char const*)valuestr);
              rt_kprintf("\r\n Wifi Device receive value = %d \r\n",value );
              if(value==1)
              {
                LEDOn(LED1);
              }
              else if(value==0)
              {
                LEDOff(LED1);
              }              
            }
          }
          /*��ȡ���ڽ������� �е� valueֵ ���� */
        }
        if (event & REV_STOPWATCH)
        {
          return;
        }
      }
    }
}

void wifiwatch(void)
{
  /* ����wifi watch�߳�*/
  rt_thread_t thread = rt_thread_create("wifiwatch",
  wifiwatch_entry, RT_NULL,
  1024, 25, 7);
  
  /* �����ɹ��������߳�*/
  if (thread != RT_NULL)
    rt_thread_startup(thread);
}
void wifistopwatch(void)
{
  rt_event_send(&rev_event, REV_STOPWATCH);
}

/*���ݵ���ص�����,�����¼���wifi_send_data_package*/
static rt_err_t wifi_uart_input(rt_device_t dev, rt_size_t size)
{
  rt_event_send(&rev_event, REV_DATA);
  return RT_EOK;
}

/*WIFI���ڷ��ͺͽ���*/
rt_bool_t wifi_send_data_package(char *cmd,char *ack,uint16_t waittime, uint8_t retrytime)
{
  rt_bool_t res = RT_FALSE; 
  rt_err_t result = RT_EOK;
  rt_uint32_t event;
  char wifi_rx_buffer[512]={0x00};
  rt_thread_t thread;
  
  thread = rt_thread_find("wifiwatch");
  if( thread != RT_NULL)
    rt_thread_delete(thread);
  
  do 
  {
    rt_device_write(wifi_device, 0, cmd, rt_strlen(cmd));   
    result = rt_event_recv(&rev_event, 
       REV_MASK, RT_EVENT_FLAG_OR | RT_EVENT_FLAG_CLEAR, 
       waittime*RT_TICK_PER_SECOND, &event);
    if (result == RT_EOK)
    {
      if (event & REV_DATA)
      {
        rt_memset(wifi_rx_buffer,0x00,sizeof(wifi_rx_buffer));
        rt_device_read(wifi_device, 0, wifi_rx_buffer, 512);
        rt_kprintf(wifi_rx_buffer);
        if((rt_strstr(wifi_rx_buffer,ack))||(rt_strstr(wifi_rx_buffer,"OK")))
          res = RT_TRUE;
        else
          res = RT_FALSE;
      }
    }
    retrytime--;
  }while((!res)&&(retrytime>=1));
  wifiwatch();
  return res;
} 


/*WIFI�˿ڳ�ʼ�������豸��ע��ص�����*/
rt_bool_t wificonfig(void)
{
  GPIO_InitTypeDef GPIO_InitStructure;
 
  RCC_APB2PeriphClockCmd(WIFI_PORT_RCC, ENABLE);
  
  GPIO_InitStructure.GPIO_Pin = WIFI_CS_PIN; 
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;
  GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_Out_PP;
  GPIO_Init(WIFI_PORT, &GPIO_InitStructure);
  //GPIO_SetBits(WIFI_PORT, WIFI_CS_PIN);
  
  wifi_device = rt_device_find("uart4");
  
  if (wifi_device != RT_NULL)    
  {
    rt_kprintf("\r\n Wifi port initialized!\r\n");
    /* ���ûص����������豸*/
    rt_device_set_rx_indicate(wifi_device, wifi_uart_input);
    rt_device_open(wifi_device, RT_DEVICE_OFLAG_RDWR);  
  }
  else
  {
    rt_kprintf("\r\n Wifi port not find !\r\n");
    return RT_FALSE;
  }
  /*WIFI���ڴ򿪺󣬳�ʼ�����ڽ����¼�*/
  rt_event_init(&rev_event, "rev_ev", RT_IPC_FLAG_FIFO);
  return RT_TRUE;
}

rt_bool_t wifiinit() //wifi����AP
{
  if(wifi_send_data_package(ESP8266_ATCMD,"OK",2,1))
  {
    rt_kprintf("\r\n Wifi AT OK !\r\n");
  }
  
  if(wifi_send_data_package(ESP8266_CWMODE_STA,"OK",2,1))
  {
    rt_kprintf("\r\n Wifi AT OK !\r\n");
  }

  rt_kprintf("\r\n Wifi Reset! Display information from module:\r\n");
  if(wifi_send_data_package(ESP8266_RESET,"ready",4,1))
  {
    rt_kprintf("\r\n Wifi Reset OK !\r\n");
  }
  rt_thread_delay(RT_TICK_PER_SECOND*5);
 
  rt_thread_delay(RT_TICK_PER_SECOND*5);
  
  //rt_kprintf("\r\n Wifi  ��ǰAP��\r\n");
  //if(wifi_send_data_package(ESP8266_CWLAP,"OK",1,1))//�г���ǰ�����
 // {
 //   rt_device_write(wifi_device, 0, ESP8266_CWLAP, rt_strlen(ESP8266_CWLAP)); 
 // }
 // rt_thread_delay(RT_TICK_PER_SECOND*10);
//  else
//    return RT_FALSE; 
  
  
//  if(wifi_send_data_package(ESP8266_CIPSTATUS,"OK",2,1))// ���TCP/UDP����״̬
//  {
//  }
//  rt_kprintf("\r\n Wifi ���TCP/UDP����״̬");
//  rt_thread_delay(RT_TICK_PER_SECOND*1);
//  else
//    return RT_FALSE; 
    return RT_TRUE; 
}
rt_bool_t wifijap() //����AP
{
  rt_kprintf("\r\n Wifi  ׼�� ����AP sundm75\r\n");

  if(wifi_send_data_package(ESP8266_CWJAP,"OK",2,1))
  {
    rt_kprintf("\r\n Wifi Join AP OK  !\r\n");
  }
  rt_thread_delay(RT_TICK_PER_SECOND*5);
  return RT_TRUE; 
}

rt_bool_t wificonnect() //��Զ����������
{
  rt_kprintf("\r\n Wifi Connect %s \r\n",ESP8266_CIPSTART);

  if(wifi_send_data_package(ESP8266_CIPSTART,"OK",2,1))
  {
    rt_kprintf("\r\n Wifi connect OK !\r\n");
  }

    rt_thread_delay(RT_TICK_PER_SECOND*5);
    return RT_TRUE; 
}

rt_bool_t wifisend(char * str) //�������ݷ���
{
  uint8_t lenstr[64]={0x00};
  rt_kprintf("\r\n Wifi send data! Display information from module:\r\n");
  sprintf((char*)lenstr,"%s%d\r\n",ESP8266_CIPSEND,strlen(str));
  if(wifi_send_data_package((char*)lenstr,"OK",2,1))
  {
    rt_kprintf("\r\n Wifi send %d data  OK !\r\n",strlen(str));
  }

  if(wifi_send_data_package(str,"OK",1,1))
  {
  }
   rt_thread_delay(RT_TICK_PER_SECOND*5);
  
    return RT_TRUE; 
}


rt_bool_t wificloseconnect()// �ر�Զ����������
{
  rt_kprintf("\r\n Wifi closeconnect! Display information from module:\r\n");

  if(wifi_send_data_package(ESP8266_CIPCLOSE,"OK",3,1))
  {
    rt_kprintf("\r\n Wifi closeconnect OK !\r\n");
  }
    rt_thread_delay(RT_TICK_PER_SECOND*5);

    return RT_TRUE; 
}


rt_bool_t wifiexit() // wifi�˳�AP
{
  rt_kprintf("\r\n Wifi wifi exit! Display information from module:\r\n");

  if(wifi_send_data_package(ESP8266_CWQAP,"OK",2,1))
  {
    rt_kprintf("\r\n Wifi exit OK !\r\n");
  }
  else
    return RT_FALSE; 
    return RT_TRUE; 
}



