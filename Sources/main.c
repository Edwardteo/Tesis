/* ###################################################################
**     Filename    : main.c
**     Project     : Wiznet5500_KL25Z
**     Processor   : MKL25Z128VLK4
**     Version     : Driver 01.01
**     Compiler    : GNU C Compiler
**     Date/Time   : 2014-07-14, 09:09, # CodeGen: 0
**     Abstract    :
**         Main module.
**         This module contains user's application code.
**     Settings    :
**     Contents    :
**         No public methods
**
** ###################################################################*/
/*!
** @file main.c
** @version 01.01
** @brief
**         Main module.
**         This module contains user's application code.
*/         
/*!
**  @addtogroup main_module main module documentation
**  @{
*/         
/* MODULE main */


/* Including needed modules to compile this module/procedure */
#include "Cpu.h"
#include "Events.h"
#include "SM1.h"
#include "CS.h"
#include "CsIO1.h"
#include "IO1.h"
#include "RST.h"
#include "WAIT1.h"
/* Including shared modules, which are used for whole project */
#include "PE_Types.h"
#include "PE_Error.h"
#include "PE_Const.h"
#include "IO_Map.h"

/* User includes (#include below this line is not maintained by Processor Expert) */
#include <stdlib.h>
#include <string.h>

#define MR        0x0000
#define GAR       0x0001
#define SUBR      0x0005
#define SHAR      0x0009
#define SIPR      0x000f
#define PHYSTATUS 0x0035

// W5500 socket register
#define Sn_MR         0x0000
#define Sn_CR         0x0001
#define Sn_IR         0x0002
#define Sn_SR         0x0003
#define Sn_PORT       0x0004
#define Sn_DHAR       0x0006
#define Sn_DIPR       0x000c
#define Sn_DPORT      0x0010
#define Sn_RXBUF_SIZE 0x001e
#define Sn_TXBUF_SIZE 0x001f
#define Sn_TX_FSR     0x0020
#define Sn_TX_WR      0x0024
#define Sn_TX_RD      0x0022
#define Sn_RX_RSR     0x0026
#define Sn_RX_RD      0x0028

//w5500 status socket
#define SOCK_CLOSED      0x00
#define SOCK_INIT        0x13
#define SOCK_LISTEN      0x14
#define SOCK_SYNSENT     0x15
#define SOCK_ESTABLISHED 0x17
#define SOCK_CLOSE_WAIT  0x1c
#define SOCK_UDP         0x22

//w5500 command socket 
#define    OPEN      0x01
#define    LISTEN    0x02
#define    CONNECT   0x04
#define    DISCON    0x08
#define	   CLOSE     0x10
#define    SEND      0x20
#define    SEND_MAC  0x21
#define    SEND_KEEP 0x22
#define    RECV      0x40

//HTML
uint8 a[]="<!DOCTYPE html>";
uint8 b[]="<html lang=\"en\" xmlns=\"http://www.w3.org/1999/xhtml\"><body>";
uint8 c[]="<div style='width: 960px;margin: 0 auto;background-color:#DDD;border-radius: 20px;text-align: center;text-shadow:5px 5px 5px gray; font-family:Calibri;color:#4898E2;'></br>";
uint8 d[]="<h1 >Test de Wiznet w5x00 Contador Simple</h1></br>";
uint8 e[]="<h2>Da click en el boton para iniciar el contador</p><button id='boton' onclick=\"contador()\">Iniciar</button>";
uint8 f[]="</br><p id='cont'></p></br></div>";
uint8 g[]="<script>var x=0; function contador(){var btn=document.getElementById('boton'); btn.style.visibility = 'hidden'; btn.style.visibility = 'hidden'; setInterval(function(){document.getElementById('cont').innerHTML = x++;}, 1000);} </script>";
uint8 h[]="</body></html> ";

int i=0;
uint8_t OutData[] = {0x88,0x09,0x00,0x01,0x00,0x01,0x00,0x01,0x00};
uint8_t InpData[10];
uint8_t Mac[6];
uint8 Ip[4];
uint8 Mask[4];
uint8 Gatway[4];
uint8 sck;

uint8 * Data;

volatile bool DataReceivedFlag = FALSE;

char mac_string[20];
char ip_string[20];
char msk_string[20];
char gwy_string[20];
const char * IP_Addr    = "169.254.175.210";//
const char * IP_Subnet  = "255.255.255.0";
const char * IP_Gateway = "169.254.175.209";//"192.168.0.1";

LDD_TError Error;
LDD_TDeviceData *MySPIPtr;

uint8 datosr[500];
/*Reset*/
void reset(void)
{
	RST_ClrVal(RST_DeviceData);
	WAIT1_Waitus(500);
	RST_SetVal(RST_DeviceData);
	WAIT1_Waitms(400);	
}

/*Escribe un byte por SPI*/
void SPI_WriteByte(unsigned char write) {
  unsigned char dummy;		/*variable auxiliar*/

  DataReceivedFlag = FALSE;									/*Bandera a la espera de interrupción*/
  (void)SM1_ReceiveBlock(MySPIPtr, &dummy, sizeof(dummy)); 	/*Solicitud a espera de una lectura via SPI*/
  (void)SM1_SendBlock(MySPIPtr, &write, sizeof(write));		/*Envia el valor en write*/
  while(!DataReceivedFlag){}								/*Espera a recibir la respuesta basura*/
}

/*Lee un byte por SPI*/
uint8_t SPI_ReadByte(void) {
  uint8_t val, write = 0xff; /* dummy */
  
  DataReceivedFlag = FALSE;		/*Bandera a la espera de interrupción*/
  (void)SM1_ReceiveBlock(MySPIPtr, &val, 1);	/*Solicitud a espera de una lectura via SPI*/
  (void)SM1_SendBlock(MySPIPtr, &write, 1);		/*Envia el valor dummy en write*/
  while(!DataReceivedFlag){}		/*Espera a recibir la respuesta */
  return val;
}

//WIZ_Write(direccion offset,Direccion de bloque,Dato a escrbir)
void WIZ_WriteByte(uint16_t addr, uint8_t cb, uint8_t data)
{
	uint8 cp=0x04;
	cb = cp | cb<<3 ;
	CS_ClrVal(CS_DeviceData);
	SPI_WriteByte(addr >> 8); /* high address */
	SPI_WriteByte(addr & 0xff); /* low address */
	SPI_WriteByte(cb);			/* control phase */
    SPI_WriteByte(data); /* data */
    CS_SetVal(CS_DeviceData);	
}

//WIZ_ReadByte(direccion offset,Direccion de bloque)
uint8_t WIZ_ReadByte(uint16_t addr, uint8_t cb)
{
	uint8 Data;
	cb  <<= 3 ;
	CS_ClrVal(CS_DeviceData);
	SPI_WriteByte(addr >> 8); /* high address */
	SPI_WriteByte(addr & 0xff); /* low address */
	SPI_WriteByte(cb);			/* control phase */
    Data=SPI_ReadByte(); /* data */
    CS_SetVal(CS_DeviceData);
    return Data;
}


/*Convierte una cadena de direccion IP en un vector de valores hexadecimales para su envio*/
void str_to_ip(const char* str, uint8 *buf)
{
    uint32_t ip = 0;
    char* p = (char*)str;
    for( i = 0; i < 4; i++) {
        ip |= atoi(p);
        buf[i]=ip;
        p = strchr(p, '.');
        ip <<= 8;
        p++;
    }
}

//WIZ_Read(direccion offset,Direccion de bloque,buffer de entrada,longitud de datos(bytes) )
void WIZ_Read(uint16_t addr, uint8_t cb, uint8_t *buf, uint16_t len)
{
	cb <<=3 ;
	CS_ClrVal(CS_DeviceData);
	SPI_WriteByte(addr >> 8); /* high address */
	SPI_WriteByte(addr & 0xff); /* low address */
	SPI_WriteByte(cb);			/* control phase */

	for( i = 0; i < len; i++) {
		  buf[i] = SPI_ReadByte(); /* data */
	}
    CS_SetVal(CS_DeviceData);
}

//WIZ_Write(direccion offset,Direccion de bloque,buffer de salida,longitud de datos(bytes))
void WIZ_Write(uint16_t addr, uint8_t cb, uint8_t *buf, uint16_t len)
{
	uint8 cp=0x04;
	cb = cp | cb<<3 ;
	CS_ClrVal(CS_DeviceData);
	SPI_WriteByte(addr >> 8); /* high address */
	SPI_WriteByte(addr & 0xff); /* low address */
	SPI_WriteByte(cb);			/* control phase */
	for( i = 0; i < len; i++) {
		  SPI_WriteByte(buf[i]); /* data */
	}
    CS_SetVal(CS_DeviceData);	
}

void WIZ_Command(uint8 sock ,uint8 command)
{
	WIZ_WriteByte(Sn_CR,sock,command);
}

/*Configura y comprueba las*/
uint8 config_net(void) 
{
	bool ipok=1, gok=1, mok=1;
	uint8 ok=0; 
	
	WIZ_Read(SHAR, 0x00, Mac, 6);
	snprintf(mac_string, sizeof(mac_string), "%02X:%02X:%02X:%02X:%02X:%02X", Mac[0], Mac[1], Mac[2], Mac[3], Mac[4], Mac[5]);  
	
	WIZ_Write(SIPR, 0x00, Ip, 4);
	WIZ_Read(SIPR, 0x00, InpData, 4);
	snprintf(ip_string, sizeof(ip_string), "%d.%d.%d.%d", InpData[0], InpData[1], InpData[2], InpData[3] );  
	for( i = 0; i < 4; i++)
	{
		if (ipok)
		{
			if (Ip[i]==InpData[i]) ipok=1;
			else ipok=0;
		} 		
	}
	  
	WIZ_Write(GAR, 0x00, Gatway, 4);
	WIZ_Read(GAR, 0x00, InpData, 4);
	snprintf(gwy_string, sizeof(gwy_string), "%d.%d.%d.%d", InpData[0], InpData[1], InpData[2], InpData[3] ); 
	for( i = 0; i < 4; i++)
	{
		if (gok)
		{
			if (Gatway[i]==InpData[i]) gok=1;
			else gok=0;
		} 		
	}
	  
	WIZ_Write(SUBR, 0x00, Mask, 4);
	WIZ_Read(SUBR, 0x00, InpData, 4);
	snprintf(msk_string, sizeof(msk_string), "%d.%d.%d.%d", InpData[0], InpData[1], InpData[2], InpData[3] );
	for( i = 0; i < 4; i++)
	{
		if (mok)
		{
			if (Mask[i]==InpData[i]) mok=1;
			else mok=0;
		} 		
	}
	  
	if (ipok && mok && gok) ok=1;
	else ok=0;
	
	return ok;
}

void init(const char* ip, const char* mask, const char* gateway)
{
	uint8 fok=0;
	printf("\n\n%s \n\r", "Iniciando servidor...");                                
	WAIT1_Waitms(1000);
    //dhcp = false;
    str_to_ip(ip,Ip);
    //ip_set = true;
    str_to_ip(mask, Mask);
    str_to_ip(gateway,Gatway);
    reset();
    
    fok=config_net();
    
    if (fok)
    {
    	printf("%s \r", "  Iniciando W5500");
    	printf("%s \n\r", "  Configuracion terminada... ");
    	printf("  MAC  =  %s\n",mac_string);
    	printf("  IP   =  %s\n",ip_string);
    	printf("  Mask =  %s\n",msk_string);
    	printf("  Gateway =%s\n",gwy_string);    	
    }
    else 
    {
    	printf("%s \n\r", "Fallo de comunicacion (W5500)");
    }
}

/*Escanea los sockets para encontrar uno libre*/
void WIZ_SckS()
{
	uint8 i;
	for(i=0;i<8;i++)
	{
		if(i>0)WIZ_Read(Sn_SR,0x01+(i*4),InpData,1);
		else  WIZ_Read(Sn_SR,0x01,InpData,1);
		if(InpData[0]==SOCK_CLOSED)
		{
			sck= ((i*4)+1);
			break;
		}
		else sck = -1;
	}
}

void FindSck()
{
	uint8 i;
	for(i=0;i<8;i++)
	{
		if(WIZ_ReadByte(Sn_SR,0x01+(i*4)) == SOCK_CLOSED)
		{
			sck = ((i*4)+1);
			break;
		}		
		else sck = -1;
	}
	if (sck < 0)
	{
		printf("%s \n\r","  No hay sockets disponibles");		
	}
}


/*Inicializa el socket para la comunicación TCP*/
void Bind()
{	
	WIZ_SckS();
	if (sck<0);
	else
	{
		WIZ_Read(Sn_SR,sck,InpData,1);
		while(InpData[0] != SOCK_LISTEN )
		{
			while (InpData[0] != SOCK_INIT )
			{
				WIZ_WriteByte(Sn_CR,sck,CLOSE);	//Cerrando el socket
				WIZ_WriteByte(Sn_MR,sck,0x01);	//modo TCP
				
				WIZ_WriteByte(Sn_PORT,sck,0x00);	//Puerto TCP 80
				WIZ_WriteByte(0x0005,sck,80);
				
				WIZ_WriteByte(Sn_CR,sck,OPEN);	//Abre Socket
				WIZ_Read(Sn_SR,sck,InpData,1);
			}
		
			WIZ_WriteByte(Sn_CR,sck,LISTEN);	//Comando Listen
			WIZ_Read(Sn_SR,sck,InpData,1);		//Verificar estatus hasta  listen
		}
		printf("\n%s \n\r","  Puerto abierto, en espera...");
	}
}

/*Inicializa el socket para la comunicación TCP*/
void OpenTcpSck()
{	
	FindSck();	//Encuentra un socket disponible
	if (sck<0);	//Si no existe socket
	else
	{
		WIZ_Read(Sn_SR,sck,InpData,1);
		while(InpData[0] != SOCK_LISTEN )
		{
			while (InpData[0] != SOCK_INIT )
			{
				WIZ_WriteByte(Sn_CR,sck,CLOSE);	//Cerrando el socket
				WIZ_WriteByte(Sn_MR,sck,0x01);	//modo TCP
				
				WIZ_WriteByte(Sn_PORT,sck,0x00);	//Puerto TCP 80
				WIZ_WriteByte(0x0005,sck,80);
				
				WIZ_WriteByte(Sn_CR,sck,OPEN);	//Abre Socket
				WIZ_Read(Sn_SR,sck,InpData,1);
			}
		
			WIZ_WriteByte(Sn_CR,sck,LISTEN);	//Comando Listen
			WIZ_Read(Sn_SR,sck,InpData,1);		//Verificar estatus hasta  listen
		}
		printf("\n%s \n\r","  Puerto abierto, en espera...");
	}
}

uint16 rxSize()
{
	return(((WIZ_ReadByte(Sn_RX_RSR,sck)&0x00FF)<<8)+WIZ_ReadByte(Sn_RX_RSR+1,sck));
}

uint8 getSnRxBlock()
{
	return (sck+2);
}

uint8 getSnTxBlock()
{
	return (sck+1);
}

uint16 getSnRxAddress()
{
	return (  (WIZ_ReadByte(Sn_RX_RD,sck)<<8)  +  WIZ_ReadByte(Sn_RX_RD+1,sck)  );
}

uint16 getFreesize (void)
{
	return (  ((WIZ_ReadByte(Sn_TX_FSR,sck)&0x00FF)<<8)  +  WIZ_ReadByte(Sn_TX_FSR+1,sck)  );
} 

uint16 getSnTxRdAddress()
{
	return (  (WIZ_ReadByte(Sn_TX_RD,sck)<<8)  +  WIZ_ReadByte(Sn_TX_RD+1,sck)  );
}

uint16 getSnTxWrAddress()
{
	return (  (WIZ_ReadByte(Sn_TX_WR,sck)<<8)  +  WIZ_ReadByte(Sn_TX_WR+1,sck)  );
}

/*envia dato*/
void WIZ_Send(uint8 * buf,uint8 length)
{
	uint8 CtrlByte;
	uint16 freesize,dst_ptr,upd;
	freesize=getFreesize();  /* first, get the free TX memory size */
	
	while(freesize < length) freesize = getFreesize();
	
	CtrlByte=getSnTxBlock(); /* select TX memory*/
	
	dst_ptr = getSnTxRdAddress(); /* Get offset address */
	
	/*printf("  bloque:   0x%02x \n\r",CtrlByte);
	printf("  Sn_TX_RD: 0x%04x \n\r",dst_ptr);
	printf("  Sn_TX_WR: 0x%04x \n\r",getSnTxWrAddress());*/
	
	
	WIZ_Write(dst_ptr,CtrlByte, buf, length);
	
	upd=(getSnTxWrAddress()+length);
	WIZ_WriteByte(Sn_TX_WR, sck, upd >> 8);
	WIZ_WriteByte(Sn_TX_WR+1, sck, upd & 0xff);
	
	/*
	printf("  Sn_TX_RD upd: 0x%04x \n\r",getSnTxRdAddress());
	printf("  Sn_TX_WR upd: 0x%04x \n\r",getSnTxWrAddress());*/
	WIZ_Command(sck,SEND);
	
	while(WIZ_ReadByte(Sn_CR,sck));	 
}

void printClientInf()
{
	char mac_clt[20];
	char ip_clt[20];

	WIZ_Read(Sn_DHAR, sck, InpData, 6);
	snprintf(mac_clt, sizeof(mac_clt), "%02X:%02X:%02X:%02X:%02X:%02X", InpData[0], InpData[1], InpData[2], InpData[3], InpData[4], InpData[5] );
	
	WIZ_Read(Sn_DIPR, sck, InpData, 4);
	snprintf(ip_clt, sizeof(ip_clt), "%d.%d.%d.%d", InpData[0], InpData[1], InpData[2], InpData[3] );
	
	printf("%s \n\r", " Informacion del Cliente: ");
	printf("  IP  =  %s\n",ip_clt);
	printf("  MAC   =  %s\n\n",mac_clt);
}

void WIZ_receive(uint8 buf[])
{
	uint8 CtrlByte;
	uint16 len,dst_ptr;
	while(WIZ_ReadByte(Sn_SR,sck) != SOCK_ESTABLISHED ); 
	
	printClientInf();
	 
	while(!WIZ_ReadByte(Sn_RX_RSR,sck));
	  
	len = rxSize();			//Obtenemos la longitud de datos recibidos
	CtrlByte=getSnRxBlock();//Obtenemos el bloque RX del socket especificado
	dst_ptr=getSnRxAddress();//obtenemos la direccion del primer dato recibido
	
	WIZ_Read(dst_ptr,CtrlByte, buf, len);	//Guardamos los datos recibidos en el buffer
	
	WIZ_WriteByte(Sn_RX_RD, sck,dst_ptr+len);//Movemos el apuntador del buffer Rx
	WIZ_Command(sck,RECV);						//Notifica la actualizacion del apuntador
	while(WIZ_ReadByte(Sn_CR,sck));	 
	
	   for(i=0;i<len;i++)
	  {printf("%c",datosr[i]);}
}

void CloseSck()
{
	//while (WIZ_ReadByte(Sn_SR,sck) != SOCK_CLOSE_WAIT);
	WIZ_Command(sck,DISCON);
	while (WIZ_ReadByte(Sn_SR,sck) != SOCK_CLOSED);
}

void Receive()
{
	uint8 CtrlByte;
	uint16 len,dst_ptr;
	while(WIZ_ReadByte(Sn_SR,sck) != SOCK_ESTABLISHED ); 
	
	printClientInf();
	 
	while(!WIZ_ReadByte(Sn_RX_RSR,sck));
	  
	len = rxSize();			//Obtenemos la longitud de datos recibidos
	CtrlByte=getSnRxBlock();//Obtenemos el bloque RX del socket especificado
	dst_ptr=getSnRxAddress();//obtenemos la direccion del primer dato recibido
	
	if ( NULL == (Data = calloc( len , sizeof (uint8) ) ) ) 
	{
		printf("  Error de memoria\n\r");
	}
	 
	WIZ_Read(dst_ptr,CtrlByte, Data, len);	//Guardamos los datos recibidos en el buffer
	
	WIZ_WriteByte(Sn_RX_RD, sck,dst_ptr+len);//Movemos el apuntador del buffer Rx
	WIZ_Command(sck,RECV);						//Notifica la actualizacion del apuntador
	while(WIZ_ReadByte(Sn_CR,sck));	 
	
	for(i=0;i<len;i++)
	{printf("%c",Data[i]);}
	printf("\n\r");
	   
	free(Data);
}


/*lint -save  -e970 Disable MISRA rule (6.3) checking. */
int main(void)
/*lint -restore Enable MISRA rule (6.3) checking. */
{
  /* Write your local variable definition here */
	
  /*** Processor Expert internal initialization. DON'T REMOVE THIS CODE!!! ***/
  PE_low_level_init();
  /*** End of Processor Expert internal initialization.                    ***/
  MySPIPtr = SM1_Init(NULL);                                

  //Inicializa servidor
  init(IP_Addr, IP_Subnet, IP_Gateway);
  
  FindSck();
  if(sck>0)
  {
	  OpenTcpSck(); //Abre y configura el puerto TCP-Listen
	  while(WIZ_ReadByte(Sn_SR,sck) != SOCK_ESTABLISHED );
	  
	  printClientInf(); //imprime la informacion del cliente
	  
	  if(WIZ_ReadByte(Sn_RX_RSR,sck)) //se recibieron datos?
	  {
		  Receive();//recibe
	//	  CheckData();//Analiza la informacion recibida
	  }
	  
	  // no estoy seguro de como analizar esta parte de hay envio o no
	  
	  //if (send)
	  //{
		  //Send();
	  //}
	  
	  WIZ_Send(a,sizeof(a));
	  WIZ_Send(b,sizeof(b));
	  WIZ_Send(c,sizeof(c));
	  WIZ_Send(d,sizeof(d));
	  WIZ_Send(e,sizeof(e));
	  WIZ_Send(f,sizeof(f));
	  WIZ_Send(g,sizeof(g));
	  WIZ_Send(h,sizeof(h));
	  
	  if (WIZ_ReadByte(Sn_SR,sck) == SOCK_CLOSE_WAIT)
	  {
		  CloseSck();
	  }  
	  CloseSck();
  }
  /*
  
  printf("%s  \n\r","INICIO");
  for(;;)
  {
	  //enlazando... 
	  Bind(); //ESPERANDO CONEXION

	  //WIZ_receive(datosr);//CONEXION ESTABLECIDA, RECIBIENDO INFORMACION	    
	  Receive();
	  WIZ_Send(a,sizeof(a));
	  WIZ_Send(b,sizeof(b));
	  WIZ_Send(c,sizeof(c));
	  WIZ_Send(d,sizeof(d));
	  WIZ_Send(e,sizeof(e));
	  WIZ_Send(f,sizeof(f));
	  WIZ_Send(g,sizeof(g));
	  WIZ_Send(h,sizeof(h));
	  
	  SckClose();
  }*/

  
  /*** Don't write any code pass this line, or it will be deleted during code generation. ***/
  /*** RTOS startup code. Macro PEX_RTOS_START is defined by the RTOS component. DON'T MODIFY THIS CODE!!! ***/
  #ifdef PEX_RTOS_START
    PEX_RTOS_START();                  /* Startup of the selected RTOS. Macro is defined by the RTOS component. */
  #endif
  /*** End of RTOS startup code.  ***/
  /*** Processor Expert end of main routine. DON'T MODIFY THIS CODE!!! ***/
  for(;;){}
  /*** Processor Expert end of main routine. DON'T WRITE CODE BELOW!!! ***/
} /*** End of main routine. DO NOT MODIFY THIS TEXT!!! ***/

/* END main */
/*!
** @}
*/
/*
** ###################################################################
**
**     This file was created by Processor Expert 10.3 [05.09]
**     for the Freescale Kinetis series of microcontrollers.
**
** ###################################################################
*/
