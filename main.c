 #include <mc30p011.h>


unsigned char   StatusBuf,ABuf;
unsigned char gLastTerminalA;
unsigned char gLastTerminalB;

unsigned int gLastChangeTick;

unsigned char gRollDirection;
unsigned char gChangeCount;
unsigned int gLastChangeCountTick;
unsigned char gTotalChangeCount;
unsigned int gTotalTempChangeCount;
unsigned int gSysTick;

unsigned int gVolChangeTick;

unsigned int gLastVolChangeTick;
unsigned char gIsInVolChangeState;

unsigned int nowTick;


#define VOL_UP_START()          P15 = 1 
#define VOL_UP_STOP()           P15 = 0
#define VOL_DN_START()          P14 = 1
#define VOL_DN_STOP()           P14 = 0

/*
#define VOL_UP_START()          P12 = P12 
#define VOL_UP_STOP()           P12 = P12
#define VOL_DN_START()          P12 = P12
#define VOL_DN_STOP()           P12 = P12
*/

#define ROOL_DIRECTION_CW       0    //顺时针
#define ROOL_DIRECTION_CCW      1
#define ROOL_DIRECTION_UNKNOWN  2

#define MAX_INTERVAL_ROOL       250
#define BT_VOL_CHANGE_ONE_TIME  9//7   //20


#define GET_TERMINAL_A_STATUS()     (P10)
#define GET_TERMINAL_B_STATUS()     (P11)

#define key_interrupt_enable()  KBIE = 1
#define key_interrupt_disable() KBIE = 0

#define EnWatchdog()            WDTEN = 1
#define DisWatchdog()   WDTEN = 0
#define ClrWdt()        __asm__("clrwdt")

#define Stop()  __asm__("stop")
#define ClrWdt()        __asm__("clrwdt")
#define NOP()   __asm__("nop")

void isr(void) __interrupt
{
         __asm
                movra   _ABuf
                swapar  _STATUS
                movra   _StatusBuf
        __endasm;

        if(T0IF)
        {
                T0IF = 0;
                gSysTick++;
        }

        if(KBIF)
        {
                KBIF =0 ;
        }
        __asm
                swapar  _StatusBuf
                movra   _STATUS
                swapr   _ABuf
                swapar  _ABuf
        __endasm;     
}

void getDiffTickFromNow(unsigned int beforeTick)
{
          GIE = 0;
        
nowTick = gSysTick;
          GIE= 1;

          if(beforeTick > nowTick)
          {
                nowTick = 0xFFFFFFFF-beforeTick + nowTick;
          }
          else
                nowTick = nowTick - beforeTick;
}


void btVolHandler(unsigned char direction)
{
        gIsInVolChangeState=1;
        if(direction == ROOL_DIRECTION_CW)
        {
                VOL_UP_START();
        }
        else if(direction == ROOL_DIRECTION_CCW)
        {
                VOL_DN_START();
        }

        GIE = 0;
        gVolChangeTick = gSysTick;
         if(gVolChangeTick == 0)
         {
                gVolChangeTick = 1;
                gSysTick = 1;
         }
        GIE = 1;
}

void checkDecoder()
{
        unsigned char terminalA,terminalB,direction,isChange;

        isChange =0;

        if(gIsInVolChangeState == 0)
        {
                getDiffTickFromNow(gLastChangeTick);
                if(nowTick > MAX_INTERVAL_ROOL)
                {
                           P12 =0;
                             DDR12 = 1;
                         DisWatchdog();
                            key_interrupt_enable();
                                NOP();NOP();NOP();
                            Stop();
                        key_interrupt_disable();
                           EnWatchdog();
                                DDR12 = 0;
                          P12=1;
       //                   gLastChangeTick = gSysTick;
                }
        }
        
        terminalA = GET_TERMINAL_A_STATUS();
        terminalB = GET_TERMINAL_B_STATUS();


        if((terminalA != gLastTerminalA) && (terminalB != gLastTerminalB))
        {
                
        }
        else if(gLastTerminalA != terminalA)
        {
                if(terminalA == terminalB)
                {
                        direction = ROOL_DIRECTION_CCW;
                }
                else
                {
                        direction = ROOL_DIRECTION_CW;
                }
                isChange = 1;
        }
        else if(gLastTerminalB != terminalB)
        {
                if(terminalB == terminalA)
                {
                        direction = ROOL_DIRECTION_CW;
                }
                else
                {
                        direction = ROOL_DIRECTION_CCW;
                }
                isChange = 1;
        }

        if(isChange)
        {
                if(direction != gRollDirection)
                {
                        gRollDirection = direction;
                        gChangeCount =1;
                        gTotalChangeCount = 0;
                          gTotalTempChangeCount = 0;
                           gVolChangeTick =0;
                              gLastChangeTick=gSysTick;
                                gIsInVolChangeState =0;
                           VOL_DN_STOP();
                           VOL_UP_STOP();
                }
                else
                {
                        getDiffTickFromNow(gLastChangeTick);
                        if(nowTick > MAX_INTERVAL_ROOL)
                        {
                                gChangeCount = 1;
                        }
                        else
                                gChangeCount++;

                        if(gChangeCount > 3)
                        {
                                /*
                                        if(P15)
                                                P15=0;
                                        else
                                                P15=1;
                                                */
                                 if(gIsInVolChangeState == 0)
                                 {
                                                btVolHandler(direction);
                                                gTotalChangeCount++;
                                                gTotalTempChangeCount++;
                                                gLastChangeCountTick = gSysTick;
                                 }
                                     else
                                     {
                                                #if 0
                                                getDiffTickFromNow(gLastChangeCountTick);
                                                if(nowTick <3)
                                                {
                                                        if(gTotalTempChangeCount <4)
                                                         {
                                                                gTotalChangeCount++;
                                                                        gTotalTempChangeCount ++;
                                                        }
                                                }
                                                else if(nowTick >20)
                                                {
                                                      gTotalTempChangeCount=0;
                                                }
                                                else
                                                {
                                                        gTotalChangeCount++;
                                                                gTotalTempChangeCount++;
                                                }
                                                        #endif
                                                        if(gTotalChangeCount <3)
                                                                gTotalChangeCount++;
                                                gLastChangeCountTick = gSysTick;                
                                     }
                                gChangeCount = 0;
                        }       
                }

                GIE = 0;
                gLastChangeTick = gSysTick;
                GIE = 1;
        }

        gLastTerminalA = terminalA;
        gLastTerminalB = terminalB;
}

void InitConfig()
{
        DDR1 = 0xCB;   // 11001111

         P15=0;
         P14 =0;
          P12 = 1;

        PUCON = 0x0B;   //pull up

         KBIE = 0;  
        KBIM0 = 1; 
        KBIM1 =1;

         T0CR = 0x07;   //16.384ms default
        T0IE = 1;

        GIE = 1;                    //使能全局中断
}


void main()
{
        InitConfig();

        EnWatchdog();
        
        gLastTerminalA = GET_TERMINAL_A_STATUS();
        gLastTerminalB = GET_TERMINAL_B_STATUS();

        gChangeCount = 0;
        gRollDirection = ROOL_DIRECTION_UNKNOWN;
        gVolChangeTick = 0;
        gLastChangeTick =0;
        gSysTick = 0;
        gTotalChangeCount =0;
         gTotalTempChangeCount=0;
        gLastVolChangeTick =0;
        gIsInVolChangeState =0;
         gLastChangeCountTick =0;

         key_interrupt_disable();
        
#if 0
        for(tempData = 0; tempData < 16; tempData++)
        {
                VOL_UP_START();
                GIE = 0;
                                        gVolChangeTick = gSysTick;
                                         if(gVolChangeTick == 0)
                                         {
                                                 gVolChangeTick = 1;
                                                  gSysTick = 1;
                                         }
                                         GIE = 1;
                while(1)
                {
                         getDiffTickFromNow(gVolChangeTick);
                        if(nowTick > BT_VOL_CHANGE_ONE_TIME)
                                        break;
                }
                VOL_UP_STOP();
                GIE = 0;
                                        gVolChangeTick = gSysTick;
                                         if(gVolChangeTick == 0)
                                         {
                                                 gVolChangeTick = 1;
                                                  gSysTick = 1;
                                         }
                                         GIE = 1;       
                while(1)
                {
                         getDiffTickFromNow(gVolChangeTick);
                        if(nowTick > 5)
                                        break;
                }
        }
 #endif       
        while(1)
        {
                
                        ClrWdt();
                checkDecoder();

                if(gVolChangeTick != 0)
                {
                        getDiffTickFromNow(gVolChangeTick);
                        if(nowTick > BT_VOL_CHANGE_ONE_TIME)
                        {
                                if(gTotalChangeCount < 2)
                                {
                                        VOL_DN_STOP();
                                        VOL_UP_STOP();
                                        gVolChangeTick = 0;
                                        gTotalChangeCount=0;
                                                gTotalTempChangeCount=0;
                                            gIsInVolChangeState =0;
                                }
                                else
                                {
                                              if(gLastVolChangeTick == 0)
                                                {
                                                        VOL_DN_STOP();
                                                        VOL_UP_STOP();
                                                        GIE = 0;
                                                        gLastVolChangeTick = gSysTick;
                                                        if(gLastVolChangeTick == 0)
                                                        {
                                                                 gLastVolChangeTick = 1;
                                                          gSysTick = 1;
                                                         }
                                                        GIE = 1;
                                                }
                                              else
                                                {
                                                        getDiffTickFromNow(gLastVolChangeTick);
                                                        if(nowTick > 5)
                                                        {
                                                                gLastVolChangeTick =0;
                                                                gTotalChangeCount--;

                                                                if(gRollDirection == ROOL_DIRECTION_CW)
                                                                        VOL_UP_START();
                                                                else
                                                                        VOL_DN_START();
                                                                
                                                                GIE = 0;
                                                                gVolChangeTick = gSysTick;
                                                                 if(gVolChangeTick == 0)
                                                         {
                                                               gVolChangeTick = 1;
                                                                  gSysTick = 1;
                                                               }
                                                                GIE = 1;
                                                         }
                                              }
                                }
                        }
                        
                }
        }
}