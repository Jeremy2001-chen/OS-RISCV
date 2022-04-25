#ifndef __DEBUG_H
#define __DEBUG_H


//print a message
#define MSG_PRINT(val) printf("[debug]in %s() %s\n",__func__, val)
//print a const char * variable
#define STR_PRINT(val) printf("[debug]in %s() "#val"=%s\n",__func__,val)
//print a variable in form of dec
#define DEC_PRINT(val) printf("[debug]in %s() "#val"=%d\n",__func__,val)
//print a variable in form of hex
#define HEX_PRINT(val) printf("[debug]in %s() "#val"=0x%x\n",__func__,val)

#endif