/******************************2014-2015, NJTECH, Edu.************************** 
FileName: NRF24L01.c 
Author:  �ﶬ÷       Version :  1.0        Date: 2015.05.30
Description:    NRF24L01-2����   ʹ��SPI3   
ע����������NRF24L01.C��ȫһ�£���ͬһ�鿪�����ϣ����2��RFģ��ʹ��
Version:         1.0 
History:         
      <author>  <time>   <version >   <desc> 
      Sundm    15/05/30     1.0     �ļ�����   
  *          STM32Board Key Pin assignment
  *          =============================
 *          +----------------------------------------------------------+
 *          |     RF devices      SPIBUS��      CE       CSN     IRQ   |
 *          +------------------+---------+-----------------------------+
 *          |      RF3         |   SPI3  |    C9    |     C8   |  C6   |
 *          +------------------+---------+-----------------------------+
*******************************************************************************/ 
#include "stm32f10x.h"
#include "rfspi.h"
#include "rtthread.h"

//NRF24L01�Ĵ�����������
#define SPI_READ_REG    0x00  //�����üĴ���,��5λΪ�Ĵ�����ַ
#define SPI_WRITE_REG   0x20  //д���üĴ���,��5λΪ�Ĵ�����ַ
#define RD_RX_PLOAD     0x61  //��RX��Ч����,1~32�ֽ�
#define WR_TX_PLOAD     0xA0  //дTX��Ч����,1~32�ֽ�
#define FLUSH_TX        0xE1  //���TX FIFO�Ĵ���.����ģʽ����
#define FLUSH_RX        0xE2  //���RX FIFO�Ĵ���.����ģʽ����
#define REUSE_TX_PL     0xE3  //����ʹ����һ������,CEΪ��,���ݰ������Ϸ���.
#define NOP             0xFF  //�ղ���,����������״̬�Ĵ���	 
//SPI(NRF24L01)�Ĵ�����ַ
#define CONFIG          0x00  //���üĴ�����ַ;bit0:1����ģʽ,0����ģʽ;bit1:��ѡ��;bit2:CRCģʽ;bit3:CRCʹ��;
                              //bit4:�ж�MAX_RT(�ﵽ����ط������ж�)ʹ��;bit5:�ж�TX_DSʹ��;bit6:�ж�RX_DRʹ��
#define EN_AA           0x01  //ʹ���Զ�Ӧ����  bit0~5,��Ӧͨ��0~5
#define EN_RXADDR       0x02  //���յ�ַ����,bit0~5,��Ӧͨ��0~5
#define SETUP_AW        0x03  //���õ�ַ���(��������ͨ��):bit1,0:00,3�ֽ�;01,4�ֽ�;02,5�ֽ�;
#define SETUP_RETR      0x04  //�����Զ��ط�;bit3:0,�Զ��ط�������;bit7:4,�Զ��ط���ʱ 250*x+86us
#define RF_CH           0x05  //RFͨ��,bit6:0,����ͨ��Ƶ��;
#define RF_SETUP        0x06  //RF�Ĵ���;bit3:��������(0:1Mbps,1:2Mbps);bit2:1,���书��;bit0:�������Ŵ�������
#define STATUS          0x07  //״̬�Ĵ���;bit0:TX FIFO����־;bit3:1,��������ͨ����(���:6);bit4,�ﵽ�����ط�
                              //bit5:���ݷ�������ж�;bit6:���������ж�;
#define MAX_TX  	    0x10  //�ﵽ����ʹ����ж�
#define TX_OK       	0x20  //TX��������ж�
#define RX_OK   	    0x40  //���յ������ж�

#define OBSERVE_TX      0x08  //���ͼ��Ĵ���,bit7:4,���ݰ���ʧ������;bit3:0,�ط�������
#define CD              0x09  //�ز����Ĵ���,bit0,�ز����;
#define RX_ADDR_P0      0x0A  //����ͨ��0���յ�ַ,��󳤶�5���ֽ�,���ֽ���ǰ
#define RX_ADDR_P1      0x0B  //����ͨ��1���յ�ַ,��󳤶�5���ֽ�,���ֽ���ǰ
#define RX_ADDR_P2      0x0C  //����ͨ��2���յ�ַ,����ֽڿ�����,���ֽ�,����ͬRX_ADDR_P1[39:8]���;
#define RX_ADDR_P3      0x0D  //����ͨ��3���յ�ַ,����ֽڿ�����,���ֽ�,����ͬRX_ADDR_P1[39:8]���;
#define RX_ADDR_P4      0x0E  //����ͨ��4���յ�ַ,����ֽڿ�����,���ֽ�,����ͬRX_ADDR_P1[39:8]���;
#define RX_ADDR_P5      0x0F  //����ͨ��5���յ�ַ,����ֽڿ�����,���ֽ�,����ͬRX_ADDR_P1[39:8]���;
#define TX_ADDR         0x10  //���͵�ַ(���ֽ���ǰ),ShockBurstTMģʽ��,RX_ADDR_P0��˵�ַ���
#define RX_PW_P0        0x11  //��������ͨ��0��Ч���ݿ��(1~32�ֽ�),����Ϊ0��Ƿ�
#define RX_PW_P1        0x12  //��������ͨ��1��Ч���ݿ��(1~32�ֽ�),����Ϊ0��Ƿ�
#define RX_PW_P2        0x13  //��������ͨ��2��Ч���ݿ��(1~32�ֽ�),����Ϊ0��Ƿ�
#define RX_PW_P3        0x14  //��������ͨ��3��Ч���ݿ��(1~32�ֽ�),����Ϊ0��Ƿ�
#define RX_PW_P4        0x15  //��������ͨ��4��Ч���ݿ��(1~32�ֽ�),����Ϊ0��Ƿ�
#define RX_PW_P5        0x16  //��������ͨ��5��Ч���ݿ��(1~32�ֽ�),����Ϊ0��Ƿ�
#define FIFO_STATUS     0x17  //FIFO״̬�Ĵ���;bit0,RX FIFO�Ĵ����ձ�־;bit1,RX FIFO����־;bit2,3,����
                              //bit4,TX FIFO�ձ�־;bit5,TX FIFO����־;bit6,1,ѭ��������һ���ݰ�.0,��ѭ��;
/**********************************************************************************************************/
//NRF24L01���Ʋ���
#define NRF24L01_CE_PIN_2     GPIO_Pin_9
#define GPIO_NRF24L01_CE_2  GPIOC
#define RCC_NRF24L01_CE_2  RCC_APB2Periph_GPIOC

//NRF24L01 SPI�ӿ�CS�ź�
#define NRF24L01_CSN_PIN_2      GPIO_Pin_8
#define GPIO_NRF24L01_CSN_2  GPIOC
#define RCC_NRF24L01_CSN_2  RCC_APB2Periph_GPIOC

#define NRF24L01_IRQ_PIN_2      GPIO_Pin_6
#define GPIO_NRF24L01_IRQ_2  GPIOC
#define RCC_NRF24L01_IRQ_2  RCC_APB2Periph_GPIOC
//NRF2401Ƭѡ�ź�
#define Clr_NRF24L01_CE_2      {GPIO_ResetBits(GPIO_NRF24L01_CE_2,NRF24L01_CE_PIN_2);}
#define Set_NRF24L01_CE_2      {GPIO_SetBits(GPIO_NRF24L01_CE_2,NRF24L01_CE_PIN_2);}

//SPIƬѡ�ź�	
#define Clr_NRF24L01_CSN_2     {GPIO_ResetBits(GPIO_NRF24L01_CSN_2,NRF24L01_CSN_PIN_2);}
#define Set_NRF24L01_CSN_2     {GPIO_SetBits(GPIO_NRF24L01_CSN_2,NRF24L01_CSN_PIN_2);}
    
//NRF2401_IRQ��������
#define Clr_NRF24L01_IRQ_2     {GPIO_ResetBits(GPIO_NRF24L01_IRQ_2, NRF24L01_IRQ_PIN_2);} 
#define Set_NRF24L01_IRQ_2    {GPIO_SetBits(GPIO_NRF24L01_IRQ_2, NRF24L01_IRQ_PIN_2);}

#define READ_NRF24L01_IRQ_2      (GPIO_ReadInputDataBit(GPIO_NRF24L01_IRQ_2,NRF24L01_IRQ_PIN_2))

//NRF24L01���ͽ������ݿ�ȶ���
#define TX_ADR_WIDTH    5                               //5�ֽڵĵ�ַ���
#define RX_ADR_WIDTH    5                               //5�ֽڵĵ�ַ���
#define TX_PLOAD_WIDTH  32                              //20�ֽڵ��û����ݿ��
#define RX_PLOAD_WIDTH  32                              //20�ֽڵ��û����ݿ��

const rt_uint8_t static  TX_ADDRESS[TX_ADR_WIDTH]={0x73,0x75,0x6E,0x64,0x6D}; //���͵�ַ
const rt_uint8_t static  RX_ADDRESS[RX_ADR_WIDTH]={0x73,0x75,0x6E,0x64,0x6D}; //���͵�ַ	

/*******************************************************************************
* Function Name  : NRF24L01_Write_Reg
* Description    : ͨ��SPIд�Ĵ���
* Input          : regaddr:Ҫд�ļĴ��� data:Ҫд������
* Output         : None
* Return         : status ,״ֵ̬
*******************************************************************************/
static rt_uint8_t NRF24L01_Write_Reg(rt_uint8_t regaddr,rt_uint8_t data)
{
  rt_uint8_t status;	
  Clr_NRF24L01_CSN_2;                    //ʹ��SPI����
  status =SPI3_ReadWriteByte(regaddr); //���ͼĴ����� 
  SPI3_ReadWriteByte(data);            //д��Ĵ�����ֵ
  Set_NRF24L01_CSN_2;                    //��ֹSPI����	   
  return(status);       		//����״ֵ̬
}

/*******************************************************************************
* Function Name  : NRF24L01_Read_Reg
* Description    : ͨ��SPI���Ĵ���
* Input          : regaddr:Ҫ���ļĴ���
* Output         : None
* Return         : reg_val ,����������
*******************************************************************************/
static rt_uint8_t NRF24L01_Read_Reg(rt_uint8_t regaddr)
{
  rt_uint8_t reg_val;	    
  Clr_NRF24L01_CSN_2;                //ʹ��SPI����		
  SPI3_ReadWriteByte(regaddr);     //���ͼĴ�����
  reg_val=SPI3_ReadWriteByte(0XFF);//��ȡ�Ĵ�������
  Set_NRF24L01_CSN_2;                //��ֹSPI����		    
  return(reg_val);                 //����״ֵ̬
}

/*******************************************************************************
* Function Name  : NRF24L01_Read_Buf
* Description    : ��ָ��λ�ö���ָ�����ȵ�����
* Input          : regaddr:Ҫ���ļĴ���  datalen:ָ������
* Output         : *pBuf:��������ָ��
* Return         : status ,������״ֵ̬
*******************************************************************************/
static rt_uint8_t NRF24L01_Read_Buf(rt_uint8_t regaddr,rt_uint8_t *pBuf,rt_uint8_t datalen)
{
  rt_uint8_t status,rt_uint8_t_ctr;	       
  Clr_NRF24L01_CSN_2;                     //ʹ��SPI����
  status=SPI3_ReadWriteByte(regaddr);   //���ͼĴ���ֵ(λ��),����ȡ״ֵ̬   	   
  for(rt_uint8_t_ctr=0;rt_uint8_t_ctr<datalen;rt_uint8_t_ctr++)pBuf[rt_uint8_t_ctr]=SPI3_ReadWriteByte(0XFF);//��������
  Set_NRF24L01_CSN_2;                     //�ر�SPI����
  return status;                        //���ض�����״ֵ̬
}

/*******************************************************************************
* Function Name  : NRF24L01_Write_Buf
* Description    : ��ָ��λ��дָ�����ȵ�����
* Input          : regaddr:Ҫд�ļĴ���  datalen:ָ������
* Output         : *pBuf:Ҫд�������ָ��
* Return         : status ,������״ֵ̬
*******************************************************************************/
static rt_uint8_t NRF24L01_Write_Buf(rt_uint8_t regaddr, rt_uint8_t *pBuf, rt_uint8_t datalen)
{
  rt_uint8_t status,rt_uint8_t_ctr;	    
  Clr_NRF24L01_CSN_2;                                    //ʹ��SPI����
  status = SPI3_ReadWriteByte(regaddr);                //���ͼĴ���ֵ(λ��),����ȡ״ֵ̬
  for(rt_uint8_t_ctr=0; rt_uint8_t_ctr<datalen; rt_uint8_t_ctr++)SPI3_ReadWriteByte(*pBuf++); //д������	 
  Set_NRF24L01_CSN_2;                                    //�ر�SPI����
  return status;                                       //���ض�����״ֵ̬
}				   

/*******************************************************************************
* Function Name  : NRF24L01_TxPacket
* Description    : ����NRF24L01����һ������
* Input          : txbuf:�����������׵�ַ
* Output         : None
* Return         : �������״��
*******************************************************************************/
static rt_err_t NRF24L01_TxPacket(rt_uint8_t *txbuf)
{
  rt_uint8_t state;   
  Clr_NRF24L01_CE_2;
  NRF24L01_Write_Buf(WR_TX_PLOAD,txbuf,TX_PLOAD_WIDTH);//д���ݵ�TX BUF  32���ֽ�
  Set_NRF24L01_CE_2;                                     //��������	   
  while(READ_NRF24L01_IRQ_2!=0);                              //�ȴ��������
  state=NRF24L01_Read_Reg(STATUS);                     //��ȡ״̬�Ĵ�����ֵ	   
  NRF24L01_Write_Reg(SPI_WRITE_REG+STATUS,state);      //���TX_DS��MAX_RT�жϱ�־
  if(state&MAX_TX)                                     //�ﵽ����ط�����
  {
          NRF24L01_Write_Reg(FLUSH_TX,0xff);               //���TX FIFO�Ĵ��� 
          return RT_ETIMEOUT; 
  }
  if(state&TX_OK)                                      //�������
  {
          return RT_EOK;
  }
  return RT_ERROR;                                         //����ԭ����ʧ��
}

/*******************************************************************************
* Function Name  : NRF24L01_RxPacket
* Description    : ����NRF24L01����һ������
* Input          : rxbuf:���������׵�ַ
* Output         : None
* Return         :  �������״��
*******************************************************************************/
static rt_err_t NRF24L01_RxPacket(rt_uint8_t *rxbuf)
{
  rt_uint8_t state;		    							      
  state=NRF24L01_Read_Reg(STATUS);                //��ȡ״̬�Ĵ�����ֵ    	 
  NRF24L01_Write_Reg(SPI_WRITE_REG+STATUS,state); //���TX_DS��MAX_RT�жϱ�־
  if(state&RX_OK)                                 //���յ�����
  {
          NRF24L01_Read_Buf(RD_RX_PLOAD,rxbuf,RX_PLOAD_WIDTH);//��ȡ����
          NRF24L01_Write_Reg(FLUSH_RX,0xff);          //���RX FIFO�Ĵ��� 
          return RT_EOK; 
  }	   
  return RT_ERROR;                                      //û�յ��κ�����
}

/*******************************************************************************
* Function Name  : RX_Mode
* Description    : �ú�����ʼ��NRF24L01��RXģʽ
*  ����RX��ַ,дRX���ݿ��,ѡ��RFƵ��,�����ʺ�LNA HCURR
*  ��CE��ߺ�,������RXģʽ,�����Խ�������
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
static void RX_Mode(void)
{
	Clr_NRF24L01_CE_2;	  
  	NRF24L01_Write_Buf(SPI_WRITE_REG+RX_ADDR_P0,(rt_uint8_t*)RX_ADDRESS,RX_ADR_WIDTH);//дRX�ڵ��ַ
	  
  	NRF24L01_Write_Reg(SPI_WRITE_REG+EN_AA,0x01);    //ʹ��ͨ��0���Զ�Ӧ��    
  	NRF24L01_Write_Reg(SPI_WRITE_REG+EN_RXADDR,0x01);//ʹ��ͨ��0�Ľ��յ�ַ  	 
  	NRF24L01_Write_Reg(SPI_WRITE_REG+RF_CH,40);	     //����RFͨ��Ƶ��		  
  	NRF24L01_Write_Reg(SPI_WRITE_REG+RX_PW_P0,RX_PLOAD_WIDTH);//ѡ��ͨ��0����Ч���ݿ�� 	    
  	NRF24L01_Write_Reg(SPI_WRITE_REG+RF_SETUP,0x0f); //����TX�������,0db����,2Mbps,���������濪��   
  	NRF24L01_Write_Reg(SPI_WRITE_REG+CONFIG, 0x0f);  //���û�������ģʽ�Ĳ���;PWR_UP,EN_CRC,16BIT_CRC,����ģʽ 
  	Set_NRF24L01_CE_2;                                //CEΪ��,�������ģʽ 
}						 

/*******************************************************************************
* Function Name  : TX_Mode
* Description    : �ú�����ʼ��NRF24L01��TXģʽ
*  ����TX��ַ,дTX���ݿ��,����RX�Զ�Ӧ��ĵ�ַ,���TX��������,ѡ��RFƵ��,
*  �����ʺ�LNA HCURR PWR_UP,CRCʹ��,��CE��ߺ�,������TXģʽ,��������
*  CEΪ�ߴ���10us,����������.
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
static void TX_Mode(void)
{														 
  Clr_NRF24L01_CE_2;	    
  NRF24L01_Write_Buf(SPI_WRITE_REG+TX_ADDR,(rt_uint8_t*)TX_ADDRESS,TX_ADR_WIDTH);    //дTX�ڵ��ַ 
  NRF24L01_Write_Buf(SPI_WRITE_REG+RX_ADDR_P0,(rt_uint8_t*)RX_ADDRESS,RX_ADR_WIDTH); //����TX�ڵ��ַ,��ҪΪ��ʹ��ACK	  

  NRF24L01_Write_Reg(SPI_WRITE_REG+EN_AA,0x01);     //ʹ��ͨ��0���Զ�Ӧ��    
  NRF24L01_Write_Reg(SPI_WRITE_REG+EN_RXADDR,0x01); //ʹ��ͨ��0�Ľ��յ�ַ  
  NRF24L01_Write_Reg(SPI_WRITE_REG+SETUP_RETR,0x1a);//�����Զ��ط����ʱ��:500us + 86us;����Զ��ط�����:10��
  NRF24L01_Write_Reg(SPI_WRITE_REG+RF_CH,40);       //����RFͨ��Ϊ40
  NRF24L01_Write_Reg(SPI_WRITE_REG+RF_SETUP,0x0f);  //����TX�������,0db����,2Mbps,���������濪��   
  NRF24L01_Write_Reg(SPI_WRITE_REG+CONFIG,0x0e);    //���û�������ģʽ�Ĳ���;PWR_UP,EN_CRC,16BIT_CRC,����ģʽ,���������ж�
  Set_NRF24L01_CE_2;                                  //CEΪ��,10us����������
}		  


/*******************************************************************************
* Function Name  : NRF24L01_Init
* Description    : ��ʼ��24L01��IO��
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
static void NRF24L01_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStructure;
  /*config CE CSN*/
  RCC_APB2PeriphClockCmd(RCC_NRF24L01_CE_2, ENABLE);           //ʹ��GPIO��ʱ��
  GPIO_InitStructure.GPIO_Pin = NRF24L01_CE_PIN_2;              //NRF24L01 ģ��Ƭѡ�ź�
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;           //�������
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_Init(GPIO_NRF24L01_CE_2, &GPIO_InitStructure);

  RCC_APB2PeriphClockCmd(RCC_NRF24L01_CSN_2, ENABLE);          //ʹ��GPIO��ʱ��
  GPIO_InitStructure.GPIO_Pin = NRF24L01_CSN_PIN_2;      
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;           //�������
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_Init(GPIO_NRF24L01_CSN_2, &GPIO_InitStructure);

  Set_NRF24L01_CE_2;                                           //��ʼ��ʱ������
  Set_NRF24L01_CSN_2;                                   //��ʼ��ʱ������

  /*config irq*/
  GPIO_InitStructure.GPIO_Pin = NRF24L01_IRQ_PIN_2;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU  ;     //��������
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_Init(GPIO_NRF24L01_IRQ_2, &GPIO_InitStructure);
  GPIO_SetBits(GPIO_NRF24L01_IRQ_2,NRF24L01_IRQ_PIN_2);

  SPI3_Init();                                       //��ʼ��SPI
  Clr_NRF24L01_CE_2; 	                               //ʹ��24L01
  Set_NRF24L01_CSN_2;                                  //SPIƬѡȡ��
}

/*******************************************************************************
* Function Name  : NRF24L01_Check
* Description    : �ϵ���NRF24L01�Ƿ���λ
* Input          : None
* Output         : None
* Return         : RT_EOK:��ʾ��λ;RT_ERROR����ʾ����λ
*******************************************************************************/
static rt_err_t NRF24L01_Check(void)
{
  rt_uint8_t buf[5]={0XA5,0XA5,0XA5,0XA5,0XA5};
  rt_uint8_t buf1[5];
  rt_uint8_t i;   

  NRF24L01_Write_Buf(SPI_WRITE_REG+TX_ADDR,buf,5);//д��5���ֽڵĵ�ַ.	
  NRF24L01_Read_Buf(TX_ADDR,buf1,5);              //����д��ĵ�ַ  	
  for(i=0;i<5;i++)if(buf1[i]!=0XA5)break;					   
  if(i!=5)return RT_ERROR;                               //NRF24L01����λ	
  return RT_EOK;		                                //NRF24L01��λ
}	 	 

/******************************����finsh�в��Ժ���*****************************/
static void rt_rf_thread_entry(void* parameter)
{
  rt_uint8_t buf[TX_ADR_WIDTH] = {0x00};

  NRF24L01_Init();  
  if(NRF24L01_Check()==RT_EOK)
  {
    rt_kprintf("\r\n RF2 ģ���ʼ���ɹ���\r\n");
  }
  else  
  {
    rt_kprintf("\r\n RF2 ģ�鲻���ڣ�\r\n");
    return;
  }
  RX_Mode();
  while (1)
  {  
    if(NRF24L01_RxPacket(buf) == RT_EOK)
    {
       rt_kprintf("\r\n RF2 ���յ����ݣ�\r\n"); 
        {
          rt_kprintf((char const*)buf);
        }
    }
    
    rt_thread_delay(RT_TICK_PER_SECOND/100);        
  }
}

int rf2start(void)
{ 
    rt_thread_t tid;

    tid = rt_thread_create("rf2",
        rt_rf_thread_entry, RT_NULL,
        2048, RT_THREAD_PRIORITY_MAX-10, 10);
    if (tid != RT_NULL) rt_thread_startup(tid);

    return 0;
}

void rf2send(rt_uint8_t * str)
{ 
      rt_kprintf("\r\n \r\n"); 
      TX_Mode();
      rt_kprintf("\r\n RF2 �������ݣ�%s \r\n", str); 
      NRF24L01_TxPacket(str);
      RX_Mode();
}

void rf2read(void)
{ 
  rt_uint8_t data;
  rt_uint8_t buf[32];

  NRF24L01_Read_Buf( CONFIG      , &data , 1);       
  rt_kprintf("\r\n CONFIG = 0x%02x", data);
  NRF24L01_Read_Buf( EN_AA       , &data , 1);       
  rt_kprintf("\r\n EN_AA = 0x%02x", data);
  NRF24L01_Read_Buf( EN_RXADDR   , &data , 1);      
  rt_kprintf("\r\n EN_RXADDR = 0x%02x", data);
  NRF24L01_Read_Buf( SETUP_AW    , &data , 1);       
  rt_kprintf("\r\n SETUP_AW = 0x%02x", data);
  NRF24L01_Read_Buf( SETUP_RETR  , &data , 1);      
  rt_kprintf("\r\n SETUP_RETR = 0x%02x",data);
  NRF24L01_Read_Buf( RF_CH       , &data , 1);       
  rt_kprintf("\r\n RF_CH = 0x%02x",data);
  NRF24L01_Read_Buf( RF_SETUP    , &data , 1);      
  rt_kprintf("\r\n RF_SETUP = 0x%02x",data);
  NRF24L01_Read_Buf( STATUS      , &data , 1);       
  rt_kprintf("\r\n STATUS = 0x%02x",data);
  NRF24L01_Read_Buf( OBSERVE_TX  , &data , 1);       
  rt_kprintf("\r\n OBSERVE_TX = 0x%02x",data);
  NRF24L01_Read_Buf( CD          , &data , 1);      
  rt_kprintf("\r\n CD = 0x%02x",data);
  NRF24L01_Read_Buf( RX_ADDR_P0  , buf , 5);       
  rt_kprintf("\r\n RX_ADDR_P0 = 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x", buf[0],buf[1],buf[2],buf[3],buf[4]);
  NRF24L01_Read_Buf( RX_ADDR_P1  , buf , 5);      
  rt_kprintf("\r\n RX_ADDR_P1 = 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x", buf[0],buf[1],buf[2],buf[3],buf[4]);
  NRF24L01_Read_Buf( RX_ADDR_P2  , &data , 1);       
  rt_kprintf("\r\n RX_ADDR_P2 = 0x%02x",data);
  NRF24L01_Read_Buf( RX_ADDR_P3  , &data , 1);      
  rt_kprintf("\r\n RX_ADDR_P3 = 0x%02x",data);
  NRF24L01_Read_Buf( RX_ADDR_P4  , &data , 1);       
  rt_kprintf("\r\n RX_ADDR_P4 = 0x%02x",data);
  NRF24L01_Read_Buf( RX_ADDR_P5  , &data , 1);      
  rt_kprintf("\r\n RX_ADDR_P5 = 0x%02x",data);
  NRF24L01_Read_Buf( TX_ADDR     , buf , 5);       
  rt_kprintf("\r\n TX_ADDR = 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x", buf[0],buf[1],buf[2],buf[3],buf[4]);
  NRF24L01_Read_Buf( RX_PW_P0    , &data , 1);      
  rt_kprintf("\r\n RX_PW_P0 = 0x%02x",data);
  NRF24L01_Read_Buf( RX_PW_P1    , &data , 1);       
  rt_kprintf("\r\n RX_PW_P1 = 0x%02x",data);
  NRF24L01_Read_Buf( RX_PW_P2    , &data , 1);      
  rt_kprintf("\r\n RX_PW_P2 = 0x%02x",data);
  NRF24L01_Read_Buf( RX_PW_P3    , &data , 1);       
  rt_kprintf("\r\n RX_PW_P3 = 0x%02x",data);
  NRF24L01_Read_Buf( RX_PW_P4    , &data , 1);      
  rt_kprintf("\r\n RX_PW_P4 = 0x%02x",data);
  NRF24L01_Read_Buf( RX_PW_P5    , &data , 1);       
  rt_kprintf("\r\n RX_PW_P5 = 0x%02x",data);
  NRF24L01_Read_Buf( FIFO_STATUS , &data , 1);      
  rt_kprintf("\r\n FIFO_STATUS = 0x%02x",data);

}

#include "rtthread.h"
#ifdef RT_USING_FINSH
#include <finsh.h>
FINSH_FUNCTION_EXPORT(rf2start, startup rf2start);
FINSH_FUNCTION_EXPORT(rf2read, startup rf2read);
FINSH_FUNCTION_EXPORT(rf2send, startup rf2send);

#endif
