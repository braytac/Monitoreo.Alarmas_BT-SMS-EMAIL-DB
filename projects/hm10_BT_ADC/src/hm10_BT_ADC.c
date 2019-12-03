#include "sapi.h"        // <= Biblioteca sAPI
#include <string.h>

/*==================[definiciones y macros]==================================*/

#define UART_PC         UART_USB
#define UART_BLUETOOTH  UART_232
#define MAX_STACK       5
#define mon_MAX         1017     // lectura ADC considerada MAX  
#define mon_MIN         0 
#define ALARMA          465      // ADC corresp. a 1.5V
#define FACTOR_INV      310.0    // float. 1023 / 3.3 para ADC 10 bit

#define BAUD_BL         9600     // Baudios BL
#define BAUD_PC         115200   // Baudios UART PC

#define T_ENVIO_LECTURA 10000    // cada 10s envío lectura a BL
#define T_MON_BL        2000     // cada 2s textear si aún está conectado el módulo BL
#define T_ADC           500      // Periodo lect. ADC
#define T_ALERTAS       200      //

#define HIST_SIZE       5        // Tamaño arrelo de valores recientes

/*==================[definiciones de datos internos]=========================*/

/*==================[definiciones de datos externos]=========================*/

/*==================[declaraciones de funciones internas]====================*/

/*==================[declaraciones de funciones externas]====================*/

void transmision();
bool_t hm10bleTest( int32_t uart );
void hm10blePrintATCommands( int32_t uart );
float adc2float( int16_t adc_r );
void valores_recientes2PC( uint16_t *arreglo_historico );
void SetEstadoAlarma( uint8_t *alarma, uint16_t *muestra, uint16_t *mon_LOW );

/*==================[funcion principal]======================================*/

// FUNCION PRINCIPAL, PUNTO DE ENTRADA AL PROGRAMA LUEGO DE ENCENDIDO O RESET.
int main( void )
{
   // ---------- CONFIGURACIONES ------------------------------

   // Inicializar y configurar la plataforma
   boardConfig();

   uartConfig( UART_PC, BAUD_PC );
   uartWriteString( UART_PC, "UART_PC configurada.\r\n" );

   /*==================== ADC CONF ==================== */
   
   adcConfig( ADC_ENABLE ); /* ADC */

   /* Variable para almacenar el valor leido del ADC CH1 */
   uint16_t muestra = 0;
   uint16_t temp_str2int = 0;
   float tension = 0;
   float umbral_float = 0;
   //float factor = (3.3 / 1023.0);
   uint16_t mon_LOW = ALARMA; // Umbral inicial por defecto
   
   /*==================== FIN ADC CONF ==================== */

   /* Configuración de estado inicial de leds */

   gpioWrite( LEDB, OFF ); // LED indicador de emparejamiento apagado
   bool_t ledState_LOW = OFF;
   bool_t ledState_MIN = OFF;
   bool_t ledState_MAX = OFF;   
   static bool_t cambio_umbral = FALSE;
   
   uint8_t alarma = 0;
           // 0 = No.
           // 1 = Sí, identificada pero aún sin enviar.
           // 2 = Sí, ya enviada por BLT.

   
   /*==================== RTC CONF ==================== */
   // Crear estructura RTC
   /*rtc_t rtc;

   // Completar estructura RTC
   rtc.year = 2019;
   rtc.month = 11;
   rtc.mday = 3;
   //rtc.wday = 3;
   rtc.hour = 22;
   rtc.min = 00;
   rtc.sec= 0;
  
   rtcInit(); // Inicializar RTC
   rtcWrite( &rtc ); // Establecer fecha y hora
   */
   /*==================== /RTC CONF ==================== */
   
   // Max "X.Y"
   static char tension_umbral[4] = "1.5";
   static char tension_str[4];

   //char historial_tension[3][4]; // últimas 3 lecturas
   //char historial_fecha[3][20]; // últimas 3 lecturas
   uint16_t historial_adc[ HIST_SIZE ] = { 0 }; // últimas 5 lecturas   
   static char clave_valor[10];   
      
   /* Contador */
   static uint8_t i,j,k, pido_historial, chequear_estado = 0;

   /* Buffer */
   static char uartBuff[10];

   uint8_t data = 0;

   /* Variables de delays no bloqueantes */
   delay_t delay_medicion;
   delay_t delay_mon;
   delay_t delay_mon_bl;
   //delay_t delay_trafico;

   /* Inicializar Retardo no bloqueante con tiempo en ms */

   delayConfig( &delay_mon_bl, T_MON_BL );   
   delayConfig( &delay_medicion, T_ADC );
   delayConfig( &delay_mon, T_ENVIO_LECTURA ); // 10000, 10 seg original
   //delayConfig( &delay_trafico, 200 );
   
   /*==================== FIN ADC CONF ==================== */

   /*==================== BLUETOOTH CONF ==================== */
   
   // Inicializar UART_232 para conectar al modulo bluetooth
   uartConfig( UART_BLUETOOTH, BAUD_BL );
   uartWriteString( UART_PC, "UART_BLUETOOTH para \
                modulo Bluetooth configurada.\r\n" );
   uartWriteString( UART_PC, "Testeo si el modulo esta conectado enviando: AT\r\n" );

   uartWriteString( UART_BLUETOOTH, "AT+RESET\r\n" );

   if( hm10bleTest( UART_BLUETOOTH ) ){
      uartWriteString( UART_PC, "Modulo conectado correctamente\r\nEnviando cmd AT:.\r\n" );

      uartWriteString( UART_PC, "AT+NAMEbraytac\r\n" );
      uartWriteString( UART_BLUETOOTH, "AT+NAMEbraytac\r\n" );
      transmision();
      uartWriteString( UART_PC, "AT+ROLE0\r\n" );
      uartWriteString( UART_BLUETOOTH, "AT+ROLE0\r\n" );   // Ajustado como Esclavo   
      transmision();
      uartWriteString( UART_PC, "AT+TYPE3\r\n" );
      uartWriteString( UART_BLUETOOTH, "AT+TYPE3\r\n" );   // Ajuste específico para el clon de HM10
      transmision();
      uartWriteString( UART_PC, "AT+RESET\r\n" );  
      uartWriteString( UART_BLUETOOTH, "AT+RESET\r\n" );      
      transmision();
      uartWriteString( UART_PC, "AT+START\r\n" );  
      uartWriteString( UART_BLUETOOTH, "AT+START\r\n" );
      transmision();
      uartWriteString( UART_PC, "Inicio del registro...\r\n" );        
   }else{
      uartWriteString( UART_PC, "El modulo Bluetooth no responde.\r\n" );
   }

   /*==================== BLUETOOTH CONF ==================== */
   
   // ---------- REPETIR POR SIEMPRE --------------------------
   while( TRUE ) {

      /* ++++++++++++++++++++++++++++++++++++++ */
      /*           Leo ADC cada 500ms           */
      /* ++++++++++++++++++++++++++++++++++++++ */

      if ( delayRead( &delay_medicion ) ){
         /* Leo la Entrada Analogica AI0 - ADC0 CH1 */
         muestra = adcRead( CH1 );         
         // Ajusto "alarma" y actúo sobre los leds:
         SetEstadoAlarma( &alarma, &muestra, &mon_LOW );
         // Me pone alarma = 1, solo si no es actualmente 1

         // si en 11s no recibo "@" considero desvinculado de app
         if ( chequear_estado == 22){ 
             gpioWrite( LEDB, OFF );
             stdioPrintf(UART_PC, "Conexion con la APP del celular interrumpida.\r\n", tension_str);
         }
         chequear_estado++;
      }

      /* retardo no bloqueante cumplido - transferir lectura a BLT */
      // Entro a los 10 segundos, o si acaba de haber una alarma
      
      if ( delayRead( &delay_mon ) || alarma == 1 ){

         tension = adc2float(muestra);

         /* Conversión de tensión float2ASCII, precisión 2  */
         floatToString( tension, tension_str, 2 );
         historial_adc[j] = muestra;

         // envío por BLT a celular tensión actual y estado de alarma.
         stdioPrintf(UART_BLUETOOTH, "{a:%u,n:%s}", alarma, tension_str);
         transmision();
         
         // envío por UART a PC la lectura actual
         stdioPrintf(UART_PC, "Tension actual: %sV\r\n", tension_str);
         transmision();

         // Saco alarma de envío inmediata. Si corresponde seguir en este estado 
         // a lo sumo en 500ms se levanta de nuevo:
         
         if ( alarma > 0 ){
            stdioPrintf(UART_PC, "a=%d\r\nAlarma: Tension debajo de %sV\r\n", alarma,tension_umbral);
            alarma = 2; // marco como ya enviada (luego de enviarla por BLT como "1" )
            transmision();
         }

         // Mantener siempre las últimas 5 lecturas.
         if ( j == 4){
            j = 0;
         }else{
            j++;
         }         

      }
      
      // Si han sido pedidos, envío los 5 últimos valores
      
      if ( pido_historial > 0 ){
         transmision();
         // Envio histórico en hexa por su menor longitud.
         // En celular lo convierto a float con mayor precisión
         // MTU típico: 23 bytes, no podría enviar los 5 valores
         // con formato actual símil json: los envío en dos partes

         if ( pido_historial == 1) {
             stdioPrintf(UART_BLUETOOTH, "{h:%x,h:%x,h:%x}",
                         historial_adc[0],
                         historial_adc[1],
                         historial_adc[2]);
             pido_historial++;
         }else{ 
            // pido_historial = 2 -> 2da vuelta
            stdioPrintf(UART_BLUETOOTH, "{h:%x,h:%x}",
                        historial_adc[3],
                        historial_adc[4]);
            pido_historial = 0;
         }

         //Envio histórico a PC como str
         valores_recientes2PC( historial_adc );
 
      }
      
      
      /* ++++++++++++++++++++++++++++++++++++++ */
      /*           SECCIÓN BLUETOOTH            */
      /* ++++++++++++++++++++++++++++++++++++++ */

      // Si leo un byte por term. serial PC lo envio por BLUETOOTH (bridge)

      if( uartReadByte( UART_PC, &data ) ) {
         uartWriteByte( UART_BLUETOOTH, data );
         transmision();
      }

      // guardo en "data" byte enviado desde BLT
      if( uartReadByte( UART_BLUETOOTH, &data ) ) {
       
         // Si paso data recibido por BL a PC por serial:
         // uartWriteByte( UART_PC, data );

         transmision();
        
         // Reviso datos recibidos por BLUETOOTH:

         switch ( data ) {
         case '{':
            i = 0;         
            break;
         case 'h':                 // pido historial
            pido_historial = 1;
            break;
         case 'u':                 // voy a cambiar el límite de tensión baja
            i++; 
            break;
         case '@':
           // Si estoy vinculado a la APP del cel:
           chequear_estado = 0; // reinicio el contador de estado.
           if ( !gpioRead( LEDB ) ){
              gpioWrite( LEDB, ON );
              stdioPrintf(UART_PC, "Vinculado con la APP celular correctamente.\r\n", tension_str);
           }         
            break;
         case ':':
            if ( i == 1 ){
               cambio_umbral = TRUE;
               i++; //2               
            }
            break;
         
         case '}':

            if ( cambio_umbral ){
               cambio_umbral = FALSE;
               tension_umbral[3] = '\0';
               temp_str2int = (uint16_t)tension_umbral[0] - 48;
               temp_str2int = temp_str2int * 10; 
               temp_str2int = temp_str2int + ((uint16_t)tension_umbral[2] - 48);
               
               umbral_float = ((float)temp_str2int * FACTOR_INV) / 10;
               mon_LOW = (uint16_t)umbral_float;
               
               stdioPrintf(UART_PC, "Nuevo umbral: %s\r\n", tension_umbral);
               transmision();
            }
            break;

         default:

            if (cambio_umbral == TRUE && i > 1 && i < 6 ){ // estoy llenando
               tension_umbral[i-2] = data; // pos 2 = unidad, 3 ="."
               i++;
            }else{
               // SI MANDO OTRAS COSAS
               //uartWriteByte( UART_PC, data ); // paso data recibido BL a PC por serial                
            }
            break;
           
         }
      }
      
      
      // Si presiono TEC1 imprime la lista de comandos AT
      if( !gpioRead( TEC1 ) ) {
         hm10blePrintATCommands( UART_BLUETOOTH );
         delay(500);
      }
   }
   // FIN While infinito
   return 0;
}

/*==================[definiciones de funciones internas]=====================*/

/*==================[definiciones de funciones externas]=====================*/

bool_t hm10bleTest( int32_t uart ) {
   uartWriteString( uart, "AT\r\n" );
   return waitForReceiveStringOrTimeoutBlocking( uart, 
                                                 "OK\r\n", strlen("OK\r\n"),
                                                 1000 );
}

void hm10blePrintATCommands( int32_t uart ) {
   uartWriteString( uart, "AT+HELP?\r\n" );
}

float adc2float( int16_t adc_r ) {
         //tension = (float)muestra * factor;
         float tension;
         tension = (float)adc_r / FACTOR_INV;
        
         return tension;
}

void transmision(){
   gpioWrite( LED1, ON );
   delay(50);
   gpioWrite( LED1, OFF );
}


void valores_recientes2PC( uint16_t *arreglo_historico ) {
   // envío 5 lecturas
   uint16_t i = 0;
   float v = 0;
   char v_str[4];

   stdioPrintf(UART_PC, "Mostrando 5 ultimas lecturas:\r\n");               
   for (i = 0; i < 5; i++){
      
      v = adc2float( arreglo_historico[i] );
      floatToString( v, v_str, 2 );
      stdioPrintf( UART_PC, "%s\n",v_str );
      transmision();               
   }
   
}

// probablemente mas rápido que con pases por valor
void SetEstadoAlarma( uint8_t *alarma, uint16_t *muestra, uint16_t *mon_LOW ){

   if ( *muestra < *mon_LOW ){

      // Si no estaba seteada
      if ( *alarma == 0 ){
         (*alarma) = 1;
      }

      gpioWrite( LED3, OFF ); // Apago led tensión MAX
      
      // Alerta luminiosa por acabar de pasar el umbral
      //ledState_LOW = !ledState_LOW;
      //gpioWrite( LEDR, ledState_LOW );

      gpioToggle( LED2 );

      // Parpadeo led naranja => valor min (0V) alcanzado
      if ( *muestra == mon_MIN ){
         //ledState_MIN = !ledState_MIN;
         //gpioWrite( LED2, ledState_MIN );
         gpioToggle( LEDR );
         gpioWrite( LEDB, OFF ); // apago azul para poder ver clara esta alarma
      }
   }else{
      (*alarma) = 0;
      gpioToggle( LED3 ); // Si no estoy en alarma parpadeo led verde

      gpioWrite( LEDR, OFF );    // LED alarma apagado
      gpioWrite( LED2, OFF );    // LED 0V apagado
      // aviso lum. de valor máx alcanzado
      if ( *muestra > mon_MAX ){
         //ledState_MAX = !ledState_MAX;
         //gpioWrite( LED3, ledState_MAX );
         gpioToggle( LEDG );
         gpioWrite( LED2, OFF );               
      }else{
         gpioWrite( LEDG, OFF );
      }
   }
}

/*==================[fin del archivo]========================================*/
